/**
 * alignment_wrapper.cpp
 *
 * Python bindings for structural alignment functions (TM-align, LDDT, etc.)
 */

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <vector>
#include <string>

#include "TMaligner.h"
#include "structureto3di.h"
#include "LDDT.h"

namespace py = pybind11;

/**
 * Python wrapper for TMscoreResult
 */
class PyTMscoreResult {
public:
    PyTMscoreResult(const TMaligner::TMscoreResult& result)
        : tmscore(result.tmscore), rmsd(result.rmsd) {
        // Copy rotation matrix
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                rotation_matrix[i][j] = result.u[i][j];
            }
        }
        // Copy translation vector
        for (int i = 0; i < 3; i++) {
            translation[i] = result.t[i];
        }
    }

    double get_tmscore() const { return tmscore; }
    double get_rmsd() const { return rmsd; }

    py::array_t<float> get_rotation_matrix() const {
        auto result = py::array_t<float>(std::vector<ptrdiff_t>{3, 3});
        auto buf = result.mutable_unchecked<2>();
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                buf(i, j) = rotation_matrix[i][j];
            }
        }
        return result;
    }

    py::array_t<float> get_translation() const {
        auto result = py::array_t<float>(3);
        auto buf = result.mutable_unchecked<1>();
        for (int i = 0; i < 3; i++) {
            buf(i) = translation[i];
        }
        return result;
    }

    std::string repr() const {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                "<TMscoreResult tmscore=%.3f rmsd=%.3f>",
                tmscore, rmsd);
        return std::string(buffer);
    }

private:
    double tmscore;
    double rmsd;
    float rotation_matrix[3][3];
    float translation[3];
};

/**
 * Python wrapper for TMaligner
 */
class PyTMaligner {
public:
    PyTMaligner(unsigned int max_seq_len = 50000,
                bool fast = true,
                bool score_only = false)
        : max_len_(max_seq_len) {
        aligner_ = new TMaligner(max_seq_len, fast, score_only, !fast);
    }

    ~PyTMaligner() {
        delete aligner_;
        if (query_x_) delete[] query_x_;
        if (query_y_) delete[] query_y_;
        if (query_z_) delete[] query_z_;
    }

    /**
     * Compute TM-score between two structures
     *
     * Parameters:
     *   query_ca: Query CA coordinates (N1, 3)
     *   target_ca: Target CA coordinates (N2, 3)
     *   query_seq: Query amino acid sequence
     *   target_seq: Target amino acid sequence
     *
     * Returns:
     *   PyTMscoreResult with TM-score, RMSD, rotation, and translation
     */
    PyTMscoreResult align(py::array_t<double> query_ca,
                          py::array_t<double> target_ca,
                          const std::string& query_seq,
                          const std::string& target_seq) {
        // Validate inputs
        if (query_ca.ndim() != 2 || query_ca.shape(1) != 3) {
            throw std::invalid_argument("Query CA coordinates must be (N, 3) array");
        }
        if (target_ca.ndim() != 2 || target_ca.shape(1) != 3) {
            throw std::invalid_argument("Target CA coordinates must be (N, 3) array");
        }

        size_t query_len = query_ca.shape(0);
        size_t target_len = target_ca.shape(0);

        if (query_seq.length() != query_len) {
            throw std::invalid_argument("Query sequence length doesn't match coordinates");
        }
        if (target_seq.length() != target_len) {
            throw std::invalid_argument("Target sequence length doesn't match coordinates");
        }

        // Allocate coordinate arrays if needed
        if (query_len > max_len_) {
            throw std::invalid_argument("Query length exceeds max_seq_len");
        }
        if (target_len > max_len_) {
            throw std::invalid_argument("Target length exceeds max_seq_len");
        }

        // Allocate arrays
        if (!query_x_) {
            query_x_ = new float[max_len_];
            query_y_ = new float[max_len_];
            query_z_ = new float[max_len_];
        }
        float* target_x = new float[target_len];
        float* target_y = new float[target_len];
        float* target_z = new float[target_len];
        char* query_seq_arr = new char[query_len + 1];
        char* target_seq_arr = new char[target_len + 1];

        // Copy data from NumPy arrays
        auto query_r = query_ca.unchecked<2>();
        auto target_r = target_ca.unchecked<2>();

        for (size_t i = 0; i < query_len; i++) {
            query_x_[i] = query_r(i, 0);
            query_y_[i] = query_r(i, 1);
            query_z_[i] = query_r(i, 2);
            query_seq_arr[i] = query_seq[i];
        }
        query_seq_arr[query_len] = '\0';

        for (size_t i = 0; i < target_len; i++) {
            target_x[i] = target_r(i, 0);
            target_y[i] = target_r(i, 1);
            target_z[i] = target_r(i, 2);
            target_seq_arr[i] = target_seq[i];
        }
        target_seq_arr[target_len] = '\0';

        // Initialize query
        aligner_->initQuery(query_x_, query_y_, query_z_, query_seq_arr, query_len);

        // Compute alignment and TM-score
        float tm_score_value;
        Matcher::result_t aln_result = aligner_->align(
            0,  // dbKey (not used for our purposes)
            target_x, target_y, target_z,
            target_seq_arr, target_len,
            tm_score_value
        );

        // Get TM-score result with rotation/translation
        // We need to call computeTMscore to get the transformation
        TMaligner::TMscoreResult tm_result = aligner_->computeTMscore(
            target_x, target_y, target_z,
            target_len,
            aln_result.qStartPos,
            aln_result.dbStartPos,
            aln_result.backtrace,
            TMaligner::normalization(0, aln_result.qEndPos - aln_result.qStartPos,
                                   query_len, target_len)
        );

        // Clean up
        delete[] target_x;
        delete[] target_y;
        delete[] target_z;
        delete[] query_seq_arr;
        delete[] target_seq_arr;

        return PyTMscoreResult(tm_result);
    }

