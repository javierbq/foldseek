# Foldseek Python Bindings - Implementation Plan

**Goal**: Create Python bindings for Foldseek with focus on PDB → 3Di conversion

**Start Date**: 2025-10-23
**Target**: Production-ready MVP in 2-3 weeks

---

## 🎯 Success Criteria

1. **Functionality**: Convert any PDB/mmCIF file to 3Di sequence
2. **Performance**: <5% overhead vs C++ implementation
3. **Usability**: One-liner API for basic use case
4. **Compatibility**: Works with Python 3.7+, Linux/macOS/Windows
5. **Quality**: 90%+ test coverage, comprehensive docs

---

## 📋 Phase 0: Minimal Viable Product (MVP)

**Timeline**: 2-3 days
**Goal**: Basic PDB → 3Di conversion working

### Core Features
```python
from pyfoldseek import Structure

struct = Structure.from_file("protein.pdb")
print(struct.sequence)      # Amino acid sequence
print(struct.seq_3di)       # 3Di structural alphabet
print(struct.length)        # Number of residues
```

### Implementation Tasks

#### 1. Project Structure Setup
- [x] Create `python/` directory
- [ ] Add pybind11 as git submodule or dependency
- [ ] Create directory structure:
  ```
  python/
  ├── CMakeLists.txt
  ├── setup.py
  ├── pyproject.toml
  ├── src/
  │   ├── bindings.cpp
  │   └── structure_wrapper.cpp
  ├── pyfoldseek/
  │   ├── __init__.py
  │   └── _version.py
  └── tests/
      └── test_structure.py
  ```

#### 2. Build System Configuration
- [ ] CMakeLists.txt with pybind11 integration
- [ ] Link against existing foldseek libraries:
  - gemmiwrapper
  - foldcomp
  - 3di library
- [ ] setup.py for pip installation
- [ ] pyproject.toml for modern Python packaging

#### 3. Core Bindings Implementation
- [ ] Wrap `Vec3` struct for coordinates
- [ ] Wrap `GemmiWrapper` class:
  - `load()` method
  - `ca`, `n`, `c`, `cb` coordinate vectors
  - `ami` (amino acid sequence)
  - `chainNames`
- [ ] Wrap `StructureTo3Di` class:
  - `structure2states()` method
  - Convert coordinates to 3Di alphabet
- [ ] Create Python `Structure` class that combines both

#### 4. Type Conversions
- [ ] std::vector<Vec3> ↔ NumPy arrays
- [ ] std::vector<char> ↔ Python strings
- [ ] std::string ↔ Python str
- [ ] Error handling (C++ exceptions → Python exceptions)

#### 5. Testing
- [ ] Unit test: Load example PDB (1tim.pdb)
- [ ] Validate 3Di sequence matches CLI output
- [ ] Test multi-chain structures
- [ ] Test error handling (invalid files)
- [ ] Benchmark performance vs CLI

#### 6. Documentation
- [ ] README.md with installation instructions
- [ ] Basic usage examples
- [ ] API reference (docstrings)

### Dependencies
- **Build**: CMake ≥3.15, C++17 compiler, pybind11 ≥2.10
- **Runtime**: Python ≥3.7, NumPy ≥1.20

### MVP API Design
```python
class Structure:
    """Represents a protein structure with 3Di encoding"""

    @staticmethod
    def from_file(filename: str) -> Structure:
        """Load structure from PDB/mmCIF/Foldcomp file"""

    @property
    def sequence(self) -> str:
        """Amino acid sequence (one-letter code)"""

    @property
    def seq_3di(self) -> str:
        """3Di structural alphabet sequence"""

    @property
    def length(self) -> int:
        """Number of residues"""

    @property
    def ca_coords(self) -> np.ndarray:
        """C-alpha coordinates (N, 3)"""

    @property
    def chain_names(self) -> List[str]:
        """Chain identifiers"""
```

---

## 📋 Phase 1: Enhanced Functionality

**Timeline**: 3-4 days
**Goal**: Complete structure processing pipeline

### Features to Add

#### 1. Multi-Chain Support
```python
struct = Structure.from_file("complex.pdb")
for chain in struct.chains:
    print(f"{chain.name}: {chain.seq_3di}")
```

#### 2. Direct Coordinate Conversion
```python
import numpy as np
from pyfoldseek import coords_to_3di

ca = np.array([[1.0, 2.0, 3.0], ...])  # (N, 3)
n = np.array([...])
c = np.array([...])
cb = np.array([...])

seq_3di = coords_to_3di(ca, n, c, cb)
```

