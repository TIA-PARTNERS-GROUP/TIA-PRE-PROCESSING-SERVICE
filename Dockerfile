FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    doxygen \
    graphviz \
    python3-pip \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install Doxygen Awesome theme
RUN mkdir -p /usr/share/doxygen-awesome && \
    wget https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/main/doxygen-awesome.css -O /usr/share/doxygen-awesome/doxygen-awesome.css

WORKDIR /workspace
COPY . .

# Create mount point for docs
RUN mkdir -p /workspace/docs

ENTRYPOINT ["./generate_docs.sh"]
