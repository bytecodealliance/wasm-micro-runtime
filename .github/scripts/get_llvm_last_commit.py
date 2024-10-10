import os
import subprocess
import sys

# Get the repository root directory from the environment variable
repo_root = os.environ["GITHUB_WORKSPACE"]

# Change to the 'build-scripts' directory within the repository
build_scripts_dir = os.path.join(repo_root, "build-scripts")
os.chdir(build_scripts_dir)

gh_token = os.environ.get("GH_TOKEN", "")
extra_options = sys.argv[1:]

command = [sys.executable, "./build_llvm.py"] + extra_options + ["--llvm-ver"]
env = os.environ.copy()
env["GH_TOKEN"] = gh_token

result = subprocess.check_output(command, env=env, text=True).strip()

with open(os.environ["GITHUB_OUTPUT"], "a") as f:
    print(f"last_commit={result}", file=f)
