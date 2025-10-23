# Installation Guide - PyFoldseek

## Quick Install

### From Wheel (Recommended)

The easiest way to install pyfoldseek is from a pre-built wheel:

```bash
pip install pyfoldseek-0.2.0-cp311-cp311-linux_x86_64.whl
```

### Requirements

- Python >= 3.7
- NumPy >= 1.20
- Linux x86_64 (for the provided wheel)

## Installation Methods

### 1. Install from Wheel File

```bash
# Install directly
pip install pyfoldseek-0.2.0-cp311-cp311-linux_x86_64.whl

# Or with specific Python version
python3.11 -m pip install pyfoldseek-0.2.0-cp311-cp311-linux_x86_64.whl
```

### 2. Install from Source (Advanced)

**Prerequisites:**
- CMake >= 3.15
- C++ compiler with C++17 support
- Foldseek source code and dependencies
- pybind11 >= 2.10.0

**Steps:**

1. Build foldseek first (required):
```bash
cd /path/to/foldseek
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make -j$(nproc)
```

2. Build and install Python bindings:
```bash
cd /path/to/foldseek/python
pip install .
```

Or for development (editable install):
```bash
pip install -e .
```

### 3. Build Your Own Wheel

```bash
cd /path/to/foldseek/python

# Install build tools
pip install build wheel

# Build the wheel
python -m build --wheel

# Install the wheel
pip install dist/pyfoldseek-0.2.0-cp311-cp311-linux_x86_64.whl
```

## Verify Installation

```python
import pyfoldseek
print(f"pyfoldseek version: {pyfoldseek.__version__}")

# Test basic functionality
from pyfoldseek import Structure
struct = Structure.from_file("protein.pdb")
print(f"Loaded structure with {struct.length} residues")
print(f"3Di sequence: {struct.seq_3di}")
```

## Platform-Specific Notes

### Linux
- Tested on Ubuntu 22.04, Debian 12
- Requires glibc 2.31+
- Pre-built wheel is for Python 3.11 on x86_64

### macOS
- Not tested yet
- May require building from source
- Install dependencies via Homebrew

### Windows
- Not supported yet
- WSL2 recommended for Windows users

## Troubleshooting

### Import Error: No module named 'pyfoldseek.pyfoldseek'

The C++ extension wasn't built properly. Try:
```bash
pip uninstall pyfoldseek
pip install pyfoldseek-0.2.0-cp311-cp311-linux_x86_64.whl --force-reinstall
```

### ImportError: undefined symbol

Missing dependencies. Make sure you have:
```bash
sudo apt-get install libomp-dev libatomic-ops-dev
```

### Multiple File Loading Crashes

This is a known limitation (see KNOWN_ISSUES.md). Use separate Python processes for multiple files:
```python
import subprocess
for file in files:
    subprocess.run(["python", "process_single.py", file])
```

## Development Installation

For contributors:

```bash
# Clone repository
git clone https://github.com/steineggerlab/foldseek.git
cd foldseek

# Build foldseek
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make -j$(nproc)

# Install Python bindings in development mode
cd ../python
pip install -e ".[dev]"

# Run tests
pytest tests/
```

## Uninstallation

```bash
pip uninstall pyfoldseek
```

## Getting Help

- Issues: https://github.com/steineggerlab/foldseek/issues
- Documentation: https://github.com/steineggerlab/foldseek/tree/master/python
- Foldseek Paper: https://www.nature.com/articles/s41587-023-01773-0

## Version History

**0.2.0** (Current)
- Multi-chain support
- Format-specific loaders (PDB, mmCIF, Foldcomp)
- Chain iteration
- NumPy coordinate arrays
- Phase 1 features complete

**0.1.0**
- Initial release
- Basic PDB → 3Di conversion
- Single structure support
