# Build instructions
## Dockerized
```sh
git clone https://github.com/yshalabi/isca19-cc-handson.git && cd isca19-cc-handson
docker build --tag=cc .
docker run cc
```

## Manual
```sh
git clone https://github.com/yshalabi/isca19-cc-handson.git && cd isca19-cc-handson
mkdir build
cd build
cmake ..
make
```
# covert-channels
## Flush-and-Reload
```sh
./fr-send FILE
./fr-recv FILE
```

## Prime-and-probe
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

# Sources
The covert-channel implementaions are sourced from the following repositories. You can examine the sources in  the"extern" directory.
- repo1
- repo2
- repo3
