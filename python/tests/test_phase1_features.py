"""
Tests for Phase 1 features: multi-chain, batch processing, features
"""
import pytest
import numpy as np
from pathlib import Path

try:
    from pyfoldseek import Structure, Chain, batch_convert
    EXTENSION_AVAILABLE = True
except ImportError:
    EXTENSION_AVAILABLE = False
    pytestmark = pytest.mark.skip("C++ extension not built")


# Test data paths
TEST_DATA_DIR = Path(__file__).parent.parent.parent.parent / "example"


class TestChainClass:
    """Test Chain class functionality"""

    @pytest.fixture
    def example_pdb(self):
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_chain_properties(self, example_pdb):
        """Test chain properties"""
        struct = Structure.from_file(example_pdb)
        if struct.num_chains == 0:
            pytest.skip("No chains in structure")

        chain = struct.get_chain(0)

        assert isinstance(chain, Chain)
        assert isinstance(chain.name, str)
        assert isinstance(chain.sequence, str)
        assert isinstance(chain.seq_3di, str)
        assert len(chain) > 0
        assert chain.length == len(chain.sequence)
        assert len(chain.seq_3di) == len(chain.sequence)

    def test_chain_coordinates(self, example_pdb):
        """Test chain coordinate access"""
        struct = Structure.from_file(example_pdb)
        chain = struct.get_chain(0)

        # Check all coordinate arrays
        assert chain.ca_coords.shape == (chain.length, 3)
        assert chain.n_coords.shape == (chain.length, 3)
        assert chain.c_coords.shape == (chain.length, 3)
        assert chain.cb_coords.shape == (chain.length, 3)

        # Check they're NumPy arrays
        assert isinstance(chain.ca_coords, np.ndarray)

    def test_chain_repr(self, example_pdb):
        """Test chain string representation"""
        struct = Structure.from_file(example_pdb)
        chain = struct.get_chain(0)

        repr_str = repr(chain)
        assert "Chain" in repr_str
        assert chain.name in repr_str
        assert str(chain.length) in repr_str


class TestMultiChainSupport:
    """Test multi-chain structure handling"""

    @pytest.fixture
    def example_pdb(self):
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_num_chains_property(self, example_pdb):
        """Test number of chains property"""
        struct = Structure.from_file(example_pdb)
        assert struct.num_chains >= 1
        assert isinstance(struct.num_chains, int)

    def test_chains_property(self, example_pdb):
        """Test chains list property"""
        struct = Structure.from_file(example_pdb)
        chains = struct.chains

        assert isinstance(chains, list)
        assert len(chains) == struct.num_chains
        assert all(isinstance(c, Chain) for c in chains)

    def test_get_chain_method(self, example_pdb):
        """Test get_chain method"""
        struct = Structure.from_file(example_pdb)

        # Should be able to get first chain
        chain0 = struct.get_chain(0)
        assert isinstance(chain0, Chain)

        # Out of range should raise
        with pytest.raises((IndexError, RuntimeError)):
            struct.get_chain(999)

    def test_chain_iteration(self, example_pdb):
        """Test iterating over chains"""
        struct = Structure.from_file(example_pdb)

        # Should be able to iterate
        chain_count = 0
        for chain in struct:
            assert isinstance(chain, Chain)
            chain_count += 1

        assert chain_count == struct.num_chains

    def test_specific_chain_loading(self, example_pdb):
        """Test loading specific chain only"""
        # Load all chains
        struct_all = Structure.from_file(example_pdb)

        if struct_all.num_chains < 2:
            pytest.skip("Need multi-chain structure")

        # Load only first chain
        struct_chain0 = Structure.from_file(example_pdb, chain_index=0)
        assert struct_chain0.num_chains == 1

        # Sequences should match
        assert struct_chain0.sequence == struct_all.get_chain(0).sequence
        assert struct_chain0.seq_3di == struct_all.get_chain(0).seq_3di

    def test_backward_compatibility(self, example_pdb):
        """Test that old API still works (first chain as default)"""
        struct = Structure.from_file(example_pdb)

        # These should all access first chain
        first_chain = struct.get_chain(0)

        assert struct.sequence == first_chain.sequence
        assert struct.seq_3di == first_chain.seq_3di
        assert struct.length == first_chain.length
        np.testing.assert_array_equal(struct.ca_coords, first_chain.ca_coords)


class TestFormatSpecificLoaders:
    """Test format-specific loading methods"""

    @pytest.fixture
    def example_pdb(self):
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_from_pdb(self, example_pdb):
        """Test from_pdb loader"""
        struct = Structure.from_pdb(example_pdb)
        assert len(struct) > 0
        assert len(struct.seq_3di) > 0

    def test_from_mmcif(self):
        """Test from_mmcif loader"""
        # Would need an actual mmCIF file
        pytest.skip("Need mmCIF test file")

    def test_from_foldcomp(self):
        """Test from_foldcomp loader"""
        # Would need an actual Foldcomp file
        pytest.skip("Need Foldcomp test file")

    def test_format_equivalence(self, example_pdb):
        """Test that different loaders give same result"""
        struct1 = Structure.from_file(example_pdb)
        struct2 = Structure.from_pdb(example_pdb)

        # Should get same 3Di sequence
        assert struct1.seq_3di == struct2.seq_3di
        assert struct1.sequence == struct2.sequence


