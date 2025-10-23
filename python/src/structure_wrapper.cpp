#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "GemmiWrapper.h"
#include "structureto3di.h"
#include "PulchraWrapper.h"

namespace py = pybind11;

// Helper function to convert std::vector<Vec3> to NumPy array
py::array_t<double> vec3_to_numpy(const std::vector<Vec3>& coords) {
    auto result = py::array_t<double>(std::vector<ptrdiff_t>{static_cast<ptrdiff_t>(coords.size()), 3});
    auto buf = result.mutable_unchecked<2>();
    for (size_t i = 0; i < coords.size(); i++) {
        buf(i, 0) = coords[i].x;
        buf(i, 1) = coords[i].y;
        buf(i, 2) = coords[i].z;
    }
    return result;
}

// Forward declaration
class PyStructure;

// Chain class - represents a single chain in a structure
class PyChain {
public:
    PyChain(const std::string& name,
            const std::string& sequence,
            const std::string& seq_3di,
            const std::vector<Vec3>& ca,
            const std::vector<Vec3>& n,
            const std::vector<Vec3>& c,
            const std::vector<Vec3>& cb)
        : name_(name), sequence_(sequence), seq_3di_(seq_3di),
          ca_coords_(ca), n_coords_(n), c_coords_(c), cb_coords_(cb) {}

    std::string get_name() const { return name_; }
    std::string get_sequence() const { return sequence_; }
    std::string get_seq_3di() const { return seq_3di_; }
    size_t get_length() const { return sequence_.length(); }

    py::array_t<double> get_ca_coords() const { return vec3_to_numpy(ca_coords_); }
    py::array_t<double> get_n_coords() const { return vec3_to_numpy(n_coords_); }
    py::array_t<double> get_c_coords() const { return vec3_to_numpy(c_coords_); }
    py::array_t<double> get_cb_coords() const { return vec3_to_numpy(cb_coords_); }

    std::string repr() const {
        return "<Chain '" + name_ + "', length=" + std::to_string(sequence_.length()) + ">";
    }

private:
    std::string name_;
    std::string sequence_;
    std::string seq_3di_;
    std::vector<Vec3> ca_coords_;
    std::vector<Vec3> n_coords_;
    std::vector<Vec3> c_coords_;
    std::vector<Vec3> cb_coords_;
};

// Feature and Embedding classes for intermediate 3Di encoding data
class PyFeature {
public:
    PyFeature(const StructureTo3Di::Feature& feat) {
        for (size_t i = 0; i < Alphabet3Di::FEATURE_CNT; i++) {
            features_[i] = feat.f[i];
        }
    }

    py::array_t<double> to_array() const {
        auto result = py::array_t<double>(Alphabet3Di::FEATURE_CNT);
        auto buf = result.mutable_unchecked<1>();
        for (size_t i = 0; i < Alphabet3Di::FEATURE_CNT; i++) {
            buf(i) = features_[i];
        }
        return result;
    }

private:
    double features_[Alphabet3Di::FEATURE_CNT];
};

class PyEmbedding {
public:
    PyEmbedding(const StructureTo3Di::Embedding& emb) {
        for (size_t i = 0; i < Alphabet3Di::EMBEDDING_DIM; i++) {
            embedding_[i] = emb.f[i];
        }
    }

    py::array_t<double> to_array() const {
        auto result = py::array_t<double>(Alphabet3Di::EMBEDDING_DIM);
        auto buf = result.mutable_unchecked<1>();
        for (size_t i = 0; i < Alphabet3Di::EMBEDDING_DIM; i++) {
            buf(i) = embedding_[i];
        }
        return result;
    }

private:
    double embedding_[Alphabet3Di::EMBEDDING_DIM];
};

// Enhanced Python Structure class with multi-chain support
class PyStructure {
public:
    PyStructure() = default;

