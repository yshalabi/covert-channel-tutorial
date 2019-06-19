FROM ubuntu:18.04

RUN \
    apt-get update && \
    apt-get -y upgrade && \
    apt-get install -y build-essential && \
    apt-get install -y gcc && \
    apt-get install -y cmake && \
    apt-get install -y bash && \
    apt-get install -y git

WORKDIR /isca19

COPY . /isca19

RUN mkdir build && cd build && cmake .. && make && ls

CMD sh isca19.sh
