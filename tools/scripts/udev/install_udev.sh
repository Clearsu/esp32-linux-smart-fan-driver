#!/bin/bash
set -euo pipefail

RULE_NAME="99-ttyfan.rules"
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_RULE="$SRC_DIR/$RULE_NAME"
DST_RULE="/etc/udev/rules.d/$RULE_NAME"

if [[ $EUID -ne 0 ]]; then
    echo "Error: this script must be run as root"
    exit 1
fi

if [[ ! -f "$SRC_RULE" ]]; then
    echo "Error: rule file not found: $SRC_RULE"
    exit 1
fi

cp "$SRC_RULE" "$DST_RULE"
chmod 644 "$DST_RULE"

udevadm control --reload-rules
udevadm trigger

echo "udev rule installed. /dev/ttyFAN should appear when device is plugged."
