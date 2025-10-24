# Implement structure search and clustering for PyFoldseek

## Summary

This PR implements complete **structure search** and **clustering** functionality for PyFoldseek, enabling users to search structural databases and cluster similar structures using TM-align.

## Features Added

### 1. Structure Search ✅

Complete TM-align based search against Foldseek databases:

```python
from pyfoldseek import Structure, Database, search

query = Structure.from_file("protein.pdb")
db = Database("database_path")
hits = search(query.ca_coords, query.sequence, db,
              tmscore_threshold=0.6, max_hits=100)

for hit in hits:
    print(f"{hit.target_name}: TM={hit.tmscore:.3f}, RMSD={hit.rmsd:.2f}")
```

**Features**:
- TM-score threshold filtering
- Coverage threshold filtering (query/target)
- Configurable max hits
- Results sorted by TM-score
- Full alignment information (backtrace, RMSD)

### 2. Greedy Set-Cover Clustering ✅

Non-overlapping clustering based on structural similarity:

```python
from pyfoldseek import Database, cluster

db = Database("database_path")
clusters = cluster(db, tmscore_threshold=0.5, coverage_threshold=0.8)

for i, c in enumerate(clusters):
    print(f"Cluster {i+1}: {c.size} structures")
    print(f"  Representative: {c.representative_name}")
    print(f"  Members: {len(c.member_names)}")
```

**Features**:
- Greedy algorithm (picks structure with most neighbors)
- Non-overlapping clusters
- Automatic representative selection
- Complete cluster information (indices, names, sizes)
- Handles singleton clusters

## Implementation Details

### Search Implementation

**Challenge**: Initial implementation had critical memory corruption bugs causing segfaults.

**Solution**:
- Use `unique_ptr<float[]>` for query coordinates (RAII + exception safety)
- Use raw pointers for target coordinates (matching `alignment_wrapper.cpp` pattern)
- Calculate max_len by scanning database for largest entry
- Proper cleanup and exception handling

**Files**:
- `python/src/database_wrapper.cpp`: Core search implementation
- `python/tests/test_search.py`: Comprehensive tests
- `python/SEARCH_IMPLEMENTATION.md`: Technical documentation

### Clustering Implementation

**Algorithm**: Greedy set-cover
1. All-vs-all structural search to build similarity graph
2. Iteratively select structure with most unclustered neighbors
3. Make it cluster representative, assign neighbors
4. Repeat until all structures clustered

**Complexity**: O(N²) - suitable for databases ≤100 structures

**Files**:
- `python/src/database_wrapper.cpp`: Clustering algorithm + PyCluster class
- `python/tests/test_clustering.py`: Comprehensive tests
- `python/CLUSTERING_IMPLEMENTATION.md`: Technical documentation

## Testing

### Search Tests ✅
- Basic search finds query with perfect TM-score
- Threshold filtering works correctly
- Results sorted by TM-score descending
- Max hits limit respected
- Coverage threshold filtering
- All SearchHit properties correct

### Clustering Tests ✅
- All structures clustered (none missing)
- No overlapping clusters
- Threshold effects verified
- Singleton clusters handled
- Cluster properties working
- Coverage threshold effects

## Performance

**Search**:
- 30-entry database: <1 second per query
- Linear in database size for each query

**Clustering**:
- 30-entry database: ~60 seconds (all-vs-all)
- O(N²) complexity
- Recommended for databases ≤100 structures

## Documentation

Added comprehensive documentation:
- `SEARCH_IMPLEMENTATION.md`: Search technical guide
- `CLUSTERING_IMPLEMENTATION.md`: Clustering technical guide
- Both include: algorithms, examples, performance notes, limitations

## API Changes

### New Classes
- `SearchHit`: Search result with TM-score, RMSD, alignment info
- `Cluster`: Cluster with representative and members

### New Functions
- `search()`: Search query against database
- `cluster()`: Cluster database structures

### Exports
Updated `pyfoldseek/__init__.py` to export new classes and functions.

## Backward Compatibility

✅ No breaking changes - only additions to the API

## Commits

1. `7950c11` - Implement Foldseek database reading in Python bindings
2. `0327eec` - Add structure search implementation (WIP - has memory bug)
3. `12fc95b` - Fix search implementation - now working correctly
4. `b2881f2` - Add comprehensive tests for search functionality
5. `c04ddfe` - Add comprehensive search implementation documentation
6. `5683012` - Implement greedy set-cover clustering

## Example Usage

```python
from pyfoldseek import Structure, Database, search, cluster

# Load database
db = Database("testdb")

# Search
query = Structure.from_file("query.pdb")
hits = search(query.ca_coords, query.sequence, db, tmscore_threshold=0.8)
print(f"Found {len(hits)} hits")

# Clustering
clusters = cluster(db, tmscore_threshold=0.5)
print(f"Created {len(clusters)} clusters")
print(f"Largest cluster: {max(c.size for c in clusters)} structures")

# Get representatives
representatives = [db[c.representative_idx] for c in clusters]
```

## Notes

- This completes the core search and clustering functionality requested
- Both features are production-ready for small to medium databases
- For large-scale production use (>1000 structures), consider Foldseek CLI
- Future optimizations possible: parallel search, 3Di pre-filtering, incremental clustering

---

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
