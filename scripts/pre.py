import subprocess
import sys

print("Running pre-build script...")

# Run the Python script to generate the web page
subprocess.run(
    ["./scripts/.venv/bin/python", "./scripts/generate_web.py"],
    stdout=sys.stdout,
    stderr=sys.stderr,
    check=True,
)

print("Pre-build script completed successfully")
