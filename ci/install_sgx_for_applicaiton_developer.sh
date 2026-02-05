#!/bin/bash

# Reference:
#   https://cc-enabling.trustedservices.intel.com/intel-sgx-sw-installation-guide-linux/02/installation_instructions/#intel-sgx-application-developer

set -euo pipefail
if [ "${DEBUG:-0}" -eq 1 ]; then
  set -o xtrace
fi

# Error trap handler - logs failure details and calls cleanup before exit
error_handler() {
    local exit_code=$?
    local line_number=${1:-$LINENO}
    local bash_lineno=${2:-$BASH_LINENO}
    local last_command=${3:-$BASH_COMMAND}
    local function_stack=${4:-${FUNCNAME[*]}}
    
    # Log error context to file
    {
        echo "=== ERROR OCCURRED ==="
        echo "Exit Code: $exit_code"
        echo "Line Number: $line_number"
        echo "Bash Line: $bash_lineno"
        echo "Failed Command: $last_command"
        echo "Function Stack: $function_stack"
        echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "======================"
    } >> "${LOG_FILE:-/tmp/install_sgx.log}" 2>/dev/null || true
    
    # Print concise error to stderr
    echo "ERROR: Script failed at line $line_number with exit code $exit_code" >&2
    echo "Failed command: $last_command" >&2
    echo "Check log file: ${LOG_FILE:-/tmp/install_sgx.log}" >&2
    
    # Call cleanup function if it exists
    if type cleanup >/dev/null 2>&1; then
        cleanup || true
    fi
    
    exit $exit_code
}

# Set up error trap
trap 'error_handler $LINENO $BASH_LINENO "$BASH_COMMAND" "${FUNCNAME[*]}"' ERR

# Platform will be detected dynamically by platform_detect() function
# Supported platforms: Debian12, Debian11, Ubuntu22.04-server, Ubuntu20.04-server  
PLATFORM=""

# Logging infrastructure
LOG_FILE="/tmp/install_sgx.log"

# Initialize log file with timestamp
init_log() {
    echo "=== Intel SGX Installation Log - $(date) ===" > "${LOG_FILE}"
    echo "Platform: ${PLATFORM}" >> "${LOG_FILE}"
    echo "Script: $0" >> "${LOG_FILE}"
    echo "Started at: $(date '+%Y-%m-%d %H:%M:%S')" >> "${LOG_FILE}"
    echo "" >> "${LOG_FILE}"
}

# Log message with timestamp
log_info() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "${LOG_FILE}"
}

# Execute command with output redirected to log
log_exec() {
    log_info "Executing: $*"
    "$@" >>"$LOG_FILE" 2>&1
}

# Print environment sourcing instructions
print_env_instructions() {
    log_info "Printing environment setup instructions"
    
    echo "========================================================================"
    echo "  IMPORTANT: Before building or running SGX applications, you must run:"
    echo "      source /opt/intel/sgxsdk/environment"
    echo "  in your current shell to activate SGX SDK environment variables."
    echo "========================================================================"
    
    log_info "Environment setup instructions displayed to user"
}

