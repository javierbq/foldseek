# PyFoldseek Implementation Status

**Created**: 2025-10-23
**Status**: MVP Complete (Not Yet Built)
**Phase**: 0 - Minimal Viable Product

## 📦 What Has Been Created

### Project Structure ✅
```
python/
├── CMakeLists.txt              # Build configuration
├── setup.py                     # pip installation
├── pyproject.toml              # Modern Python packaging
├── MANIFEST.in                  # Package data files
├── README.md                    # User documentation
├── IMPLEMENTATION_PLAN.md       # Detailed implementation plan
├── STATUS.md                    # This file
│
├── src/                         # C++ binding code
│   ├── bindings.cpp            # Main pybind11 module
│   └── structure_wrapper.cpp   # Structure/3Di bindings
│
├── pyfoldseek/                 # Python package
│   ├── __init__.py             # Package initialization
│   └── _version.py             # Version info
│
├── tests/                       # Test suite
│   └── test_structure.py       # Comprehensive tests
│
└── examples/                    # Example scripts
    └── basic_usage.py          # Usage demonstration
```

### Core Features Implemented ✅

1. **Structure Loading**
   - `Structure.from_file()` - Load PDB/mmCIF/Foldcomp files
   - Support for backbone reconstruction (CA-only structures)
   - Automatic format detection

2. **3Di Conversion**
   - Integrated `StructureTo3Di` converter
   - Direct access via `Structure.seq_3di` property
   - Standalone `coords_to_3di()` function for NumPy arrays

3. **Data Access**
   - Amino acid sequence: `Structure.sequence`
   - 3Di sequence: `Structure.seq_3di`
   - Coordinates as NumPy arrays: `ca_coords`, `n_coords`, `c_coords`, `cb_coords`
   - Metadata: `length`, `chain_names`, `filename`

4. **Type Conversions**
   - C++ `std::vector<Vec3>` ↔ NumPy `(N, 3)` arrays
   - Proper memory management with pybind11
   - Exception handling (C++ → Python)

## 🚧 What Needs to Be Done

### Immediate (To Test MVP)

1. **Fix CMakeLists.txt** ⚠️
   - Current version has incorrect library linking paths
   - Need to link against libraries from parent build directory
   - Update include paths to point to correct locations

2. **Build the Extension**
   ```bash
   cd /home/user/foldseek/python
   pip install -e .
   ```

3. **Run Tests**
   ```bash
   pytest tests/ -v
   ```

4. **Fix Any Build Errors**
   - Likely issues with library paths
   - May need to adjust include directories
   - Could need to statically link some libraries

### Updated CMakeLists.txt (Needed)

The current `CMakeLists.txt` needs to be updated to properly link against the main foldseek build. Here's what needs to change:

```cmake
# Should link against libraries from ../build/
target_link_libraries(pyfoldseek PRIVATE
    ${CMAKE_SOURCE_DIR}/../build/src/strucclustutils/libgemmiwrapper.a
    ${CMAKE_SOURCE_DIR}/../build/lib/foldcomp/libfoldcomp.a
    # ... other libraries from build directory
)
```

Or better: integrate into main CMakeLists.txt with an option:
```cmake
# In root CMakeLists.txt
option(BUILD_PYTHON "Build Python bindings" OFF)
if(BUILD_PYTHON)
    add_subdirectory(python)
endif()
```

## 📊 Testing Strategy

### Unit Tests Created ✅
- `TestVec3` - Test coordinate class
- `TestStructure` - Test structure loading and properties
- `TestCoordsTo3Di` - Test standalone conversion function
- `TestIntegration` - Compare with CLI output
- `TestPerformance` - Benchmark performance

### Tests to Run ⏳
1. Load example PDB (1tim.pdb.gz)
2. Verify 3Di sequence generation
3. Check coordinate extraction
4. Validate NumPy array shapes
5. Compare with CLI foldseek output

## 🔧 Known Issues & TODOs

### Build System
- [ ] Fix library linking in CMakeLists.txt
- [ ] Test on clean system (fresh build)
- [ ] Add Windows support (MSVC)
- [ ] Create wheel building workflow

### Features
- [ ] Multi-chain support (iterate over chains)
- [ ] Better error messages
- [ ] Progress callbacks for large files
- [ ] Memory optimization

### Documentation
- [ ] Add docstring examples
- [ ] Create Jupyter notebook tutorial
- [ ] API reference (Sphinx)
- [ ] Contribution guide

### Testing
- [ ] Test with various PDB formats
- [ ] Test with compressed files
- [ ] Test with malformed structures
- [ ] Performance benchmarks vs CLI

