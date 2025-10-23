#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>
#include <stdexcept>

// IMPORTANT: Redefine EXIT macro to throw exception instead of calling exit()
// This prevents DBReader from terminating the Python interpreter
#ifdef EXIT
#undef EXIT
#endif
#define EXIT(code) throw std::runtime_error(std::string("DBReader EXIT with code ") + std::to_string(code))

#include "DBReader.h"
#include "Parameters.h"
#include "Util.h"
#include "FileUtil.h"
#include "structureto3di.h"
#include "Coordinate16.h"
#include "TMaligner.h"

namespace py = pybind11;

/**
 * Database Entry - represents a single entry from a Foldseek database
 */
class PyDatabaseEntry {
public:
    PyDatabaseEntry() = default;

    PyDatabaseEntry(unsigned int key, const std::string& name,
                    const std::string& sequence, const std::string& seq_3di,
                    const std::vector<float>& ca_coords,
                    size_t internal_id)
        : key_(key), name_(name), sequence_(sequence), seq_3di_(seq_3di),
          ca_coords_(ca_coords), internal_id_(internal_id) {}

    unsigned int get_key() const { return key_; }
    std::string get_name() const { return name_; }
    std::string get_sequence() const { return sequence_; }
    std::string get_seq_3di() const { return seq_3di_; }
    size_t get_internal_id() const { return internal_id_; }

    py::array_t<float> get_ca_coords() const {
        if (ca_coords_.empty()) {
            return py::array_t<float>();
        }

        size_t num_residues = ca_coords_.size() / 3;
        std::vector<py::ssize_t> shape = {static_cast<py::ssize_t>(num_residues), 3};
        auto result = py::array_t<float>(shape);
        auto buf = result.mutable_unchecked<2>();

        for (size_t i = 0; i < num_residues; i++) {
            buf(i, 0) = ca_coords_[i * 3 + 0];
            buf(i, 1) = ca_coords_[i * 3 + 1];
            buf(i, 2) = ca_coords_[i * 3 + 2];
        }

        return result;
    }

    int get_length() const { return sequence_.length(); }

    std::string repr() const {
        return "<DatabaseEntry key=" + std::to_string(key_) +
               " name='" + name_ + "' length=" + std::to_string(sequence_.length()) + ">";
    }

private:
    unsigned int key_;
    std::string name_;
    std::string sequence_;
    std::string seq_3di_;
    std::vector<float> ca_coords_;
    size_t internal_id_;
};


/**
 * Database Reader - wraps DBReader<unsigned int> for Python
 */
class PyDatabase {
public:
    PyDatabase(const std::string& db_path, int threads = 1)
        : db_path_(db_path), threads_(threads), reader_(nullptr),
          header_reader_(nullptr), ca_reader_(nullptr) {

        // Check if database exists
        std::string data_file = db_path_;
        std::string index_file = db_path_ + ".index";

        if (!FileUtil::fileExists(data_file.c_str())) {
            throw std::runtime_error("Database data file not found: " + data_file);
        }
        if (!FileUtil::fileExists(index_file.c_str())) {
            throw std::runtime_error("Database index file not found: " + index_file);
        }

        // Open main database reader
        try {
            // Note: Don't use USE_LOOKUP initially to avoid errors
            reader_ = new DBReader<unsigned int>(
                data_file.c_str(),
                index_file.c_str(),
                threads_,
                DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA
            );

        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to create DBReader: " + std::string(e.what()));
        }

        try {
            reader_->open(DBReader<unsigned int>::NOSORT);
        } catch (const std::exception& e) {
            delete reader_;
            reader_ = nullptr;
            throw std::runtime_error("Failed to open database: " + std::string(e.what()));
        }

        // Try to open header database (optional)
        std::string header_data = db_path_ + "_h";
        std::string header_index = db_path_ + "_h.index";
        if (FileUtil::fileExists(header_data.c_str()) && FileUtil::fileExists(header_index.c_str())) {
            try {
                header_reader_ = new DBReader<unsigned int>(
                    header_data.c_str(),
                    header_index.c_str(),
                    threads_,
                    DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA
                );
                header_reader_->open(DBReader<unsigned int>::NOSORT);
            } catch (const std::exception& e) {
                // Header database is optional, skip if it fails
                if (header_reader_) {
                    delete header_reader_;
                    header_reader_ = nullptr;
                }
            }
        }

        // Try to open CA coordinate database (optional)
        std::string ca_data = db_path_ + "_ca";
        std::string ca_index = db_path_ + "_ca.index";
        if (FileUtil::fileExists(ca_data.c_str()) && FileUtil::fileExists(ca_index.c_str())) {
            try {
                ca_reader_ = new DBReader<unsigned int>(
                    ca_data.c_str(),
                    ca_index.c_str(),
                    threads_,
                    DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA
                );
                ca_reader_->open(DBReader<unsigned int>::NOSORT);
            } catch (const std::exception& e) {
                // CA database is optional, skip if it fails
                if (ca_reader_) {
                    delete ca_reader_;
                    ca_reader_ = nullptr;
                }
            }
        }

        // Try to open 3Di sequence database (optional)
        std::string ss_data = db_path_ + "_ss";
        std::string ss_index = db_path_ + "_ss.index";
        if (FileUtil::fileExists(ss_data.c_str()) && FileUtil::fileExists(ss_index.c_str())) {
            try {
                ss_reader_ = new DBReader<unsigned int>(
                    ss_data.c_str(),
                    ss_index.c_str(),
                    threads_,
                    DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA
                );
                ss_reader_->open(DBReader<unsigned int>::NOSORT);
            } catch (const std::exception& e) {
                // 3Di database is optional, skip if it fails
                if (ss_reader_) {
                    delete ss_reader_;
                    ss_reader_ = nullptr;
                }
            }
        }
    }

