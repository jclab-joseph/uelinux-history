#!/bin/bash

set -ex

SCRIPT_DIR=$(dirname -- "${BASH_SOURCE[0]}")
SCRIPT_DIR=$(realpath ${SCRIPT_DIR})
ROOT_DIR=$(dirname "${SCRIPT_DIR}")
WORK_DIR=$PWD/.tmp

mkdir -p ${WORK_DIR}

EDK2_DIR=${ROOT_DIR}/edk2
export PACKAGES_PATH=${EDK2_DIR}
export EDK_TOOLS_PATH=${EDK2_DIR}/BaseTools

source ${EDK2_DIR}/edksetup.sh

build -p UELinuxPkg/UELinuxPkg.dsc -t GCC5 -a X64 -b DEBUG

${ROOT_DIR}/test/run-efi.sh \
  ${PWD}/Build/UELinuxPkg/DEBUG_GCC5/X64/UELinuxLoader.efi \
  -m 768m \
  -debugcon file:${WORK_DIR}/debug.log -global isa-debugcon.iobase=0x402 \
  -chardev pty,id=char0,logfile=${WORK_DIR}/serial.log,mux=on,signal=off -serial chardev:char0 \
  -chardev pty,id=char1,logfile=${WORK_DIR}/kern.log,signal=off -serial chardev:char1 \
  -display none -daemonize -pidfile ${WORK_DIR}/qemu.pid

sleep 5
kill $(cat "${WORK_DIR}/qemu.pid")


