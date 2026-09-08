FROM ubuntu:24.04

# ================ INSTALL DEPS
RUN apt-get update && \
    apt-get install -y gcc-14 g++-14 cmake && \
    apt-get install -y gdb valgrind clang-format cmake-format  && \
    apt-get install -y catch2 && \
    apt-get install -y openmpi-bin libopenmpi-dev && \
    apt-get install -y git mc vim sudo && \
    apt-get install -y python3 python3-pip && \
    pip3 install --break-system-packages --no-cache-dir black && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100 && \
    rm -rf /var/lib/apt/lists/*

# ================ CREATE USER
RUN usermod -l user ubuntu && \
    groupmod -n user ubuntu && \
    usermod -d /home/user -m user && \
    echo "user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

ARG HOST_UID=1000
ARG HOST_GID=1000

RUN if [ "$HOST_UID" != "1000" ]; then usermod -u $HOST_UID user; fi && \
    if [ "$HOST_GID" != "1000" ]; then \
    if ! getent group $HOST_GID >/dev/null; then \
    groupmod -g $HOST_GID user; \
    else \
    usermod -g $HOST_GID user; \
    fi \
    fi

USER user

# ================ ENVIRONMENT
ENV CC=/usr/bin/gcc-14
ENV CXX=/usr/bin/g++-14

WORKDIR /app

CMD ["bash"]