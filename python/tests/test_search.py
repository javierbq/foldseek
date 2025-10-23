"""
Tests for structure search functionality
"""
import pytest
import numpy as np
from pathlib import Path

try:
    from pyfoldseek import Structure, Database, search, SearchHit
    EXTENSION_AVAILABLE = True
except ImportError:
    EXTENSION_AVAILABLE = False
    pytestmark = pytest.mark.skip("C++ extension not built")


class TestSearch:
    """Test structure search functionality"""

    @pytest.fixture
    def test_database(self):
        """Get test database path"""
        db_path = Path("/home/user/foldseek/testdb_py")
        if not db_path.exists():
            pytest.skip(f"Test database not found: {db_path}")
        return str(db_path)

    @pytest.fixture
    def query_structure(self):
        """Get query structure"""
        pdb_path = Path("/home/user/foldseek/example/d1asha_")
        if not pdb_path.exists():
            pytest.skip(f"Query structure not found: {pdb_path}")
        return Structure.from_file(str(pdb_path))

    def test_search_basic(self, query_structure, test_database):
        """Test basic search functionality"""
        db = Database(test_database)

        # Search with high threshold
        hits = search(
            query_structure.ca_coords,
            query_structure.sequence,
            db,
            tmscore_threshold=0.8,
            max_hits=10
        )

        # Should find at least the query itself
        assert len(hits) > 0
        assert isinstance(hits[0], SearchHit)

        # First hit should be the query with perfect score
        assert hits[0].target_name == "d1asha_"
        assert abs(hits[0].tmscore - 1.0) < 0.001
        assert abs(hits[0].rmsd - 0.0) < 0.1

    def test_search_hit_properties(self, query_structure, test_database):
        """Test SearchHit properties"""
        db = Database(test_database)
        hits = search(query_structure.ca_coords, query_structure.sequence, db)

        hit = hits[0]
        assert hasattr(hit, 'target_key')
        assert hasattr(hit, 'target_name')
        assert hasattr(hit, 'tmscore')
        assert hasattr(hit, 'rmsd')
        assert hasattr(hit, 'alignment_length')
        assert hasattr(hit, 'query_coverage')
        assert hasattr(hit, 'target_coverage')
        assert hasattr(hit, 'alignment')

        # Check types
        assert isinstance(hit.target_key, int)
        assert isinstance(hit.target_name, str)
        assert isinstance(hit.tmscore, float)
        assert isinstance(hit.rmsd, float)
        assert isinstance(hit.alignment_length, int)
        assert isinstance(hit.query_coverage, float)
        assert isinstance(hit.target_coverage, float)
        assert isinstance(hit.alignment, str)

    def test_search_threshold(self, query_structure, test_database):
        """Test TM-score threshold filtering"""
        db = Database(test_database)

        # Higher threshold should give fewer hits
        hits_high = search(
            query_structure.ca_coords,
            query_structure.sequence,
            db,
            tmscore_threshold=0.9
        )

        hits_low = search(
            query_structure.ca_coords,
            query_structure.sequence,
            db,
            tmscore_threshold=0.7
        )

        assert len(hits_high) <= len(hits_low)

        # All hits should be above threshold
        for hit in hits_high:
            assert hit.tmscore >= 0.9

        for hit in hits_low:
            assert hit.tmscore >= 0.7

    def test_search_max_hits(self, query_structure, test_database):
        """Test max_hits parameter"""
        db = Database(test_database)

        # Search with max_hits limit
        hits = search(
            query_structure.ca_coords,
            query_structure.sequence,
            db,
            tmscore_threshold=0.5,
            max_hits=5
        )

        assert len(hits) <= 5

    def test_search_sorting(self, query_structure, test_database):
        """Test that results are sorted by TM-score"""
        db = Database(test_database)

        hits = search(
            query_structure.ca_coords,
            query_structure.sequence,
            db,
            tmscore_threshold=0.5
        )

        # Check that hits are sorted in descending order
        for i in range(len(hits) - 1):
            assert hits[i].tmscore >= hits[i+1].tmscore

    def test_search_coverage(self, query_structure, test_database):
        """Test coverage threshold filtering"""
        db = Database(test_database)

        # Search with coverage threshold
        hits = search(
            query_structure.ca_coords,
            query_structure.sequence,
            db,
            tmscore_threshold=0.5,
            coverage_threshold=0.9
        )

        # All hits should meet coverage threshold
        for hit in hits:
            assert hit.query_coverage >= 0.9 or hit.target_coverage >= 0.9

    def test_search_empty_result(self, query_structure, test_database):
        """Test search with very high threshold returns empty"""
        db = Database(test_database)

        # Search with impossible threshold
        hits = search(
            query_structure.ca_coords,
            query_structure.sequence,
            db,
            tmscore_threshold=1.1  # Impossible
        )

        # Should return empty list, not crash
        assert isinstance(hits, list)
        assert len(hits) == 0

    def test_search_different_structures(self, test_database):
        """Test search with different query structures"""
        # Load a different structure from the database
        pdb_path = Path("/home/user/foldseek/example/d1cg5a_")
        if not pdb_path.exists():
            pytest.skip("Test structure not found")

        query = Structure.from_file(str(pdb_path))
        db = Database(test_database)

        hits = search(
            query.ca_coords,
            query.sequence,
            db,
            tmscore_threshold=0.7
        )

        # Should find results
        assert len(hits) > 0

        # First hit should be the query itself
        assert "d1cg5a_" in hits[0].target_name

if __name__ == "__main__":
    pytest.main([__file__, "-v"])
