# Pull base image.
FROM intellabs/vdms:base

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends python3-pip && \
    pip3 install --upgrade pip && \
    rm -rf /var/lib/apt/lists/* /root/.cache

RUN pip3 install numpy vdms

# VDMS/PMGD install
RUN git clone https://github.com/intellabs/vdms/ vdms_install && \
    cd vdms_install && git checkout develop && \
    git clone https://github.com/intellabs/pmgd/ && \
    scons -j16

RUN rm /usr/bin/python && \
    ln -s /usr/bin/python3 /usr/bin/python

# Setup entry point
RUN echo '#!/bin/bash' > /start.sh && \
    echo 'cd /vdms_install/tests'   >> /start.sh && \
    echo 'sh run_tests.sh'          >> /start.sh && \
    chmod 755 /start.sh

EXPOSE 55555
CMD ["/start.sh"]