#### 3. Format Support
```python
Structure.from_pdb("file.pdb")
Structure.from_mmcif("file.cif")
Structure.from_foldcomp("file.fcz")
```

#### 4. Backbone Reconstruction
```python
# Rebuild N, C, CB from CA-only structures
struct = Structure.from_file("ca_only.pdb", reconstruct_backbone=True)
```

#### 5. Batch Processing
```python
from pyfoldseek import batch_convert

files = ["p1.pdb", "p2.pdb", "p3.pdb"]
results = batch_convert(files, num_threads=4)
```

#### 6. Intermediate Features
```python
struct = Structure.from_file("1tim.pdb", compute_features=True)
print(struct.features.shape)      # (N, 10) geometric features
print(struct.embeddings.shape)    # (N, 2) NN embeddings
```

### Implementation Tasks
- [ ] Expose PulchraWrapper for backbone reconstruction
- [ ] Add multi-chain iterator
- [ ] Implement batch processing with threading
- [ ] Expose Feature and Embedding classes
- [ ] Add more comprehensive tests

---

## 📋 Phase 2: Alignment & Scoring

**Timeline**: 4-5 days
**Goal**: TM-align and structure alignment capabilities

### Features

#### 1. TM-score Calculation
```python
from pyfoldseek import TMAligner

s1 = Structure.from_file("protein1.pdb")
s2 = Structure.from_file("protein2.pdb")

aligner = TMAligner()
result = aligner.align(s1, s2)

print(f"TM-score: {result.tmscore:.3f}")
print(f"RMSD: {result.rmsd:.3f}")
print(f"Rotation matrix:\n{result.rotation_matrix}")  # (3, 3)
print(f"Translation vector: {result.translation}")     # (3,)
```

#### 2. Local Alignment (Smith-Waterman)
```python
from pyfoldseek import StructureAligner

aligner = StructureAligner()
alignment = aligner.align(s1.seq_3di, s2.seq_3di)

print(f"Score: {alignment.score}")
print(f"Identity: {alignment.identity}%")
print(f"Query coverage: {alignment.query_coverage:.2f}")
print(f"CIGAR: {alignment.cigar}")
```

#### 3. LDDT Calculation
```python
from pyfoldseek import compute_lddt

lddt = compute_lddt(alignment, s1.ca_coords, s2.ca_coords)
print(f"Average LDDT: {lddt.average:.3f}")
print(f"Per-residue LDDT: {lddt.per_residue}")  # NumPy array
```

### Implementation Tasks
- [ ] Wrap TMaligner class
- [ ] Wrap TMscoreResult struct
- [ ] Wrap StructureSmithWaterman
- [ ] Wrap s_align result structure
- [ ] Wrap LDDTCalculator
- [ ] Add rotation/translation matrix support
- [ ] Handle CIGAR strings
- [ ] Comprehensive alignment tests

---

## 📋 Phase 3: Database & Advanced Features

**Timeline**: 3-4 days
**Goal**: Database reading and clustering support

### Features

#### 1. Database Reading
```python
from pyfoldseek import Database

# Read pre-computed foldseek database
db = Database.open("path/to/afdb50")

# Get structure by ID
struct = db.get_structure("AF-P12345-F1")

# Iterate over database
for entry in db:
    print(f"{entry.id}: {entry.seq_3di}")
```

#### 2. Clustering
```python
from pyfoldseek import cluster_structures

structures = [s1, s2, s3, s4, s5]
clusters = cluster_structures(structures, min_seq_id=0.3)

for cluster_id, members in clusters.items():
    print(f"Cluster {cluster_id}: {len(members)} members")
```

#### 3. ProstT5 Integration (Optional)
```python
from pyfoldseek import predict_3di_from_sequence

# Predict 3Di from amino acid sequence
seq_3di = predict_3di_from_sequence("MKTAYIAKQRQIS...")
```

### Implementation Tasks
- [ ] Wrap DBReader/DBWriter
- [ ] Database iterator support
- [ ] Clustering algorithm bindings
- [ ] Optional ProstT5 wrapper
- [ ] Memory-efficient streaming

---

## 📋 Phase 4: Polish & Release

**Timeline**: 4-5 days
**Goal**: Production-ready release

### Tasks

#### 1. Documentation
- [ ] Comprehensive README.md
- [ ] Installation guide (pip, conda, from source)
- [ ] API reference (Sphinx)
- [ ] Tutorial notebooks:
  - Basic PDB → 3Di conversion
  - Structure alignment
  - AlphaFold database analysis
  - MD trajectory analysis
