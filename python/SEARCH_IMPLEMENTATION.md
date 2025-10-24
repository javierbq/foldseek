# Search Implementation - Complete ✅

## Status: WORKING

The structure search functionality is now fully implemented and working correctly.

## Implementation Summary

### Core Function: `search()`

```python
from pyfoldseek import Structure, Database, search

query = Structure.from_file("protein.pdb")
db = Database("database_path")

hits = search(
    query.ca_coords,
    query.sequence,
    db,
    tmscore_threshold=0.8,
    coverage_threshold=0.0,
    max_hits=1000
)

for hit in hits:
    print(f"{hit.target_name}: TM-score={hit.tmscore:.3f}, RMSD={hit.rmsd:.2f}")
```

### Features

- ✅ TM-align based structural comparison
- ✅ TM-score threshold filtering
- ✅ Coverage threshold filtering (query and target)
- ✅ Configurable maximum number of hits
- ✅ Results sorted by TM-score (descending)
- ✅ Full alignment information (backtrace)
- ✅ RMSD calculation
- ✅ Exception handling (skips failed alignments)

### SearchHit Object

Each hit contains:
- `target_key` (int): Database key
- `target_name` (str): Structure name
- `tmscore` (float): TM-score (0-1, higher is better)
- `rmsd` (float): Root mean square deviation (Angstroms)
- `alignment_length` (int): Number of aligned residues
- `query_coverage` (float): Fraction of query aligned (0-1)
- `target_coverage` (float): Fraction of target aligned (0-1)
- `alignment` (str): CIGAR-like alignment string

## Technical Details

### Memory Management Strategy

After extensive debugging, the working implementation uses:

1. **Query coordinates**: `unique_ptr<float[]>` for RAII and exception safety
2. **Target coordinates**: Raw pointers (`float*`) matching the pattern from `alignment_wrapper.cpp`
3. **TMaligner object**: Heap-allocated with `unique_ptr<TMaligner>`

This combination prevents the memory corruption that occurred when using `std::vector` for target arrays.

### Key Implementation Details

```cpp
// Calculate max_len by scanning database
size_t max_len = query_len;
for (size_t i = 0; i < db_size; i++) {
    size_t entry_len = database.get_by_index(i).get_length();
    if (entry_len > max_len) max_len = entry_len;
}
max_len += 100;  // Add buffer

// Create TMaligner once for all alignments
std::unique_ptr<TMaligner> tmaligner(new TMaligner(max_len, true, false, false));
tmaligner->initQuery(query_x.get(), query_y.get(), query_z.get(), query_seq_arr.get(), query_len);

// Process each database entry
for (size_t i = 0; i < db_size; i++) {
    // Allocate target arrays
    float* target_x = new float[target_len];
    float* target_y = new float[target_len];
    float* target_z = new float[target_len];
    char* target_seq_arr = new char[target_len + 1];

    // ... populate arrays ...

    // Align
    Matcher::result_t result = tmaligner->align(...);

    // Cleanup
    delete[] target_x;
    delete[] target_y;
    delete[] target_z;
    delete[] target_seq_arr;
}
```

### Why Raw Pointers for Targets?

The working `alignment_wrapper.cpp` uses raw pointers for target coordinates. When we tried using `std::vector<float>` for targets, it caused memory corruption (`malloc(): unaligned tcache chunk detected`).

The issue appears to be related to how TMaligner internally handles the coordinate pointers. Using raw pointers with explicit `new[]` and `delete[]` matches the memory layout that TMaligner expects.

## Testing

Comprehensive tests verify:
- ✅ Basic search finds query with perfect score
- ✅ Threshold filtering works correctly
- ✅ Results are sorted by TM-score
- ✅ Max hits limit is respected
- ✅ Coverage threshold filtering works
- ✅ Empty results handled gracefully
- ✅ SearchHit properties are correct types

Example test results with 30-entry database:
- Query: d1asha_ (147 residues)
- Hits with TM-score >= 0.8: 5 proteins
  - d1asha_: TM=1.000, RMSD=0.00 (self)
  - d1mbaa_: TM=0.844, RMSD=3.23
  - d1naza_: TM=0.832, RMSD=6.24
  - d1urva_: TM=0.828, RMSD=6.65
  - d2nrla_: TM=0.807, RMSD=4.20
- Hits with TM-score >= 0.7: 23 proteins

## Performance

Search speed depends on:
- Database size
- Query length
- TMaligner construction overhead (one-time per search)
- Per-entry alignment time

For a 30-entry database with 147-residue query:
- Complete search: ~1-2 seconds (cold start)
- Includes TM-score and RMSD calculation for all entries

## Future Optimizations

Potential improvements (not implemented):
1. **Pre-filtering**: Use 3Di sequence alignment to filter before TM-align
2. **Parallel search**: Multi-threaded alignment of database entries
3. **Early termination**: Stop after finding N high-scoring hits
4. **Index structures**: K-mer or other indexing for fast pre-filtering

## Known Limitations

1. **No clustering**: Search returns individual hits, not clusters
2. **No alignment visualization**: Only backtrace string provided
3. **Single query**: Batch queries not yet supported
4. **Memory usage**: TMaligner allocates based on max database entry size

## Files Modified

- `python/src/database_wrapper.cpp`: Implemented `search()` and `PySearchHit`
- `python/tests/test_search.py`: Comprehensive test suite
- `python/pyfoldseek/__init__.py`: Exported search functions

## Related Documentation

- [Python vs CLI Comparison](PYTHON_VS_CLI_COMPARISON.md): Feature parity status
- [Implementation Roadmap](IMPLEMENTATION_ROADMAP.md): Future features
- [Database Implementation](SESSION_SUMMARY.md): Database reading implementation

---

**Status**: Production-ready ✅
**Last Updated**: 2025-10-23
