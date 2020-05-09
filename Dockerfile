FROM ubuntu:focal
ENV DEBIAN_FRONTEND=noninteractive

COPY src /src/

RUN apt-get update && apt-get install -y \
  cmake \
  g++ \
  gdb \
  valgrind \
  libcap2-bin  \
  libleveldb-dev
# && rm -rf /var/lib/apt/lists/*

WORKDIR /build/
RUN cmake /src/ 
#RUN make
#RUN setcap 'CAP_NET_RAW+ep CAP_NET_ADMIN+ep CAP_IPC_LOCK+ep' /build/afpacket_test.exe

# RUN valgrind /build/afpacket_test.exe

CMD [ "/build/afpacket_test.exe"]