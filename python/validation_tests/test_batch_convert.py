#!/usr/bin/env python3
"""
Test script for batch_convert functionality

NOTE: batch_convert was REMOVED from pyfoldseek due to unresolved segfault
issues when loading multiple structures in the same session. The root cause
is in the underlying gemmi library.

This test file is kept for documentation purposes and demonstrates the issue.
The tests will skip/fail since batch_convert no longer exists.

WORKAROUND: Load structures in separate Python processes or one at a time.
"""

import sys
import os

# Add parent directory to path to import pyfoldseek
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    import pyfoldseek
    from pyfoldseek import Structure
    # batch_convert is no longer available
    batch_convert = None
except ImportError as e:
    print(f"ERROR: Could not import pyfoldseek: {e}")
    print("Please build the package first using: pip install -e .")
    sys.exit(1)


def test_single_structure():
    """Test loading a single structure (baseline test)"""
    print("=" * 60)
    print("Test 1: Loading single structure")
    print("=" * 60)

    # Try multiple possible locations
    test_files = [
        "../../example/1tim.pdb.gz",
        "../example/1tim.pdb.gz",
        "/home/user/foldseek/example/1tim.pdb.gz"
    ]

    test_file = None
    for f in test_files:
        if os.path.exists(f):
            test_file = f
            break

    if not test_file:
        print(f"SKIP: Test file not found in any location")
        return False

    try:
        s = Structure.from_file(test_file)
        print(f"✓ Loaded structure: {s}")
        print(f"  - Length: {s.length}")
        print(f"  - Sequence: {s.sequence[:50]}...")
        print(f"  - 3Di: {s.seq_3di[:50]}...")
        return True
    except Exception as e:
        print(f"✗ FAILED: {e}")
        return False


def test_multiple_structures_sequential():
    """Test loading multiple structures sequentially (the problematic case)"""
    print("\n" + "=" * 60)
    print("Test 2: Loading multiple structures sequentially")
    print("=" * 60)

    # Try multiple possible locations
    test_file_names = ["1tim.pdb.gz", "8tim.pdb.gz"]
    base_dirs = ["../../example/", "../example/", "/home/user/foldseek/example/"]

    test_files = []
    for base_dir in base_dirs:
        if os.path.exists(base_dir):
            test_files = [os.path.join(base_dir, f) for f in test_file_names]
            if all(os.path.exists(f) for f in test_files):
                break

    if not test_files or not all(os.path.exists(f) for f in test_files):
        print(f"SKIP: Test files not found")
        return False

    try:
        structures = []
        for i, filename in enumerate(test_files):
            print(f"\nLoading structure {i+1}/{len(test_files)}: {filename}")
            s = Structure.from_file(filename)
            structures.append(s)
            print(f"✓ Loaded: length={s.length}, seq={s.sequence[:20]}...")

        print(f"\n✓ SUCCESS: Loaded {len(structures)} structures sequentially")
        return True
    except Exception as e:
        print(f"\n✗ FAILED: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_batch_convert():
    """Test the batch_convert function (REMOVED - will skip)"""
    print("\n" + "=" * 60)
    print("Test 3: Using batch_convert function (REMOVED)")
    print("=" * 60)

    if batch_convert is None:
        print("SKIP: batch_convert was removed due to segfault issues")
        return False

    # Try multiple possible locations
    test_file_names = ["1tim.pdb.gz", "8tim.pdb.gz"]
    base_dirs = ["../../example/", "../example/", "/home/user/foldseek/example/"]

    test_files = []
    for base_dir in base_dirs:
        if os.path.exists(base_dir):
            test_files = [os.path.join(base_dir, f) for f in test_file_names]
            if all(os.path.exists(f) for f in test_files):
                break

    if not test_files or not all(os.path.exists(f) for f in test_files):
        print(f"SKIP: Test files not found")
        return False

    try:
        print(f"Batch converting {len(test_files)} files...")
        structures = batch_convert(test_files, reconstruct_backbone=False, num_threads=1)

        print(f"✓ batch_convert returned {len(structures)} structures")
        for i, s in enumerate(structures):
            print(f"  Structure {i+1}: length={s.length}, seq={s.sequence[:20]}...")

        print(f"\n✓ SUCCESS: batch_convert processed all files")
        return True
    except Exception as e:
        print(f"\n✗ FAILED: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_batch_convert_many_files():
    """Test batch_convert with more files (REMOVED - will skip)"""
    print("\n" + "=" * 60)
    print("Test 4: batch_convert with multiple different files (REMOVED)")
    print("=" * 60)

    if batch_convert is None:
        print("SKIP: batch_convert was removed due to segfault issues")
        return False

    # Try to find PDB files in example directory
    test_dirs = ["../../example/", "../example/", "/home/user/foldseek/example/"]
    test_files = []

    for test_dir in test_dirs:
        if os.path.exists(test_dir):
            files = [os.path.join(test_dir, f) for f in os.listdir(test_dir)
                    if f.endswith(('.pdb', '.pdb.gz', '.cif', '.cif.gz'))]
            test_files = files[:5]  # Limit to 5 files
            break

    if len(test_files) < 2:
        print(f"SKIP: Not enough test files found (need at least 2), found {len(test_files)}")
        return False

    print(f"Found {len(test_files)} test files")

    try:
        structures = batch_convert(test_files, reconstruct_backbone=False, num_threads=1)

        print(f"✓ batch_convert returned {len(structures)} structures")
        for i, s in enumerate(structures):
            if hasattr(s, 'length') and hasattr(s, 'sequence'):
                print(f"  Structure {i+1}: length={s.length}")

        print(f"\n✓ SUCCESS: Processed {len(structures)}/{len(test_files)} files")
        return len(structures) > 0
    except Exception as e:
        print(f"\n✗ FAILED: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    """Run all tests"""
    print("=" * 60)
    print("WARNING: batch_convert has been REMOVED")
    print("=" * 60)
    print("batch_convert was removed from pyfoldseek due to unresolved")
    print("segfault issues in the underlying gemmi library.")
    print("\nWorkaround: Load structures in separate processes or one at a time.")
    print("=" * 60)
    print("\nTesting remaining functionality...\n")

    tests = [
        test_single_structure,
        test_multiple_structures_sequential,
        test_batch_convert,
        test_batch_convert_many_files,
    ]

    results = []
    for test in tests:
        result = test()
        results.append(result)

    # Summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)
    passed = sum(1 for r in results if r)
    total = len(results)
    print(f"Passed: {passed}/{total}")

    if passed == total:
        print("\n✓ ALL TESTS PASSED!")
        return 0
    else:
        print(f"\n✗ {total - passed} TEST(S) FAILED")
        return 1


if __name__ == "__main__":
    sys.exit(main())
