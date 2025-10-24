# Foldseek Python Bindings: Implementation Status

**Date**: 2025-10-23
**Python Version**: 0.2.0
**Status**: Early development - ~21% of CLI features implemented

---

## Quick Summary

| Category | Status | Priority |
|----------|--------|----------|
| **✅ Implemented & Working** | 22 features (21%) | - |
| **❌ Not Implemented** | 78 features (76%) | High |
| **⚠️ Broken/Removed** | 3 features (3%) | Fix needed |

---

## ✅ What's Implemented (Working)

### 1. File I/O & Structure Loading (7/8 features)
```python
from pyfoldseek import Structure

# Load structures
s = Structure.from_file("protein.pdb")        # ✅ PDB
s = Structure.from_file("protein.cif.gz")     # ✅ mmCIF (gzipped)
s = Structure.from_file("protein.fcz")        # ✅ Foldcomp
s = Structure.from_pdb("protein.pdb")         # ✅ Format-specific
s = Structure.from_mmcif("protein.cif")       # ✅ Format-specific
s = Structure.from_foldcomp("protein.fcz")    # ✅ Format-specific

# Multi-chain support
for chain in s:                                # ✅ Iterate chains
    print(chain.name, chain.sequence)
```

**Missing**: Load from tar.gz archives

---

### 2. 3Di Structural Alphabet (8/9 features)
```python
# Get 3Di encoding
print(s.seq_3di)           # ✅ 3Di sequence
print(s.sequence)          # ✅ Amino acid sequence
print(s.ca_coords)         # ✅ CA coordinates (NumPy)
print(s.n_coords)          # ✅ N coordinates
print(s.c_coords)          # ✅ C coordinates
print(s.cb_coords)         # ✅ CB coordinates

# Direct conversion from coordinates
from pyfoldseek import coords_to_3di
seq_3di = coords_to_3di(ca, n, c, cb)  # ✅ Works
```

**Missing**: ProstT5 prediction from FASTA sequence

---

### 3. Alignment & Scoring (5/14 features)
```python
from pyfoldseek import TMaligner, compute_tmscore, compute_lddt

# TM-align
s1 = Structure.from_file("protein1.pdb")
s2 = Structure.from_file("protein2.pdb")

result = compute_tmscore(s1.ca_coords, s2.ca_coords,
                         s1.sequence, s2.sequence)
print(f"TM-score: {result.tmscore}")        # ✅ Works
print(f"RMSD: {result.rmsd}")                # ✅ Works
print(f"Rotation: {result.rotation_matrix}") # ✅ 3x3 matrix
print(f"Translation: {result.translation}")  # ✅ 3D vector

# LDDT (newly added!)
alignment = "MMM" + "I" + "MMM" + "D" + "M" * 100
lddt = compute_lddt(s1.ca_coords, s2.ca_coords, alignment)
print(f"LDDT: {lddt.average}")               # ✅ Works
print(f"Per-residue: {lddt.per_residue}")    # ✅ NumPy array
```

**Missing**:
- 3Di+AA alignment (default Foldseek alignment)
- LoLalign
- 3Di Smith-Waterman
- Alignment output (CIGAR, aligned sequences)
- E-value, bit score
- Target-normalized TM-score

**Broken**: TM-score normalization (gives 0.004 instead of 1.0 for self-alignment)

---

## ❌ What's Missing (Critical for Foldseek)

### 1. Structure Search - **COMPLETELY MISSING** ⚠️
**This is the PRIMARY use case of Foldseek!**

```bash
# CLI works:
foldseek easy-search query.pdb targetDB.fdb result.m8 tmp/

# Python: NOT AVAILABLE ❌
```

**What's needed**:
- `search()` - Search a structure against database
- `easy_search()` - Simple search interface
- Database format reading
- Result parsing (m8 format, custom formats)

**Impact**: Cannot use Python for main Foldseek workflow

---

### 2. Database Management - **COMPLETELY MISSING**

```bash
# CLI works:
foldseek createdb proteins/ db
foldseek databases PDB pdb tmp/

# Python: NOT AVAILABLE ❌
```

**What's needed**:
- Read Foldseek databases
- Create databases (optional)
- Database iteration
- Index management

**Impact**: Cannot access pre-computed databases (PDB, AlphaFold, etc.)

---

### 3. Clustering - **COMPLETELY MISSING**

```bash
# CLI works:
foldseek easy-cluster proteins/ results tmp/

# Python: NOT AVAILABLE ❌
```

**What's needed**:
- Cluster structures by similarity
- Set-cover clustering
- Connected-component clustering

**Impact**: Cannot cluster large structure sets

---

### 4. 3Di+AA Alignment - **MISSING** (Important!)

```bash
# CLI default alignment uses 3Di + amino acid
foldseek align query.pdb target.pdb

# Python: Only TM-align available ❌
```

