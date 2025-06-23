# Fetch ubuntu image
FROM ubuntu:22.04

# Install prerequisites
RUN \
    apt update && \
    apt install -y git python3 && \
    apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
    
# Install Pico SDK
RUN \
    mkdir -p /project/src/ && \
    cd /project/ && \
    git clone https://github.com/raspberrypi/pico-sdk.git --branch master && \
    cd pico-sdk/ && \
    git submodule update --init && \
    cd /
    
# Set the Pico SDK environment variable
ENV PICO_SDK_PATH=/project/pico-sdk/

# Copy in our source files
COPY src/. /project/src/

# CMSIS-DAP git submodule
# RUN git init
# RUN git submodule add https://github.com/ARM-software/CMSIS_5.git /project/src/modules/cmsis-dap/CMSIS_5
# RUN cd /project/src/modules/cmsis-dap/CMSIS_5 && git checkout 2b7495b8535bdcb306dac29b9ded4cfb679d7e5c

# Build project
# Build Pico (RP2040) project
RUN \
    mkdir -p /project/src/build_rp2040 && \
    cd /project/src/build_rp2040 && \
    cmake .. && \
    make

# Build RP2350 project
RUN \
    mkdir -p /project/src/build_rp2350 && \
    cd /project/src/build_rp2350 && \
    cmake -DPICO_PLATFORM=rp2350 .. && \
    make
    
# Command that will be invoked when the container starts
ENTRYPOINT ["/bin/bash"]
