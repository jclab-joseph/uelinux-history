#!/bin/bash

set -ex

EFI_FILE=$1
shift

SCRIPT_DIR=$(dirname -- "${BASH_SOURCE[0]}")
SCRIPT_DIR=$(realpath ${SCRIPT_DIR})
WORK_DIR=$PWD/.tmp

[ -d "${WORK_DIR}" ] && rm -rf ${WORK_DIR} || true
mkdir -p ${WORK_DIR}

fallocate -l 32M ${WORK_DIR}/efiboot.img || dd if=/dev/zero of=${WORK_DIR}/efiboot.img bs=1M count=32
mkfs.vfat ${WORK_DIR}/efiboot.img
mmd -i ${WORK_DIR}/efiboot.img EFI EFI/BOOT
mcopy -vi ${WORK_DIR}/efiboot.img ${SCRIPT_DIR}/bzImage ::EFI/BOOT/KERNEL.EFI
mcopy -vi ${WORK_DIR}/efiboot.img ${EFI_FILE} ::EFI/BOOT/BOOTX64.EFI

${SCRIPT_DIR}/run-qemu.sh -drive if=virtio,file=${WORK_DIR}/efiboot.img,format=raw,readonly=off "$@"
