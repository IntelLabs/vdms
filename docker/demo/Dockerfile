# Pull base image.
FROM intellabs/vdms:base

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
    python3-pip \
    python3-dev && \
    rm -rf /var/lib/apt/lists/* /root/.cache && \
    pip3 install --upgrade pip

RUN pip3 install numpy && \
    pip3 install setuptools && \
    pip3 install notebook && \
    pip3 install vdms

RUN pip3 install opencv-python && \
    pip3 install face_recognition

RUN jupyter notebook --generate-config && \
    echo "c.NotebookApp.password='sha1:d12a2dd64552:86359144939d58019e6e5b37e373a50426ad7b2b' ">> /root/.jupyter/jupyter_notebook_config.py

# Decompress Demo Database and Sample Files
RUN cd /vdms && wget -q remis.io/share/demo.tgz
COPY examples /vdms/examples

# Setup entry point
RUN echo '#!/bin/bash' > /start.sh && \
    echo 'cd /vdms' >> /start.sh && \
    echo 'tar xzf demo.tgz' >> /start.sh && \
    echo 'vdms &' >> /start.sh && \
    echo 'cd examples' >> /start.sh && \
    echo 'jupyter notebook --port=8888 --no-browser --allow-root --ip=0.0.0.0' >> /start.sh && \
    chmod 755 /start.sh

# Add Tini. Tini operates as a process subreaper for jupyter.
# This prevents kernel crashes.
ENV TINI_VERSION v0.6.0
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/bin/tini
RUN chmod +x /usr/bin/tini
ENTRYPOINT ["/usr/bin/tini", "--"]

EXPOSE 8888
EXPOSE 55555
CMD ["/start.sh"]
