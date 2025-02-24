FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y build-essential cmake g++-multilib libgcc-11-dev lib32gcc-11-dev ccache

WORKDIR /wamr/product-mini/platforms/armA7