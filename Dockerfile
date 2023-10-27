FROM debian:12.1

RUN apt-get update && apt-get install -y wget git bzip2 unzip m4 nasm openssl libssl-dev python3 libtool g++ gcc make sed gdb

RUN gcc --version

# cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.3/cmake-3.27.3.tar.gz \
    && tar -xzf cmake-3.27.3.tar.gz \
    && cd cmake-3.27.3 \
    && ./bootstrap \
    && make \
    && make install

# boost
RUN git clone https://github.com/boostorg/boost \
    && cd boost \
    && git checkout boost-1.82.0 \
    && git submodule update --init --recursive \
    && ./bootstrap.sh --with-libraries=all link=static \
    && ./b2 install -j2 --with-system --with-thread --with-regex link=static -mt

# cryptoTools
RUN git clone https://github.com/ladnir/cryptoTools \
    && cd cryptoTools \
    && git checkout cb188c99ca0bf9287903fa5b14872600eccc3c42 \
    && python3 build.py --install --setup --boost

# vole & vole build(fail)
RUN git clone https://github.com/Visa-Research/volepsi.git \
    && cd volepsi \
    && git checkout 687ca2dd03fd663a216b6ede9d2707f6d5b10b00 \
    && python3 build.py -DVOLE_PSI_ENABLE_BOOST=ON

# fix about std::exchange
RUN sed -i -e "1i #include <utility>" /volepsi/out/macoro/macoro/when_all.h \
    && sed -i -e "1i #include <utility>" /volepsi/out/macoro/macoro/channel.h \
    && sed -i -e "1i #include <utility>" /volepsi/out/libOTe/cryptoTools/cryptoTools/Crypto/AES.h \
    && sed -i -e "1i #include <utility>" /volepsi/out/libOTe/cryptoTools/cryptoTools/Common/Aligned.h

# vole build(success)
RUN cd volepsi \
    && python3 build.py --install -DVOLE_PSI_ENABLE_BOOST=ON \
    && cmake -S . \
    && cp /volepsi/volePSI/config.h /usr/local/include/volePSI/

# vole test
RUN cd volepsi/out/build/linux/frontend && ./frontend -messagePassing