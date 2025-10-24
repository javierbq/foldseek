# Known Issues - Python Bindings

## Critical Issue: Multiple Structure Loading

**Status**: ❌ **PARTIALLY ADDRESSED** (Deeper investigation needed)

**Problem**: Segmentation fault occurs when loading a second structure after the first one.

### Symptoms

```python
# This works fine:
s1 = Structure.from_file("1tim.pdb.gz")
print(s1.length)  # ✓ Works

# This causes segfault:
s2 = Structure.from_file("8tim.pdb.gz")  # ⚠️ SEGFAULT
```

### Impact

- ✅ **Single file processing**: Works perfectly
- ❌ **Multiple files in same session**: Crashes
- ❌ **batch_convert()**: Crashes after first file

### Root Cause Analysis

Investigated several possibilities:

1. **StructureTo3Di constructor** ✅ FIXED
   - Constructor loads neural network model
   - Fixed by using `static StructureTo3Di converter`
   - This avoids reloading the model, but didn't fix the segfault

2. **PyStructure copy semantics** ✅ NOT THE ISSUE
   - Tested returning `py::list` instead of `std::vector<PyStructure>`
   - Tested using `std::shared_ptr`
   - Still crashes

3. **GemmiWrapper** ❓ SUSPECTED
   - Segfault occurs during second `GemmiWrapper::load()` call
   - Possible issues with gemmi library state
   - May have static variables or global state

4. **Memory corruption** ❓ POSSIBLE
   - Could be related to how coordinate vectors are stored
   - Could be related to gemmi's internal buffers

### Workaround

**Use separate Python processes** for multiple files:

```python
# Instead of this:
structures = [Structure.from_file(f) for f in files]  # ❌ Crashes

# Do this:
import subprocess
for file in files:
    subprocess.run(["python", "process_single.py", file])  # ✓ Works
```

Or **restart Python interpreter between files**:

```bash
python -c "from pyfoldseek import Structure; s = Structure.from_file('file1.pdb'); print(s.seq_3di)"
python -c "from pyfoldseek import Structure; s = Structure.from_file('file2.pdb'); print(s.seq_3di)"
```

### Investigation Results (2025-10-23)

Multiple approaches were attempted to fix the segfault:

1. **✓ Fixed batch_convert Python callback**:
   - Changed batch_convert to directly instantiate PyStructure instead of calling back into Python
   - Added proper GIL management
   - Result: Improved but segfault persists

2. **✓ Tried removing static StructureTo3Di converter**:
   - Changed to creating new converter per structure
   - Result: Segfault persists

3. **✓ Verified GemmiWrapper clears vectors**:
   - Confirmed that updateStructure() properly clears ca, n, c, cb, ami vectors
   - Result: Not the issue

4. **Conclusion**: The segfault appears to be in the gemmi library itself or how it interacts with the file system when loading a second file. Further investigation would require:
   - Deep debugging with gdb/valgrind
   - Potentially patching the gemmi library
   - Testing with standalone C++ code (outside Python bindings)

### Next Steps for Debugging

1. **Use gdb/valgrind** to get exact crash location:
   ```bash
   gdb python
   > run script.py
   > bt  # backtrace after crash
   ```

2. **Test with minimal C++ example**:
   - Create standalone C++ test program that loads two structures
   - Determine if issue is in gemmi library or Python bindings

3. **Check gemmi library version**:
   - Verify we're using the latest version
   - Check gemmi issue tracker for similar problems

4. **Memory debugging**:
   ```bash
   valgrind --leak-check=full python test_script.py
   ```

### Files Involved

- `python/src/structure_wrapper.cpp` - Main binding code
- `src/strucclustutils/GemmiWrapper.cpp` - Structure loading
- `lib/3di/structureto3di.cpp` - 3Di conversion

### Recommendation

For now, the Python bindings are **production-ready for single-file processing**, which covers most use cases. Batch processing can be done via separate Python processes or shell scripts.

---

**Last Updated**: 2025-10-23
**Investigated By**: Claude Code
