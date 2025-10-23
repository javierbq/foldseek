"""
PyFoldseek - Python bindings for Foldseek
==========================================

Fast and accurate protein structure search and alignment.

Main classes and functions:
- Structure: Load and analyze protein structures
- Chain: Individual chain within a structure
- coords_to_3di: Convert coordinates to 3Di alphabet
- TMaligner: TM-align structural alignment
- compute_tmscore: Compute TM-score between structures
- LDDTCalculator: LDDT structural similarity calculator
- compute_lddt: Compute LDDT between structures
- Database: Read Foldseek databases
- DatabaseEntry: Single database entry
- search: Search structures against a database
- SearchHit: Single search result
- cluster: Cluster database structures by similarity
- Cluster: Clustering result

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
    >>> # TM-align
    >>> from pyfoldseek import compute_tmscore
    >>> s1 = Structure.from_file("protein1.pdb")
    >>> s2 = Structure.from_file("protein2.pdb")
    >>> result = compute_tmscore(s1.ca_coords, s2.ca_coords, s1.sequence, s2.sequence)
    >>> print(f"TM-score: {result.tmscore:.3f}")
"""

from ._version import __version__

# Import C++ extension
try:
    from .pyfoldseek import (
        Structure,
        Chain,
        Vec3,
        coords_to_3di,
        TMaligner,
        TMscoreResult,
        compute_tmscore,
        LDDTCalculator,
        LDDTResult,
        compute_lddt,
        Database,
        DatabaseEntry,
        search,
        SearchHit,
        cluster,
        Cluster,
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
    "TMaligner",
    "TMscoreResult",
    "compute_tmscore",
    "LDDTCalculator",
    "LDDTResult",
    "compute_lddt",
    "Database",
    "DatabaseEntry",
    "search",
    "SearchHit",
    "cluster",
    "Cluster",
    "__version__",
]
