FROM ubuntu:focal
ENV DEBIAN_FRONTEND=noninteractive

COPY src /src/

RUN apt-get update && apt-get install -y \
  cmake \
  build-essential \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /build/
RUN cmake /src/ && make

CMD [ "/build/af_packet.exe"]