    ~PyDatabase() {
        if (reader_) {
            reader_->close();
            delete reader_;
        }
        if (header_reader_) {
            header_reader_->close();
            delete header_reader_;
        }
        if (ca_reader_) {
            ca_reader_->close();
            delete ca_reader_;
        }
        if (ss_reader_) {
            ss_reader_->close();
            delete ss_reader_;
        }
    }

    size_t size() const {
        if (!reader_) return 0;
        return reader_->getSize();
    }

    PyDatabaseEntry get_by_index(size_t idx) const {
        if (!reader_) {
            throw std::runtime_error("Database not open");
        }
        if (idx >= reader_->getSize()) {
            throw std::out_of_range("Database index out of range: " + std::to_string(idx) +
                                   " >= " + std::to_string(reader_->getSize()));
        }

        return get_entry_at_index(idx);
    }

    PyDatabaseEntry get_by_key(unsigned int key) const {
        if (!reader_) {
            throw std::runtime_error("Database not open");
        }

        size_t idx = reader_->getId(key);
        if (idx == UINT_MAX) {
            throw std::runtime_error("Key not found in database: " + std::to_string(key));
        }

        return get_entry_at_index(idx);
    }

    std::vector<unsigned int> get_keys() const {
        if (!reader_) {
            throw std::runtime_error("Database not open");
        }

        std::vector<unsigned int> keys;
        keys.reserve(reader_->getSize());

        for (size_t i = 0; i < reader_->getSize(); i++) {
            keys.push_back(reader_->getDbKey(i));
        }

        return keys;
    }

    std::string repr() const {
        return "<Database path='" + db_path_ + "' size=" + std::to_string(size()) + ">";
    }

private:
    PyDatabaseEntry get_entry_at_index(size_t idx) const {
        unsigned int key = reader_->getDbKey(idx);

        // Get sequence
        char* data = reader_->getData(idx, 0);
        size_t seq_len = reader_->getSeqLen(idx);
        std::string sequence(data, seq_len);

        // Get header/name
        std::string name;
        if (header_reader_ && idx < header_reader_->getSize()) {
            char* header_data = header_reader_->getData(idx, 0);
            size_t header_len = header_reader_->getSeqLen(idx);
            name = std::string(header_data, header_len);
            // Remove trailing newlines
            while (!name.empty() && (name.back() == '\n' || name.back() == '\r')) {
                name.pop_back();
            }
        } else {
            // Use key as name if no header available
            name = std::to_string(key);
        }

        // Get 3Di sequence
        std::string seq_3di;
        if (ss_reader_ && idx < ss_reader_->getSize()) {
            char* ss_data = ss_reader_->getData(idx, 0);
            size_t ss_len = ss_reader_->getSeqLen(idx);
            seq_3di = std::string(ss_data, ss_len);
        }

        // Get CA coordinates
        std::vector<float> ca_coords;
        if (ca_reader_ && idx < ca_reader_->getSize()) {
            char* ca_data = ca_reader_->getData(idx, 0);
            size_t ca_data_len = ca_reader_->getEntryLen(idx);
            size_t num_residues = seq_len;

            // Decode compressed coordinates using Coordinate16
            Coordinate16 decoder;
            float* coords = decoder.read(ca_data, num_residues, ca_data_len);

            // Copy to vector (coords is x0, x1, ..., xN, y0, y1, ..., yN, z0, z1, ..., zN format)
            ca_coords.reserve(num_residues * 3);
            for (size_t i = 0; i < num_residues; i++) {
                ca_coords.push_back(coords[i]);                     // x
                ca_coords.push_back(coords[i + num_residues]);       // y
                ca_coords.push_back(coords[i + 2 * num_residues]);   // z
            }
        }

        return PyDatabaseEntry(key, name, sequence, seq_3di, ca_coords, idx);
    }

