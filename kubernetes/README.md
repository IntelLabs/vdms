# Prerequisites:

Use the following steps to create the VDMS tar file.

+ Change to the docker/base/ directory
+ Follow the README to generate the VDMS docker image
+ Run the following command to create the tar file `sudo docker save -o vdms.tar vdms`

Use the following steps to create the remote UDF tar file.

+ Change to the remote_function directory
+ Follow the README to generate the remote UDF docker image
+ Run the following command to create the tar file `sudo docker save -o remote_segment.tar rudf`

# Configure kubeConfig.json #
Sample kubeConfig file that can be used to add details of Master and Worker nodes.

```json
{
    "MasterNodeDetail": {
		"_HOST-NAME-OF-MASTER-NODE_": "_IPADDRESS-OF-MASTER-NODE_"
	},
    "WorkerNodeDetail": [
		{"_HOST-NAME-OF-WORKER-NODE_1": "_IPADDRESS-OF-WORKER-NODE_1"},
		{"_HOST-NAME-OF-WORKER-NODE_2" : "_IPADDRESS-OF-WORKER-NODE_2"},
		{"_use-similar-blocks-to-add-more-node_"}
	]
}
```

# Proxy setting for running containerd behind a proxy #

Follow the steps below for containerd

```bash
    sudo mkdir -p /etc/systemd/system/containerd.service.d
    sudo touch /etc/systemd/system/containerd.service.d/http-proxy.conf
    sudo nano /etc/systemd/system/containerd.service.d/http-proxy.conf
```

Edit the http-proxy.conf as below, add the proxy details as per your system for containerd

```bash
    [Service]
    Environment="HTTP_PROXY=http://proxy.example.com"
    Environment="HTTPS_PROXY=http://proxy.example.com"
    Environment="NO_PROXY=localhost"
```

Restart the services as mentioned below

```bash
    sudo systemctl daemon-reload
    sudo systemctl restart containerd
```

Follow the steps below for Docker

```bash
    sudo mkdir -p /etc/systemd/system/docker.service.d
    sudo touch /etc/systemd/system/docker.service.d/http-proxy.conf
    sudo nano /etc/systemd/system/docker.service.d/http-proxy.conf
```

Edit the http-proxy.conf as below, add the proxy details as per your system for containerd

```bash
    [Service]
    Environment="HTTP_PROXY=http://proxy.example.com"
    Environment="HTTPS_PROXY=http://proxy.example.com"
    Environment="NO_PROXY=localhost"
```

Restart the services as mentioned below

```bash
    sudo systemctl daemon-reload
    sudo systemctl restart docker
```
# Bringing up your cluster to run Multi-node Cluster for VDMS application #

Clone the VDMS github repository on the Master and Worker nodes.

On the Master node follow the steps below after downloading the VDMS image -
```bash
    cd kubernetes/
    chmod +x global_vdms_setup_script.sh
    ./global_vdms_setup_script.sh -m master -i yes
```
On the Worker Node follow the steps below after downloading/creating the remote UDF -

```bash
    cd kubernetes/
    chmod +x global_vdms_setup_script.sh
    ./global_vdms_setup_script.sh -m remote -i yes
```

Now update the kubeConfig.json file to add the Master and Worker Node details as per steps provided in first section

On the Worker Node follow the steps below to load the remote UDF image locally

```bash
    ./global_vdms_setup_script.sh -m remote -s yes
```

## Setting up the Multinode Cluster and running VDMS Application ##

On the Master Node execute the following command
```bash
    ./global_vdms_setup_script.sh -m master -s yes -j <path to kubeConfig.json>
```

The file named join_vdms_cluster.sh will be created in <mark>kubernetes/</mark> folder, copy/transfer that to the <mark>kubernetes/</mark> folder at the Worker nodes

On the Worker Node execute the following command
```bash
    ./global_vdms_setup_script.sh -m remote -k yes
```

Final step, On the Master Node execute the following command
```bash
    ./global_vdms_setup_script.sh -m master -k yes -j <path to kubeConfig.json>
```

Use ipconfig/ip addr to get the IP address of the Control plane.