class TestBatchProcessing:
    """Test batch conversion functionality"""

    @pytest.fixture
    def example_files(self):
        """Get list of example PDB files"""
        pdb_files = list(TEST_DATA_DIR.glob("*.pdb*"))
        if not pdb_files:
            pytest.skip("No PDB files in example directory")
        return [str(f) for f in pdb_files[:3]]  # Take first 3

    def test_batch_convert_basic(self, example_files):
        """Test basic batch conversion"""
        results = batch_convert(example_files)

        assert isinstance(results, list)
        assert len(results) <= len(example_files)  # May be less if some fail
        assert all(isinstance(s, Structure) for s in results)

    def test_batch_convert_empty_list(self):
        """Test batch convert with empty list"""
        results = batch_convert([])
        assert isinstance(results, list)
        assert len(results) == 0

    def test_batch_convert_with_invalid_files(self):
        """Test batch convert handles invalid files gracefully"""
        files = ["nonexistent1.pdb", "nonexistent2.pdb"]
        results = batch_convert(files)

        # Should return empty list (all failed) but not crash
        assert isinstance(results, list)

    def test_batch_convert_mixed_valid_invalid(self, example_files):
        """Test batch convert with mix of valid and invalid files"""
        if not example_files:
            pytest.skip("No example files")

        mixed_files = example_files + ["nonexistent.pdb"]
        results = batch_convert(mixed_files)

        # Should get at least the valid files
        assert len(results) >= len(example_files) - 1

    def test_batch_convert_reconstruct_option(self, example_files):
        """Test batch convert with backbone reconstruction"""
        if not example_files:
            pytest.skip("No example files")

        results = batch_convert(example_files, reconstruct_backbone=True)
        assert len(results) > 0


class TestIntermediateFeatures:
    """Test intermediate geometric features"""

    @pytest.fixture
    def example_pdb(self):
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_features_not_computed_by_default(self, example_pdb):
        """Test that features are not computed by default"""
        struct = Structure.from_file(example_pdb)

        with pytest.raises(RuntimeError):
            _ = struct.features

    def test_features_computed_when_requested(self, example_pdb):
        """Test that features can be computed"""
        struct = Structure.from_file(example_pdb, compute_features=True)

        features = struct.features
        assert isinstance(features, np.ndarray)
        assert features.ndim == 2
        assert features.shape[0] == struct.length
        assert features.shape[1] == 10  # FEATURE_CNT

    def test_features_statistics(self, example_pdb):
        """Test feature statistics are reasonable"""
        struct = Structure.from_file(example_pdb, compute_features=True)
        features = struct.features

        # Features should be numeric
        assert not np.any(np.isnan(features))
        assert not np.any(np.isinf(features))

        # Should have variation
        assert features.std() > 0


class TestEnhancedStructureRepr:
    """Test enhanced structure representation"""

    @pytest.fixture
    def example_pdb(self):
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_repr_shows_chain_count(self, example_pdb):
        """Test that repr shows chain count"""
        struct = Structure.from_file(example_pdb)
        repr_str = repr(struct)

        assert "chains=" in repr_str
        assert str(struct.num_chains) in repr_str

    def test_repr_shows_length(self, example_pdb):
        """Test that repr shows length"""
        struct = Structure.from_file(example_pdb)
        repr_str = repr(struct)

        assert "length=" in repr_str
        assert str(struct.length) in repr_str


class TestBackwardCompatibility:
    """Test that Phase 0 code still works with Phase 1"""

    @pytest.fixture
    def example_pdb(self):
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_old_api_unchanged(self, example_pdb):
        """Test that Phase 0 API still works"""
        # This is the original Phase 0 usage
        struct = Structure.from_file(example_pdb)

        # All these should still work
        assert isinstance(struct.sequence, str)
        assert isinstance(struct.seq_3di, str)
        assert isinstance(struct.length, int)
        assert isinstance(struct.ca_coords, np.ndarray)
        assert isinstance(struct.filename, str)

    def test_old_coords_to_3di_unchanged(self):
        """Test that coords_to_3di still works"""
        from pyfoldseek import coords_to_3di

        n = 10
        ca = np.random.randn(n, 3)
        n_coords = np.random.randn(n, 3)
        c_coords = np.random.randn(n, 3)
        cb = np.random.randn(n, 3)

        seq_3di = coords_to_3di(ca, n_coords, c_coords, cb)
        assert isinstance(seq_3di, str)
        assert len(seq_3di) == n


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