    std::string db_path_;
    int threads_;
    DBReader<unsigned int>* reader_;
    DBReader<unsigned int>* header_reader_;
    DBReader<unsigned int>* ca_reader_;
    DBReader<unsigned int>* ss_reader_;
};


/**
 * Search Hit - represents a single search result
 */
class PySearchHit {
public:
    PySearchHit() = default;

    PySearchHit(unsigned int target_key, const std::string& target_name,
                float tmscore, float rmsd, int alignment_length,
                float query_coverage, float target_coverage,
                const std::string& alignment)
        : target_key_(target_key), target_name_(target_name),
          tmscore_(tmscore), rmsd_(rmsd), alignment_length_(alignment_length),
          query_coverage_(query_coverage), target_coverage_(target_coverage),
          alignment_(alignment) {}

    unsigned int get_target_key() const { return target_key_; }
    std::string get_target_name() const { return target_name_; }
    float get_tmscore() const { return tmscore_; }
    float get_rmsd() const { return rmsd_; }
    int get_alignment_length() const { return alignment_length_; }
    float get_query_coverage() const { return query_coverage_; }
    float get_target_coverage() const { return target_coverage_; }
    std::string get_alignment() const { return alignment_; }

    std::string repr() const {
        char buf[256];
        snprintf(buf, sizeof(buf), "<SearchHit target='%s' TM-score=%.3f RMSD=%.2f alnlen=%d>",
                 target_name_.c_str(), tmscore_, rmsd_, alignment_length_);
        return std::string(buf);
    }

private:
    unsigned int target_key_;
    std::string target_name_;
    float tmscore_;
    float rmsd_;
    int alignment_length_;
    float query_coverage_;
    float target_coverage_;
    std::string alignment_;
};


/**
 * Perform structure search using TM-align
 */