    std::string repr() const {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                "<TMaligner max_len=%u>",
                max_len_);
        return std::string(buffer);
    }

private:
    TMaligner* aligner_;
    unsigned int max_len_;
    float* query_x_ = nullptr;
    float* query_y_ = nullptr;
    float* query_z_ = nullptr;
};

// Helper function for simple TM-score computation
PyTMscoreResult compute_tmscore(py::array_t<double> ca1,
                                py::array_t<double> ca2,
                                const std::string& seq1,
                                const std::string& seq2,
                                bool fast = true) {
    unsigned int max_len = std::max(ca1.shape(0), ca2.shape(0));
    PyTMaligner aligner(max_len, fast, false);
    return aligner.align(ca1, ca2, seq1, seq2);
}

/**
 * Python wrapper for LDDTScoreResult
 */
class PyLDDTResult {
public:
    PyLDDTResult(const LDDTCalculator::LDDTScoreResult& result)
        : average_lddt(result.avgLddtScore), length(result.scoreLength) {
        // Copy per-residue scores
        per_residue_scores.resize(result.scoreLength);
        for (int i = 0; i < result.scoreLength; i++) {
            per_residue_scores[i] = result.perCaLddtScore[i];
        }
    }

    double get_average() const { return average_lddt; }
    int get_length() const { return length; }

    py::array_t<float> get_per_residue() const {
        auto result = py::array_t<float>(per_residue_scores.size());
        auto buf = result.mutable_unchecked<1>();
        for (size_t i = 0; i < per_residue_scores.size(); i++) {
            buf(i) = per_residue_scores[i];
        }
        return result;
    }

    std::string repr() const {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                "<LDDTResult average=%.3f length=%d>",
                average_lddt, length);
        return std::string(buffer);
    }

private:
    double average_lddt;
    int length;
    std::vector<float> per_residue_scores;
};

/**
 * Python wrapper for LDDTCalculator
 */
class PyLDDTCalculator {
public:
    PyLDDTCalculator(unsigned int max_query_len = 50000,
                     unsigned int max_target_len = 50000)
        : max_query_len_(max_query_len), max_target_len_(max_target_len) {
        calculator_ = new LDDTCalculator(max_query_len, max_target_len);
    }

