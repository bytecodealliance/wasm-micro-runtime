echo "Running finalize script..."

#
# Python Package Installation
#

# Upgrade pip first
python3 -m pip install --no-cache-dir --break-system-packages --upgrade pip
# Install required packages
pip3 install --no-cache-dir --break-system-packages -r .devcontainer/requirements.txt

echo "Finalize script completed. ✅"
