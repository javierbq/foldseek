#!/usr/bin/env python3
"""
Multi-chain structure processing example

Demonstrates:
1. Loading multi-chain structures
2. Iterating over chains
3. Processing individual chains
4. Batch processing multiple files
"""

import sys
from pathlib import Path

try:
    from pyfoldseek import Structure, batch_convert
    import numpy as np
except ImportError as e:
    print(f"Error importing: {e}")
    print("Make sure you have built and installed the package:")
    print("  cd python && pip install -e .")
    sys.exit(1)


def example_multi_chain():
    """Example: Working with multi-chain structures"""
    print("=" * 60)
    print("Example 1: Multi-chain structure analysis")
    print("=" * 60)

    # Use example PDB (or provide your own multi-chain structure)
    example_pdb = Path(__file__).parent.parent.parent.parent / "example" / "1tim.pdb.gz"

    if len(sys.argv) > 1:
        example_pdb = Path(sys.argv[1])

    if not example_pdb.exists():
        print(f"PDB file not found: {example_pdb}")
        print("Usage: python multi_chain_example.py [structure.pdb]")
        return

    print(f"\nLoading structure: {example_pdb}")
    struct = Structure.from_file(str(example_pdb))

    print(f"\nStructure information:")
    print(f"  File: {struct.filename}")
    print(f"  Number of chains: {struct.num_chains}")
    print(f"  Total residues (first chain): {struct.length}")

    # Iterate over all chains
    print(f"\nChain details:")
    for i, chain in enumerate(struct):
        print(f"\n  Chain {i}: {chain.name}")
        print(f"    Length: {len(chain)} residues")
        print(f"    Sequence (first 30): {chain.sequence[:30]}...")
        print(f"    3Di (first 30): {chain.seq_3di[:30]}...")
        print(f"    CA coords shape: {chain.ca_coords.shape}")

    # Access specific chain
    print(f"\n" + "=" * 60)
    print("Example 2: Accessing specific chains")
    print("=" * 60)

    if struct.num_chains > 0:
        first_chain = struct.get_chain(0)
        print(f"\nFirst chain: {first_chain.name}")
        print(f"  Sequence: {first_chain.sequence[:50]}...")
        print(f"  3Di: {first_chain.seq_3di[:50]}...")

        # Calculate some statistics
        ca = first_chain.ca_coords
        distances = np.linalg.norm(ca[1:] - ca[:-1], axis=1)
        print(f"\n  CA-CA distances:")
        print(f"    Mean: {distances.mean():.2f} Å")
        print(f"    Std: {distances.std():.2f} Å")


def example_batch_processing():
    """Example: Batch processing multiple structures"""
    print("\n" + "=" * 60)
    print("Example 3: Batch processing")
    print("=" * 60)

    # Create a list of PDB files to process
    example_dir = Path(__file__).parent.parent.parent.parent / "example"

    if not example_dir.exists():
        print("Example directory not found")
        return

    # Find all PDB files in the directory
    pdb_files = list(example_dir.glob("*.pdb*"))

    if not pdb_files:
        print("No PDB files found in example directory")
        return

    # Take first few files
    files_to_process = [str(f) for f in pdb_files[:3]]

    print(f"\nProcessing {len(files_to_process)} files:")
    for f in files_to_process:
        print(f"  - {Path(f).name}")

    # Batch convert
    print(f"\nConverting...")
    structures = batch_convert(files_to_process)

    print(f"\nSuccessfully loaded {len(structures)} structures")

    # Summary
    for struct in structures:
        print(f"\n  {Path(struct.filename).name}:")
        print(f"    Chains: {struct.num_chains}")
        print(f"    Length: {struct.length}")
        print(f"    3Di (first 20): {struct.seq_3di[:20]}...")


def example_chain_comparison():
    """Example: Compare chains within a structure"""
    print("\n" + "=" * 60)
    print("Example 4: Chain comparison")
    print("=" * 60)

    example_pdb = Path(__file__).parent.parent.parent.parent / "example" / "1tim.pdb.gz"

    if not example_pdb.exists():
        print("Example PDB not found")
        return

    struct = Structure.from_file(str(example_pdb))

    if struct.num_chains < 2:
        print("Structure has only one chain, skipping comparison")
        return

    # Compare first two chains
    chain1 = struct.get_chain(0)
    chain2 = struct.get_chain(1)

    print(f"\nComparing chains {chain1.name} and {chain2.name}:")
    print(f"  Chain {chain1.name}: {len(chain1)} residues")
    print(f"  Chain {chain2.name}: {len(chain2)} residues")

    # Simple sequence identity
    if len(chain1.sequence) == len(chain2.sequence):
        matches = sum(a == b for a, b in zip(chain1.sequence, chain2.sequence))
        identity = 100.0 * matches / len(chain1.sequence)
        print(f"  Sequence identity: {identity:.1f}%")

        # 3Di similarity
        matches_3di = sum(a == b for a, b in zip(chain1.seq_3di, chain2.seq_3di))
        similarity_3di = 100.0 * matches_3di / len(chain1.seq_3di)
        print(f"  3Di similarity: {similarity_3di:.1f}%")
    else:
        print(f"  Chains have different lengths, skipping comparison")


def example_features():
    """Example: Access intermediate geometric features"""
    print("\n" + "=" * 60)
    print("Example 5: Intermediate geometric features")
    print("=" * 60)

    example_pdb = Path(__file__).parent.parent.parent.parent / "example" / "1tim.pdb.gz"

    if not example_pdb.exists():
        print("Example PDB not found")
        return

    # Load with features
    print("\nLoading structure with geometric features...")
    struct = Structure.from_file(str(example_pdb), compute_features=True)

    print(f"\nStructure: {Path(struct.filename).name}")
    print(f"  Length: {struct.length}")

    try:
        features = struct.features
        print(f"  Features shape: {features.shape}")
        print(f"  Features are {features.shape[1]}-dimensional geometric descriptors")
        print(f"\n  Feature statistics:")
        print(f"    Mean: {features.mean(axis=0)}")
        print(f"    Std: {features.std(axis=0)}")
    except RuntimeError as e:
        print(f"  Error accessing features: {e}")


def main():
    """Run all examples"""
    try:
        example_multi_chain()
        example_batch_processing()
        example_chain_comparison()
        example_features()

        print("\n" + "=" * 60)
        print("All Phase 1 examples completed!")
        print("=" * 60)

    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
