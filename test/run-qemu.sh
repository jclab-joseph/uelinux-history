#!/bin/bash

set -ex

SCRIPT_DIR=$(dirname -- "${BASH_SOURCE[0]}")
SCRIPT_DIR=$(realpath ${SCRIPT_DIR})

QEMU_MACHINE="q35,smm=on"
[ -w /dev/kvm ] && QEMU_MACHINE="${QEMU_MACHINE},accel=kvm" || true

qemu-system-x86_64 -smp 2 \
  -machine ${QEMU_MACHINE} \
  -D /dev/stderr \
  -device ahci,id=ahci \
  -drive file=${SCRIPT_DIR}/OVMF_CODE.debug.fd,if=pflash,format=raw,unit=0,readonly=on \
  -drive file=${SCRIPT_DIR}/OVMF_VARS.fd,if=pflash,format=raw,unit=1,readonly=on \
  "$@"