- [ ] Contributing guide
- [ ] Changelog

#### 2. Packaging
- [ ] Build wheels for Linux (manylinux)
- [ ] Build wheels for macOS (universal2)
- [ ] Build wheels for Windows
- [ ] PyPI release setup
- [ ] Conda package (conda-forge)
- [ ] Docker container with examples

#### 3. Testing & CI
- [ ] GitHub Actions workflow
- [ ] Test matrix (Python 3.7, 3.8, 3.9, 3.10, 3.11)
- [ ] Platform testing (Linux, macOS, Windows)
- [ ] Code coverage reporting (codecov)
- [ ] Linting (flake8, black)
- [ ] Type checking (mypy)

#### 4. Performance
- [ ] Benchmark suite
- [ ] Profile hot paths
- [ ] Release GIL for long operations
- [ ] Parallel processing support
- [ ] Memory optimization

#### 5. Examples & Demos
- [ ] Jupyter notebooks in `examples/`
- [ ] Command-line scripts
- [ ] Integration examples:
  - With Biopython
  - With MDAnalysis
  - With AlphaFold predictions
  - With PyMOL

---

## 🛠️ Technical Implementation Details

### Directory Structure (Final)
```
foldseek/
├── python/
│   ├── CMakeLists.txt              # Build configuration
│   ├── setup.py                     # Setuptools config
│   ├── pyproject.toml              # Modern Python packaging
│   ├── README.md                    # Python bindings README
│   ├── MANIFEST.in                  # Package data files
│   │
│   ├── src/                         # C++ binding code
│   │   ├── bindings.cpp            # Main pybind11 module
│   │   ├── structure_wrapper.cpp   # Structure/3Di bindings
│   │   ├── alignment_wrapper.cpp   # TMaligner/SSW bindings
│   │   ├── database_wrapper.cpp    # Database bindings
│   │   └── utils.cpp               # Helper functions
│   │
│   ├── pyfoldseek/                 # Python package
│   │   ├── __init__.py
│   │   ├── _version.py
│   │   ├── structure.py            # Pure Python helpers
│   │   ├── alignment.py
│   │   ├── database.py
│   │   └── utils.py
│   │
│   ├── tests/                       # Test suite
│   │   ├── __init__.py
│   │   ├── test_structure.py
│   │   ├── test_3di_conversion.py
│   │   ├── test_alignment.py
│   │   ├── test_database.py
│   │   └── test_performance.py
│   │
│   ├── examples/                    # Example notebooks/scripts
│   │   ├── 01_basic_usage.ipynb
│   │   ├── 02_alignment.ipynb
│   │   ├── 03_alphafold_integration.ipynb
│   │   └── 04_batch_processing.py
│   │
│   ├── docs/                        # Sphinx documentation
│   │   ├── conf.py
│   │   ├── index.rst
│   │   ├── api.rst
│   │   └── tutorials/
│   │
│   └── .github/
│       └── workflows/
│           └── build-and-test.yml
```

### Build System Flow

1. **User runs**: `pip install .` or `pip install pyfoldseek`
2. **setup.py** invokes CMake to build C++ extension
3. **CMakeLists.txt** compiles bindings and links to foldseek libraries
4. **Extension module** (`pyfoldseek.so` or `pyfoldseek.pyd`) is installed
5. **Python package** wraps extension with pure Python helpers

### Key Design Decisions

#### Memory Management
- Use `py::return_value_policy::copy` for small objects
- Use `py::return_value_policy::reference_internal` for arrays owned by objects
- Wrap `std::vector` with `py::array_t` for NumPy zero-copy access

#### Performance
- Release GIL during CPU-intensive operations:
  ```cpp
  .def("structure2states", [](/*...*/) {
      py::gil_scoped_release release;
      return structure2states(...);
  })
  ```
- Use `py::array_t` with strides for efficient NumPy access
- Avoid unnecessary copies with move semantics

#### Error Handling
- Convert all C++ exceptions to Python exceptions
- Provide meaningful error messages
- Validate inputs early (Python side when possible)

#### Type Conversions
```cpp
// Automatic conversions provided by pybind11
std::string ↔ str
std::vector<T> ↔ list
std::map<K,V> ↔ dict

// Custom conversions needed
Vec3 → (x, y, z) tuple or NumPy array
std::vector<Vec3> → NumPy array (N, 3)
char* → str (handle memory ownership)
```

---

## 🧪 Testing Strategy

### Unit Tests
- Test each binding independently
- Validate type conversions
- Test error conditions
- Memory leak detection (valgrind)

