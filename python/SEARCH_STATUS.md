# Structure Search Implementation Status

## Summary

This document describes the implementation status of structure search functionality in the Python bindings.

## Completed ✅

### 1. Database Reading (`Database` and `DatabaseEntry` classes)
- **Status**: Fully implemented and tested
- **Features**:
  - Opens Foldseek databases created with `foldseek createdb`
  - Reads main database, headers, CA coordinates, and 3Di sequences
  - Provides Python-friendly API with NumPy arrays
  - Handles compressed Coordinate16 format
  - Comprehensive test suite (7 tests, all passing)

**Usage**:
```python
from pyfoldseek import Database

db = Database("/path/to/database")
print(f"Database has {len(db)} entries")

entry = db[0]
print(f"Sequence: {entry.sequence}")
print(f"3Di: {entry.seq_3di}")
print(f"CA coordinates: {entry.ca_coords.shape}")  # (N, 3) NumPy array
```

## In Progress ⚠️

### 2. Structure Search (`search()` function)
- **Status**: Code written, has memory corruption issues
- **Implementation**: Located in `python/src/database_wrapper.cpp`
- **Approach**: Uses TM-align to search query structure against database entries

**Current Issue**:
The search function encounters memory corruption errors (`malloc(): unaligned tcache chunk detected` or `double free`). This appears to be related to:
1. Memory management of coordinate arrays passed to TMaligner
2. Array lifetime management between Python and C++
3. Possible issues with array layout (separate x/y/z vs interleaved)

**Code Location**:
- Function: `std::vector<PySearchHit> search(...)` in `database_wrapper.cpp` (lines ~366-510)
- Bindings: Added to `init_database()` (lines ~587-638)

**What Works**:
- API design is sound
- SearchHit class properly defined
- Python bindings are correct
- Compilation succeeds

**What Needs Fixing**:
1. **Memory Management**: Replace raw pointers with `std::vector` or `std::unique_ptr`
2. **Array Layout**: Verify TM-aligner expects separate x/y/z arrays (currently implemented)
3. **Testing**: Need valgrind or gdb session to identify exact memory issue

**Recommended Fix**:
```cpp
// Instead of:
float* query_x = new float[query_len];

// Use:
std::vector<float> query_x(query_len);
//  Then pass: query_x.data()
```

## Not Started ⏸️

### 3. Clustering
- **Status**: Not yet implemented
- **Planned Approach**: Greedy set-cover clustering based on TM-scores
- **Dependencies**: Requires working search function

## Testing

### Database Tests ✅
Location: `python/validation_tests/test_database.py`
- 7 tests, all passing
- Tests: open, entry access, data retrieval, key lookup, iteration, repr

### Search Tests ⏸️
Location: Not yet created
- Blocked by memory issues in search function

## Next Steps

1. **Fix Search Memory Issues** (Priority 1)
   - Replace raw pointers with RAII wrappers
   - Add memory debugging (valgrind, sanitizers)
   - Verify coordinate array layout

2. **Test Search Function** (Priority 2)
   - Create test database with known structures
   - Verify TM-scores match expectations
   - Test edge cases (empty database, single entry, etc.)

3. **Implement Clustering** (Priority 3)
   - Greedy clustering algorithm
   - Use search results for similarity matrix
   - Return cluster assignments

4. **Documentation** (Priority 4)
   - Update API documentation
   - Add usage examples
   - Update feature comparison table

## API Design (Proposed)

### Search (Current Implementation)
```python
from pyfoldseek import Structure, Database, search

query = Structure.from_file("query.pdb")
db = Database("/path/to/database")

hits = search(
    query.ca_coords,
    query.sequence,
    db,
    tmscore_threshold=0.5,
    coverage_threshold=0.0,
    max_hits=1000
)

for hit in hits:
    print(f"{hit.target_name}: TM-score={hit.tmscore:.3f}")
```

### Clustering (Not Yet Implemented)
```python
from pyfoldseek import Database, cluster

db = Database("/path/to/database")

clusters = cluster(
    db,
    tmscore_threshold=0.5,
    coverage_threshold=0.8,
    mode="greedy"  # or "connected-component"
)

for cluster_id, members in clusters.items():
    print(f"Cluster {cluster_id}: {len(members)} members")
```

## Performance Considerations

**Current Search Implementation**:
- Uses TM-align for all-vs-all comparison
- No prefiltering or indexing
- Suitable for small databases (<1000 entries)
- For large-scale search, recommend using Foldseek CLI

**Optimization Opportunities** (Future Work):
1. Add 3Di+AA Smith-Waterman prefiltering
2. Implement E-value calculation using EvalueNeuralNet
3. Add multi-threading support
4. Cache query profile between searches

## Files Modified

- `python/src/database_wrapper.cpp`: Added SearchHit class and search() function
- `python/pyfoldseek/__init__.py`: Exported search and SearchHit
- `python/CMakeLists.txt`: (No changes needed, uses same source file)

## References

- TM-align implementation: `src/commons/TMaligner.h`
- Database reading: Uses DBReader from MMseqs2
- Coordinate compression: `src/commons/Coordinate16.h`
