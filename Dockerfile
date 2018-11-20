FROM ubuntu
MAINTAINER Lazar Demin (lazar@onion.io)

RUN apt-get update && apt-get install -y \
    build-essential \
    vim \
    git \
    wget \
    curl \
    time \
    subversion \
    build-essential \
    libncurses5-dev \
    zlib1g-dev \
    gawk \
    flex \
    quilt \
    git-core \
    unzip \
    libssl-dev \
    python-dev \
    python-pip \
    libxml-parser-perl \
    default-jdk \
    npm

RUN npm install -g n node-gyp && n 4.3.1 && dpkg -r --force-depends nodejs

ENV FORCE_UNSAFE_CONFIGURE 1

COPY . /root/source
WORKDIR /root/source

RUN sh scripts/onion-feed-setup.sh && git checkout .config

