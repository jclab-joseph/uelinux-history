FROM ubuntu:22.04 as builder

RUN apt-get update -y && \
    apt-get install -y \
    bash curl wget gnupg2 \
    build-essential automake \
    debhelper dh-exec \
    device-tree-compiler bc gcc-arm-none-eabi \
    python3 python3-pip python3-setuptools python3-wheel ninja-build \
    debhelper-compat flex bison libglib2.0-dev zlib1g-dev libaio-dev libjack-dev libpulse-dev libasound2-dev libbpf-dev libbrlapi-dev libcap-ng-dev libcurl4-gnutls-dev libfdt-dev libfuse3-dev libgtk-3-dev libvte-2.91-dev libncurses-dev librbd-dev libsdl2-dev libseccomp-dev libslirp-dev liburing-dev libusb-1.0-0-dev libzstd-dev nettle-dev libudev-dev libjpeg-dev libpng-dev libpmem-dev \
 libssl-dev

RUN pip3 install meson

COPY ["qemu_8.0.4+dfsg.orig.tar.xz", "/work/"]
RUN cd /work && \
    tar -xf qemu_8.0.4+dfsg.orig.tar.xz

COPY ["debian", "/work/qemu-8.0.4/debian"]

RUN mkdir -p /work/output
VOLUME /work/output
WORKDIR /work/qemu-8.0.4

