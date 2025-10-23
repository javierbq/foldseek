#include <pybind11/pybind11.h>
#include <vector>

namespace py = pybind11;

// Required global symbols that foldseek libraries expect
// These are normally defined in foldseek.cpp for the executable
const char* binary_name = "pyfoldseek";
const char* tool_name = "pyfoldseek";
const char* tool_introduction = "Python bindings for Foldseek";
const char* main_author = "Foldseek team";
const char* show_extended_help = "0";
const char* show_bash_info = nullptr;
const char* index_version_compatible = "fs1";
bool hide_base_commands = true;
bool hide_base_downloads = true;

// Command system symbols (declared extern, already defined in libraries)
struct Command;
extern std::vector<Command> baseCommands;
extern std::vector<Command> foldseekCommands;
void (*initCommands)(void) = nullptr;

// Parameter singleton initialization
// This is required by foldseek libraries but we don't need to actually initialize parameters
// for the Python bindings (we're not using the command system)
void initParameterSingleton() {
    // No-op for Python bindings
}

// Forward declarations of init functions from other files
void init_structure(py::module &m);
void init_alignment(py::module &m);

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

            >>> # TM-align example
            >>> from pyfoldseek import compute_tmscore
            >>> s1 = Structure.from_file("protein1.pdb")
            >>> s2 = Structure.from_file("protein2.pdb")
            >>> result = compute_tmscore(s1.ca_coords, s2.ca_coords, s1.sequence, s2.sequence)
            >>> print(f"TM-score: {result.tmscore:.3f}")
    )pbdoc";

    // Add version
    m.attr("__version__") = "0.2.0";

    // Initialize submodules
    init_structure(m);
    init_alignment(m);

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
