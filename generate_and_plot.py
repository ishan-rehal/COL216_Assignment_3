#!/usr/bin/env python3
import os
import re
import csv
import subprocess
import sys
import matplotlib.pyplot as plt
import platform
import shutil  # Add this import for file operations

# Path to your simulator executable - adjust based on OS
if platform.system() == "Windows":
    SIM_CMD = ".\\L1simulate.exe"
else:
    SIM_CMD = "./L1simulate"

# Base directory containing your test‐case subfolders (tc_1…tc_4)
BASE_DIR = "graph_tc"
TC_LIST  = ["tc_1", "tc_2", "tc_3", "tc_4"]

# The four cache configurations to test
PARAMS = [
    ("default", {"s": 6, "E": 2, "b": 5}),
    ("sets×2",  {"s": 7, "E": 2, "b": 5}),
    ("block×2", {"s": 6, "E": 2, "b": 6}),
    ("assoc×2", {"s": 6, "E": 4, "b": 5}),
]

OUTPUT_CSV = "max_cycles.csv"

def parse_max_cycles(output: str) -> int:
    # The simulator outputs "Global Clock: N cycles" based on main.cpp
    matches = re.findall(r"Global Clock:\s*(\d+)\s+cycles", output)
    if matches:
        print(f"Found Global Clock match: {matches[0]}")
        return int(matches[0])
    
    # Try alternative patterns if the first one doesn't match
    alt_patterns = [
        r"Global Clock:\s*(\d+)", 
        r"Total Execution Cycles:\s*(\d+)",
        r"Cycles:\s*(\d+)"
    ]
    
    for pattern in alt_patterns:
        matches = re.findall(pattern, output)
        if matches:
            print(f"Found match with pattern '{pattern}': {matches[0]}")
            return int(matches[0])
    
    # For debugging, print part of the output if no matches
    if output:
        print("Debug - No cycle count found. Output excerpt:")
        print("--- First 200 chars ---")
        print(output[:200])
        print("--- Last 500 chars ---")
        print(output[-500:])
    
    return 0

def find_prefix(tc_dir: str) -> str:
    # Look for either X_proc0.trace or X_0.trace
    print(f"Looking for trace files in {tc_dir}...")
    print(f"Files in directory: {os.listdir(tc_dir)}")
    
    for fname in os.listdir(tc_dir):
        m = re.match(r"^(.+?)_proc[0-3]\.trace$", fname)
        if m: 
            print(f"Found proc format: {fname}")
            return m.group(1)
        m2 = re.match(r"^(.+?)_[0-3]\.trace$", fname)
        if m2:
            print(f"Found indexed format: {fname}")
            return m2.group(1)
    
    return None

def create_trace_links(src_dir, prefix):
    """Create proper trace file links/copies for the simulator"""
    created_files = []
    for i in range(4):
        src = os.path.join(src_dir, f"{prefix}_{i}.trace")
        dst = os.path.join(src_dir, f"{prefix}_proc{i}.trace")
        
        if os.path.exists(src) and not os.path.exists(dst):
            try:
                print(f"Creating link: {src} -> {dst}")
                if platform.system() == "Windows":
                    # Windows doesn't easily support symlinks, copy the file
                    shutil.copy2(src, dst)
                else:
                    # Unix-like systems can use symlinks
                    os.symlink(src, dst)
                created_files.append(dst)
            except Exception as e:
                print(f"Warning: Failed to create link {dst}: {e}")
    
    # Verify the files were created
    for i in range(4):
        check_file = os.path.join(src_dir, f"{prefix}_proc{i}.trace")
        print(f"Checking {check_file}: exists = {os.path.exists(check_file)}")
    
    return created_files