check_sgx_packages() {
    log_info "Checking for existing SGX packages..."
    
    local packages=("libsgx-quote-ex" "libsgx-dcap-ql" "libsgx-enclave-common-dev" "libsgx-dcap-ql-dev" "libsgx-dcap-default-qpl-dev" "tee-appraisal-tool")
    local missing_packages=()
    
    for package in "${packages[@]}"; do
        if ! dpkg -l "$package" >> "${LOG_FILE}" 2>&1; then
            missing_packages+=("$package")
            log_info "Package $package not installed"
        else
            log_info "Package $package already installed"
        fi
    done
    
    if [ ${#missing_packages[@]} -eq 0 ]; then
        log_info "All SGX packages are already installed"
        return 0
    else
        log_info "Missing SGX packages: ${missing_packages[*]}"
        return 1
    fi
}

check_sgx_sdk() {
    log_info "Checking for existing SGX SDK..."
    
    if [ -d "/opt/intel/sgxsdk" ] && [ -f "/opt/intel/sgxsdk/environment" ]; then
        log_info "SGX SDK already installed at /opt/intel/sgxsdk"
        
        # Validate SDK installation by checking key components
        if [ -f "/opt/intel/sgxsdk/bin/sgx-gdb" ] && [ -d "/opt/intel/sgxsdk/include" ]; then
            log_info "SGX SDK installation appears complete"
            return 0
        else
            log_info "SGX SDK installation incomplete - missing key components"
            return 1
        fi
    else
        log_info "SGX SDK not found"
        return 1
    fi
}

check_sgx_repo() {
    log_info "Checking for existing SGX local repository..."
    
    if [ -d "/opt/intel/sgx_debian_local_repo" ] && [ -f "/etc/apt/sources.list.d/sgx_debian_local_repo.list" ]; then
        log_info "SGX local repository already configured"
        return 0
    else
        log_info "SGX local repository not configured"
        return 1
    fi
}

# Modular installation functions

# Platform detection and configuration
platform_detect() {
    log_info "Entering platform_detect() function"
    
    if [ ! -f "/etc/os-release" ]; then
        log_info "ERROR: /etc/os-release not found - cannot detect OS"
        echo "ERROR: Cannot detect operating system. /etc/os-release not found." >&2
        log_info "Exiting platform_detect() function"
        return 1
    fi
    
    # Parse OS information from /etc/os-release
    local os_id=$(grep '^ID=' /etc/os-release | cut -d'=' -f2 | tr -d '"')
    local version_id=$(grep '^VERSION_ID=' /etc/os-release | cut -d'=' -f2 | tr -d '"')
    
    log_info "Raw OS detection: ID=${os_id}, VERSION_ID=${version_id}"
    
    # Determine platform string based on OS and version
    case "${os_id}" in
        "ubuntu")
            case "${version_id}" in
                "20.04")
                    PLATFORM="Ubuntu20.04-server"
                    ;;
                "22.04")
                    PLATFORM="Ubuntu22.04-server"
                    ;;
                *)
                    log_info "ERROR: Unsupported Ubuntu version ${version_id}. Supported: 20.04, 22.04"
                    echo "ERROR: Unsupported Ubuntu version ${version_id}. This script supports Ubuntu 20.04 and 22.04 only." >&2
                    log_info "Exiting platform_detect() function"
                    return 1
                    ;;
            esac
            ;;
        "debian")
            case "${version_id}" in
                "11")
                    PLATFORM="Debian11"
                    ;;
                "12")
                    PLATFORM="Debian12"
                    ;;
                *)
                    log_info "ERROR: Unsupported Debian version ${version_id}. Supported: 11, 12"
                    echo "ERROR: Unsupported Debian version ${version_id}. This script supports Debian 11 and 12 only." >&2
                    log_info "Exiting platform_detect() function"
                    return 1
                    ;;
            esac
            ;;
        *)
            log_info "ERROR: Unsupported OS ${os_id}. Supported: ubuntu, debian"
            echo "ERROR: Unsupported operating system '${os_id}'. This script supports Ubuntu and Debian only." >&2
            log_info "Exiting platform_detect() function"
            return 1
            ;;
    esac
    
    log_info "Successfully detected platform: ${PLATFORM}"
    echo "Detected platform: ${PLATFORM}"
    
    log_info "Exiting platform_detect() function"
    return 0
}

