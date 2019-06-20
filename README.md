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
The covert-channel implementaions are sourced from the following repositories. You can examine the sources in  the "extern" subdirectory.
- repo1
- repo2
- repo3
