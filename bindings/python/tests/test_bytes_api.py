"""Tests for the in-memory byte APIs: StarDataset.open_bytes() / write_bytes()."""
import numpy as np
import pytest

from pystards import StarDataset


def _make_dataset(tmp_path):
    path = str(tmp_path / "src.stards")
    ds = StarDataset.create(path)
    ds["signal"] = np.arange(256, dtype=np.float64)
    ds["image"] = np.full((4, 5), 7, dtype=np.int32)
    ds.meta["instrument"] = "AVIRIS"
    ds.meta["gain"] = 2.25
    ds.flush()
    ds.close()
    return path


def test_write_bytes_returns_bytes(tmp_path):
    path = _make_dataset(tmp_path)
    ds = StarDataset.open(path, mode="r")
    blob = ds.write_bytes()
    assert isinstance(blob, bytes)
    assert len(blob) > 0


def test_write_then_open_bytes_round_trip(tmp_path):
    path = _make_dataset(tmp_path)
    blob = StarDataset.open(path, mode="r").write_bytes()

    ds = StarDataset.open_bytes(blob)
    assert np.array_equal(ds["signal"], np.arange(256, dtype=np.float64))
    img = ds["image"]
    assert img.shape == (4, 5)
    assert img[3, 4] == 7
    assert ds.meta["instrument"] == "AVIRIS"
    assert ds.meta["gain"] == 2.25


def test_open_bytes_is_read_only(tmp_path):
    path = _make_dataset(tmp_path)
    blob = StarDataset.open(path, mode="r").write_bytes()

    ds = StarDataset.open_bytes(blob)
    assert ds.is_read_only()


def test_write_bytes_matches_file_bytes(tmp_path):
    # write_bytes() should equal the bytes save_to() writes to disk.
    path = _make_dataset(tmp_path)
    ds = StarDataset.open(path, mode="r")
    blob = ds.write_bytes()

    out_path = str(tmp_path / "out.stards")
    ds.save_to(out_path)
    with open(out_path, "rb") as fh:
        file_bytes = fh.read()

    assert blob == file_bytes


def test_open_bytes_accepts_bytes_like(tmp_path):
    path = _make_dataset(tmp_path)
    blob = StarDataset.open(path, mode="r").write_bytes()

    for buf in (blob, bytearray(blob), memoryview(blob),
                np.frombuffer(blob, dtype=np.uint8)):
        ds = StarDataset.open_bytes(buf)
        assert ds["signal"][100] == 100.0


def test_layers_survive_byte_round_trip(tmp_path):
    path = str(tmp_path / "layers.stards")
    ds = StarDataset.create(path)
    ds["base"] = np.full(300, 3.0)
    layer = ds.create_layer("proc")
    layer["base"] = np.full(300, 9.0)
    ds.flush()
    ds.close()

    blob = StarDataset.open(path, mode="r").write_bytes()
    ds = StarDataset.open_bytes(blob)
    assert ds.has_layer("proc")
    assert ds.get_layer("proc")["base"][0] == 9.0
    assert ds["base"][0] == 3.0


def test_modify_in_memory_then_reserialize(tmp_path):
    path = _make_dataset(tmp_path)
    blob = StarDataset.open(path, mode="r").write_bytes()

    ds = StarDataset.open_bytes(blob)
    ds["extra"] = np.full(128, 42.0)
    blob2 = ds.write_bytes()

    ds2 = StarDataset.open_bytes(blob2)
    assert np.array_equal(ds2["extra"], np.full(128, 42.0))
    assert ds2["signal"][0] == 0.0


def test_open_bytes_rejects_garbage():
    with pytest.raises(Exception):
        StarDataset.open_bytes(b"\x00" * 64)
    with pytest.raises(Exception):
        StarDataset.open_bytes(b"")