    // Load from file with enhanced options
    static PyStructure from_file(const std::string& filename,
                                  bool reconstruct_backbone = false,
                                  bool compute_features = false,
                                  int chain_index = -1) {
        PyStructure result;
        result.filename_ = filename;
        result.compute_features_ = compute_features;

        GemmiWrapper gemmi;

        // Load structure
        if (!gemmi.load(filename, GemmiWrapper::Format::Detect)) {
            throw std::runtime_error("Failed to load structure from: " + filename);
        }

        // Process chains
        PulchraWrapper pulchra;

        // Create a shared converter for this structure
        // We need to be careful about the neural network model loading
        // Using a local variable that gets destroyed after this structure is processed
        StructureTo3Di* converter = new StructureTo3Di();

        // Iterate over all chains in the structure
        for (size_t chain_count = 0; chain_count < gemmi.chain.size(); chain_count++) {
            // If specific chain requested, skip others
            if (chain_index >= 0 && static_cast<int>(chain_count) != chain_index) {
                continue;
            }

            size_t start = gemmi.chain[chain_count].first;
            size_t end = gemmi.chain[chain_count].second;
            size_t len = end - start;

            // Store amino acid sequence
            std::string sequence(gemmi.ami.begin() + start, gemmi.ami.begin() + end);

            // Reconstruct backbone if needed
            if (reconstruct_backbone) {
                bool needs_reconstruction = false;
                for (size_t i = start; i < end; i++) {
                    if (std::isnan(gemmi.n[i].x) || std::isnan(gemmi.c[i].x)) {
                        needs_reconstruction = true;
                        break;
                    }
                }

                if (needs_reconstruction) {
                    pulchra.rebuildBackbone(&gemmi.ca[start], &gemmi.n[start],
                                           &gemmi.c[start], &gemmi.ami[start], len);
                }
            }

            // Compute 3Di sequence
            char* states = converter->structure2states(
                &gemmi.ca[start],
                &gemmi.n[start],
                &gemmi.c[start],
                &gemmi.cb[start],
                len
            );
            std::string seq_3di(states, len);

            // Store intermediate features if requested
            if (compute_features) {
                auto features = converter->getFeatures();
                for (const auto& feat : features) {
                    result.features_.push_back(PyFeature(feat));
                }
            }

            // Create coordinates
            std::vector<Vec3> ca_coords(gemmi.ca.begin() + start, gemmi.ca.begin() + end);
            std::vector<Vec3> n_coords(gemmi.n.begin() + start, gemmi.n.begin() + end);
            std::vector<Vec3> c_coords(gemmi.c.begin() + start, gemmi.c.begin() + end);
            std::vector<Vec3> cb_coords(gemmi.cb.begin() + start, gemmi.cb.begin() + end);

            // Get chain name
            std::string chain_name = chain_count < gemmi.chainNames.size()
                                    ? gemmi.chainNames[chain_count]
                                    : std::to_string(chain_count);

            // Create chain object
            PyChain chain(chain_name, sequence, seq_3di,
                         ca_coords, n_coords, c_coords, cb_coords);
            result.chains_.push_back(chain);

            // For backward compatibility, first chain is the "default"
            if (chain_count == 0) {
                result.sequence_ = sequence;
                result.seq_3di_ = seq_3di;
                result.ca_coords_ = ca_coords;
                result.n_coords_ = n_coords;
                result.c_coords_ = c_coords;
                result.cb_coords_ = cb_coords;
            }

            chain_count++;

            // If specific chain requested, stop after finding it
            if (chain_index >= 0 && chain_count > chain_index) {
                break;
            }
        }

        if (result.chains_.empty()) {
            delete converter;
            throw std::runtime_error("No valid chains found in structure");
        }

        // Clean up converter
        delete converter;

        return result;
    }

    // Format-specific loaders
    static PyStructure from_pdb(const std::string& filename,
                                 bool reconstruct_backbone = false) {
        return from_file(filename, reconstruct_backbone);
    }

    static PyStructure from_mmcif(const std::string& filename,
                                   bool reconstruct_backbone = false) {
        return from_file(filename, reconstruct_backbone);
    }

    static PyStructure from_foldcomp(const std::string& filename) {
        return from_file(filename, false);
    }

    // Properties for backward compatibility (first chain)
    std::string get_sequence() const { return sequence_; }
    std::string get_seq_3di() const { return seq_3di_; }
    size_t get_length() const { return sequence_.length(); }
    std::string get_filename() const { return filename_; }

    py::array_t<double> get_ca_coords() const { return vec3_to_numpy(ca_coords_); }
    py::array_t<double> get_n_coords() const { return vec3_to_numpy(n_coords_); }
    py::array_t<double> get_c_coords() const { return vec3_to_numpy(c_coords_); }
    py::array_t<double> get_cb_coords() const { return vec3_to_numpy(cb_coords_); }

    // Multi-chain support
    const std::vector<PyChain>& get_chains() const { return chains_; }
    size_t num_chains() const { return chains_.size(); }
    const PyChain& get_chain(size_t index) const {
        if (index >= chains_.size()) {
            throw std::out_of_range("Chain index out of range");
        }
        return chains_[index];
    }

    // Intermediate features
    py::array_t<double> get_features() const {
        if (features_.empty()) {
            throw std::runtime_error(
                "Features not computed. Set compute_features=True when loading."
            );
        }

        auto result = py::array_t<double>({features_.size(), Alphabet3Di::FEATURE_CNT});
        auto buf = result.mutable_unchecked<2>();
        for (size_t i = 0; i < features_.size(); i++) {
            auto feat_array = features_[i].to_array();
            auto feat_buf = feat_array.unchecked<1>();
            for (size_t j = 0; j < Alphabet3Di::FEATURE_CNT; j++) {
                buf(i, j) = feat_buf(j);
            }
        }
        return result;
    }

