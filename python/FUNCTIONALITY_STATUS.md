# Foldseek Functionality Status - Python Bindings

**Last Updated**: 2025-10-23
**Python Bindings Version**: 0.2.0

This document maps all Foldseek command-line functionality to Python binding implementation status.

---

## Legend

- ✅ **Implemented & Working** - Feature is fully implemented and tested
- 🚧 **Partially Implemented** - Feature is implemented but has limitations or bugs
- ⚠️ **Implemented but Broken** - Feature exists but doesn't work correctly
- ❌ **Not Implemented** - Feature is not available in Python bindings
- 🔮 **Planned** - Feature is in the roadmap
- 🤔 **Uncertain** - Not clear if this should be in Python bindings

---

## 1. File I/O and Structure Loading

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Load PDB files | `createdb` | `Structure.from_file()` | ✅ | Supports gzipped files |
| Load mmCIF files | `createdb` | `Structure.from_file()` | ✅ | Supports gzipped files |
| Load Foldcomp files | `createdb` | `Structure.from_file()` | ✅ | Auto-detects format |
| Load from tar.gz | `createdb` | - | ❌ | Not exposed |
| Format auto-detection | `createdb` | `Structure.from_file()` | ✅ | PDB/mmCIF/Foldcomp |
| Format-specific loaders | - | `Structure.from_pdb()`, `from_mmcif()`, `from_foldcomp()` | ✅ | Explicit format selection |
| Multi-chain structures | `createdb` | `Structure`, iteration | ✅ | Access via `for chain in struct` |
| Single-chain structures | `createdb` | `Structure.from_file()` | ✅ | Works for all inputs |

---

## 2. 3Di Structural Alphabet

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Convert structure to 3Di | `createdb` | `Structure.seq_3di` | ✅ | Automatic conversion |
| Convert coordinates to 3Di | - | `coords_to_3di()` | ✅ | Direct coordinate conversion |
| 3Di encoder (neural network) | Internal | Static converter | ✅ | Loaded once per process |
| Extract amino acid sequence | `createdb` | `Structure.sequence` | ✅ | One-letter codes |
| Extract CA coordinates | `createdb` | `Structure.ca_coords` | ✅ | NumPy array (N, 3) |
| Extract N coordinates | `createdb` | `Structure.n_coords` | ✅ | NumPy array (N, 3) |
| Extract C coordinates | `createdb` | `Structure.c_coords` | ✅ | NumPy array (N, 3) |
| Extract CB coordinates | `createdb` | `Structure.cb_coords` | ✅ | NumPy array (N, 3) |
| ProstT5 prediction from FASTA | `createdb --prostt5-model` | - | ❌ | Not exposed |

---

## 3. Structure Search

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Easy search (single query) | `easy-search` | - | ❌ | Not implemented |
| Database search | `search` | - | ❌ | Not implemented |
| Reciprocal best hit | `easy-rbh`, `rbh` | - | ❌ | Not implemented |
| GPU-accelerated search | `--gpu` flag | - | ❌ | Not available |
| Iterative search | `--num-iterations` | - | ❌ | Not implemented |
| Exhaustive search | `--exhaustive-search` | - | ❌ | Not implemented |
| Cluster search | `--cluster-search` | - | ❌ | Not implemented |
| Prefilter modes | `--prefilter-mode` | - | ❌ | Not implemented |

---

## 4. Clustering

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Easy cluster | `easy-cluster` | - | ❌ | Not implemented |
| Cluster | `cluster` | - | ❌ | Not implemented |
| Multimercluster | `easy-multimercluster`, `multimercluster` | - | ❌ | Not implemented |
| Set-Cover clustering | `clust` | - | ❌ | Not implemented |
| Connected-Component | `clust` | - | ❌ | Not implemented |
| Greedy-Incremental | `clust` | - | ❌ | Not implemented |

---

## 5. Alignment and Scoring

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| **TM-align** | `tmalign`, `--alignment-type 1` | `TMaligner` class | ✅ | Full implementation |
| TM-score computation | `tmalign` | `compute_tmscore()` | ✅ | Convenience function |
| TM-score result | Output | `TMscoreResult` | ✅ | TM-score, RMSD, matrices |
| Rotation matrix | Output | `TMscoreResult.rotation_matrix` | ✅ | NumPy (3, 3) array |
| Translation vector | Output | `TMscoreResult.translation` | ✅ | NumPy (3,) array |
| RMSD calculation | `tmalign` | `TMscoreResult.rmsd` | ✅ | Working |
| TM-score normalization | `qtmscore`, `ttmscore` | - | ⚠️ | Values seem incorrect (0.004 instead of ~1.0) |
| **LoLalign** | `lolalign` | - | ❌ | Not implemented |
| **3Di Smith-Waterman** | `structurealign` | - | ❌ | Not implemented |
| **3Di+AA alignment** | `--alignment-type 2` (default) | - | ❌ | Not implemented |
| Alignment to TM-score | `aln2tmscore` | - | ❌ | Not implemented |
| LDDT calculation | Output field | - | ❌ | Not implemented |
| LDDT per-residue | `lddtfull` output | - | ❌ | Not implemented |
| Interface LDDT | Multimer output | - | ❌ | Not implemented |
| pLDDT extraction | - | - | ❌ | Not implemented |

