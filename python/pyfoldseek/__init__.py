"""
PyFoldseek - Python bindings for Foldseek
==========================================

Fast and accurate protein structure search and alignment.

Main classes and functions:
- Structure: Load and analyze protein structures
- coords_to_3di: Convert coordinates to 3Di alphabet

Example usage:
    >>> from pyfoldseek import Structure
    >>> struct = Structure.from_file("protein.pdb")
    >>> print(struct.seq_3di)
    >>> print(struct.sequence)
    >>> print(struct.ca_coords.shape)
"""

from ._version import __version__

# Import C++ extension
try:
    from .pyfoldseek import (
        Structure,
        Vec3,
        coords_to_3di,
        __version__ as _cpp_version,
    )
except ImportError as e:
    raise ImportError(
        "Failed to import pyfoldseek C++ extension. "
        "Make sure the package is properly installed. "
        f"Error: {e}"
    )

__all__ = [
    "Structure",
    "Vec3",
    "coords_to_3di",
    "__version__",
]
