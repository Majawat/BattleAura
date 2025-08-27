#!/usr/bin/env python3
"""
PlatformIO pre-build script to embed web files
"""

Import("env")
import subprocess
import sys
import os
from pathlib import Path

def embed_files(source, target, env):
    """Run the file embedding script"""
    project_dir = env.get("PROJECT_DIR")
    script_path = os.path.join(project_dir, "scripts", "embed_files.py")
    
    print("=" * 50)
    print("Embedding web files...")
    print("=" * 50)
    
    try:
        subprocess.run([sys.executable, script_path], check=True, cwd=project_dir)
        print("Web files embedded successfully!")
    except subprocess.CalledProcessError as e:
        print(f"Failed to embed web files: {e}")
        sys.exit(1)

# Register the pre-build action
env.AddPreAction("$BUILD_DIR/src/main.cpp.o", embed_files)