# Install SGX packages and SDK
install_packages() {
    log_info "Entering install_packages() function"
    
    # Skip repo setup if already configured
    if ! check_sgx_repo; then
        log_info "Setting up SGX local repository..."
        
        pushd /tmp >> "${LOG_FILE}" 2>&1
        log_exec curl -fsSLO \
            https://download.01.org/intel-sgx/latest/linux-latest/distro/${PLATFORM}/sgx_debian_local_repo.tgz

        local_sum=$(sha256sum sgx_debian_local_repo.tgz | awk '{print $1}')
        remote_sum=$(curl -s https://download.01.org/intel-sgx/latest/dcap-latest/linux/SHA256SUM_dcap_1.24.cfg | grep "distro/${PLATFORM}/sgx_debian_local_repo.tgz" | awk '{print $1}')
        if [[ "$local_sum" == "$remote_sum" ]]; then
            log_info "Checksum matches"
        else
            log_info "Checksum mismatch!"
        fi

        log_exec sudo mkdir -p /opt/intel
        log_exec sudo tar xzf sgx_debian_local_repo.tgz -C /opt/intel

        echo 'deb [signed-by=/etc/apt/keyrings/intel-sgx-keyring.asc arch=amd64] file:///opt/intel/sgx_debian_local_repo bookworm main' \
            | sudo tee /etc/apt/sources.list.d/sgx_debian_local_repo.list | tee -a "${LOG_FILE}" > /dev/null

        log_exec sudo cp /opt/intel/sgx_debian_local_repo/keys/intel-sgx.key /etc/apt/keyrings/intel-sgx-keyring.asc
        popd >> "${LOG_FILE}" 2>&1
    else
        log_info "SGX repository already configured, skipping setup"
    fi

    # Install SGX packages only if missing
    if ! check_sgx_packages; then
        log_info "Installing missing SGX packages..."
        log_exec sudo apt-get update
        log_exec sudo apt-get install -y libsgx-quote-ex libsgx-dcap-ql
    else
        log_info "SGX packages already installed, skipping"
    fi

    # Install build dependencies
    log_exec sudo apt-get update --quiet
    log_exec sudo apt-get install --quiet -y build-essential python3
    log_exec sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 1

    # Install Intel SGX SDK only if missing
    if ! check_sgx_sdk; then
        log_info "Installing Intel SGX SDK for Application Developer..."
        
        pushd /opt/intel >> "${LOG_FILE}" 2>&1
        log_exec sudo curl -fsSLo sgx_linux_x64_sdk.bin \
            https://download.01.org/intel-sgx/latest/linux-latest/distro/${PLATFORM}/sgx_linux_x64_sdk_2.27.100.1.bin
        log_exec sudo chmod +x sgx_linux_x64_sdk.bin
        log_exec sudo ./sgx_linux_x64_sdk.bin --prefix /opt/intel

        # Log environment setup instructions for user
        log_info "SGX SDK installation completed successfully."
        log_info "IMPORTANT: To use the SGX SDK in your development session:"
        log_info "  1. Run: source /opt/intel/sgxsdk/environment"
        log_info "  2. This must be done in each shell session where you use SGX"
        log_info "  3. Environment variables are session-specific and cannot be exported by this script"
        log_info "  4. Consider adding 'source /opt/intel/sgxsdk/environment' to your ~/.bashrc for automatic setup"
        popd >> "${LOG_FILE}" 2>&1
    else
        log_info "SGX SDK already installed, skipping"
    fi

    # Install Developer packages for Intel SGX only if missing
    if ! check_sgx_packages; then
        log_info "Installing Intel SGX Developer packages..."
        
        log_exec sudo apt-get install -y libsgx-enclave-common-dev \
            libsgx-dcap-ql-dev \
            libsgx-dcap-default-qpl-dev \
            tee-appraisal-tool
    else
        log_info "SGX Developer packages already installed, skipping"
    fi
    
    log_info "Exiting install_packages() function"
    return 0
}

# Validate the installation was successful
validate_installation() {
    log_info "Entering validate_installation() function"
    
    local validation_failed=0
    
    # Re-check all components after installation
    if ! check_sgx_packages; then
        log_info "VALIDATION FAILED: SGX packages not properly installed"
        validation_failed=1
    fi
    
    if ! check_sgx_sdk; then
        log_info "VALIDATION FAILED: SGX SDK not properly installed"
        validation_failed=1
    fi
    
    if ! check_sgx_repo; then
        log_info "VALIDATION FAILED: SGX repository not properly configured"
        validation_failed=1
    fi
    
    if [ $validation_failed -eq 0 ]; then
        log_info "VALIDATION SUCCESS: All SGX components properly installed"
    else
        log_info "VALIDATION FAILED: Some SGX components failed installation"
        log_info "Exiting validate_installation() function"
        return 1
    fi
    
    log_info "Exiting validate_installation() function"
    return 0
}

# Clean up temporary files
cleanup() {
    log_info "Entering cleanup() function"
    
    # Clean up any temporary files in /tmp related to SGX installation
    if [ -f "/tmp/sgx_debian_local_repo.tgz" ]; then
        log_info "Removing temporary SGX repository archive"
        rm -f /tmp/sgx_debian_local_repo.tgz
    fi
    
    # Additional cleanup can be added here as needed
    log_info "Temporary file cleanup completed"
    
    log_info "Exiting cleanup() function"
    return 0
}

# Initialize logging
init_log

log_info "Starting idempotency checks..."

# Check if everything is already installed
if check_sgx_packages && check_sgx_sdk && check_sgx_repo; then
    log_info "Complete SGX installation detected - no action needed"
    echo "Intel SGX for Application Developer is already installed and configured."
    print_env_instructions
    exit 0
fi

log_info "Partial or missing SGX installation detected - proceeding with installation"

# Main installation flow using modular functions

log_info "Starting Intel SGX for Application Developer installation..."

# Execute installation steps in modular fashion
platform_detect
if [ $? -ne 0 ]; then
    log_info "Platform detection failed"
    exit 1
fi

install_packages
if [ $? -ne 0 ]; then
    log_info "Package installation failed"
    exit 1
fi

validate_installation
if [ $? -ne 0 ]; then
    log_info "Installation validation failed"
    exit 1
fi

cleanup
if [ $? -ne 0 ]; then
    log_info "Cleanup failed"
    exit 1
fi

cleanup
if [ $? -ne 0 ]; then
    log_info "Cleanup failed"
    exit 1
fi

echo "Intel SGX for Application Developer installation completed."
print_env_instructions
