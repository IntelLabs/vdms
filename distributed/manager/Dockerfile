FROM ubuntu:20.04
RUN apt-get update && apt-get install -y python3 python3-pip && pip3 install vdms
COPY ./worker_manager.py .
ENTRYPOINT ["tail", "-f"]