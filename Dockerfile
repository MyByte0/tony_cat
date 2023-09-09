FROM docker.io/debian:12.1

#COPY ./ ./src_code
WORKDIR .

# for debain cn sources, use: 
RUN sed -i 's/deb.debian.org/mirrors.ustc.edu.cn/g' /etc/apt/sources.list.d/debian.sources

RUN apt-get update
RUN apt install -y apt-utils
RUN apt install -y gcc-12 build-essential
RUN apt install -y git cmake python3 python3-pip python3-openpyxl
RUN apt install -y libasio-dev libfmt-dev
RUN apt install -y libprotobuf-dev protobuf-compiler
RUN apt install -y default-libmysqlclient-dev
RUN apt install -y librocksdb-dev

#
RUN git clone -b 9.0.0 https://github.com/leethomason/tinyxml2.git && \
    cd tinyxml2 && cmake . && make && make install && cd .. && \
    rm -rf tinyxml2


RUN git clone -b v0.6.0 https://github.com/google/glog.git && \
    cd glog && cmake . && make && make install && cd .. && \
    rm -rf glog
