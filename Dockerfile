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

RUN wget https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/main/doxygen-awesome.css -O /doxygen-awesome.css

WORKDIR /workspace
COPY . .

RUN chmod +x generate_docs.sh

CMD ["./generate_docs.sh"]
