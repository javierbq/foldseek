#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "GemmiWrapper.h"
#include "structureto3di.h"
#include "PulchraWrapper.h"

namespace py = pybind11;

// Helper function to convert std::vector<Vec3> to NumPy array
py::array_t<double> vec3_to_numpy(const std::vector<Vec3>& coords) {
    auto result = py::array_t<double>({coords.size(), 3});
    auto buf = result.mutable_unchecked<2>();
    for (size_t i = 0; i < coords.size(); i++) {
        buf(i, 0) = coords[i].x;
        buf(i, 1) = coords[i].y;
        buf(i, 2) = coords[i].z;
    }
    return result;
}

// Python-friendly Structure class
class PyStructure {
public:
    PyStructure() = default;

    // Load from file
    static PyStructure from_file(const std::string& filename,
                                  bool reconstruct_backbone = false) {
        PyStructure result;
        GemmiWrapper gemmi;

        // Load structure
        if (!gemmi.load(filename, GemmiWrapper::Format::Detect)) {
            throw std::runtime_error("Failed to load structure from: " + filename);
        }

        // Get first chain (for now, multi-chain support comes later)
        auto chain = gemmi.nextChain();
        if (chain.first == chain.second) {
            throw std::runtime_error("No valid chains found in structure");
        }

        size_t start = chain.first;
        size_t end = chain.second;
        size_t len = end - start;

        // Store amino acid sequence
        result.sequence = std::string(gemmi.ami.begin() + start,
                                     gemmi.ami.begin() + end);

        // Reconstruct backbone if needed (CA-only structures)
        if (reconstruct_backbone) {
            PulchraWrapper pulchra;
            // Check if we need reconstruction
            bool needs_reconstruction = false;
            for (size_t i = start; i < end; i++) {
                if (std::isnan(gemmi.n[i].x) || std::isnan(gemmi.c[i].x)) {
                    needs_reconstruction = true;
                    break;
                }
            }

            if (needs_reconstruction) {
                pulchra.recFromCAlphaTrace(&gemmi.ca[start], &gemmi.n[start],
                                          &gemmi.c[start], len);
            }
        }

        // Compute 3Di sequence
        StructureTo3Di converter;
        char* states = converter.structure2states(
            &gemmi.ca[start],
            &gemmi.n[start],
            &gemmi.c[start],
            &gemmi.cb[start],
            len
        );
        result.seq_3di = std::string(states, len);

        // Store coordinates
        result.ca_coords = std::vector<Vec3>(gemmi.ca.begin() + start,
                                            gemmi.ca.begin() + end);
        result.n_coords = std::vector<Vec3>(gemmi.n.begin() + start,
                                           gemmi.n.begin() + end);
        result.c_coords = std::vector<Vec3>(gemmi.c.begin() + start,
                                           gemmi.c.begin() + end);
        result.cb_coords = std::vector<Vec3>(gemmi.cb.begin() + start,
                                            gemmi.cb.begin() + end);

        // Store metadata
        result.chain_names = gemmi.chainNames;
        result.filename = filename;

        return result;
    }

    // Properties
    std::string get_sequence() const { return sequence; }
    std::string get_seq_3di() const { return seq_3di; }
    size_t get_length() const { return sequence.length(); }
    std::vector<std::string> get_chain_names() const { return chain_names; }
    std::string get_filename() const { return filename; }

    // Get coordinates as NumPy arrays
    py::array_t<double> get_ca_coords() const { return vec3_to_numpy(ca_coords); }
    py::array_t<double> get_n_coords() const { return vec3_to_numpy(n_coords); }
    py::array_t<double> get_c_coords() const { return vec3_to_numpy(c_coords); }
    py::array_t<double> get_cb_coords() const { return vec3_to_numpy(cb_coords); }

    // String representation
    std::string repr() const {
        return "<Structure: " + filename +
               ", length=" + std::to_string(sequence.length()) +
               ", chains=" + std::to_string(chain_names.size()) + ">";
    }

private:
    std::string sequence;
    std::string seq_3di;
    std::vector<Vec3> ca_coords;
    std::vector<Vec3> n_coords;
    std::vector<Vec3> c_coords;
    std::vector<Vec3> cb_coords;
    std::vector<std::string> chain_names;
    std::string filename;
};

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

    // Compute 3Di
    StructureTo3Di converter;
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

    // Structure class
    py::class_<PyStructure>(m, "Structure", R"pbdoc(
        Represents a protein structure with 3Di encoding.

        Load structures from PDB/mmCIF/Foldcomp files and access:
        - Amino acid sequence
        - 3Di structural alphabet sequence
        - Atomic coordinates (CA, N, C, CB)
        - Chain information
    )pbdoc")
        .def(py::init<>())
        .def_static("from_file", &PyStructure::from_file,
                   py::arg("filename"),
                   py::arg("reconstruct_backbone") = false,
                   R"pbdoc(
            Load structure from file.

            Parameters
            ----------
            filename : str
                Path to PDB, mmCIF, or Foldcomp file
            reconstruct_backbone : bool, optional
                Reconstruct N, C, CB atoms from CA-only structures (default: False)

            Returns
            -------
            Structure
                Loaded structure with 3Di encoding

            Examples
            --------
            >>> struct = Structure.from_file("protein.pdb")
            >>> print(struct.seq_3di)
        )pbdoc")
        .def_property_readonly("sequence", &PyStructure::get_sequence,
                              "Amino acid sequence (one-letter code)")
        .def_property_readonly("seq_3di", &PyStructure::get_seq_3di,
                              "3Di structural alphabet sequence")
        .def_property_readonly("length", &PyStructure::get_length,
                              "Number of residues")
        .def_property_readonly("chain_names", &PyStructure::get_chain_names,
                              "List of chain identifiers")
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
        .def("__repr__", &PyStructure::repr)
        .def("__len__", &PyStructure::get_length);

    // Standalone function
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
