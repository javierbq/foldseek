# PyFoldseek - Python Bindings for Foldseek

Fast and accurate protein structure search with a Pythonic interface.

## Features

- 🚀 **Fast PDB → 3Di conversion**: Convert protein structures to 3Di structural alphabet
- 🔬 **High-performance C++ backend**: Same speed as command-line Foldseek
- 🐍 **Pythonic API**: Easy to use with NumPy integration
- 📦 **Multiple format support**: PDB, mmCIF, Foldcomp
- 🧬 **Rich structure data**: Access coordinates, sequences, and metadata

## Installation

### From source (current status)

```bash
cd foldseek/python
pip install -e .
```

### Requirements

- Python ≥ 3.7
- NumPy ≥ 1.20
- CMake ≥ 3.15
- C++17 compiler

## Quick Start

```python
from pyfoldseek import Structure

# Load a protein structure
struct = Structure.from_file("protein.pdb")

# Get the 3Di structural alphabet sequence
print(struct.seq_3di)
# Output: "DEHCAAACNLMKMLNILKVNNMCGNKNCAAACVLVNLVLCPMLKAVLLMLL..."

# Get the amino acid sequence
print(struct.sequence)
# Output: "VKVGVNGFGRIGRLVTRAAFNSGKVDVVAINDPFIDLNYMVYMFQYDSTHGK..."

# Access coordinates as NumPy arrays
print(struct.ca_coords.shape)  # (247, 3)
print(struct.n_coords.shape)   # (247, 3)
print(struct.c_coords.shape)   # (247, 3)
print(struct.cb_coords.shape)  # (247, 3)

# Get structure metadata
print(struct.length)         # 247
print(struct.chain_names)    # ['A']
```

## Advanced Usage

### Convert coordinates directly

```python
import numpy as np
from pyfoldseek import coords_to_3di

# Your coordinate arrays (from MD simulation, AlphaFold, etc.)
ca = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], ...])  # (N, 3)
n = np.array([...])   # (N, 3)
c = np.array([...])   # (N, 3)
cb = np.array([...])  # (N, 3)

seq_3di = coords_to_3di(ca, n, c, cb)
print(seq_3di)
```

### Reconstruct backbone from CA-only structures

```python
# For C-alpha only structures (e.g., some AlphaFold models)
struct = Structure.from_file("ca_only.pdb", reconstruct_backbone=True)
```

### Integration with Biopython

```python
from Bio.PDB import PDBParser
from pyfoldseek import coords_to_3di
import numpy as np

# Parse with Biopython
parser = PDBParser()
structure = parser.get_structure("protein", "protein.pdb")

# Extract coordinates
ca_coords = []
n_coords = []
c_coords = []
cb_coords = []

for model in structure:
    for chain in model:
        for residue in chain:
            if 'CA' in residue:
                ca_coords.append(residue['CA'].get_coord())
            if 'N' in residue:
                n_coords.append(residue['N'].get_coord())
            # ... etc

# Convert to 3Di
seq_3di = coords_to_3di(
    np.array(ca_coords),
    np.array(n_coords),
    np.array(c_coords),
    np.array(cb_coords)
)
```

### Batch processing

```python
from pyfoldseek import Structure
from pathlib import Path

# Process multiple structures
structures = []
for pdb_file in Path("structures/").glob("*.pdb"):
    struct = Structure.from_file(str(pdb_file))
    structures.append({
        "name": pdb_file.stem,
        "length": struct.length,
        "seq_3di": struct.seq_3di,
        "sequence": struct.sequence
    })

# Save to file
with open("3di_sequences.fasta", "w") as f:
    for s in structures:
        f.write(f">{s['name']}\n{s['seq_3di']}\n")
```

### MD Trajectory Analysis

