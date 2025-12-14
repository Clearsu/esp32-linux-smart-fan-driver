#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

# If QEMU is installed:
if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
	echo "qemu-system-aarch64 not found."
	echo "Install: brew install qemu"
	exit 1
fi

UBUNTU_ISO="ubuntu-24.04.3-live-server-arm64.iso"
UBUNTU_ISO_URL="https://cdimage.ubuntu.com/releases/24.04/release/${UBUNTU_ISO}"

# UEFI firmware (QEMU_EFI.fd)
EFI_TARBALL="QEMU_EFI-a096471-edk2-stable202011.tar.gz"
EFI_TARBALL_URL="https://gist.githubusercontent.com/theboreddev/5f79f86a0f163e4a1f9df919da5eea20/raw/f546faea68f4149c06cca88fa67ace07a3758268/${EFI_TARBALL}"

# VM disk
DISK="ubuntu-arm64.qcow2"
DISK_SIZE="30G"

echo "[1/3] Downloading Ubuntu ISO..."
if [ ! -f "${UBUNTU_ISO}" ]; then
	curl -L -o "${UBUNTU_ISO}" "${UBUNTU_ISO_URL}"
else
	echo "  - already exists: ${UBUNTU_ISO}"
fi

echo "[2/3] Downloading and extracting QEMU_EFI.fd..."
if [ ! -f "QEMU_EFI.fd" ]; then
	curl -L -o "${EFI_TARBALL}" "${EFI_TARBALL_URL}"
	tar -xvf "${EFI_TARBALL}"
	if [ ! -f "QEMU_EFI.fd" ]; then
		echo "ERROR: QEMU_EFI.fd not found after extracting ${EFI_TARBALL}"
		exit 1
	fi
else
	echo "  - already exists: QEMU_EFI.fd"
fi

echo "[3/3] Creating qcow2 disk..."
if [ ! -f "${DISK}" ]; then
	qemu-img create -f qcow2 "${DISK}" "${DISK_SIZE}"
else
	echo "  - already exists: ${DISK}"
fi

echo "Done."
echo ""
echo "Next:"
echo "  ./run.sh --install   # first-time install"
echo "  ./run.sh             # boot installed system"
