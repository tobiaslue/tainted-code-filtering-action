# Container image that runs your code
FROM mcopik/clang-dfsan:dfsan-9.0

# Copies your code file from your action repository to the filesystem path `/` of the container
COPY entrypoint.sh /entrypoint.sh
COPY lib /lib
COPY bin/clang++1 /opt/llvm/bin

RUN deps='cmake build-essential xz-utils curl git'\
    && sudo apt-get -y update\
    && sudo apt-get install -y ${deps}\
    && curl -SL http://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz | tar -xJC .\
    && mv clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04 clang_9.0.0\
    && sudo mv clang_9.0.0 /usr/local
    
RUN git clone https://github.com/nlohmann/json.git\
    && sudo mv json /usr/local\
    && cd /usr/local/json\
    && cmake .

RUN chmod +x /opt/llvm/bin/clang++1
ENV PATH=/opt/llvm/bin/:$PATH

# Code file to execute when the docker container starts up (`entrypoint.sh`)
ENTRYPOINT ["/entrypoint.sh"]
