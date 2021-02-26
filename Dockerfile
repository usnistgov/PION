FROM debian:buster
ARG NDNPH_VERSION=develop
RUN apt-get -y -qq update && \
    apt-get -y -qq install --no-install-recommends build-essential ca-certificates curl libboost-dev libmbedtls-dev meson ninja-build && \
    rm -rf /var/lib/apt/lists/*
RUN mkdir -p /root/NDNph && \
    curl -sfL https://github.com/yoursunny/NDNph/archive/${NDNPH_VERSION}.tar.gz | tar -C /root/NDNph -xz --strip-components=1 && \
    cd /root/NDNph && \
    meson build -Dunittest=disabled -Dprograms=enabled && \
    ninja -C build -j1 install && \
    find ./build/programs -type f -maxdepth 1 | xargs install -t /usr/local/bin && \
    rm -rf /root/NDNph
ADD . /root/ndn-onboarding/
RUN cd /root/ndn-onboarding && \
    meson build && \
    ninja -C build -j1 && \
    find ./build/programs -type f -maxdepth 1 | xargs install -t /usr/local/bin && \
    rm -rf build
