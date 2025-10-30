# MinUI Development Environment
# ARM64 Ubuntu-based container for visual testing and debugging

FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install required packages
RUN apt-get update && apt-get install -y \
    # X server and VNC
    xvfb \
    x11vnc \
    x11-utils \
    # Video recording
    ffmpeg \
    # Python environment
    python3 \
    python3-venv \
    python3-pip \
    # Build tools
    git \
    build-essential \
    ca-certificates \
    # QEMU for running ARM binaries (if needed)
    qemu-user-static \
    # Additional utilities
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -s /bin/bash dev && \
    mkdir -p /work && \
    chown dev:dev /work

# Set working directory
WORKDIR /work

# Copy requirements.txt if it exists and install Python dependencies
COPY requirements.txt /tmp/requirements.txt
RUN pip3 install --no-cache-dir -r /tmp/requirements.txt || true

# Copy entrypoint script
COPY scripts/dev-entrypoint.sh /usr/local/bin/dev-entrypoint.sh
RUN chmod +x /usr/local/bin/dev-entrypoint.sh

# Switch to non-root user
USER dev

# Set environment variables
ENV DISPLAY=:99
ENV PYTHONUNBUFFERED=1

# Expose VNC port
EXPOSE 5900

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/dev-entrypoint.sh"]
