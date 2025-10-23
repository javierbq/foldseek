# Implementation Roadmap for Missing Python Bindings

**Date**: 2025-10-23
**Status**: Analysis Complete - Ready for Implementation

---

## Executive Summary

After analyzing the Foldseek codebase, implementing the missing critical features requires wrapping several complex C++ classes. This document provides a detailed roadmap with realistic effort estimates.

**Estimated Total Time**: 6-8 weeks for full feature parity

---

## Priority 1: Database Reading (CRITICAL - Week 1-2)

### Overview
Enable reading Foldseek databases to access pre-computed structure databases (PDB, AlphaFold, etc.)

### C++ Classes to Wrap

#### 1. DBReader<unsigned int>
**Location**: `lib/mmseqs/src/commons/DBReader.h`

**Key Methods**:
```cpp
class DBReader<unsigned int> {
    // Open database
    void open(int dataMode);

    // Get entry count
    size_t getSize();

    // Get data by index
    char* getData(size_t id, int thread_idx);
    size_t getSeqLen(size_t id);

    // Get key by index
    unsigned int getDbKey(size_t id);

    // Iterate
    Index* getIndex(size_t id);
};
```

**Dependencies**:
- `DBReader.h` (main class)
- `MemoryTracker.h`
- `FileUtil.h`
- `Debug.h`

**Python Binding Approach**:
```python
class Database:
    """Read Foldseek structure database"""

    def __init__(self, path: str):
        """Open database from path"""

    def __len__(self) -> int:
        """Number of entries"""

    def __getitem__(self, idx: int) -> DatabaseEntry:
        """Get entry by index"""

    def keys(self) -> List[int]:
        """Get all database keys"""

    def get(self, key: int) -> DatabaseEntry:
        """Get entry by key"""

class DatabaseEntry:
    """Single database entry"""
    sequence: str        # Amino acid sequence
    seq_3di: str        # 3Di sequence
    key: int            # Database key
    header: str         # Entry header/name
    coordinates: np.ndarray  # CA coordinates if available
```

**Estimated Effort**: 1 week
- Python wrapper: 2-3 days
- Testing: 1-2 days
- Documentation: 1 day

---

## Priority 2: 3Di+AA Alignment (HIGH - Week 3-4)

### Overview
Implement Foldseek's default alignment algorithm (combines 3Di and amino acid information)

### C++ Classes to Wrap

#### 1. StructureSmithWaterman
**Location**: `src/commons/StructureSmithWaterman.h`

**Key Methods**:
```cpp
class StructureSmithWaterman {
    // Initialize with query
    void ssw_init(Sequence *q_aa, Sequence *q_3di,
                  const int8_t *mat_aa, const int8_t *mat_3di);

    // Align and get result
    s_align alignScoreEndPos(
        const unsigned char *db_aa_sequence,
        const unsigned char *db_3di_sequence,
        int32_t db_length,
        uint8_t gap_open,
        uint8_t gap_extend,
        int32_t maskLen
    );

    // Get alignment with backtrace (CIGAR)
    s_align alignStartPosBacktrace(...);
};

struct s_align {
    uint32_t score1;         // Alignment score
    int32_t qStartPos1;      // Query start
    int32_t qEndPos1;        // Query end
    int32_t dbStartPos1;     // Target start
    int32_t dbEndPos1;       // Target end
    float qCov;              // Query coverage
    float tCov;              // Target coverage
    uint32_t* cigar;         // CIGAR string
    int32_t cigarLen;        // CIGAR length
    double evalue;           // E-value
    int identicalAACnt;      // Identical amino acids
};
```

**Dependencies**:
- `SubstitutionMatrix` (scoring matrices)
- `Sequence` (sequence representation)
- `BaseMatrix`
- `EvalueNeuralNet` (E-value calculation)
- SIMD libraries (SSE2/AVX2)

**Python Binding Approach**:
```python
class StructureAligner:
    """3Di+AA Smith-Waterman aligner"""

    def __init__(self, max_seq_len: int = 50000):
        """Initialize aligner"""

    def align(self,
              query_seq: str, query_3di: str,
              target_seq: str, target_3di: str,
              gap_open: int = 10, gap_extend: int = 1) -> AlignmentResult:
        """Align two structures"""

class AlignmentResult:
    """Alignment result"""
    score: float
    evalue: float
    query_start: int
    query_end: int
    target_start: int
    target_end: int
    query_coverage: float
    target_coverage: float
    cigar: str                    # CIGAR string (e.g., "10M2I5M3D20M")
    query_aligned: str            # Aligned query sequence
    target_aligned: str           # Aligned target sequence
    identity: float               # Sequence identity
    alignment_length: int         # Alignment length
```

**Challenges**:
- Complex SIMD optimizations
- Multiple profile types (byte/word/int)
- Gap position scoring (conditional compilation)
- Composition bias correction
- E-value computation with neural network

**Estimated Effort**: 2 weeks
- Understand SIMD code: 2-3 days
- Python wrapper (basic): 3-4 days
- CIGAR parsing: 1-2 days
- E-value integration: 2 days
- Testing: 2-3 days

