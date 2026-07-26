#!/usr/bin/env python3
"""
Benchmark script: measures speedup of MPI version vs serial version
for all .in inputs in tests/.
"""

import subprocess
import sys
import os
import re
import glob

# ── Paths ──────────────────────────────────────────────────────────────
ROOT        = os.path.dirname(os.path.abspath(__file__))
SERIAL_DIR  = os.path.join(ROOT, "g20", "serial", "src")
MPI_DIR     = os.path.join(ROOT, "g20", "mpi", "src")
INPUT_DIRS  = [
    os.path.join(ROOT, "tests"),
]

SERIAL_BIN  = os.path.join(SERIAL_DIR, "docs")
MPI_BIN     = os.path.join(MPI_DIR, "docs")

FIXED_NP = 8
RUNS        = 1                   # runs per configuration
SERIAL_SKIP_DOCS = 100_000        # skip serial if D exceeds this (use MPI np=1 as baseline)


def gen_np_list(max_np: int) -> list[int]:
    """Generate powers-of-two list from 1 up to max_np (inclusive)."""
    nps = []
    v = 1
    while v <= max_np:
        nps.append(v)
        v *= 2
    return nps

# ── Helpers ────────────────────────────────────────────────────────────
def read_header(input_file: str) -> tuple[int, int, int]:
    """Return (C, D, S) from the first line of an input file."""
    with open(input_file) as f:
        c, d, s = map(int, f.readline().split())
    return c, d, s


def build(directory, target=None):
    """Run make (optionally a specific target) in the given directory."""
    subprocess.run(["make", "-C", directory, "clean"], capture_output=True)
    cmd = ["make", "-C", directory]
    if target:
        cmd.append(target)
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Build failed in {directory}:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)


def parse_time(stderr_output: str) -> float | None:
    """Extract the last occurrence of '<number>s' from stderr."""
    matches = re.findall(r"([\d.]+)s", stderr_output)
    return float(matches[-1]) if matches else None


def run_serial(input_file: str) -> float | None:
    """Run the serial binary and return the best (minimum) time."""
    times = []
    for _ in range(RUNS):
        proc = subprocess.run(
            [SERIAL_BIN, input_file],
            capture_output=True, text=True
        )
        t = parse_time(proc.stderr)
        if t is not None:
            times.append(t)
    return min(times) if times else None


def run_mpi(input_file: str, np: int) -> tuple[float | None, str]:
    """Run the MPI binary with `np` processes and return (best time, last stdout)."""
    times = []
    last_stdout = ""
    for _ in range(RUNS):
        proc = subprocess.run(
            ["mpirun", "--oversubscribe", "-np", str(np), MPI_BIN, input_file],
            capture_output=True, text=True
        )
        t = parse_time(proc.stderr)
        if t is not None:
            times.append(t)
            last_stdout = proc.stdout
    return (min(times) if times else None, last_stdout)


def check_output(input_file: str, actual_output: str) -> bool | None:
    """Compare actual output against the expected .out file. Returns None if no .out exists."""
    out_file = os.path.splitext(input_file)[0] + ".out"
    if not os.path.exists(out_file):
        return None
    with open(out_file) as f:
        expected = f.read().strip()
    return actual_output.strip() == expected


# ── Main ───────────────────────────────────────────────────────────────
def main():
    # Build both versions
    print("Building serial version …")
    build(SERIAL_DIR)
    print("Building MPI version (docs-mpi target) …")
    build(MPI_DIR, target="docs-mpi")
    print()

    inputs = sorted(
        f
        for d in INPUT_DIRS
        for f in glob.glob(os.path.join(d, "**", "*.in"), recursive=True)
    )
    if not inputs:
        print("No .in files found in", INPUT_DIRS)
        sys.exit(1)

    # Determine max NP from environment (SLURM) or fallback to FIXED_NP
    max_np = int(os.environ.get("SLURM_NTASKS", FIXED_NP))
    all_nps = gen_np_list(max_np)
    np_headers = "".join(f"{'MPI np=' + str(np):>14s}  {'Speedup':>8s}" for np in all_nps)
    header = f"{'Input':<30s}  {'Serial':>10s}" + np_headers
    print(header)
    print("─" * len(header))

    for inp in inputs:
        name = os.path.basename(inp)
        _, D, _ = read_header(inp)
        large = D > SERIAL_SKIP_DOCS

        serial_t = None if large else run_serial(inp)

        row = f"{name:<30s}  "
        if large:
            row += f"{'(skipped)':>10s}"
        else:
            row += f"{serial_t:10.3f}s" if serial_t is not None else f"{'N/A':>10s}"

        # For large inputs baseline is the smallest tested NP (>=16)
        np_list = gen_np_list(max_np)
        if large:
            np_list = [n for n in np_list if n >= 16]

        baseline_t = serial_t
        last_output = ""
        if large:
            # measure baseline at smallest tested NP
            baseline_t, last_output = run_mpi(inp, np_list[0])

        for np in all_nps:
            if np not in np_list:
                # column not applicable for this input size
                row += f"  {'---':>14s}  {'---':>8s}"
                continue
            mpi_t, stdout = run_mpi(inp, np)
            last_output = stdout
            if mpi_t is not None:
                row += f"  {mpi_t:12.3f}s"
                if baseline_t is not None and mpi_t > 0:
                    speedup = baseline_t / mpi_t
                    row += f"  {speedup:7.2f}x"
                else:
                    row += f"  {'N/A':>8s}"
            else:
                row += f"  {'N/A':>14s}  {'N/A':>8s}"

        # Validate output against .out file using last MPI run
        valid = check_output(inp, last_output)
        if valid is True:
            row += "  OK"
        elif valid is False:
            row += "  FAIL"
        # else: no .out file — omit

        print(row)

    print()
    print(f"(best of {RUNS} runs per configuration)")
    print(f"Speedup for large inputs (D>{SERIAL_SKIP_DOCS}) is relative to the smallest tested MPI (>=16).")


if __name__ == "__main__":
    main()
