# Known Issues - Python Bindings

## Critical Issue: Multiple Structure Loading

**Status**: ❌ **UNRESOLVED**

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

### Next Steps for Debugging

1. **Use gdb/valgrind** to get exact crash location:
   ```bash
   gdb python
   > run script.py
   > bt  # backtrace after crash
   ```

2. **Check gemmi library**:
   - Look for static variables in GemmiWrapper
   - Check if gemmi has initialization requirements

3. **Test with minimal example**:
   - Create C++ test program that loads two structures
   - See if issue is in Python bindings or underlying C++ code

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