    // String representation
    std::string repr() const {
        return "<Structure: " + filename_ +
               ", chains=" + std::to_string(chains_.size()) +
               ", length=" + std::to_string(sequence_.length()) + ">";
    }

private:
    std::string sequence_;
    std::string seq_3di_;
    std::vector<Vec3> ca_coords_;
    std::vector<Vec3> n_coords_;
    std::vector<Vec3> c_coords_;
    std::vector<Vec3> cb_coords_;
    std::vector<PyChain> chains_;
    std::string filename_;
    bool compute_features_;
    std::vector<PyFeature> features_;
};

// NOTE: batch_convert was removed due to segfault issues when loading multiple
// structures in the same session. The issue is in the underlying gemmi library.
// Workaround: Load structures in separate Python processes or one at a time.

// Standalone function: convert coordinates to 3Di
std::string coords_to_3di(py::array_t<double> ca,
                          py::array_t<double> n,
                          py::array_t<double> c,
                          py::array_t<double> cb) {
    // Check dimensions
    if (ca.ndim() != 2 || ca.shape(1) != 3) {
        throw std::invalid_argument("CA coordinates must be (N, 3) array");
    }
    if (n.ndim() != 2 || n.shape(1) != 3) {
        throw std::invalid_argument("N coordinates must be (N, 3) array");
    }
    if (c.ndim() != 2 || c.shape(1) != 3) {
        throw std::invalid_argument("C coordinates must be (N, 3) array");
    }
    if (cb.ndim() != 2 || cb.shape(1) != 3) {
        throw std::invalid_argument("CB coordinates must be (N, 3) array");
    }

    size_t len = ca.shape(0);
    if (n.shape(0) != len || c.shape(0) != len || cb.shape(0) != len) {
        throw std::invalid_argument("All coordinate arrays must have same length");
    }

    // Convert NumPy arrays to Vec3 vectors
    std::vector<Vec3> ca_vec(len), n_vec(len), c_vec(len), cb_vec(len);

    auto ca_r = ca.unchecked<2>();
    auto n_r = n.unchecked<2>();
    auto c_r = c.unchecked<2>();
    auto cb_r = cb.unchecked<2>();

    for (size_t i = 0; i < len; i++) {
        ca_vec[i] = Vec3(ca_r(i, 0), ca_r(i, 1), ca_r(i, 2));
        n_vec[i] = Vec3(n_r(i, 0), n_r(i, 1), n_r(i, 2));
        c_vec[i] = Vec3(c_r(i, 0), c_r(i, 1), c_r(i, 2));
        cb_vec[i] = Vec3(cb_r(i, 0), cb_r(i, 1), cb_r(i, 2));
    }

    // Compute 3Di using static converter
    static StructureTo3Di converter;
    char* states = converter.structure2states(
        ca_vec.data(), n_vec.data(), c_vec.data(), cb_vec.data(), len
    );

    return std::string(states, len);
}

