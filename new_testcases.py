#!/usr/bin/env python3
"""
Script to create a new 4-file test case in the existing "header/Test_cases" directory.
Each invocation takes an optional prefix or auto-increments from existing files.
Usage:
    python3 header/Test_cases/new_testcases.py [START_PREFIX]
If START_PREFIX is omitted, the script finds the highest existing numeric prefix and uses +1.
Generates files:
    <PREFIX>_0.trace
    <PREFIX>_1.trace
    <PREFIX>_2.trace
    <PREFIX>_3.trace
"""
import os
import re
import sys

# Configure the location of the Test_cases directory relative to the project root
def get_test_dir():
    # Assume current working directory is project root
    return os.path.join(os.getcwd(), 'header', 'Test_cases')


def main():
    test_dir = get_test_dir()
    # Ensure directory exists
    os.makedirs(test_dir, exist_ok=True)

    # Determine prefix
    if len(sys.argv) > 1:
        try:
            prefix = int(sys.argv[1])
        except ValueError:
            print(f"Invalid prefix '{sys.argv[1]}', must be an integer.")
            sys.exit(1)
    else:
        # Scan existing files for numeric prefixes
        pattern = re.compile(r'^(\d+)_([0-3])\.trace$')
        max_prefix = -1
        for fname in os.listdir(test_dir):
            m = pattern.match(fname)
            if m:
                num = int(m.group(1))
                if num > max_prefix:
                    max_prefix = num
        prefix = max_prefix + 1

    # Create the four trace files
    created = []
    for core in range(4):
        filename = f"{prefix}_{core}.trace"
        path = os.path.join(test_dir, filename)
        # Avoid overwriting existing
        if os.path.exists(path):
            print(f"Warning: {filename} already exists, skipping.")
        else:
            with open(path, 'w'):
                pass
            created.append(filename)

    if created:
        print("Created test case files:")
        for f in created:
            print(f"  {os.path.join(test_dir, f)}")
    else:
        print("No new files created.")

if __name__ == '__main__':
    main()
