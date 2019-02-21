# Pull base image.
FROM intellabs/vdms:base

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    python3-pip

RUN pip3 install --upgrade pip
RUN pip3 install opencv-python
RUN pip3 install face_recognition
RUN pip3 install numpy
RUN pip3 install notebook
RUN pip3 install vdms

RUN jupyter notebook --generate-config
RUN echo "c.NotebookApp.password='sha1:d12a2dd64552:86359144939d58019e6e5b37e373a50426ad7b2b' ">> /root/.jupyter/jupyter_notebook_config.py

# Decompress Demo Database and Sample Files
RUN cd /vdms && wget -q remis.io/share/demo.tgz
COPY examples /vdms/examples

# Setup entry point

RUN echo '#!/bin/bash' > /start.sh
RUN echo 'export LD_LIBRARY_PATH=/vdms/utils:/pmgd/lib:$(find /usr/local/lib/ / -type f -name "*.so" | xargs dirname | sort | uniq | tr "\n" ":")' >> /start.sh
RUN echo 'cd /vdms' >> /start.sh
RUN echo 'rm -rf db' >> /start.sh
RUN echo 'tar xzf demo.tgz' >> /start.sh
RUN echo './vdms &' >> /start.sh
RUN echo 'cd examples' >> /start.sh
RUN chmod 755 /start.sh
RUN echo 'jupyter notebook --port=8888 --no-browser --allow-root --ip=0.0.0.0' >> /start.sh
RUN chmod 755 /start.sh

# Add Tini. Tini operates as a process subreaper for jupyter.
# This prevents kernel crashes.
ENV TINI_VERSION v0.6.0
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/bin/tini
RUN chmod +x /usr/bin/tini
ENTRYPOINT ["/usr/bin/tini", "--"]

EXPOSE 8888
EXPOSE 55555
CMD ["/start.sh"]
