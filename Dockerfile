# =============================================================================
# Stage 1 – Builder
#   Installs all build-time and test dependencies, compiles the entire project,
#   and runs the full GoogleTest + CTest suite.  The image build FAILS if any
#   test fails, giving you a hard gate on correctness.
# =============================================================================
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

# Copy source tree (build/ and .git/ are excluded by .dockerignore)
COPY . .

# Configure – out-of-source Release build
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compile everything: main executable + all unit-test and scenario targets.
# GoogleTest is fetched from GitHub by CMake's FetchContent during this step.
RUN cmake --build build --parallel "$(nproc)"

# Run the full test suite.
# scenario_real_capture is included: it detects missing privileges and exits 0
# (SKIPPED), so all tests pass even without CAP_NET_RAW.
RUN ctest --test-dir build --output-on-failure

# =============================================================================
# Stage 2 – Runtime
#   Lean final image: only the compiled binary + its shared-library runtime
#   dependency.  No build tools, no headers, no GoogleTest, no temp files.
# =============================================================================
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libpcap0.8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only the compiled application binary from the builder stage
COPY --from=builder /app/build/NetworkPacketSniffer /app/NetworkPacketSniffer

# The sniffer is an interactive console application; always run with -it.
# To perform real packet capture you also need:
#   --cap-add NET_RAW --net=host
ENTRYPOINT ["/app/NetworkPacketSniffer"]