std::vector<PySearchHit> search(
    py::array_t<double> query_ca,
    const std::string& query_sequence,
    PyDatabase& database,
    float tmscore_threshold = 0.5,
    float coverage_threshold = 0.0,
    int max_hits = 1000
) {
    // Validate input
    if (query_ca.ndim() != 2 || query_ca.shape(1) != 3) {
        throw std::invalid_argument("query_ca must be an (N, 3) array");
    }

    size_t query_len = query_ca.shape(0);
    if (query_sequence.length() != query_len) {
        throw std::invalid_argument("query_sequence length must match query_ca length");
    }

    // Convert query coordinates to separate x, y, z arrays
    auto query_buf = query_ca.unchecked<2>();
    float* query_x = new float[query_len];
    float* query_y = new float[query_len];
    float* query_z = new float[query_len];
    char* query_seq_arr = new char[query_len + 1];

    for (size_t i = 0; i < query_len; i++) {
        query_x[i] = query_buf(i, 0);
        query_y[i] = query_buf(i, 1);
        query_z[i] = query_buf(i, 2);
        query_seq_arr[i] = query_sequence[i];
    }
    query_seq_arr[query_len] = '\0';

    // Create TM-aligner (estimate max length as 2x query)
    size_t max_len = query_len * 2;
    if (max_len < 1000) max_len = 1000;
    TMaligner tmaligner(max_len, true, false, false);

    // Initialize query (once before loop)
    tmaligner.initQuery(query_x, query_y, query_z, query_seq_arr, query_len);

    // Results vector
    std::vector<PySearchHit> hits;

    // Iterate over database
    size_t db_size = database.size();
    for (size_t i = 0; i < db_size; i++) {
        // Get database entry
        PyDatabaseEntry entry = database.get_by_index(i);

        // Get target coordinates
        py::array_t<float> target_ca = entry.get_ca_coords();
        if (target_ca.size() == 0) {
            continue;  // Skip entries without coordinates
        }

        size_t target_len = target_ca.shape(0);

        // Convert target coordinates to separate x, y, z arrays
        auto target_buf = target_ca.unchecked<2>();
        float* target_x = new float[target_len];
        float* target_y = new float[target_len];
        float* target_z = new float[target_len];
        char* target_seq_arr = new char[target_len + 1];

        for (size_t j = 0; j < target_len; j++) {
            target_x[j] = target_buf(j, 0);
            target_y[j] = target_buf(j, 1);
            target_z[j] = target_buf(j, 2);
        }
        std::copy(entry.get_sequence().begin(), entry.get_sequence().end(), target_seq_arr);
        target_seq_arr[target_len] = '\0';

        try {
            // Perform TM-align
            float tm_score_value;
            Matcher::result_t aln_result = tmaligner.align(
                entry.get_key(),
                target_x, target_y, target_z,
                target_seq_arr,
                target_len,
                tm_score_value
            );

            // Check thresholds
            if (tm_score_value >= tmscore_threshold) {
                int alignment_length = aln_result.qEndPos - aln_result.qStartPos;
                float query_cov = static_cast<float>(alignment_length) / query_len;
                float target_cov = static_cast<float>(alignment_length) / target_len;

                if (query_cov >= coverage_threshold && target_cov >= coverage_threshold) {
                    // Get TM-score result for RMSD
                    TMaligner::TMscoreResult tm_result = tmaligner.computeTMscore(
                        target_x, target_y, target_z,
                        target_len,
                        aln_result.qStartPos,
                        aln_result.dbStartPos,
                        aln_result.backtrace,
                        TMaligner::normalization(0, alignment_length, query_len, target_len)
                    );

                    PySearchHit hit(
                        entry.get_key(),
                        entry.get_name(),
                        tm_score_value,
                        tm_result.rmsd,
                        alignment_length,
                        query_cov,
                        target_cov,
                        aln_result.backtrace
                    );
                    hits.push_back(hit);
                }
            }
        } catch (const std::exception& e) {
            // Skip entries that fail alignment, but still clean up
        }

        // Clean up target arrays (always executed)
        delete[] target_x;
        delete[] target_y;
        delete[] target_z;
        delete[] target_seq_arr;
    }

    // Sort by TM-score (descending)
    std::sort(hits.begin(), hits.end(), [](const PySearchHit& a, const PySearchHit& b) {
        return a.get_tmscore() > b.get_tmscore();
    });

    // Limit to max_hits
    if (hits.size() > static_cast<size_t>(max_hits)) {
        hits.resize(max_hits);
    }

    // Clean up query arrays
    delete[] query_x;
    delete[] query_y;
    delete[] query_z;
    delete[] query_seq_arr;

    return hits;
}


/**
 * Python module initialization
 */
