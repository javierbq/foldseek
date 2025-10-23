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
}