void init_structure(py::module &m) {
    // Vec3 structure
    py::class_<Vec3>(m, "Vec3", "3D coordinate vector")
        .def(py::init<>())
        .def(py::init<double, double, double>())
        .def_readwrite("x", &Vec3::x, "X coordinate")
        .def_readwrite("y", &Vec3::y, "Y coordinate")
        .def_readwrite("z", &Vec3::z, "Z coordinate")
        .def("__repr__", [](const Vec3 &v) {
            return "Vec3(" + std::to_string(v.x) + ", " +
                   std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        });

    // Chain class
    py::class_<PyChain>(m, "Chain", R"pbdoc(
        Represents a single chain in a protein structure.

        Each chain has its own sequence, 3Di encoding, and coordinates.
    )pbdoc")
        .def_property_readonly("name", &PyChain::get_name, "Chain identifier")
        .def_property_readonly("sequence", &PyChain::get_sequence, "Amino acid sequence")
        .def_property_readonly("seq_3di", &PyChain::get_seq_3di, "3Di structural alphabet sequence")
        .def_property_readonly("length", &PyChain::get_length, "Number of residues")
        .def_property_readonly("ca_coords", &PyChain::get_ca_coords, "C-alpha coordinates (N, 3)")
        .def_property_readonly("n_coords", &PyChain::get_n_coords, "Nitrogen coordinates (N, 3)")
        .def_property_readonly("c_coords", &PyChain::get_c_coords, "Carbon coordinates (N, 3)")
        .def_property_readonly("cb_coords", &PyChain::get_cb_coords, "C-beta coordinates (N, 3)")
        .def("__repr__", &PyChain::repr)
        .def("__len__", &PyChain::get_length);

    // Structure class
    py::class_<PyStructure>(m, "Structure", R"pbdoc(
        Represents a protein structure with 3Di encoding.

        Supports multi-chain structures and provides access to:
        - Amino acid sequences
        - 3Di structural alphabet sequences
        - Atomic coordinates (CA, N, C, CB)
        - Chain information
        - Intermediate encoding features
    )pbdoc")
        .def(py::init<>())
        .def_static("from_file", &PyStructure::from_file,
                   py::arg("filename"),
                   py::arg("reconstruct_backbone") = false,
                   py::arg("compute_features") = false,
                   py::arg("chain_index") = -1,
                   R"pbdoc(
            Load structure from file.

            Parameters
            ----------
            filename : str
                Path to PDB, mmCIF, or Foldcomp file
            reconstruct_backbone : bool, optional
                Reconstruct N, C, CB atoms from CA-only structures (default: False)
            compute_features : bool, optional
                Compute and store intermediate geometric features (default: False)
            chain_index : int, optional
                Load only specific chain index, -1 for all chains (default: -1)

            Returns
            -------
            Structure
                Loaded structure with 3Di encoding

            Examples
            --------
            >>> struct = Structure.from_file("protein.pdb")
            >>> struct = Structure.from_file("protein.pdb", compute_features=True)
            >>> struct = Structure.from_file("complex.pdb", chain_index=0)
        )pbdoc")
        .def_static("from_pdb", &PyStructure::from_pdb,
                   py::arg("filename"),
                   py::arg("reconstruct_backbone") = false,
                   "Load structure from PDB file")
        .def_static("from_mmcif", &PyStructure::from_mmcif,
                   py::arg("filename"),
                   py::arg("reconstruct_backbone") = false,
                   "Load structure from mmCIF file")
        .def_static("from_foldcomp", &PyStructure::from_foldcomp,
                   py::arg("filename"),
                   "Load structure from Foldcomp compressed file")
        .def_property_readonly("sequence", &PyStructure::get_sequence,
                              "Amino acid sequence (first chain)")
        .def_property_readonly("seq_3di", &PyStructure::get_seq_3di,
                              "3Di structural alphabet sequence (first chain)")
        .def_property_readonly("length", &PyStructure::get_length,
                              "Number of residues (first chain)")
        .def_property_readonly("filename", &PyStructure::get_filename,
                              "Source filename")
        .def_property_readonly("ca_coords", &PyStructure::get_ca_coords,
                              "C-alpha coordinates as NumPy array (N, 3)")
        .def_property_readonly("n_coords", &PyStructure::get_n_coords,
                              "Nitrogen coordinates as NumPy array (N, 3)")
        .def_property_readonly("c_coords", &PyStructure::get_c_coords,
                              "Carbon coordinates as NumPy array (N, 3)")
        .def_property_readonly("cb_coords", &PyStructure::get_cb_coords,
                              "C-beta coordinates as NumPy array (N, 3)")
        .def_property_readonly("chains", &PyStructure::get_chains,
                              "List of all chains in structure")
        .def_property_readonly("num_chains", &PyStructure::num_chains,
                              "Number of chains in structure")
        .def("get_chain", &PyStructure::get_chain,
             py::arg("index"),
             "Get specific chain by index")
        .def_property_readonly("features", &PyStructure::get_features,
                              "Intermediate geometric features (N, 10)")
        .def("__repr__", &PyStructure::repr)
        .def("__len__", &PyStructure::get_length)
        .def("__iter__", [](const PyStructure &s) {
            return py::make_iterator(s.get_chains().begin(), s.get_chains().end());
        }, py::keep_alive<0, 1>(), "Iterate over chains");

    // NOTE: batch_convert was removed due to segfault issues when loading
    // multiple structures. Use separate processes or load one at a time.

    // Standalone coordinate conversion function
    m.def("coords_to_3di", &coords_to_3di,
          py::arg("ca"), py::arg("n"), py::arg("c"), py::arg("cb"),
          R"pbdoc(
        Convert atomic coordinates to 3Di structural alphabet.

        Parameters
        ----------
        ca : np.ndarray
            C-alpha coordinates, shape (N, 3)
        n : np.ndarray
            Nitrogen coordinates, shape (N, 3)
        c : np.ndarray
            Carbon coordinates, shape (N, 3)
        cb : np.ndarray
            C-beta coordinates, shape (N, 3)

        Returns
        -------
        str
            3Di sequence

        Examples
        --------
        >>> import numpy as np
        >>> ca = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]])
        >>> # ... (create n, c, cb similarly)
        >>> seq_3di = coords_to_3di(ca, n, c, cb)
    )pbdoc");
}
