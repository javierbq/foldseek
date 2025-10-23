#include <pybind11/pybind11.h>

namespace py = pybind11;

// Forward declarations of init functions from other files
void init_structure(py::module &m);

PYBIND11_MODULE(pyfoldseek, m) {
    m.doc() = R"pbdoc(
        Foldseek Python Bindings
        ------------------------

        Python bindings for Foldseek - fast and accurate protein structure search.

        Main features:
        - Convert PDB/mmCIF files to 3Di structural alphabet
        - Perform structural alignments (TM-align, Smith-Waterman)
        - Calculate structural similarity scores
        - Read and write foldseek databases

        Example:
            >>> from pyfoldseek import Structure
            >>> struct = Structure.from_file("protein.pdb")
            >>> print(struct.seq_3di)
    )pbdoc";

    // Add version
    m.attr("__version__") = "0.1.0";

    // Initialize submodules
    init_structure(m);

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
