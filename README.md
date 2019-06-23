# Overview
This repo contains a variety of software useful to those researching covert-channels.

# Building repo
## dockerized build
```sh
git clone https://github.com/yshalabi/covert-channel-tutorial.git && cd covert-channel-tutorial
docker build -t compile .
docker run -v `pwd`/cc:/isca19/bins compile
```

## cmake build
```sh
git clone https://github.com/yshalabi/covert-channel-tutorial.git && cd covert-channel-tutorial
mkdir build
cd build
cmake ..
make
```
# covert-channel implementations
## Flush+Reload
Running the flush+reload
```sh
./fr-send FILE
./fr-recv FILE
```

## L1D Prime+Probe
Running the l1d prime+probe
```sh
# Y,X are sibling threads
taskset -c Y ./pp-l1d-send
taskset -c X ./pp-l1d-recv
```

## LLC Prime+Probe

Running the LLC prime+probe (chat mode)
```sh
# X is a range of cores on the same socket
taskset -c X /pp-llc-send
taskset -c X /pp-llc-recv
```

To run the LLC prime+probe in benchmark mode:  
Specify option `-b` to both reader and sender and launch reader first.
OR run
```sh
./pp-llc-benchmark.py
```

# Sources
This main pieces of this repository are [three covert channel implementations](../master/extern). The implementations and their sources:
- [Synchronous, Shared-Memory, Flush+Reload Covert Channel](https://github.com/moehajj/Flush-Reload)
- [Synchronous Prime+Probe, Last-Level Cache](https://github.com/0x161e-swei/covert-channel-101)
- [Asynchronous Prime+Probe, L1 Data-Cache](https://github.com/yshalabi/covert-channel-toolkit)

