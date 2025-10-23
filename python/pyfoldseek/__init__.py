"""
PyFoldseek - Python bindings for Foldseek
==========================================

Fast and accurate protein structure search and alignment.

Main classes and functions:
- Structure: Load and analyze protein structures
- Chain: Individual chain within a structure
- coords_to_3di: Convert coordinates to 3Di alphabet
- batch_convert: Process multiple structures in parallel

Example usage:
    >>> from pyfoldseek import Structure
    >>> struct = Structure.from_file("protein.pdb")
    >>> print(struct.seq_3di)
    >>> print(struct.sequence)
    >>>
    >>> # Multi-chain support
    >>> for chain in struct:
    >>>     print(f"{chain.name}: {chain.seq_3di}")
    >>>
    >>> # Batch processing
    >>> from pyfoldseek import batch_convert
    >>> structures = batch_convert(["p1.pdb", "p2.pdb", "p3.pdb"])
"""

from ._version import __version__

# Import C++ extension
try:
    from .pyfoldseek import (
        Structure,
        Chain,
        Vec3,
        coords_to_3di,
        batch_convert,
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
    "Chain",
    "Vec3",
    "coords_to_3di",
    "batch_convert",
    "__version__",
]