### Integration Tests
- Compare Python output vs CLI
- Multi-chain structures
- Large files (performance)
- Edge cases (missing atoms, etc.)

### Validation Tests
```python
# Ensure 3Di matches CLI output
def test_3di_matches_cli():
    struct = Structure.from_file("1tim.pdb")

    # Run CLI
    os.system("foldseek createdb 1tim.pdb tmp/db")
    cli_3di = read_foldseek_db("tmp/db")

    # Compare
    assert struct.seq_3di == cli_3di
```

### Performance Benchmarks
- Conversion speed (structures/second)
- Memory usage (compared to CLI)
- Scalability (batch processing)

---

## 📦 Dependencies

### Build Dependencies
- CMake ≥ 3.15
- C++17 compiler (GCC ≥7, Clang ≥5, MSVC ≥2019)
- pybind11 ≥ 2.10
- Python development headers
- Existing foldseek build dependencies

### Runtime Dependencies
- Python ≥ 3.7
- NumPy ≥ 1.20

### Optional Dependencies
- Biopython (PDB I/O integration)
- MDAnalysis (trajectory analysis)
- matplotlib (visualization)
- pytest (testing)
- sphinx (documentation)

---

## 🚀 Release Strategy

### Version Numbering
- Follow foldseek version (e.g., foldseek 8.0 → pyfoldseek 8.0.0)
- Add patch version for binding-only changes

### Release Checklist
- [ ] All tests passing
- [ ] Documentation complete
- [ ] Wheels built for all platforms
- [ ] PyPI upload
- [ ] Conda package submitted
- [ ] GitHub release with notes
- [ ] Announce on foldseek discussions

### Distribution Channels
1. **PyPI**: `pip install pyfoldseek`
2. **Conda**: `conda install -c conda-forge pyfoldseek`
3. **Source**: `pip install git+https://github.com/steineggerlab/foldseek.git#subdirectory=python`

---

## 📊 Success Metrics

### Adoption
- 100+ pip downloads in first month
- 5+ GitHub stars on foldseek repo
- Active issues/discussions from users

### Quality
- 90%+ test coverage
- <5 open bugs
- Documentation for all public APIs
- Type hints for all Python code

### Performance
- 3Di conversion: <5% overhead vs CLI
- TM-align: <10% overhead vs CLI
- Memory usage: Similar to CLI

---

## 🎯 Milestones

### Week 1
- [ ] MVP working (PDB → 3Di)
- [ ] Basic tests passing
- [ ] Installation via pip

### Week 2
- [ ] Multi-chain support
- [ ] TM-align bindings
- [ ] Comprehensive tests
- [ ] Initial documentation

### Week 3
- [ ] Database reading
- [ ] Performance optimization
- [ ] Complete documentation
- [ ] Example notebooks

### Week 4
- [ ] Wheels for all platforms
- [ ] CI/CD setup
- [ ] PyPI/Conda release
- [ ] Community announcement

---

## 🤝 Community Engagement

### Before Starting
- [ ] Open GitHub issue to discuss approach
- [ ] Get maintainer buy-in
- [ ] Coordinate with foldseek team

### During Development
- [ ] Regular progress updates
- [ ] Request feedback on API design
- [ ] Share early prototypes

### After Release
- [ ] Tutorial blog post
- [ ] Social media announcement
- [ ] Help users with issues
- [ ] Incorporate feedback

---

## 📝 Notes & Considerations

### Challenges
1. **Neural network weights**: 3Di encoder needs model file
   - Solution: Bundle with package or download on first use
2. **Large dependencies**: Gemmi, foldcomp add size
   - Solution: Static linking where possible
3. **Windows build**: More complex than Linux/macOS
   - Solution: GitHub Actions with MSVC
4. **API stability**: foldseek is actively developed
   - Solution: Version pinning, follow semantic versioning

### Future Enhancements
- GPU support (if foldseek adds it)
- Streaming API for huge databases
- Async I/O for parallel file loading
- Cython optimization for hot Python paths
- CFFI alternative for PyPy compatibility

---

## 🔗 References

- [pybind11 documentation](https://pybind11.readthedocs.io/)
- [Foldseek paper](https://www.nature.com/articles/s41587-023-01773-0)
- [NumPy C API](https://numpy.org/doc/stable/reference/c-api/)
- [Python Packaging Guide](https://packaging.python.org/)

---

**Last Updated**: 2025-10-23
**Status**: Planning → Implementation
**Next Steps**: Set up project structure and implement MVP
