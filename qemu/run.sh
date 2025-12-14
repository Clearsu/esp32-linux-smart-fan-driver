#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

DISK="ubuntu-arm64.qcow2"
ISO="ubuntu-24.04.3-live-server-arm64.iso"
EFI="QEMU_EFI.fd"

if [ ! -f "${DISK}" ] || [ ! -f "${ISO}" ] || [ ! -f "${EFI}" ]; then
	echo "Missing files. Run:"
	echo "  ./download.sh"
	exit 1
fi

INSTALL_MODE=0
if [ "${1:-}" = "--install" ]; then
	INSTALL_MODE=1
fi

COMMON_OPTS=(
	-machine virt,accel=hvf
	-cpu host
	-m 2048
	-smp 4
	-bios "${EFI}"
	-drive file="${DISK}",if=virtio,cache=writethrough
	-netdev user,id=net0,hostfwd=tcp::2222-:22
	-device virtio-net-device,netdev=net0
)

if [ "${INSTALL_MODE}" -eq 1 ]; then
	qemu-system-aarch64 \
		"${COMMON_OPTS[@]}" \
		-cdrom "${ISO}" \
		-device virtio-gpu-pci \
		-display default \
		-device qemu-xhci,id=xhci \
		-device usb-kbd,bus=xhci.0 \
		-device usb-mouse,bus=xhci.0
else
	qemu-system-aarch64 \
		"${COMMON_OPTS[@]}" \
		-nographic \
		-serial stdio \
		-monitor none
fi