    ~PyLDDTCalculator() {
        delete calculator_;
        if (query_x_) delete[] query_x_;
        if (query_y_) delete[] query_y_;
        if (query_z_) delete[] query_z_;
    }

    /**
     * Compute LDDT score between two aligned structures
     *
     * Parameters:
     *   query_ca: Query CA coordinates (N1, 3)
     *   target_ca: Target CA coordinates (N2, 3)
     *   alignment: CIGAR string representing the alignment (M=match, I=insertion, D=deletion)
     *   query_start: Start position in query (default: 0)
     *   target_start: Start position in target (default: 0)
     *
     * Returns:
     *   PyLDDTResult with average LDDT and per-residue scores
     */
    PyLDDTResult compute_lddt(py::array_t<double> query_ca,
                              py::array_t<double> target_ca,
                              const std::string& alignment,
                              int query_start = 0,
                              int target_start = 0) {
        // Validate inputs
        if (query_ca.ndim() != 2 || query_ca.shape(1) != 3) {
            throw std::invalid_argument("Query CA coordinates must be (N, 3) array");
        }
        if (target_ca.ndim() != 2 || target_ca.shape(1) != 3) {
            throw std::invalid_argument("Target CA coordinates must be (N, 3) array");
        }

        size_t query_len = query_ca.shape(0);
        size_t target_len = target_ca.shape(0);

        if (query_len > max_query_len_) {
            throw std::invalid_argument("Query length exceeds max_query_len");
        }
        if (target_len > max_target_len_) {
            throw std::invalid_argument("Target length exceeds max_target_len");
        }

        // Allocate arrays for query if needed
        if (!query_x_) {
            query_x_ = new float[max_query_len_];
            query_y_ = new float[max_query_len_];
            query_z_ = new float[max_query_len_];
        }

        // Allocate arrays for target
        float* target_x = new float[target_len];
        float* target_y = new float[target_len];
        float* target_z = new float[target_len];

        // Copy data from NumPy arrays
        auto query_r = query_ca.unchecked<2>();
        auto target_r = target_ca.unchecked<2>();

        for (size_t i = 0; i < query_len; i++) {
            query_x_[i] = query_r(i, 0);
            query_y_[i] = query_r(i, 1);
            query_z_[i] = query_r(i, 2);
        }

        for (size_t i = 0; i < target_len; i++) {
            target_x[i] = target_r(i, 0);
            target_y[i] = target_r(i, 1);
            target_z[i] = target_r(i, 2);
        }

        // Initialize query structure
        calculator_->initQuery(query_len, query_x_, query_y_, query_z_);

        // Compute LDDT score
        LDDTCalculator::LDDTScoreResult result = calculator_->computeLDDTScore(
            target_len, query_start, target_start, alignment,
            target_x, target_y, target_z
        );

        // Clean up
        delete[] target_x;
        delete[] target_y;
        delete[] target_z;

        return PyLDDTResult(result);
    }

    std::string repr() const {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                "<LDDTCalculator max_query_len=%u max_target_len=%u>",
                max_query_len_, max_target_len_);
        return std::string(buffer);
    }

private:
    LDDTCalculator* calculator_;
    unsigned int max_query_len_;
    unsigned int max_target_len_;
    float* query_x_ = nullptr;
    float* query_y_ = nullptr;
    float* query_z_ = nullptr;
};

// Helper function for simple LDDT computation
PyLDDTResult compute_lddt(py::array_t<double> ca1,
                          py::array_t<double> ca2,
                          const std::string& alignment,
                          int query_start = 0,
                          int target_start = 0) {
    unsigned int max_len1 = ca1.shape(0);
    unsigned int max_len2 = ca2.shape(0);
    PyLDDTCalculator calculator(max_len1, max_len2);
    return calculator.compute_lddt(ca1, ca2, alignment, query_start, target_start);
}

