# PyFoldseek Quick Start Guide

## Installation

```bash
pip install pyfoldseek-0.2.0-cp311-cp311-linux_x86_64.whl
```

## Basic Usage

### 1. Load a Structure and Get 3Di Sequence

```python
from pyfoldseek import Structure

# Load PDB file
struct = Structure.from_file("protein.pdb")

# Get amino acid sequence
print(struct.sequence)
# Output: APRKFFVGGNWKMNGKRKSLGELIHTLDGA...

# Get 3Di structural alphabet
print(struct.seq_3di)
# Output: (binary 3Di encoding)

# Basic info
print(f"Length: {struct.length} residues")
print(f"Chains: {struct.num_chains}")
```

### 2. Access Coordinates as NumPy Arrays

```python
import numpy as np

# Get coordinate arrays
ca_coords = struct.ca_coords   # Shape: (N, 3)
n_coords = struct.n_coords
c_coords = struct.c_coords
cb_coords = struct.cb_coords

# Calculate CA-CA distances
distances = np.linalg.norm(ca_coords[1:] - ca_coords[:-1], axis=1)
print(f"Mean CA-CA distance: {distances.mean():.2f} Å")
```

### 3. Work with Multi-Chain Structures

```python
# Iterate over chains
for chain in struct:
    print(f"Chain {chain.name}:")
    print(f"  Sequence: {chain.sequence[:30]}...")
    print(f"  3Di: {chain.seq_3di[:30]}...")
    print(f"  Length: {chain.length}")

# Access specific chain
chain_a = struct.get_chain(0)
print(chain_a.ca_coords)  # NumPy array for this chain only
```

### 4. Use Format-Specific Loaders

```python
# Load PDB
struct_pdb = Structure.from_pdb("protein.pdb")

# Load mmCIF
struct_cif = Structure.from_mmcif("protein.cif")

# Load Foldcomp (compressed format)
struct_fcz = Structure.from_foldcomp("protein.fcz")

# Auto-detect format
struct_auto = Structure.from_file("protein.pdb")  # Detects automatically
```

### 5. Convert Coordinates Directly to 3Di

```python
import numpy as np
from pyfoldseek import coords_to_3di

# Prepare coordinates (N residues x 3 dimensions)
ca = np.random.randn(100, 3)
n = np.random.randn(100, 3)
c = np.random.randn(100, 3)
cb = np.random.randn(100, 3)

# Convert to 3Di
seq_3di = coords_to_3di(ca, n, c, cb)
print(f"3Di sequence: {seq_3di}")
```

## Common Workflows

### Workflow 1: Compare Two Structures

```python
# Load two structures
struct1 = Structure.from_file("protein1.pdb")
struct2 = Structure.from_file("protein2.pdb")

# Compare sequences
print(f"Structure 1: {len(struct1.sequence)} residues")
print(f"Structure 2: {len(struct2.sequence)} residues")

# Compare 3Di sequences (basic comparison)
# For proper alignment, use external tools or future Phase 2 features
```

### Workflow 2: Extract Chain from Complex

```python
# Load multi-chain complex
complex_struct = Structure.from_file("complex.pdb")

print(f"Total chains: {complex_struct.num_chains}")

# Process each chain separately
for i, chain in enumerate(complex_struct):
    print(f"\nChain {i} ({chain.name}):")
    print(f"  Length: {chain.length}")
    print(f"  First 20 residues: {chain.sequence[:20]}")

    # Calculate RMSD (example)
    ca = chain.ca_coords
    centroid = ca.mean(axis=0)
    centered = ca - centroid
    rmsd = np.sqrt((centered**2).sum() / len(ca))
    print(f"  RMSD from centroid: {rmsd:.2f} Å")
```

### Workflow 3: Batch Analysis with Separate Processes

Due to known limitations with multiple file loading, process files separately:

```python
import subprocess
import glob

# Find all PDB files
pdb_files = glob.glob("structures/*.pdb")

# Process each in separate Python process
for pdb_file in pdb_files:
    result = subprocess.run([
        "python", "-c",
        f"from pyfoldseek import Structure; "
        f"s = Structure.from_file('{pdb_file}'); "
        f"print(f'{pdb_file}: {{s.length}} residues')"
    ], capture_output=True, text=True)
    print(result.stdout)
```

## Tips and Best Practices

### 1. Check Structure Before Processing

```python
try:
    struct = Structure.from_file("protein.pdb")
    if struct.length == 0:
        print("Warning: Empty structure")
    elif struct.num_chains == 0:
        print("Warning: No chains found")
    else:
        print(f"✓ Loaded {struct.length} residues in {struct.num_chains} chain(s)")
except Exception as e:
    print(f"✗ Failed to load: {e}")
```

### 2. Handle Compressed Files

```python
# Works with gzipped files
struct = Structure.from_file("protein.pdb.gz")

# Works with Foldcomp compressed format
struct = Structure.from_file("protein.fcz")
```

### 3. Memory-Efficient Processing

For large-scale analysis:

```python
# Don't store all structures in memory
results = []
for pdb_file in large_file_list:
    # Process in separate script or subprocess
    subprocess.run(["python", "process_one.py", pdb_file])
    # Only save results, not full structures
```

## Performance Notes

- **Single file processing**: ~0.1s for 247 residues
- **3Di conversion**: Highly optimized C++ backend
- **NumPy integration**: Zero-copy where possible
- **Multi-chain**: Efficient iteration without copying

## Known Limitations

1. **Multiple file loading in same session crashes** - Use separate processes
2. **Batch processing** - `batch_convert()` function is not working (see KNOWN_ISSUES.md)
3. **Platform**: Currently tested on Linux only

See KNOWN_ISSUES.md for details and workarounds.

## Next Steps

- Explore multi-chain structures
- Integrate with structural analysis pipelines
- Combine with AlphaFold predictions
- Use for high-throughput structure screening

## Getting Help

- Documentation: See README.md and BUILD_STATUS.md
- Issues: https://github.com/steineggerlab/foldseek/issues
- Foldseek Paper: https://www.nature.com/articles/s41587-023-01773-0

## Version

This guide is for **pyfoldseek 0.2.0** (Phase 1 features)
