FROM rockylinux:9

# Swap curl-minimal with full curl to avoid conflicts
RUN dnf -y swap curl-minimal curl

# Install development tools and dependencies
RUN dnf groupinstall -y "Development Tools" && \
    dnf install -y --allowerasing \
      autoconf \
      libtool \
      pkgconf-pkg-config \
      cmake \
      openssl-devel \
      git \
      curl \
      unzip 
RUN dnf install -y  perl-FindBin perl-IPC-Cmd 

RUN dnf clean all

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg && \
    cd /opt/vcpkg && \
    ./bootstrap-vcpkg.sh

ENV PATH="/opt/vcpkg:${PATH}"