void init_alignment(py::module &m) {
    // TMscoreResult class
    py::class_<PyTMscoreResult>(m, "TMscoreResult", R"pbdoc(
        Result of TM-align structural alignment.

        Contains TM-score, RMSD, rotation matrix, and translation vector.
    )pbdoc")
        .def_property_readonly("tmscore", &PyTMscoreResult::get_tmscore,
                             "TM-score (0-1, higher is better)")
        .def_property_readonly("rmsd", &PyTMscoreResult::get_rmsd,
                             "Root mean square deviation in Angstroms")
        .def_property_readonly("rotation_matrix", &PyTMscoreResult::get_rotation_matrix,
                             "Rotation matrix (3, 3) to superpose target onto query")
        .def_property_readonly("translation", &PyTMscoreResult::get_translation,
                             "Translation vector (3,) to superpose target onto query")
        .def("__repr__", &PyTMscoreResult::repr);

    // TMaligner class
    py::class_<PyTMaligner>(m, "TMaligner", R"pbdoc(
        TM-align structural alignment algorithm.

        Computes optimal superposition and TM-score between two protein structures.

        Examples
        --------
        >>> from pyfoldseek import Structure, TMaligner
        >>> s1 = Structure.from_file("protein1.pdb")
        >>> s2 = Structure.from_file("protein2.pdb")
        >>> aligner = TMaligner()
        >>> result = aligner.align(s1.ca_coords, s2.ca_coords, s1.sequence, s2.sequence)
        >>> print(f"TM-score: {result.tmscore:.3f}")
        >>> print(f"RMSD: {result.rmsd:.3f} Å")
    )pbdoc")
        .def(py::init<unsigned int, bool, bool>(),
             py::arg("max_seq_len") = 50000,
             py::arg("fast") = true,
             py::arg("score_only") = false,
             R"pbdoc(
            Initialize TMaligner.

            Parameters
            ----------
            max_seq_len : int, optional
                Maximum sequence length to support (default: 50000)
            fast : bool, optional
                Use fast algorithm (default: True)
            score_only : bool, optional
                Compute only TM-score without superposition (default: False)
        )pbdoc")
        .def("align", &PyTMaligner::align,
             py::arg("query_ca"), py::arg("target_ca"),
             py::arg("query_seq"), py::arg("target_seq"),
             R"pbdoc(
            Align two structures and compute TM-score.

            Parameters
            ----------
            query_ca : numpy.ndarray
                Query CA coordinates (N1, 3)
            target_ca : numpy.ndarray
                Target CA coordinates (N2, 3)
            query_seq : str
                Query amino acid sequence
            target_seq : str
                Target amino acid sequence

            Returns
            -------
            TMscoreResult
                Alignment result with TM-score, RMSD, rotation, and translation
        )pbdoc")
        .def("__repr__", &PyTMaligner::repr);

    // Convenience function
    m.def("compute_tmscore", &compute_tmscore,
          py::arg("ca1"), py::arg("ca2"),
          py::arg("seq1"), py::arg("seq2"),
          py::arg("fast") = true,
          R"pbdoc(
        Compute TM-score between two structures.

        Convenience function that creates a TMaligner and computes the score.

        Parameters
        ----------
        ca1 : numpy.ndarray
            First structure CA coordinates (N1, 3)
        ca2 : numpy.ndarray
            Second structure CA coordinates (N2, 3)
        seq1 : str
            First structure amino acid sequence
        seq2 : str
            Second structure amino acid sequence
        fast : bool, optional
            Use fast algorithm (default: True)

        Returns
        -------
        TMscoreResult
            Alignment result with TM-score, RMSD, rotation, and translation

        Examples
        --------
        >>> from pyfoldseek import Structure, compute_tmscore
        >>> s1 = Structure.from_file("protein1.pdb")
        >>> s2 = Structure.from_file("protein2.pdb")
        >>> result = compute_tmscore(s1.ca_coords, s2.ca_coords, s1.sequence, s2.sequence)
        >>> print(f"TM-score: {result.tmscore:.3f}")
    )pbdoc");

    // LDDTResult class
    py::class_<PyLDDTResult>(m, "LDDTResult", R"pbdoc(
        Result of LDDT calculation.

        Contains average LDDT score and per-residue LDDT scores.
    )pbdoc")
        .def_property_readonly("average", &PyLDDTResult::get_average,
                             "Average LDDT score (0-1, higher is better)")
        .def_property_readonly("length", &PyLDDTResult::get_length,
                             "Number of aligned residues")
        .def_property_readonly("per_residue", &PyLDDTResult::get_per_residue,
                             "Per-residue LDDT scores as NumPy array")
        .def("__repr__", &PyLDDTResult::repr);

    // LDDTCalculator class
    py::class_<PyLDDTCalculator>(m, "LDDTCalculator", R"pbdoc(
        LDDT (Local Distance Difference Test) calculator.

        Computes local structural similarity between two aligned protein structures.
        LDDT measures how well local distances are preserved in an alignment.

        Examples
        --------
        >>> from pyfoldseek import Structure, LDDTCalculator
        >>> s1 = Structure.from_file("protein1.pdb")
        >>> s2 = Structure.from_file("protein2.pdb")
        >>> calculator = LDDTCalculator()
        >>> # Assume we have an alignment CIGAR string
        >>> result = calculator.compute_lddt(s1.ca_coords, s2.ca_coords, "MMMM")
        >>> print(f"Average LDDT: {result.average:.3f}")
        >>> print(f"Per-residue LDDT: {result.per_residue}")
    )pbdoc")
        .def(py::init<unsigned int, unsigned int>(),
             py::arg("max_query_len") = 50000,
             py::arg("max_target_len") = 50000,
             R"pbdoc(
            Initialize LDDTCalculator.

            Parameters
            ----------
            max_query_len : int, optional
                Maximum query sequence length to support (default: 50000)
            max_target_len : int, optional
                Maximum target sequence length to support (default: 50000)
        )pbdoc")
        .def("compute_lddt", &PyLDDTCalculator::compute_lddt,
             py::arg("query_ca"), py::arg("target_ca"),
             py::arg("alignment"),
             py::arg("query_start") = 0,
             py::arg("target_start") = 0,
             R"pbdoc(
            Compute LDDT score between two aligned structures.

            Parameters
            ----------
            query_ca : numpy.ndarray
                Query CA coordinates (N1, 3)
            target_ca : numpy.ndarray
                Target CA coordinates (N2, 3)
            alignment : str
                CIGAR string representing alignment (M=match, I=insertion, D=deletion)
            query_start : int, optional
                Start position in query sequence (default: 0)
            target_start : int, optional
                Start position in target sequence (default: 0)

            Returns
            -------
            LDDTResult
                LDDT result with average score and per-residue scores

            Notes
            -----
            LDDT score ranges from 0 to 1, where 1 indicates perfect local structure preservation.
            The score is based on preservation of distances within a 15 Angstrom radius.
        )pbdoc")
        .def("__repr__", &PyLDDTCalculator::repr);

    // Convenience function for LDDT
    m.def("compute_lddt", &compute_lddt,
          py::arg("ca1"), py::arg("ca2"),
          py::arg("alignment"),
          py::arg("query_start") = 0,
          py::arg("target_start") = 0,
          R"pbdoc(
        Compute LDDT score between two aligned structures.

        Convenience function that creates an LDDTCalculator and computes the score.

        Parameters
        ----------
        ca1 : numpy.ndarray
            First structure CA coordinates (N1, 3)
        ca2 : numpy.ndarray
            Second structure CA coordinates (N2, 3)
        alignment : str
            CIGAR string representing alignment (M=match, I=insertion, D=deletion)
        query_start : int, optional
            Start position in query (default: 0)
        target_start : int, optional
            Start position in target (default: 0)

        Returns
        -------
        LDDTResult
            LDDT result with average score and per-residue scores

        Examples
        --------
        >>> from pyfoldseek import Structure, compute_lddt
        >>> s1 = Structure.from_file("protein1.pdb")
        >>> s2 = Structure.from_file("protein2.pdb")
        >>> # Simple alignment where all residues match
        >>> alignment = "M" * min(len(s1.sequence), len(s2.sequence))
        >>> result = compute_lddt(s1.ca_coords, s2.ca_coords, alignment)
        >>> print(f"Average LDDT: {result.average:.3f}")
    )pbdoc");
}
