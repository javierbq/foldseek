#!/usr/bin/env python3
"""
Test database reading functionality

Tests the Database and DatabaseEntry classes for reading Foldseek databases.
"""

import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from pyfoldseek import Database
import numpy as np


def get_test_db_path():
    """Get absolute path to test database"""
    return os.path.abspath(os.path.join(os.path.dirname(__file__), '../../testdb_py'))


def test_database_open():
    """Test opening a Foldseek database"""
    print("TEST: Database opening")

    # Open database
    db = Database(get_test_db_path())

    # Check basic properties
    assert len(db) > 0, "Database should not be empty"
    print(f"  ✓ Opened database with {len(db)} entries")

    return True


def test_database_entry_access():
    """Test accessing database entries"""
    print("TEST: Database entry access")

    db = Database(get_test_db_path())

    # Get entry by index
    entry = db[0]
    assert entry is not None, "Entry should not be None"
    assert entry.key >= 0, "Entry key should be non-negative"
    assert len(entry.name) > 0, "Entry name should not be empty"
    assert len(entry.sequence) > 0, "Entry sequence should not be empty"

    print(f"  ✓ Entry access works")
    print(f"    Key: {entry.key}")
    print(f"    Name: {entry.name[:60]}...")
    print(f"    Length: {entry.length}")

    return True


def test_database_entry_data():
    """Test database entry sequence and coordinate data"""
    print("TEST: Database entry data")

    db = Database(get_test_db_path())

    entry = db[0]

    # Check sequence
    assert len(entry.sequence) == entry.length, "Sequence length should match entry.length"
    print(f"  ✓ Sequence: {entry.sequence[:30]}... (len={len(entry.sequence)})")

    # Check 3Di sequence
    if entry.seq_3di:
        assert len(entry.seq_3di) == entry.length, "3Di sequence length should match entry.length"
        print(f"  ✓ 3Di: {entry.seq_3di[:30]}... (len={len(entry.seq_3di)})")
    else:
        print(f"  ⚠ No 3Di sequence available")

    # Check CA coordinates
    if entry.ca_coords.size > 0:
        assert entry.ca_coords.shape == (entry.length, 3), f"CA coords shape should be ({entry.length}, 3)"
        print(f"  ✓ CA coordinates: shape={entry.ca_coords.shape}, dtype={entry.ca_coords.dtype}")
        print(f"    First residue CA: [{entry.ca_coords[0,0]:.2f}, {entry.ca_coords[0,1]:.2f}, {entry.ca_coords[0,2]:.2f}]")
    else:
        print(f"  ⚠ No CA coordinates available")

    return True


def test_database_get_by_key():
    """Test getting database entries by key"""
    print("TEST: Get entry by key")

    db = Database(get_test_db_path())

    # Get first entry to find its key
    entry1 = db[0]
    key = entry1.key

    # Get same entry by key
    entry2 = db.get(key)

    # Verify they're the same
    assert entry2.key == entry1.key, "Keys should match"
    assert entry2.sequence == entry1.sequence, "Sequences should match"
    assert entry2.name == entry1.name, "Names should match"

    print(f"  ✓ Retrieved entry by key {key}")
    print(f"    Entry: {entry2.name[:60]}...")

    return True


def test_database_keys():
    """Test getting all database keys"""
    print("TEST: Get all database keys")

    db = Database(get_test_db_path())

    keys = db.keys()

    assert len(keys) == len(db), "Number of keys should match database size"
    assert all(isinstance(k, int) for k in keys), "All keys should be integers"

    print(f"  ✓ Got {len(keys)} keys")
    print(f"    First 5 keys: {keys[:5]}")

    return True


def test_database_iteration():
    """Test iterating over database entries"""
    print("TEST: Database iteration")

    db = Database(get_test_db_path())

    count = 0
    for i in range(min(5, len(db))):
        entry = db[i]
        assert entry is not None, f"Entry {i} should not be None"
        count += 1

    print(f"  ✓ Iterated over {count} entries")

    return True


def test_database_repr():
    """Test database and entry string representations"""
    print("TEST: String representations")

    db = Database(get_test_db_path())

    db_repr = repr(db)
    assert "Database" in db_repr, "Database repr should contain 'Database'"
    assert "size=" in db_repr, "Database repr should contain size"
    print(f"  ✓ Database: {db_repr}")

    entry = db[0]
    entry_repr = repr(entry)
    assert "DatabaseEntry" in entry_repr, "Entry repr should contain 'DatabaseEntry'"
    assert "key=" in entry_repr, "Entry repr should contain key"
    print(f"  ✓ Entry: {entry_repr}")

    return True


def main():
    """Run all tests"""
    print("="*70)
    print("Database Functionality Tests")
    print("="*70)
    print()

    tests = [
        test_database_open,
        test_database_entry_access,
        test_database_entry_data,
        test_database_get_by_key,
        test_database_keys,
        test_database_iteration,
        test_database_repr,
    ]

    passed = 0
    failed = 0

    for test in tests:
        try:
            if test():
                passed += 1
            else:
                failed += 1
                print(f"  ✗ FAILED")
        except Exception as e:
            failed += 1
            print(f"  ✗ FAILED with exception: {e}")
            import traceback
            traceback.print_exc()
        print()

    print("="*70)
    print(f"Results: {passed} passed, {failed} failed")
    print("="*70)

    return failed == 0


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
