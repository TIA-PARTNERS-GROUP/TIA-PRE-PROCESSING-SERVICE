FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    doxygen \
    graphviz \
    python3-pip \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/share/doxygen-awesome && \
    wget https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/main/doxygen-awesome.css -O /usr/share/doxygen-awesome/doxygen-awesome.css

WORKDIR /workspace
COPY . .

RUN mkdir -p /workspace/docs

ENTRYPOINT ["./generate_docs.sh"]