**Simplification Option**:
- Start with PROFILE mode only (simpler)
- Skip composition bias initially
- Use default gap penalties
- This could reduce to 1 week

---

## Priority 3: Structure Search (CRITICAL - Week 5-6)

### Overview
Search a query structure against a database

### C++ Classes/Functions to Wrap

#### 1. Search Workflow
**Location**: `src/workflow/StructureSearch.cpp`

**Key Components**:
```cpp
// Main search function (simplified)
int structuresearch(int argc, const char **argv) {
    // 1. Open query and target databases
    DBReader<unsigned int> qDbr, tDbr;

    // 2. Create aligners
    StructureSmithWaterman aligner;

    // 3. For each query:
    for (size_t queryId = 0; queryId < qDbr.getSize(); queryId++) {
        // Get query sequence
        Sequence qSeq, qSeq3Di;

        // Initialize aligner with query
        aligner.ssw_init(&qSeq, &qSeq3Di, ...);

        // 4. For each target:
        for (size_t targetId = 0; targetId < tDbr.getSize(); targetId++) {
            // Align
            auto result = aligner.align(...);

            // Filter by E-value, coverage
            if (result.evalue < threshold) {
                results.push_back(result);
            }
        }

        // Sort and output results
    }
}
```

**Dependencies**:
- `DBReader` (databases)
- `StructureSmithWaterman` (alignment)
- `EvalueNeuralNet` (E-values)
- `Matcher` (result handling)
- `DBWriter` (output results - optional for Python)

**Python Binding Approach**:
```python
def search(query: Union[str, Structure],
           database: Union[str, Database],
           evalue_threshold: float = 0.001,
           coverage_threshold: float = 0.5,
           max_hits: int = 1000,
           threads: int = 1) -> List[SearchHit]:
    """
    Search a structure against a database

    Parameters
    ----------
    query : str or Structure
        Query structure file or Structure object
    database : str or Database
        Path to database or Database object
    evalue_threshold : float
        E-value threshold for hits
    coverage_threshold : float
        Minimum coverage (0-1)
    max_hits : int
        Maximum number of hits to return
    threads : int
        Number of threads (future)

    Returns
    -------
    List[SearchHit]
        List of hits sorted by E-value
    """

class SearchHit:
    """Search result hit"""
    target_key: int               # Database key
    target_name: str              # Target name
    score: float                  # Alignment score
    evalue: float                 # E-value
    identity: float               # Sequence identity
    query_coverage: float         # Query coverage
    target_coverage: float        # Target coverage
    alignment: AlignmentResult    # Full alignment
```

**Estimated Effort**: 2 weeks
- Database integration: 2-3 days
- Search logic: 3-4 days
- Result filtering/sorting: 1-2 days
- Threading (basic): 2 days
- Testing: 2-3 days

---

## Priority 4: Clustering (Week 7)

### Overview
Cluster structures by similarity

### C++ Classes to Wrap

#### 1. Clustering
**Location**: `workflow/StructureCluster.cpp`

**Approach**:
```python
def cluster(database: Union[str, Database],
            similarity_threshold: float = 0.7,
            coverage_threshold: float = 0.8,
            mode: str = "set-cover",
            threads: int = 1) -> ClusteringResult:
    """
    Cluster structures

    Parameters
    ----------
    database : str or Database
        Database to cluster
    similarity_threshold : float
        Minimum similarity for clustering
    coverage_threshold : float
        Minimum coverage
    mode : str
        Clustering mode: "set-cover", "connected-component", "greedy"

    Returns
    -------
    ClusteringResult
        Cluster assignments
    """

class ClusteringResult:
    """Clustering result"""
    clusters: List[Cluster]       # List of clusters
    representatives: List[int]    # Representative keys
    assignments: Dict[int, int]   # Key -> cluster ID
```

**Estimated Effort**: 1 week

---

## Priority 5: Fix TM-score Normalization (Week 8)

### Current Issue
Self-alignment gives TM-score of 0.004 instead of ~1.0

### Investigation Needed
```python
# Current (broken):
result = compute_tmscore(s.ca_coords, s.ca_coords, s.sequence, s.sequence)
print(result.tmscore)  # 0.004 ❌

# Expected:
print(result.tmscore)  # ~1.0 ✅
```

### Root Cause Analysis
Look at `python/src/alignment_wrapper.cpp`:
- Check if query/target length passed correctly to TMaligner
- Verify normalization factor calculation
- Compare with CLI implementation

**Estimated Effort**: 2-3 days

---

## Implementation Phases

### Phase 1: Database Reading (2 weeks)
**Deliverables**:
- `Database` class to read Foldseek databases
- `DatabaseEntry` class for entries
- Iterator support
- Tests with real databases (PDB, AlphaFold)

**Success Criteria**:
```python
db = Database("/path/to/pdb/db")
print(len(db))  # Number of entries

entry = db[0]
print(entry.sequence)
print(entry.seq_3di)
print(entry.header)
```

### Phase 2: 3Di+AA Alignment (2 weeks)
**Deliverables**:
- `StructureAligner` class
- `AlignmentResult` with CIGAR, coverage, E-value
- Integration with existing Structure class

