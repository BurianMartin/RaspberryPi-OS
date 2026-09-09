#!/usr/bin/env bash
# Fixed verification script for the kernel -- don't edit it. There's no
# libc, no way to link a normal assert-based test against freestanding
# code, so this actually boots the kernel under QEMU and checks the one
# thing observable from the outside: UART serial output.
#
# The kernel is designed to never exit -- kernel_main loops forever
# blinking the LED, which has no ARM equivalent of x86's isa-debug-exit
# device to signal "done" with. So here, QEMU getting killed by the
# timeout is the NORMAL, expected way every run of this script ends --
# success or failure is decided purely by whether the expected message
# showed up on the serial port before that happened, not by the exit code.
#
# Build/run with: make kernel_test

set -euo pipefail

ELF="build/kernel.elf"
EXPECTED_MESSAGE="hello from pi zero"

if [ ! -f "$ELF" ]; then
    echo "FAIL: $ELF not found -- build it first (this script is normally run via 'make kernel_test')"
    exit 1
fi

OUT="$(mktemp)"
trap 'rm -f "$OUT"' EXIT

# -kernel with the ELF directly, not the flattened .img: qemu-system-arm's
# raspi0 machine doesn't correctly jump to a raw flat binary given via
# -kernel (it never reaches the loaded code at all), but does correctly
# read the entry point out of an ELF header and jump there. The flattened
# kernel.img (via objcopy) is still what real hardware needs on the SD
# card -- this is a QEMU-verification-specific difference, not something
# that changes what goes on the SD card.
timeout 5 qemu-system-arm \
    -M raspi0 \
    -kernel "$ELF" \
    -serial stdio \
    -display none \
    >"$OUT" 2>&1 </dev/null || true

if ! grep -qF "$EXPECTED_MESSAGE" "$OUT"; then
    echo "FAIL: expected serial output to contain '$EXPECTED_MESSAGE', got:"
    cat "$OUT"
    exit 1
fi

echo "serial output: $(cat "$OUT")"
echo "test_kernel_boots_and_prints_via_serial: PASS"
echo "All kernel tests passed."
echo
echo "Note: this only verifies the UART side. The LED blink (GPIO47) has"
echo "no QEMU-visible signal -- that's confirmed on real hardware only,"
echo "by flashing build/kernel.img to an SD card and watching the board."
