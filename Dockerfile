
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake \
        g++ \
        make \
        git \
        ca-certificates \
        libpcap-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app


COPY . .


RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

RUN cmake --build build --parallel "$(nproc)"


RUN ctest --test-dir build --output-on-failure


FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libpcap0.8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app


COPY --from=builder /app/build/NetworkPacketSniffer /app/NetworkPacketSniffer


ENTRYPOINT ["/app/NetworkPacketSniffer"]