```python
import MDAnalysis as mda
from pyfoldseek import coords_to_3di

# Load trajectory
u = mda.Universe("topology.pdb", "trajectory.dcd")

# Analyze structural changes
seq_3di_over_time = []
for ts in u.trajectory[::100]:  # Every 100th frame
    ca = u.select_atoms("name CA").positions
    n = u.select_atoms("name N").positions
    c = u.select_atoms("name C").positions
    cb = u.select_atoms("name CB").positions

    seq_3di = coords_to_3di(ca, n, c, cb)
    seq_3di_over_time.append(seq_3di)

# Analyze how structure changes
print(f"Analyzed {len(seq_3di_over_time)} frames")
```

## API Reference

### `Structure` class

Main class for loading and analyzing protein structures.

#### Methods

- **`Structure.from_file(filename, reconstruct_backbone=False)`**

  Load structure from file.

  **Parameters:**
  - `filename` (str): Path to PDB/mmCIF/Foldcomp file
  - `reconstruct_backbone` (bool): Reconstruct N, C, CB from CA-only structures

  **Returns:** `Structure` object

#### Properties

- `sequence` (str): Amino acid sequence (one-letter code)
- `seq_3di` (str): 3Di structural alphabet sequence
- `length` (int): Number of residues
- `ca_coords` (np.ndarray): C-alpha coordinates, shape (N, 3)
- `n_coords` (np.ndarray): Nitrogen coordinates, shape (N, 3)
- `c_coords` (np.ndarray): Carbon coordinates, shape (N, 3)
- `cb_coords` (np.ndarray): C-beta coordinates, shape (N, 3)
- `chain_names` (List[str]): Chain identifiers
- `filename` (str): Source filename

### `coords_to_3di()` function

Convert atomic coordinates to 3Di structural alphabet.

**Parameters:**
- `ca` (np.ndarray): C-alpha coordinates, shape (N, 3)
- `n` (np.ndarray): Nitrogen coordinates, shape (N, 3)
- `c` (np.ndarray): Carbon coordinates, shape (N, 3)
- `cb` (np.ndarray): C-beta coordinates, shape (N, 3)

**Returns:** str - 3Di sequence

### `Vec3` class

3D coordinate vector (mainly for internal use).

**Attributes:**
- `x` (float): X coordinate
- `y` (float): Y coordinate
- `z` (float): Z coordinate

## Development

### Building from source

```bash
# Clone the repository
git clone https://github.com/steineggerlab/foldseek.git
cd foldseek

# Build main foldseek first
mkdir build && cd build
cmake ..
make -j$(nproc)

# Build Python bindings
cd ../python
pip install -e ".[dev]"
```

### Running tests

```bash
cd python
pytest tests/ -v
```

### Running with coverage

```bash
pytest tests/ --cov=pyfoldseek --cov-report=html
```

## Roadmap

### Phase 1 (Current - MVP)
- [x] PDB/mmCIF loading
- [x] 3Di sequence generation
- [x] Coordinate access as NumPy arrays
- [x] Basic tests

### Phase 2 (Planned)
- [ ] Multi-chain support
- [ ] TM-align bindings
- [ ] Structure alignment
- [ ] LDDT calculation

### Phase 3 (Future)
- [ ] Database reading/writing
- [ ] Clustering algorithms
- [ ] Search functionality
- [ ] ProstT5 integration

## Citation

If you use PyFoldseek in your research, please cite the original Foldseek paper:

```bibtex
@article{foldseek2023,
  title={Fast and accurate protein structure search with Foldseek},
  author={van Kempen, Michel and Kim, Stephanie S and Tumescheit, Charlotte and Mirdita, Milot and Lee, Jeongjae and Gilchrist, Cameron LM and S{\"o}ding, Johannes and Steinegger, Martin},
  journal={Nature Biotechnology},
  year={2023},
  doi={10.1038/s41587-023-01773-0}
}
```

## License

Same license as Foldseek (GPL-3.0)

## Support

- **Issues**: [GitHub Issues](https://github.com/steineggerlab/foldseek/issues)
- **Discussions**: [GitHub Discussions](https://github.com/steineggerlab/foldseek/discussions)
- **Documentation**: [Foldseek Docs](https://github.com/steineggerlab/foldseek)

## Contributing

Contributions are welcome! Please see the main Foldseek repository for contribution guidelines.
