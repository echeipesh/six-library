
FROM --platform=linux/amd64 ubuntu:22.04

RUN apt-get update \
    && apt-get install \
    build-essential -y \
    cmake \
    git \
    curl \
    file \
    vim \
    pip

RUN pip install nump

  # Build and install the python bindings
#  COPY . /six-library
#  WORKDIR /six-library
#   RUN /usr/bin/python3 waf configure --prefix=install \
#     && /usr/bin/python3 waf install \
#     && /usr/bin/python3 waf makewheel \
#     && pip install install/bin/pysix-*.whl
