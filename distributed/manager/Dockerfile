FROM ubuntu:20.04
RUN apt-get update && apt-get install -y python3 python3-pip
COPY ./worker_manager.py .
ENTRYPOINT ["tail", "-f"]