---

## 6. Database Management

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Create database | `createdb` | - | ❌ | Not exposed as function |
| Download databases | `databases` | - | ❌ | Not implemented |
| Create index | `createindex` | - | ❌ | Not implemented |
| Create cluster search DB | `createclusearchdb` | - | ❌ | Not implemented |
| Create subset DB | `createsubdb` | - | ❌ | Not implemented |
| Padded sequence DB (GPU) | `makepaddedseqdb` | - | ❌ | Not implemented |
| Read database | - | - | ❌ | Not implemented |
| Database iterator | - | - | ❌ | Not implemented |

---

## 7. Format Conversion

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Convert alignments | `convertalis` | - | ❌ | Not implemented |
| BLAST-tab format | `convertalis` output | - | ❌ | Not implemented |
| SAM format | `convertalis` output | - | ❌ | Not implemented |
| Custom format | `--format-output` | - | ❌ | Not implemented |
| Superposed PDB output | `--format-mode 5` | - | ❌ | Not implemented |
| Interactive HTML | `--format-mode 3` | - | ❌ | Not implemented |
| Convert to PDB | `convert2pdb` | - | ❌ | Not implemented |
| Compress CA coords | `compressca` | - | ❌ | Not implemented |
| Create TSV | `createtsv` | - | ❌ | Not implemented |
| Multimer report | `createmultimerreport` | - | ❌ | Not implemented |

---

## 8. Multimer Operations

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Multimer search | `easy-multimersearch`, `multimersearch` | - | ❌ | Not implemented |
| Multimer clustering | `easy-multimercluster`, `multimercluster` | - | ❌ | Not implemented |
| Score multimer | `scoremultimer` | - | ❌ | Not implemented |
| Expand multimer | `expandmultimer` | - | ❌ | Not implemented |
| Complex TM-score | `complexqtmscore`, `complexttmscore` | - | ❌ | Not implemented |
| Complex assignment ID | Output | - | ❌ | Not implemented |
| Interface LDDT | Multimer scoring | - | ❌ | Not implemented |

---

## 9. Advanced Features

| Foldseek Feature | Category | Python Bindings | Status | Notes |
|------------------|----------|-----------------|--------|-------|
| Batch processing | - | `batch_convert()` | ⚠️ | Segfaults on multiple files |
| Parallel processing | `--threads` | - | ❌ | Not exposed |
| GPU acceleration | `--gpu` | - | ❌ | Not available |
| Profile databases | `result2profile` | - | ❌ | Not implemented |
| Multiple sequence alignment | `result2msa` | - | ❌ | Not implemented |
| Query-centered MSA | `result2msa` | - | ❌ | Not implemented |
| Backbone reconstruction | Pulchra wrapper | - | 🔮 | Planned for Phase 1 |
| Structure features | Internal | - | 🔮 | Planned for Phase 1 |
| Embeddings | Internal | - | 🔮 | Planned for Phase 1 |

---

## 10. Output Fields (Available in CLI)

| Output Field | Description | Python Bindings | Status |
|--------------|-------------|-----------------|--------|
| `query` | Query identifier | - | ❌ |
| `target` | Target identifier | - | ❌ |
| `qca` | Query CA coordinates | `Structure.ca_coords` | ✅ |
| `tca` | Target CA coordinates | `Structure.ca_coords` | ✅ |
| `alntmscore` | Alignment TM-score | - | ❌ |
| `qtmscore` | Query-normalized TM-score | `TMscoreResult.tmscore` | ⚠️ |
| `ttmscore` | Target-normalized TM-score | - | ❌ |
| `u` | Rotation matrix | `TMscoreResult.rotation_matrix` | ✅ |
| `t` | Translation vector | `TMscoreResult.translation` | ✅ |
| `lddt` | Average LDDT | - | ❌ |
| `lddtfull` | Per-residue LDDT | - | ❌ |
| `prob` | Homology probability | - | ❌ |
| `fident` | Fraction identical | - | ❌ |
| `alnlen` | Alignment length | - | ❌ |
| `mismatch` | Number of mismatches | - | ❌ |
| `gapopen` | Number of gap opens | - | ❌ |
| `qstart`, `qend` | Query alignment positions | - | ❌ |
| `tstart`, `tend` | Target alignment positions | - | ❌ |
| `evalue` | E-value | - | ❌ |
| `bits` | Bit score | - | ❌ |
| `qaln`, `taln` | Query/target alignment strings | - | ❌ |
| `cigar` | CIGAR string | - | ❌ |

