FROM debian:stretch
LABEL maintainer="Reaction Developers <dev@reaction.network>"
LABEL description="Dockerised Reaction, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