**What's needed**:
- 3Di Smith-Waterman alignment
- 3Di+AA combined alignment (Foldseek's secret sauce)
- Alignment statistics (coverage, identity, etc.)
- CIGAR string output

**Impact**: Cannot do the same alignments as CLI

---

### 5. Alignment Output & Format Conversion - **MISSING**

```bash
# CLI works:
foldseek convertalis db1 db2 result.m8 result.html

# Python: NOT AVAILABLE ❌
```

**What's needed**:
- Parse alignment results
- Convert to different formats (BLAST-tab, SAM, HTML)
- Extract alignment strings
- Superposed PDB output

---

## 📊 Feature Comparison by Category

### Core Features

| Feature | CLI | Python | Notes |
|---------|-----|--------|-------|
| **Load PDB/mmCIF** | ✅ | ✅ | Working |
| **3Di conversion** | ✅ | ✅ | Working |
| **TM-align** | ✅ | ✅ | Working (normalization bug) |
| **LDDT** | ✅ | ✅ | **NEW! Just added** |
| **Structure search** | ✅ | ❌ | **CRITICAL MISSING** |
| **Database access** | ✅ | ❌ | **CRITICAL MISSING** |
| **Clustering** | ✅ | ❌ | Missing |
| **3Di+AA align** | ✅ | ❌ | Missing (important) |
| **Alignment output** | ✅ | ❌ | Missing |

### Advanced Features

| Feature | CLI | Python | Notes |
|---------|-----|--------|-------|
| **Multimer search** | ✅ | ❌ | Missing |
| **Multimer clustering** | ✅ | ❌ | Missing |
| **Profile databases** | ✅ | ❌ | Missing |
| **MSA generation** | ✅ | ❌ | Missing |
| **GPU acceleration** | ✅ | ❌ | Not available |
| **Parallel processing** | ✅ | ❌ | Not exposed |

---

## 🎯 Priority Implementation List

### Tier 1: CRITICAL (Makes Python useful for main workflow)

1. **Database Reading** (Highest priority!)
   - Read Foldseek database format
   - Iterate over database entries
   - Extract structures and 3Di sequences
   - **Impact**: Can access PDB, AlphaFold databases

2. **Structure Search**
   - Basic search against database
   - Return alignment results
   - Parse m8 format output
   - **Impact**: Main use case of Foldseek

3. **3Di+AA Alignment**
   - Smith-Waterman on 3Di
   - Combined 3Di+AA scoring
   - CIGAR string output
   - **Impact**: Same alignments as CLI

4. **Fix TM-score normalization**
   - Should give ~1.0 for self-alignment
   - Currently gives 0.004
   - **Impact**: TM-scores are wrong

### Tier 2: Important (Extends functionality)

5. **Clustering**
   - Basic clustering algorithm
   - Set-cover clustering
   - **Impact**: Process large datasets

6. **Alignment Output**
   - Extract aligned sequences
   - CIGAR strings
   - Coverage, identity statistics
   - **Impact**: Analyze alignment quality

7. **Format Conversion**
   - BLAST-tab format
   - HTML visualization
   - Superposed PDB output
   - **Impact**: Better visualization

### Tier 3: Nice to have

8. **Multimer operations**
9. **Profile databases**
10. **MSA generation**
11. **GPU acceleration**

---

## 🚧 Current Limitations

### What Works Well ✅
- Single structure loading and 3Di conversion
- TM-align (except normalization bug)
- LDDT calculation (just added!)
- Multi-chain support
- NumPy integration

### What Doesn't Work ⚠️
- **batch_convert()** - Removed due to segfaults
- **TM-score normalization** - Wrong values
- **Multiple file loading** - Need separate processes

### What's Not Available ❌
- **Structure search** - Primary Foldseek feature
- **Database access** - Can't read PDB/AlphaFold DBs
- **Clustering** - Can't cluster structures
- **3Di+AA alignment** - Different from CLI
- **Alignment parsing** - Can't process results

---

## 💡 Use Case Analysis

### What You CAN Do Now

```python
# ✅ Convert single structure to 3Di
s = Structure.from_file("protein.pdb")
print(s.seq_3di)

# ✅ Align two structures with TM-align
result = compute_tmscore(s1.ca_coords, s2.ca_coords, s1.sequence, s2.sequence)

# ✅ Calculate LDDT between aligned structures
lddt = compute_lddt(s1.ca_coords, s2.ca_coords, alignment)

# ✅ Extract coordinates for downstream analysis
import numpy as np
ca = s.ca_coords  # Use with other tools
```

### What You CANNOT Do (Must use CLI)

```bash
# ❌ Search against database
foldseek easy-search query.pdb pdb results.m8 tmp/

# ❌ Cluster many structures
foldseek easy-cluster proteins/ results tmp/

# ❌ Access pre-computed databases
foldseek databases PDB pdb tmp/

# ❌ Get alignment details
foldseek convertalis db1 db2 aln result.html --format-mode 3
```

---

## 📈 Implementation Status

```
Total Features: 103
Implemented: 25 (24.3%)
Working: 22 (21.4%)
Broken: 3 (2.9%)
Missing: 78 (75.7%)
```

### By Category:
- ✅ File I/O: 88% complete (7/8)
- ✅ 3Di Conversion: 89% complete (8/9)
- ⚠️ Alignment: 36% complete (5/14, 1 broken)
- ❌ Search: 0% complete (0/8)
- ❌ Clustering: 0% complete (0/6)
- ❌ Database: 0% complete (0/7)
- ❌ Format conversion: 0% complete (0/9)
- ❌ Multimer: 0% complete (0/7)

---

## 🎓 Recommendations

### For Users

**Current Python bindings are good for**:
- Prototyping 3Di conversion
- Computing TM-align and LDDT on pairs
- Extracting structural features
- Integration with other Python tools

**Use CLI for**:
- Structure search (primary use case)
- Database operations
- Clustering
- Production workflows

### For Developers

**Priority order to make Python useful**:
1. Database reading (critical!)
2. Structure search (critical!)
3. 3Di+AA alignment (important)
4. Fix TM-score normalization (bug)
5. Clustering (nice to have)

**Estimated effort**:
- Tier 1 features: 2-3 weeks
- Tier 2 features: 1-2 weeks
- Tier 3 features: 2-3 weeks

**Total to feature parity**: 5-8 weeks of development

---

## 📝 Summary

The Python bindings currently provide **basic structure loading and alignment** but are **missing the core Foldseek functionality** (search, databases, clustering).

To make the Python bindings truly useful, we need to implement **database reading** and **structure search** - these are what Foldseek is primarily used for!

**Current status**: Good for prototyping, not yet for production workflows.
