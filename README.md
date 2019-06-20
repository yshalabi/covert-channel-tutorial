# Overview
This repo contains a variety of software useful to those researching covert-channels.

# Building repo
## dockerized build
```sh
git clone https://github.com/yshalabi/isca19-cc-handson.git && cd isca19-cc-handson
docker build -t compile .
docker run -v `pwd`/cc:/isca19/bins compile
```

## cmake build
```sh
git clone https://github.com/yshalabi/isca19-cc-handson.git && cd isca19-cc-handson
mkdir build
cd build
cmake ..
make
```
# covert-channel implementations
## Flush+Reload (Link to implementation)
```sh
./fr-send FILE
./fr-recv FILE
```

## L1D Prime+Probe (Link to implementation)
Running the l1d prime+probe
```sh
# Y,X are sibling threads
taskset -c Y ./pp-l1d-send
taskset -c X ./pp-l1d-recv
```

Running the LLC prime+probe;
```sh
./pp-llc-send
./pp-llc-recv
```

## LLC Prime+Probe (Link to implementation)

# Sources
This main pieces of this repository are [three covert channel implementations](../blob/master/extern). The implementations and their sources:
- [Synchronous, Shared-Memory, Flush+Reload Covert Channel](https://github.com/moehajj/Flush-Reload)
- [Synchronous Prime+Probe, Last-Level Cache](https://github.com/0x161e-swei/covert-channel-101)
- [Asynchronous Prime+Probe, L1 Data-Cache](https://github.com/yshalabi/cc-fun)
