#!/usr/bin/env python3
"""
Basic usage example for PyFoldseek

This script demonstrates:
1. Loading a protein structure
2. Accessing the 3Di sequence
3. Accessing coordinates
4. Converting coordinates to 3Di
"""

import sys
from pathlib import Path

try:
    from pyfoldseek import Structure, coords_to_3di
    import numpy as np
except ImportError as e:
    print(f"Error importing pyfoldseek: {e}")
    print("Make sure you have built and installed the package:")
    print("  cd python && pip install -e .")
    sys.exit(1)


def main():
    # Example 1: Load a structure
    print("=" * 60)
    print("Example 1: Loading a protein structure")
    print("=" * 60)

    # Use example PDB from foldseek
    example_pdb = Path(__file__).parent.parent.parent.parent / "example" / "1tim.pdb.gz"

    if not example_pdb.exists():
        print(f"Example PDB not found: {example_pdb}")
        print("Please provide a PDB file as argument")
        if len(sys.argv) > 1:
            example_pdb = Path(sys.argv[1])
        else:
            sys.exit(1)

    print(f"Loading structure from: {example_pdb}")
    struct = Structure.from_file(str(example_pdb))

    print(f"\nStructure loaded successfully!")
    print(f"  Length: {struct.length} residues")
    print(f"  Chains: {struct.chain_names}")
    print(f"  Filename: {struct.filename}")

    # Example 2: Access sequences
    print("\n" + "=" * 60)
    print("Example 2: Accessing sequences")
    print("=" * 60)

    print(f"\nAmino acid sequence (first 50):")
    print(f"  {struct.sequence[:50]}...")

    print(f"\n3Di sequence (first 50):")
    print(f"  {struct.seq_3di[:50]}...")

    print(f"\nSequence lengths:")
    print(f"  AA sequence: {len(struct.sequence)}")
    print(f"  3Di sequence: {len(struct.seq_3di)}")
    print(f"  Match: {len(struct.sequence) == len(struct.seq_3di)}")

    # Example 3: Access coordinates
    print("\n" + "=" * 60)
    print("Example 3: Accessing coordinates")
    print("=" * 60)

    ca_coords = struct.ca_coords
    print(f"\nC-alpha coordinates:")
    print(f"  Shape: {ca_coords.shape}")
    print(f"  Type: {type(ca_coords)}")
    print(f"  First 3 CA atoms:")
    print(ca_coords[:3])

    # Example 4: Convert coordinates to 3Di
    print("\n" + "=" * 60)
    print("Example 4: Converting coordinates to 3Di")
    print("=" * 60)

    # Get all backbone coordinates
    ca = struct.ca_coords
    n = struct.n_coords
    c = struct.c_coords
    cb = struct.cb_coords

    # Convert using standalone function
    seq_3di_converted = coords_to_3di(ca, n, c, cb)

    print(f"\nCoordinate shapes:")
    print(f"  CA: {ca.shape}")
    print(f"  N:  {n.shape}")
    print(f"  C:  {c.shape}")
    print(f"  CB: {cb.shape}")

    print(f"\nConverted 3Di sequence (first 50):")
    print(f"  {seq_3di_converted[:50]}...")

    print(f"\nVerification:")
    print(f"  Matches struct.seq_3di: {seq_3di_converted == struct.seq_3di}")

    # Example 5: Simple analysis
    print("\n" + "=" * 60)
    print("Example 5: Simple structural analysis")
    print("=" * 60)

    # Calculate CA-CA distances
    ca_coords = struct.ca_coords
    distances = np.linalg.norm(ca_coords[1:] - ca_coords[:-1], axis=1)

    print(f"\nCA-CA distances:")
    print(f"  Mean: {distances.mean():.2f} Å")
    print(f"  Std:  {distances.std():.2f} Å")
    print(f"  Min:  {distances.min():.2f} Å")
    print(f"  Max:  {distances.max():.2f} Å")

    # Count 3Di states
    from collections import Counter
    state_counts = Counter(struct.seq_3di)

    print(f"\n3Di state distribution:")
    print(f"  Unique states: {len(state_counts)}")
    print(f"  Most common states:")
    for state, count in state_counts.most_common(5):
        percentage = 100 * count / len(struct.seq_3di)
        print(f"    '{state}': {count} ({percentage:.1f}%)")

    print("\n" + "=" * 60)
    print("All examples completed successfully!")
    print("=" * 60)


if __name__ == "__main__":
    main()
