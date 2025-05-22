# Kafka-Connectors for VDMS

**All the sample files of these instructions are in the distributed folder in VDMS.**<br>

The folder contains the following files:
1. `docker-compose.yml` runs one kafka-broker (`broker:19092`), one zookeeper server (`2181`) and two VDMS servers (`localhost:55560`, `localhost:55561`).
If you are under proxy, use this command to run the file:
`docker-compose build --build-arg "HTTP_PROXY=$HTTP_PROXY" --build-arg "HTTPS_PROXY=$HTTPS_PROXY" --build-arg "http_proxy=$HTTP_PROXY" --build-arg "https_proxy=$HTTPS_PROXY"
`. If you are not behind a proxy, use: `docker-compose up -d` where `-d` launches it in detached mode.

2. `kafka_sender.h` and `kafka_receiver.h` are header files that configure the connection between application code, vdms, and Kafka. These have been configured to accept the largest message size to accept the blob size like images and descriptors.

3. `helper.h` is a header which contains the sample queries (meta_data, images, descriptors) to VDMS. Also, it contains the code that converts the query from string to proto message and vice versa.

4. Example applications:<br>To run the example that inserts metadata to a VDMS server via port 55561 (`kafka_test.cpp`), in build folder of VDMS, run the following: ```./distributed/meta_data```.<br><br> To run the example that simulate the behavior of multi-modal (sending blob image, descriptors and metadata) to the same VDMS server via port 55561 (`multi_modal.cpp`), run the following: ```./distributed/image_data```.<br><br> To run the example that inserts metadata for multiple topics to a VDMS server via port 55561 (`adaptive_platform.cpp`), run the following: ```./distributed/multi-modal```.

