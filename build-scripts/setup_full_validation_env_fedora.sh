#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

LLVM_ARCH="${LLVM_ARCH:-X86}"

log() {
    printf '[setup] %s\n' "$*"
}

die() {
    printf '[setup] error: %s\n' "$*" >&2
    exit 1
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "missing required command: $1"
}

ensure_repo_root() {
    [[ -f "${REPO_ROOT}/tests/wamr-test-suites/test_wamr.sh" ]] \
        || die "script must live inside the WAMR repository"
}

ensure_fedora() {
    [[ -f /etc/os-release ]] || die "cannot detect OS"
    # shellcheck disable=SC1091
    source /etc/os-release
    [[ "${ID:-}" == "fedora" ]] || die "this setup script currently targets Fedora only"
}

install_dnf_packages() {
    local packages=(
        bubblewrap
        cmake
        curl
        gcc
        gcc-c++
        libasan
        git
        glibc-devel
        glibc-devel.i686
        libstdc++-devel
        libstdc++-devel.i686
        libzstd-devel
        libzstd-devel.i686
        m4
        make
        ninja-build
        ocaml
        ocaml-dune
        ocaml-findlib
        ocaml-menhir
        ocaml-ocamlbuild
        opam
        patch
        pkgconf-pkg-config
        python3
        python3-pip
        rsync
        tar
        unzip
        wabt
        wget
        # Fedora 43 ships zlib development headers via zlib-ng-compat-devel.
        zlib-ng-compat-devel
        zlib-ng-compat-devel.i686
    )

    log "installing Fedora packages required for LLVM, unit tests, and GC spec tests"
    sudo dnf install -y "${packages[@]}"
}

ensure_ninja_command() {
    if command -v ninja >/dev/null 2>&1; then
        return
    fi

    if command -v ninja-build >/dev/null 2>&1; then
        log "creating /usr/local/bin/ninja shim for CMake/Ninja detection"
        sudo ln -sf "$(command -v ninja-build)" /usr/local/bin/ninja
        return
    fi

    die "ninja-build package installed but neither ninja nor ninja-build is available"
}

install_python_requirements() {
    log "installing Python requirements used by build scripts"
    python3 -m pip install --user -r "${REPO_ROOT}/build-scripts/requirements.txt"
}

ensure_system_ocaml_toolchain() {
    log "using Fedora OCaml toolchain for GC spec tests"
    log "opam 4.13.1 build is not used here because it fails on Fedora 43 with GCC 15"
}

verify_host_tools() {
    require_cmd cmake
    require_cmd gcc
    require_cmd g++
    require_cmd git
    require_cmd make
    require_cmd ninja
    require_cmd ocaml
    require_cmd ocamlbuild
    require_cmd ocamlfind
    require_cmd python3
    require_cmd menhir
    require_cmd wget
}

verify_x86_32_support() {
    local tmpdir
    tmpdir="$(mktemp -d)"

    cat > "${tmpdir}/hello.c" <<'EOF'
int main(void) { return 0; }
EOF

    log "verifying 32-bit compiler support with gcc -m32"
    gcc -m32 "${tmpdir}/hello.c" -o "${tmpdir}/hello32"
    rm -rf "${tmpdir}"
}

print_versions() {
    log "tool versions"
    cmake --version | sed -n '1p'
    gcc --version | sed -n '1p'
    ninja --version
    ocaml -version
    python3 --version
}

print_next_steps() {
    cat <<EOF

[setup] environment is ready for the local validation path
[setup] next commands:

cd "${REPO_ROOT}"
python3 build-scripts/build_llvm.py --arch ${LLVM_ARCH}
cmake -S wamr-compiler -B wamr-compiler/build
cmake --build wamr-compiler/build --parallel 4
cmake -S tests/unit -B build/contrib-unit-x86_64-full -DWAMR_BUILD_TARGET=X86_64 -DFULL_TEST=ON
cmake --build build/contrib-unit-x86_64-full --parallel 4
ctest --test-dir build/contrib-unit-x86_64-full --output-on-failure
cmake -S tests/unit -B build/contrib-unit-x86_32-full -DWAMR_BUILD_TARGET=X86_32 -DFULL_TEST=ON
cmake --build build/contrib-unit-x86_32-full --parallel 4
ctest --test-dir build/contrib-unit-x86_32-full --output-on-failure
cd tests/wamr-test-suites
./test_wamr.sh -s spec -G -b -P -t classic-interp
./test_wamr.sh -s spec -G -b -P -t fast-interp
./test_wamr.sh -s spec -G -b -P -t aot

EOF
}

main() {
    ensure_repo_root
    ensure_fedora
    require_cmd sudo
    require_cmd dnf

    cd "${REPO_ROOT}"

    install_dnf_packages
    ensure_ninja_command
    install_python_requirements
    ensure_system_ocaml_toolchain
    verify_host_tools
    verify_x86_32_support
    print_versions
    print_next_steps
}

main "$@"
