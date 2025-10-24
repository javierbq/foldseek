"""
Tests for clustering functionality
"""
import pytest
from pathlib import Path

try:
    from pyfoldseek import Database, cluster, Cluster
    EXTENSION_AVAILABLE = True
except ImportError:
    EXTENSION_AVAILABLE = False
    pytestmark = pytest.mark.skip("C++ extension not built")


class TestClustering:
    """Test structure clustering functionality"""

    @pytest.fixture
    def test_database(self):
        """Get test database path"""
        db_path = Path("/home/user/foldseek/testdb_py")
        if not db_path.exists():
            pytest.skip(f"Test database not found: {db_path}")
        return str(db_path)

    def test_cluster_basic(self, test_database):
        """Test basic clustering functionality"""
        db = Database(test_database)

        # Cluster with threshold 0.5
        clusters = cluster(db, tmscore_threshold=0.5, coverage_threshold=0.8)

        # Should create at least one cluster
        assert len(clusters) > 0
        assert isinstance(clusters[0], Cluster)

        # All structures should be clustered
        total_structures = sum(c.size for c in clusters)
        assert total_structures == len(db)

    def test_cluster_properties(self, test_database):
        """Test Cluster object properties"""
        db = Database(test_database)
        clusters = cluster(db, tmscore_threshold=0.8)

        for c in clusters:
            # Check properties exist and have correct types
            assert isinstance(c.representative_idx, int)
            assert isinstance(c.representative_name, str)
            assert isinstance(c.member_indices, list)
            assert isinstance(c.member_names, list)
            assert isinstance(c.size, int)

            # Size should be consistent
            assert c.size == len(c.member_indices) + 1  # +1 for representative
            assert len(c.member_indices) == len(c.member_names)

            # __len__ should work
            assert len(c) == c.size

    def test_cluster_threshold_effect(self, test_database):
        """Test that threshold affects number of clusters"""
        db = Database(test_database)

        # Lower threshold = fewer, larger clusters
        clusters_low = cluster(db, tmscore_threshold=0.5)

        # Higher threshold = more, smaller clusters
        clusters_high = cluster(db, tmscore_threshold=0.9)

        # Higher threshold should generally produce more clusters
        # (though not guaranteed for all databases)
        assert len(clusters_high) >= len(clusters_low)

        # Both should cluster all structures
        assert sum(c.size for c in clusters_low) == len(db)
        assert sum(c.size for c in clusters_high) == len(db)

    def test_cluster_no_overlap(self, test_database):
        """Test that clusters don't overlap"""
        db = Database(test_database)
        clusters = cluster(db, tmscore_threshold=0.7)

        # Collect all structure indices
        all_indices = set()

        for c in clusters:
            # Add representative
            assert c.representative_idx not in all_indices, \
                f"Duplicate representative: {c.representative_idx}"
            all_indices.add(c.representative_idx)

            # Add members
            for idx in c.member_indices:
                assert idx not in all_indices, f"Duplicate member: {idx}"
                all_indices.add(idx)

        # Should have exactly db_size unique indices
        assert len(all_indices) == len(db)

    def test_cluster_singleton(self, test_database):
        """Test clusters with single member (no neighbors)"""
        db = Database(test_database)

        # Very high threshold should create many singletons
        clusters = cluster(db, tmscore_threshold=0.95)

        # Find singleton clusters
        singletons = [c for c in clusters if c.size == 1]

        # Singletons should have no members
        for c in singletons:
            assert len(c.member_indices) == 0
            assert len(c.member_names) == 0

    def test_cluster_empty_database(self):
        """Test clustering empty database"""
        # This test would need an empty database
        # For now, just document expected behavior:
        # Empty database should return empty cluster list
        pass

    def test_cluster_coverage_threshold(self, test_database):
        """Test coverage threshold parameter"""
        db = Database(test_database)

        # Low coverage should give larger clusters
        clusters_low = cluster(db, tmscore_threshold=0.7, coverage_threshold=0.5)

        # High coverage should give smaller clusters
        clusters_high = cluster(db, tmscore_threshold=0.7, coverage_threshold=0.9)

        # Both should cluster all structures
        assert sum(c.size for c in clusters_low) == len(db)
        assert sum(c.size for c in clusters_high) == len(db)

        # High coverage is more stringent, may produce more clusters
        # (though not guaranteed for all databases)
        assert len(clusters_high) >= len(clusters_low)

    def test_cluster_mode_parameter(self, test_database):
        """Test clustering mode parameter"""
        db = Database(test_database)

        # Greedy mode should work
        clusters = cluster(db, mode="greedy")
        assert len(clusters) > 0

        # Invalid mode should raise error
        with pytest.raises(ValueError):
            cluster(db, mode="invalid_mode")

    def test_cluster_representative_indices(self, test_database):
        """Test that representative indices are valid"""
        db = Database(test_database)
        clusters = cluster(db, tmscore_threshold=0.7)

        for c in clusters:
            # Representative index should be in valid range
            assert 0 <= c.representative_idx < len(db)

            # All member indices should be in valid range
            for idx in c.member_indices:
                assert 0 <= idx < len(db)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
