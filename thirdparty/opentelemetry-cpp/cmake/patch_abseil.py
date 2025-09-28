#!/usr/bin/env python3
import sys
import os

copts_file = sys.argv[1]

with open(copts_file, 'r') as f:
    content = f.read()

# Check if import already added
if 'import sys' not in content:
    # Add after """
    lines = content.split('\n')
    for i, line in enumerate(lines):
        if line.strip() == '"""':
            lines.insert(i + 1, '')
            lines.insert(i + 1, 'import sys')
            lines.insert(i + 1, 'import platform')
            lines.insert(i + 1, '')
            lines.insert(i + 1, "# Determine if we're building for ARM64 (Apple Silicon or ARM Linux)")
            lines.insert(i + 1, "is_arm64 = platform.machine() in ('arm64', 'aarch64')")
            break

    content = '\n'.join(lines)

# Check if override already added
if 'COPT_VARS["ABSL_RANDOM_HWAES_X64_FLAGS"] = []' not in content:
    # Add after COPT_VARS = {
    lines = content.split('\n')
    in_dict = False
    for i, line in enumerate(lines):
        if 'COPT_VARS = {' in line:
            in_dict = True
        elif in_dict and line.strip() == '}':
            lines.insert(i + 1, '')
            lines.insert(i + 1, '# Override X64 HWAES flags for ARM64 to prevent SSE4.1 compiler errors')
            lines.insert(i + 1, 'if is_arm64:')
            lines.insert(i + 1, '    COPT_VARS["ABSL_RANDOM_HWAES_X64_FLAGS"] = []')
            break

    content = '\n'.join(lines)

# Write back
with open(copts_file, 'w') as f:
    f.write(content)

print("Patched abseil copts.py")