---

## Summary Statistics

### Overall Implementation Status

| Category | Total Features | Implemented | Working | Broken | Not Implemented |
|----------|----------------|-------------|---------|--------|-----------------|
| **File I/O** | 8 | 7 | 7 | 0 | 1 |
| **3Di Conversion** | 9 | 8 | 8 | 0 | 1 |
| **Structure Search** | 8 | 0 | 0 | 0 | 8 |
| **Clustering** | 6 | 0 | 0 | 0 | 6 |
| **Alignment/Scoring** | 14 | 5 | 4 | 1 | 9 |
| **Database Management** | 7 | 0 | 0 | 0 | 7 |
| **Format Conversion** | 9 | 0 | 0 | 0 | 9 |
| **Multimer Operations** | 7 | 0 | 0 | 0 | 7 |
| **Advanced Features** | 9 | 1 | 0 | 1 | 8 |
| **Output Fields** | 26 | 4 | 3 | 1 | 22 |
| **TOTAL** | **103** | **25** | **22** | **3** | **78** |

### Percentage Breakdown

- **✅ Working**: 21.4% (22/103)
- **🚧 Partial/Broken**: 2.9% (3/103)
- **❌ Not Implemented**: 75.7% (78/103)

---

## Phase Completion Status

### ✅ Phase 0: MVP (Complete)
- Load PDB/mmCIF/Foldcomp files
- Convert to 3Di structural alphabet
- Extract sequences and coordinates
- NumPy integration for coordinates

### ✅ Phase 1: Enhanced Functionality (Complete)
- Multi-chain structure support
- Chain iteration
- Format-specific loaders
- Direct coordinate conversion
- ⚠️ Batch processing (broken - segfaults)

### ✅ Phase 2: Alignment & Scoring (Complete)
- TM-align implementation (`TMaligner` class)
- TM-score computation (`compute_tmscore()`)
- Result objects with matrices
- ⚠️ TM-score normalization needs fixing

### ❌ Phase 3: Database & Advanced (Not Started)
- Database reading/writing
- Clustering
- Search functionality
- Profile databases
- Multiple sequence alignment

### ❌ Phase 4: Polish & Release (Not Started)
- Comprehensive documentation
- Multi-platform wheels
- CI/CD setup
- PyPI/Conda release
- Example notebooks

---

## Priority Recommendations

### High Priority (Core Functionality)
1. **Fix TM-score normalization** - Self-alignment should give ~1.0, not 0.004
2. **Fix batch_convert segfault** - Critical for multi-file workflows
3. **LDDT calculation** - Essential structural quality metric
4. **3Di+AA alignment** - Default alignment mode in Foldseek
5. **Database reading** - Access pre-computed Foldseek databases

### Medium Priority (Useful Features)
6. **Structure search** - Core Foldseek use case
7. **Clustering** - Important for large datasets
8. **Format conversion** - Output various formats
9. **Alignment output** - Get aligned sequences, CIGAR strings

### Low Priority (Advanced/Optional)
10. **Multimer operations** - Specialized use case
11. **GPU acceleration** - Performance optimization
12. **Profile databases** - Advanced feature
13. **ProstT5 integration** - Sequence-only predictions

---

## Known Issues

### 🐛 Critical Bugs
1. **Batch processing segfault** (`batch_convert()`)
   - **Status**: Documented in KNOWN_ISSUES.md
   - **Workaround**: Load structures in separate processes
   - **Root cause**: Second `GemmiWrapper::load()` call crashes
   - **Priority**: High

2. **TM-score normalization incorrect**
   - **Status**: Values are wrong (~0.004 for self-alignment instead of ~1.0)
   - **RMSD**: Working correctly (0.0 for self-alignment)
   - **Root cause**: Possibly wrong normalization parameters passed to TMaligner
   - **Priority**: High

### ⚠️ Limitations
- No database I/O (can't read/write Foldseek databases)
- No search functionality (primary use case of Foldseek)
- No clustering support
- Single-process only (no parallelization exposed)
- Limited to structure → 3Di conversion and basic TM-align

---

## Next Steps

Based on this analysis, recommended development priorities:

1. **Bug Fixes** (1-2 days)
   - Debug and fix TM-score normalization
   - Investigate batch_convert segfault

2. **Phase 2 Completion** (2-3 days)
   - Add LDDT calculation
   - Implement 3Di+AA alignment
   - Add alignment output formats

3. **Phase 3: Essential Features** (1-2 weeks)
   - Database reading (read-only first)
   - Basic search functionality
   - Clustering support

4. **Phase 4: Polish** (1 week)
   - Documentation
   - Testing
   - Packaging
   - Release

---

**Total Foldseek Features**: 103
**Implemented in Python**: 25 (24.3%)
**Working Correctly**: 22 (21.4%)
**Ready for Production**: Phase 0-1 features only

This document will be updated as new features are implemented.