void init_database(py::module &m) {
    py::class_<PyDatabaseEntry>(m, "DatabaseEntry",
        R"pbdoc(
        Single entry from a Foldseek database.

        Attributes
        ----------
        key : int
            Database key (unsigned int)
        name : str
            Entry name/header
        sequence : str
            Amino acid sequence
        seq_3di : str
            3Di structural alphabet sequence
        length : int
            Sequence length
        internal_id : int
            Internal index in database
        ca_coords : numpy.ndarray
            CA coordinates as (N, 3) array
        )pbdoc")
        .def(py::init<>())
        .def_property_readonly("key", &PyDatabaseEntry::get_key)
        .def_property_readonly("name", &PyDatabaseEntry::get_name)
        .def_property_readonly("sequence", &PyDatabaseEntry::get_sequence)
        .def_property_readonly("seq_3di", &PyDatabaseEntry::get_seq_3di)
        .def_property_readonly("length", &PyDatabaseEntry::get_length)
        .def_property_readonly("internal_id", &PyDatabaseEntry::get_internal_id)
        .def_property_readonly("ca_coords", &PyDatabaseEntry::get_ca_coords)
        .def("__repr__", &PyDatabaseEntry::repr);

    py::class_<PyDatabase>(m, "Database",
        R"pbdoc(
        Foldseek database reader.

        Opens and reads Foldseek structure databases created with `foldseek createdb`.

        Parameters
        ----------
        db_path : str
            Path to database (without extension)
        threads : int, optional
            Number of threads for parallel access (default: 1)

        Examples
        --------
        >>> db = Database("/path/to/pdb")
        >>> print(len(db))
        >>> entry = db[0]
        >>> print(entry.sequence)
        >>> print(entry.seq_3di)

        >>> # Iterate over database
        >>> for entry in db:
        ...     print(entry.name, entry.length)

        >>> # Access by key
        >>> entry = db.get(12345)
        )pbdoc")
        .def(py::init<const std::string&, int>(),
             py::arg("db_path"),
             py::arg("threads") = 1)
        .def("__len__", &PyDatabase::size)
        .def("__getitem__", &PyDatabase::get_by_index,
             py::arg("index"),
             "Get entry by index")
        .def("get", &PyDatabase::get_by_key,
             py::arg("key"),
             "Get entry by database key")
        .def("keys", &PyDatabase::get_keys,
             "Get all database keys")
        .def("__repr__", &PyDatabase::repr);

    py::class_<PySearchHit>(m, "SearchHit",
        R"pbdoc(
        Single structure search result.

        Attributes
        ----------
        target_key : int
            Database key of the target structure
        target_name : str
            Name of the target structure
        tmscore : float
            TM-score (normalized by query length)
        rmsd : float
            Root mean square deviation (Å)
        alignment_length : int
            Number of aligned residues
        query_coverage : float
            Fraction of query aligned (0.0-1.0)
        target_coverage : float
            Fraction of target aligned (0.0-1.0)
        alignment : str
            Alignment string (CIGAR-like format)
        )pbdoc")
        .def(py::init<>())
        .def_property_readonly("target_key", &PySearchHit::get_target_key)
        .def_property_readonly("target_name", &PySearchHit::get_target_name)
        .def_property_readonly("tmscore", &PySearchHit::get_tmscore)
        .def_property_readonly("rmsd", &PySearchHit::get_rmsd)
        .def_property_readonly("alignment_length", &PySearchHit::get_alignment_length)
        .def_property_readonly("query_coverage", &PySearchHit::get_query_coverage)
        .def_property_readonly("target_coverage", &PySearchHit::get_target_coverage)
        .def_property_readonly("alignment", &PySearchHit::get_alignment)
        .def("__repr__", &PySearchHit::repr);

    m.def("search", &search,
        py::arg("query_ca"),
        py::arg("query_sequence"),
        py::arg("database"),
        py::arg("tmscore_threshold") = 0.5,
        py::arg("coverage_threshold") = 0.0,
        py::arg("max_hits") = 1000,
        R"pbdoc(
        Search a query structure against a database using TM-align.

        This is a simplified search that performs all-vs-all TM-align comparisons.
        For large-scale searches, consider using the Foldseek CLI which uses
        optimized prefiltering.

        Parameters
        ----------
        query_ca : numpy.ndarray
            Query CA coordinates as (N, 3) array
        query_sequence : str
            Query amino acid sequence
        database : Database
            Target database to search against
        tmscore_threshold : float, optional
            Minimum TM-score to report (default: 0.5)
        coverage_threshold : float, optional
            Minimum query/target coverage (default: 0.0)
        max_hits : int, optional
            Maximum number of hits to return (default: 1000)

        Returns
        -------
        list of SearchHit
            Search results sorted by TM-score (descending)

        Examples
        --------
        >>> from pyfoldseek import Structure, Database, search
        >>>
        >>> # Load query structure
        >>> query = Structure.from_file("query.pdb")
        >>>
        >>> # Open database
        >>> db = Database("/path/to/database")
        >>>
        >>> # Search
        >>> hits = search(query.ca_coords, query.sequence, db,
        ...               tmscore_threshold=0.6, max_hits=100)
        >>>
        >>> # Print top hits
        >>> for hit in hits[:10]:
        ...     print(f"{hit.target_name}: TM-score={hit.tmscore:.3f}")
        )pbdoc");
}
