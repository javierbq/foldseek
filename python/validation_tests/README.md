# Foldseek Python Bindings - Validation Tests

This directory contains validation test scripts for the Python bindings, specifically testing:

1. **batch_convert functionality** - Tests the fix for the segfault issue when loading multiple structures
2. **LDDT calculation** - Tests the newly implemented LDDT (Local Distance Difference Test) functionality

## Running the Tests

### Prerequisites

Make sure you have built and installed the Python bindings:

```bash
cd /home/user/foldseek/python
pip install -e .
```

### Test Scripts

#### 1. Test batch_convert (Priority 2)

This tests the fix for the critical segfault issue when loading multiple structures:

```bash
cd /home/user/foldseek/python/validation_tests
python test_batch_convert.py
```

**What it tests:**
- Loading a single structure (baseline)
- Loading multiple structures sequentially (the problematic case)
- Using the `batch_convert()` function with multiple files
- Batch processing with many files

**Expected behavior:**
- All tests should pass without segfaults
- Multiple structures can be loaded in the same Python session
- The `batch_convert()` function processes all files successfully

#### 2. Test LDDT Calculation (Priority 3)

This tests the newly implemented LDDT functionality:

```bash
cd /home/user/foldseek/python/validation_tests
python test_lddt.py
```

**What it tests:**
- Importing LDDT classes (`LDDTCalculator`, `LDDTResult`)
- Computing LDDT for identical structures (should give ~1.0)
- Using the convenience `compute_lddt()` function
- LDDT with gapped alignments (insertions/deletions)
- LDDT between two different structures
- LDDT with synthetic coordinate data

**Expected behavior:**
- All tests should pass
- Identical structures should have LDDT score close to 1.0
- Per-residue LDDT scores should be returned as NumPy arrays
- LDDT should handle various alignment scenarios (gaps, different structures)

### Running All Tests

To run all validation tests:

```bash
cd /home/user/foldseek/python/validation_tests
python test_batch_convert.py && python test_lddt.py
```

Or use a simple wrapper:

```bash
for test in test_*.py; do
    echo "Running $test..."
    python "$test"
    echo ""
done
```

## Test Data

The tests use files from the `data/test/` directory in the foldseek repository. If test data is not found, some tests will be skipped with a "SKIP" message.

## Understanding the Fixes

### Fix 1: batch_convert Segfault

**Root Cause:** The original `batch_convert()` function tried to call back into Python from C++, causing issues with the Python/C++ boundary and GemmiWrapper state management.

**Solution:** Modified `batch_convert()` in `structure_wrapper.cpp` to:
- Directly instantiate `PyStructure` objects in C++
- Properly manage GIL (Global Interpreter Lock) for thread safety
- Create a fresh `GemmiWrapper` instance for each file

**Files modified:**
- `python/src/structure_wrapper.cpp` (lines 305-333)

### Fix 2: LDDT Calculation Implementation

**What was added:** Complete Python bindings for Foldseek's LDDT calculator, which measures local structural similarity between aligned protein structures.

**Components added:**
- `PyLDDTResult` class - Wraps LDDT results (average score + per-residue scores)
- `PyLDDTCalculator` class - Main LDDT calculation interface
- `compute_lddt()` convenience function - Simple one-liner LDDT computation

**Files modified:**
- `python/src/alignment_wrapper.cpp` (added ~300 lines)

## Success Criteria

### batch_convert tests:
- ✅ No segfaults when loading multiple structures
- ✅ batch_convert() successfully processes multiple files
- ✅ Can load 5+ structures in a single Python session

### LDDT tests:
- ✅ LDDT classes import successfully
- ✅ Identical structures give LDDT ≥ 0.95
- ✅ Per-residue scores returned as NumPy arrays
- ✅ Handles gapped alignments correctly
- ✅ Works with synthetic coordinate data

## Troubleshooting

If tests fail:

1. **Import errors**: Make sure you've built the bindings:
   ```bash
   cd /home/user/foldseek/python
   pip install -e .
   ```

2. **Test data not found**: Check that test files exist:
   ```bash
   ls ../data/test/  # or data/test/
   ```

3. **Segfaults**: If batch_convert still segfaults, check the C++ build:
   ```bash
   cd /home/user/foldseek/python
   rm -rf build/
   pip install -e . --force-reinstall
   ```

4. **LDDT errors**: Check that LDDT.h and LDDT.cpp are being compiled:
   ```bash
   grep -r "LDDT" build/temp.*/
   ```

## Next Steps

After validating these fixes:

1. Update `KNOWN_ISSUES.md` to mark batch_convert as fixed
2. Update `FUNCTIONALITY_STATUS.md` to mark LDDT as implemented
3. Add integration tests to the main test suite
4. Consider adding to CI/CD pipeline
5. Update documentation with LDDT examples

## Contact

These tests were created as part of implementing priorities 2 and 3 from the Python bindings roadmap.

For issues, refer to the main project repository.
