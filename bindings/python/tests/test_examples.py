"""Smoke tests for the runnable examples in ``bindings/python/examples/``.

The examples are user-facing documentation that is easy to break silently when
the API changes (e.g. flipping the layer-inheritance default). These tests run
each example end-to-end as a subprocess and assert it exits cleanly, so a broken
example fails CI.

Each example is run in its own temporary working directory: several write
``*.stards`` files using relative paths, and this keeps those out of the repo /
the test runner's cwd and cleans them up automatically.

``s3_example.py`` is intentionally excluded: it requires network / S3 access and
credentials, so it cannot run in a hermetic CI job.
"""
import os
import subprocess
import sys

import pytest

EXAMPLES_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "examples")
)

# Examples that are safe to run in a hermetic CI environment (no network/creds).
CI_SAFE_EXAMPLES = [
    "basic_usage.py",
    "layers_example.py",
    "new_features.py",
    "numpy_interop.py",
]


@pytest.mark.parametrize("example", CI_SAFE_EXAMPLES)
def test_example_runs(example, tmp_path):
    """Each CI-safe example runs to completion with a zero exit code."""
    script = os.path.join(EXAMPLES_DIR, example)
    assert os.path.isfile(script), f"missing example: {script}"

    # Inherit the environment so the example imports the same pystards the test
    # suite uses (conftest puts the built extension on sys.path; propagate that
    # to the child via PYTHONPATH).
    env = dict(os.environ)
    env["PYTHONPATH"] = os.pathsep.join(
        p for p in [os.pathsep.join(sys.path), env.get("PYTHONPATH", "")] if p
    )

    result = subprocess.run(
        [sys.executable, script],
        cwd=tmp_path,  # isolate the *.stards files the examples write
        env=env,
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, (
        f"{example} exited {result.returncode}\n"
        f"--- stdout ---\n{result.stdout}\n"
        f"--- stderr ---\n{result.stderr}"
    )


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
