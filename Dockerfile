FROM ubuntu:18.04

RUN \
    apt-get update && \
    apt-get -y upgrade && \
    apt-get install -y build-essential && \
    apt-get install -y gcc && \
    apt-get install -y cmake && \
    apt-get install -y bash && \
    apt-get install -y git

# Set environment variables.
#ENV HOME /root

WORKDIR /isca19

# Copy the current directory contents into the container at /app
COPY . /isca19

RUN cmake . && make && ls

# Define default command.
CMD ["bash", "isca19.sh"]