**Success Criteria**:
```python
aligner = StructureAligner()
result = aligner.align(s1.sequence, s1.seq_3di,
                       s2.sequence, s2.seq_3di)
print(f"E-value: {result.evalue}")
print(f"CIGAR: {result.cigar}")
print(f"Identity: {result.identity}")
```

### Phase 3: Structure Search (2 weeks)
**Deliverables**:
- `search()` function
- `SearchHit` class
- Integration with Database and StructureAligner

**Success Criteria**:
```python
hits = search("query.pdb", "/path/to/pdb/db", evalue_threshold=1e-3)
for hit in hits[:10]:
    print(f"{hit.target_name}: E={hit.evalue:.2e}, identity={hit.identity:.2%}")
```

### Phase 4: Clustering (1 week)
**Deliverables**:
- `cluster()` function
- `ClusteringResult` class

**Success Criteria**:
```python
result = cluster("/path/to/db", similarity_threshold=0.7)
print(f"Found {len(result.clusters)} clusters")
```

### Phase 5: Bug Fixes & Polish (1 week)
**Deliverables**:
- Fix TM-score normalization
- Optimize performance
- Comprehensive tests
- Update documentation

---

## Dependencies & Prerequisites

### Required C++ Classes (in order)
1. `DBReader<unsigned int>` - Database reading
2. `Sequence` - Sequence representation
3. `SubstitutionMatrix` - Scoring matrices
4. `StructureSmithWaterman` - 3Di+AA alignment
5. `EvalueNeuralNet` - E-value calculation
6. `Matcher` - Result handling

### Python Dependencies
- NumPy (already required)
- pybind11 (already used)
- No new dependencies needed

### Build System Changes
- Add mmseqs2 source files to CMake
- Link against mmseqs2 libraries
- Handle SIMD compilation flags

---

## Simplified Implementation Strategy

### Option 1: Full Implementation (8 weeks)
- Wrap all C++ classes completely
- Support all features and parameters
- Full optimization (SIMD, threading)

### Option 2: MVP Implementation (4 weeks)
- Database reading: Basic functionality only
- Alignment: PROFILE mode only, no composition bias
- Search: Single-threaded, basic filtering
- Skip clustering initially

### Option 3: Hybrid Approach (6 weeks)
- Database reading: Full implementation
- Alignment: Simplified (no SIMD optimization wrapper, use existing C++)
- Search: Full but single-threaded
- Clustering: Basic

**Recommendation**: Start with Option 2 (MVP) to get something working quickly, then expand.

---

## Risk Assessment

### High Risk
1. **SIMD Code Complexity**: StructureSmithWaterman uses heavy SIMD optimization
   - **Mitigation**: Wrap existing compiled code, don't reimplement

2. **MMseqs2 Dependencies**: Tight coupling with MMseqs2 infrastructure
   - **Mitigation**: Include mmseqs2 sources in build

3. **Performance**: Python overhead could slow down searches
   - **Mitigation**: Release GIL for C++ operations

### Medium Risk
1. **Memory Management**: Crossing Python/C++ boundary
   - **Mitigation**: Use pybind11 smart pointers

2. **Database Format Changes**: Foldseek database format might change
   - **Mitigation**: Version checking in code

### Low Risk
1. **API Design**: Getting Python API right
   - **Mitigation**: Prototype with users first

---

## Testing Strategy

### Unit Tests
- Database reading with known files
- Alignment with verified pairs
- Search against small database

### Integration Tests
- Search PDB database
- Cluster AlphaFold database
- Performance benchmarks

### Regression Tests
- Compare Python vs CLI results
- Verify E-values match
- Check alignment statistics

---

## Success Metrics

### Functionality
- ✅ Can read Foldseek databases
- ✅ Can perform 3Di+AA alignment
- ✅ Can search databases
- ✅ Results match CLI tool

### Performance
- Database reading: <100ms overhead vs CLI
- Alignment: Within 2x of CLI speed
- Search: Single-threaded within 2x of CLI

### Usability
- Clear, Pythonic API
- Good error messages
- Comprehensive documentation

---

## Timeline Summary

| Week | Phase | Deliverable |
|------|-------|-------------|
| 1-2  | Database Reading | `Database` class working |
| 3-4  | 3Di+AA Alignment | `StructureAligner` class working |
| 5-6  | Structure Search | `search()` function working |
| 7    | Clustering | `cluster()` function working |
| 8    | Polish | Bug fixes, optimization, docs |

**Total**: 8 weeks for full implementation
**MVP**: 4 weeks for core features

---

## Next Steps

1. **Decide on implementation scope**: Full vs MVP vs Hybrid
2. **Set up build system**: Include mmseqs2 sources
3. **Start with Database Reading**: Most foundational feature
4. **Prototype API**: Get feedback before full implementation
5. **Incremental testing**: Test each component thoroughly

---

## Current Status

- ✅ Analysis complete
- ✅ Roadmap documented
- ⏳ Ready to begin implementation
- ❌ No code written yet

**This document provides the blueprint for completing Python bindings feature parity with the CLI tool.**
