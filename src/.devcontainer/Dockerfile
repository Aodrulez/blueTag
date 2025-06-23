ARG VARIANT="ubuntu-22.04"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

ENV TOOLCHAINURL="https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel"
ENV TOOLCHAINARCHIVE="arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz"
ENV TOOLCHAINDIR="arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi"

RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \
    apt-get -y install --no-install-recommends \
        cmake \
        build-essential \
        wget \
        ca-certificates \
        gdb-multiarch \
        binutils-multiarch \
        automake \
        autoconf \
        libtool \
        libftdi-dev \
        libusb-1.0-0-dev \
        libhidapi-dev \
        pkg-config \
        clang-format \
        python3 \
        python3-tk \
	libjim-dev \
	jimsh

WORKDIR /apps

RUN wget $TOOLCHAINURL/$TOOLCHAINARCHIVE 
RUN tar -J --xz  -xf $TOOLCHAINARCHIVE

RUN mv $TOOLCHAINDIR gcc-arm-none && \
    rm $TOOLCHAINARCHIVE 

ENV PATH=$PATH:/apps/gcc-arm-none/bin

RUN git clone https://github.com/raspberrypi/pico-sdk.git --depth=1 && git -C pico-sdk submodule update --init
ENV PICO_SDK_PATH=/apps/pico-sdk

ENV PICO_SDK_PATH=/apps/pico-sdk

RUN cd /apps && \
    git clone -b master https://github.com/raspberrypi/pico-extras.git --depth=1 \
    && cd pico-extras && \
    git submodule update --init 

ENV PICO_EXTRAS_PATH=/apps/pico-extras

RUN git clone https://github.com/openocd-org/openocd.git --depth=1 --recurse-submodules \
    && cd openocd && ./bootstrap && ./configure --enable-ftdi --enable-sysfsgpio --enable-picoprobe --enable-cmsis-dap && make -j 8 install 

RUN cd /apps && git clone https://github.com/raspberrypi/picotool.git --depth=1 \
    && cd picotool && mkdir build && cd build && cmake ../ && make -j 8 && cp picotool /usr/local/bin

RUN cd /usr/bin && ln -s /usr/bin/objdump objdump-multiarch && ln -s /usr/bin/nm nm-multiarch 

RUN usermod -aG plugdev vscode