def main():
    # First check if simulator exists and is executable
    if not os.path.exists(SIM_CMD):
        print(f"ERROR: Simulator not found at '{SIM_CMD}'")
        print(f"Current directory: {os.getcwd()}")
        print(f"Files in directory: {os.listdir('.')}")
        sys.exit(1)
    
    # Run a simple test to make sure the simulator works
    test_cmd = [SIM_CMD, "--version"]
    try:
        print("Testing simulator...")
        test_proc = subprocess.run(test_cmd, capture_output=True, text=True)
        print(f"Simulator test output: {test_proc.stdout}")
    except Exception as e:
        print(f"WARNING: Simulator test failed: {e}")

    rows = []
    for tc in TC_LIST:
        tc_dir = os.path.join(BASE_DIR, tc)
        if not os.path.isdir(tc_dir):
            print(f"Skipping missing directory {tc_dir}")
            continue

        prefix = find_prefix(tc_dir)
        if not prefix:
            print(f"ERROR: no trace prefix in {tc_dir}")
            continue

        # Create proper trace file links
        created_files = create_trace_links(tc_dir, prefix)
        
        # Get the absolute path for the trace prefix (without the trailing _)
        trace_prefix = os.path.abspath(os.path.join(tc_dir, prefix))
        
        print(f"[DEBUG] {tc}: running with trace prefix: {trace_prefix}")
        print(f"Trace files:")
        for i in range(4):
            tf = f"{trace_prefix}_proc{i}.trace"
            print(f"    {tf} → exists? {os.path.exists(tf)}")

        for label, params in PARAMS:
            cmd = [
                SIM_CMD,
                "-t", trace_prefix,
                "-s", str(params["s"]),
                "-E", str(params["E"]),
                "-b", str(params["b"])
            ]
            print("\n" + "="*40)
            print(f"Running: {' '.join(cmd)}")
            print("="*40)
            
            try:
                # Try running with shell=True to capture all output properly
                proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
                
                # Explicitly check return code
                if proc.returncode != 0:
                    print(f"WARNING: Command returned non-zero exit code: {proc.returncode}")
                    print(f"STDERR: {proc.stderr}")
                
                # Print full output for debugging
                print("--- STDOUT START ---")
                print(proc.stdout)
                print("--- STDOUT END ---")
                
                mx = parse_max_cycles(proc.stdout)
                if mx == 0:
                    print("WARNING: Could not parse cycles from output, trying stderr")
                    mx = parse_max_cycles(proc.stderr)
                
                # If still 0, try running directly with os.system for debugging
                if mx == 0 or mx == 1:
                    print("WARNING: Still couldn't get meaningful cycle count. Trying direct execution:")
                    direct_cmd = " ".join(cmd)
                    print(f"Running directly: {direct_cmd}")
                    os.system(direct_cmd + " > direct_output.txt")
                    
                    # Read the direct output file
                    if os.path.exists("direct_output.txt"):
                        with open("direct_output.txt", "r") as f:
                            direct_output = f.read()
                        print("--- DIRECT OUTPUT ---")
                        print(direct_output[-1000:])  # Last 1000 chars
                        mx = parse_max_cycles(direct_output)
            
            except subprocess.CalledProcessError as e:
                print(f"ERROR {tc}/{label}: Command failed with return code {e.returncode}")
                print(f"STDERR: {e.stderr}")
                mx = 0
            except Exception as e:
                print(f"ERROR {tc}/{label}: {e}")
                mx = 0
                
            print(f"  -> {label} max cycles = {mx}")
            rows.append([tc, label, mx])

        # Clean up temporary files if needed
        for f in created_files:
            if os.path.exists(f):
                try:
                    os.remove(f)
                    print(f"Removed temporary file: {f}")
                except Exception as e:
                    print(f"Warning: Failed to remove {f}: {e}")
                    pass

    # write CSV
    with open(OUTPUT_CSV, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["test_case", "parameter", "max_cycles"])
        w.writerows(rows)
    print(f"Wrote {OUTPUT_CSV}")

    # plot bar‐chart per test-case
    for tc in TC_LIST:
        subset = [r for r in rows if r[0] == tc]
        if not subset: continue
        labels = [r[1] for r in subset]
        values = [r[2] for r in subset]
        plt.figure(figsize=(10, 6))
        plt.bar(labels, values, color=["#4C72B0","#55A868","#C44E52","#8172B3"])
        plt.title(f"{tc}: Max Core Cycles vs Parameter")
        plt.xlabel("Parameter Setting")
        plt.ylabel("Max Core Cycles")
        plt.grid(axis="y", linestyle="--", alpha=0.5)
        plt.tight_layout()
        out_png = f"{tc}_maxcycles.png"
        plt.savefig(out_png, dpi=200)
        print(f"Saved plot {out_png}")

if __name__ == "__main__":
    main()