## 🎯 Next Steps (Priority Order)

### 1. Fix and Build (Critical)
```bash
# Fix CMakeLists.txt to properly find/link libraries
# Then try building:
cd /home/user/foldseek/python
mkdir build
cd build
cmake ..
make -j$(nproc)

# Or use pip:
cd /home/user/foldseek/python
pip install -e .
```

### 2. Test Basic Functionality
```python
from pyfoldseek import Structure
struct = Structure.from_file("../example/1tim.pdb.gz")
print(struct.seq_3di)
```

### 3. Fix Issues Found During Testing
- Likely: include path issues
- Likely: missing library dependencies
- Possible: symbol conflicts

### 4. Validate Against CLI
```bash
# Compare Python output with CLI
foldseek createdb example/1tim.pdb.gz tmp/db
foldseek lndb tmp/db_ss tmp/db_3di
foldseek convert2fasta tmp/db_3di output.fasta
# Compare with Python struct.seq_3di
```

### 5. Create More Examples
- [ ] Batch processing script
- [ ] AlphaFold integration example
- [ ] MD trajectory analysis
- [ ] Jupyter notebook walkthrough

## 📈 Success Metrics

### MVP Success Criteria
- [x] Code written and organized
- [ ] Extension builds successfully
- [ ] Tests pass
- [ ] 3Di output matches CLI
- [ ] Documentation complete

### Performance Targets
- [ ] <5% overhead vs CLI for 3Di conversion
- [ ] Memory usage similar to CLI
- [ ] NumPy array access without copies

## 🤝 Community Engagement

### Before Merging
1. **Test thoroughly** on different platforms
2. **Get feedback** from foldseek maintainers
3. **Write tutorial** blog post
4. **Create demo** Jupyter notebook

### GitHub Issue Template
```markdown
# Python Bindings for Foldseek

## Summary
Proposal to add Python bindings for Foldseek using pybind11, focusing initially on PDB → 3Di conversion.

## Motivation
- Enable Python users to leverage Foldseek's speed
- Integration with AlphaFold, MDAnalysis, Biopython
- Simplify structure analysis workflows

## Implementation
- pybind11-based bindings
- MVP: Structure loading + 3Di conversion
- Future: TM-align, clustering, database search

## Status
- Phase 0 (MVP) code complete
- Needs: Build testing, validation, feedback

## Questions
1. Interest in merging this into main repo?
2. Preferred integration approach?
3. Any design concerns?
```

## 📝 Design Decisions Log

### Why pybind11?
- Modern C++11/14 support
- Automatic type conversions
- Excellent NumPy integration
- Used by many scientific Python packages

### Why not SWIG/ctypes/Cython?
- SWIG: Outdated, verbose bindings
- ctypes: No type safety, manual memory management
- Cython: Additional language to learn, extra compilation step

### Why NumPy for coordinates?
- Standard in scientific Python
- Zero-copy access possible
- Compatible with other tools (MDAnalysis, Biopython)

### Why single-chain initially?
- Simpler API for MVP
- Most common use case
- Can iterate multiple times for multi-chain
- Phase 1 will add proper multi-chain support

## 🐛 Debugging Notes

### If build fails:
1. Check pybind11 submodule: `git submodule update --init`
2. Check Python development headers: `python3 -m pip install pybind11`
3. Check CMake version: `cmake --version` (need ≥3.15)
4. Verbose build: `pip install -e . -v`

### If import fails:
1. Check extension built: `ls pyfoldseek/*.so` or `*.pyd`
2. Check Python path: `import sys; print(sys.path)`
3. Check for undefined symbols: `ldd pyfoldseek*.so`

### If 3Di doesn't match CLI:
1. Check model file loaded correctly
2. Verify coordinate arrays are correct
3. Check for chain selection differences
4. Compare intermediate features

## 📚 References

- [Foldseek Paper](https://www.nature.com/articles/s41587-023-01773-0)
- [pybind11 Docs](https://pybind11.readthedocs.io/)
- [NumPy C API](https://numpy.org/doc/stable/reference/c-api/)
- [Python Packaging Guide](https://packaging.python.org/)

## 🏁 Conclusion

**Status**: MVP code is complete and ready for building/testing.

**Blockers**: Need to fix CMakeLists.txt to properly link against foldseek libraries.

**Next Action**: Fix build configuration and test with example PDB.

**Timeline Estimate**:
- Fix build issues: 2-4 hours
- Test and debug: 4-8 hours
- Validate against CLI: 2 hours
- Polish and document: 4 hours
- **Total: 1-2 days to working MVP**

---

Last Updated: 2025-10-23
