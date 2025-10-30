FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    xvfb x11vnc ffmpeg python3 python3-venv python3-pip git build-essential ca-certificates x11-utils make sudo \
  && rm -rf /var/lib/apt/lists/*

# create dev user
RUN useradd -m dev && echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev
WORKDIR /work
COPY . /work/src
RUN chown -R dev:dev /work/src
USER dev
ENV PATH="/home/dev/.local/bin:${PATH}"

# install python deps if present
RUN python3 -m venv /home/dev/venv && /home/dev/venv/bin/pip install --upgrade pip || true
RUN if [ -f /work/src/requirements.txt ]; then /home/dev/venv/bin/pip install -r /work/src/requirements.txt || true; fi

# copy entrypoint
COPY --chown=dev:dev scripts/dev-entrypoint.sh /usr/local/bin/dev-entrypoint.sh
RUN chmod +x /usr/local/bin/dev-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/dev-entrypoint.sh"]
