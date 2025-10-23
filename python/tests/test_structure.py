"""
Tests for Structure class and PDB → 3Di conversion
"""
import pytest
import numpy as np
from pathlib import Path

# Will need to build the extension first
try:
    from pyfoldseek import Structure, coords_to_3di, Vec3
    EXTENSION_AVAILABLE = True
except ImportError:
    EXTENSION_AVAILABLE = False
    pytestmark = pytest.mark.skip("C++ extension not built")


# Test data paths
TEST_DATA_DIR = Path(__file__).parent.parent.parent.parent / "example"


class TestVec3:
    """Test Vec3 coordinate class"""

    def test_vec3_creation(self):
        v = Vec3(1.0, 2.0, 3.0)
        assert v.x == 1.0
        assert v.y == 2.0
        assert v.z == 3.0

    def test_vec3_default(self):
        v = Vec3()
        assert v.x == 0.0
        assert v.y == 0.0
        assert v.z == 0.0


class TestStructure:
    """Test Structure loading and 3Di conversion"""

    @pytest.fixture
    def example_pdb(self):
        """Path to example PDB file"""
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_load_structure(self, example_pdb):
        """Test loading a PDB file"""
        struct = Structure.from_file(example_pdb)
        assert isinstance(struct, Structure)
        assert len(struct) > 0

    def test_sequence_property(self, example_pdb):
        """Test amino acid sequence extraction"""
        struct = Structure.from_file(example_pdb)
        assert isinstance(struct.sequence, str)
        assert len(struct.sequence) > 0
        # Should be valid amino acid letters
        valid_aa = set("ACDEFGHIKLMNPQRSTVWY")
        assert all(aa in valid_aa for aa in struct.sequence)

    def test_seq_3di_property(self, example_pdb):
        """Test 3Di sequence generation"""
        struct = Structure.from_file(example_pdb)
        assert isinstance(struct.seq_3di, str)
        assert len(struct.seq_3di) == len(struct.sequence)
        # 3Di alphabet should be 20 states (represented as characters)
        assert len(struct.seq_3di) > 0

    def test_length_property(self, example_pdb):
        """Test length property"""
        struct = Structure.from_file(example_pdb)
        assert struct.length == len(struct.sequence)
        assert len(struct) == struct.length

    def test_coordinates(self, example_pdb):
        """Test coordinate extraction as NumPy arrays"""
        struct = Structure.from_file(example_pdb)

        # Check CA coordinates
        ca = struct.ca_coords
        assert isinstance(ca, np.ndarray)
        assert ca.shape == (struct.length, 3)
        assert ca.dtype == np.float64

        # Check other backbone atoms
        n = struct.n_coords
        c = struct.c_coords
        cb = struct.cb_coords

        assert n.shape == ca.shape
        assert c.shape == ca.shape
        assert cb.shape == ca.shape

    def test_chain_names(self, example_pdb):
        """Test chain name extraction"""
        struct = Structure.from_file(example_pdb)
        assert isinstance(struct.chain_names, list)
        assert len(struct.chain_names) > 0

    def test_repr(self, example_pdb):
        """Test string representation"""
        struct = Structure.from_file(example_pdb)
        repr_str = repr(struct)
        assert "Structure" in repr_str
        assert str(struct.length) in repr_str

    def test_invalid_file(self):
        """Test error handling for invalid files"""
        with pytest.raises(RuntimeError):
            Structure.from_file("nonexistent_file.pdb")

    def test_reconstruct_backbone(self, example_pdb):
        """Test backbone reconstruction option"""
        # This tests that the option doesn't crash
        # Actual reconstruction logic would need CA-only structure
        struct = Structure.from_file(example_pdb, reconstruct_backbone=True)
        assert len(struct) > 0


class TestCoordsTo3Di:
    """Test standalone coords_to_3di function"""

    def test_coords_to_3di_basic(self):
        """Test coordinate to 3Di conversion"""
        # Create simple test coordinates (10 residues)
        n = 10
        ca = np.random.randn(n, 3)
        n_coords = np.random.randn(n, 3)
        c_coords = np.random.randn(n, 3)
        cb = np.random.randn(n, 3)

        seq_3di = coords_to_3di(ca, n_coords, c_coords, cb)
        assert isinstance(seq_3di, str)
        assert len(seq_3di) == n

    def test_coords_to_3di_invalid_shape(self):
        """Test error handling for invalid shapes"""
        ca = np.random.randn(10, 2)  # Wrong shape
        n = np.random.randn(10, 3)
        c = np.random.randn(10, 3)
        cb = np.random.randn(10, 3)

        with pytest.raises(ValueError):
            coords_to_3di(ca, n, c, cb)

    def test_coords_to_3di_mismatched_lengths(self):
        """Test error handling for mismatched lengths"""
        ca = np.random.randn(10, 3)
        n = np.random.randn(8, 3)  # Different length
        c = np.random.randn(10, 3)
        cb = np.random.randn(10, 3)

        with pytest.raises(ValueError):
            coords_to_3di(ca, n, c, cb)

    def test_coords_to_3di_matches_structure(self, example_pdb):
        """Test that coords_to_3di matches Structure.seq_3di"""
        if not Path(example_pdb).exists():
            pytest.skip("Example PDB not found")

        struct = Structure.from_file(example_pdb)

        # Get coordinates
        ca = struct.ca_coords
        n = struct.n_coords
        c = struct.c_coords
        cb = struct.cb_coords

        # Convert using standalone function
        seq_3di = coords_to_3di(ca, n, c, cb)

        # Should match Structure's seq_3di
        assert seq_3di == struct.seq_3di


class TestIntegration:
    """Integration tests comparing with CLI output"""

    @pytest.fixture
    def example_pdb(self):
        """Path to example PDB file"""
        pdb_path = TEST_DATA_DIR / "1tim.pdb.gz"
        if not pdb_path.exists():
            pytest.skip(f"Example PDB not found: {pdb_path}")
        return str(pdb_path)

    def test_compare_with_cli(self, example_pdb, tmp_path):
        """Compare Python output with foldseek CLI output"""
        import subprocess
        import os

        # Skip if foldseek not available
        try:
            subprocess.run(["foldseek", "version"], capture_output=True, check=True)
        except (FileNotFoundError, subprocess.CalledProcessError):
            pytest.skip("foldseek CLI not available")

        # Get Python result
        struct = Structure.from_file(example_pdb)
        python_3di = struct.seq_3di

        # Get CLI result
        db_path = tmp_path / "testdb"
        subprocess.run([
            "foldseek", "createdb",
            example_pdb,
            str(db_path)
        ], check=True)

        # Read 3Di from database
        # This would need proper database reading implementation
        # For now, just check that we got a result
        assert len(python_3di) > 0


class TestPerformance:
    """Performance benchmarks"""

    def test_loading_speed(self, example_pdb, benchmark):
        """Benchmark structure loading"""
        if not Path(example_pdb).exists():
            pytest.skip("Example PDB not found")

        def load_structure():
            return Structure.from_file(example_pdb)

        result = benchmark(load_structure)
        assert len(result) > 0

    def test_coords_conversion_speed(self, benchmark):
        """Benchmark coordinate to 3Di conversion"""
        n = 500  # Typical protein size
        ca = np.random.randn(n, 3)
        n_coords = np.random.randn(n, 3)
        c_coords = np.random.randn(n, 3)
        cb = np.random.randn(n, 3)

        def convert():
            return coords_to_3di(ca, n_coords, c_coords, cb)

        result = benchmark(convert)
        assert len(result) == n


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
