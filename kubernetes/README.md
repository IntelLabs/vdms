# Prerequisites:

### Setup Installation Config
Create/Modify the `installConfig.json` file with necessary values. It should have the following details. Use the below values if not sure what to update. The installation config should be in the same parent directory as the `global_vdms_setup_script.sh` file.

```json
{
    "cri_socket": "unix:///var/run/containerd/containerd.sock",
    "remote_udf_tar": "remote_segment.tar",
    "vdms_tar": "vdms.tar",
    "CONTAINERD_VERSION": "1.6.2",
    "RUNC_VERSION": "1.1.3",
    "CNI_PLUGINS_VERSION": "v1.3.0",
    "ARCH": "amd64",
    "CLI_ARCH": "amd64",
    "DEST": "/opt/cni/bin",
    "DOWNLOAD_DIR": "/usr/local/bin",
    "CRICTL_VERSION": "v1.31.0",
    "RELEASE_VERSION": "v0.16.2",
    "CILIUM_VERSION": "1.16.0",
    "configmap": "kubeConfig.json"
}
```


### Setup VDMS tar file
Use the following steps to create the VDMS tar file.

+ Change to the docker/base/ directory
+ Follow the README to generate the VDMS docker image
+ Run the following command to create the tar file `sudo docker save -o vdms.tar vdms`
+ Update the `vdms_tar` entry in `installConfig.json` if using a different tar file name.


### Setup Remote UDF tar file
Use the following steps to create the remote UDF tar file.

+ Change to the remote_function directory
+ Follow the README to generate the remote UDF docker image
+ Run the following command to create the tar file `sudo docker save -o remote_segment.tar rudf`
+ Update the `remote_udf_tar` entry in `installConfig.json` if using a different tar file name.

# Configure kubeConfig.json
Sample kubeConfig file that can be used to add details for the Control Plane (primary) and Worker nodes, respectively.

```json
{
    "ControlPlaneNodeDetail": {
		"_HOST-NAME-OF-PRIMARY-NODE_": "_IPADDRESS-OF-PRIMARY-NODE_"
	},
    "WorkerNodeDetail": [
		{"_HOST-NAME-OF-WORKER-NODE_1": "_IPADDRESS-OF-WORKER-NODE_1"},
		{"_HOST-NAME-OF-WORKER-NODE_2" : "_IPADDRESS-OF-WORKER-NODE_2"},
		{"_use-similar-blocks-to-add-more-node_"}
	]
}
```

# Proxy setting for running containerd behind a proxy

Follow the steps below for containerd:
```bash
sudo mkdir -p /etc/systemd/system/containerd.service.d
sudo touch /etc/systemd/system/containerd.service.d/http-proxy.conf
sudo nano /etc/systemd/system/containerd.service.d/http-proxy.conf
```

Edit the http-proxy.conf as below, add the proxy details as per your system for containerd:
```bash
[Service]
Environment="HTTP_PROXY=http://proxy.example.com"
Environment="HTTPS_PROXY=http://proxy.example.com"
Environment="NO_PROXY=localhost"
```

Restart the services as mentioned below:
```bash
sudo systemctl daemon-reload
sudo systemctl restart containerd
```

Follow the steps below for Docker:
```bash
sudo mkdir -p /etc/systemd/system/docker.service.d
sudo touch /etc/systemd/system/docker.service.d/http-proxy.conf
sudo nano /etc/systemd/system/docker.service.d/http-proxy.conf
```

Edit the http-proxy.conf as below, add the proxy details as per your system for containerd:
```bash
[Service]
Environment="HTTP_PROXY=http://proxy.example.com"
Environment="HTTPS_PROXY=http://proxy.example.com"
Environment="NO_PROXY=localhost"
```

Restart the services as mentioned below:
```bash
sudo systemctl daemon-reload
sudo systemctl restart docker
```


# Bringing up your cluster to run Multi-node Cluster for VDMS application #

Clone the VDMS github repository on the Primary and Worker nodes.

On the Control Plane (Primary) node follow the steps below after downloading the VDMS image:
```bash
cd kubernetes/
chmod +x global_vdms_setup_script.sh
./global_vdms_setup_script.sh -m primary -i yes
```

On the Worker Node follow the steps below after downloading/creating the remote UDF:
```bash
cd kubernetes/
chmod +x global_vdms_setup_script.sh
./global_vdms_setup_script.sh -m remote -i yes
```

Now update the kubeConfig.json file to add the Control Plane and Worker Node details as per steps provided in first section

On the Worker Node follow the steps below to load the remote UDF image locally

```bash
./global_vdms_setup_script.sh -m remote -s yes
```


## Setting up the Multinode Cluster and running VDMS Application ##

On the Control Plane (Primary) Node execute the following command
```bash
./global_vdms_setup_script.sh -m primary -s yes -j <path to kubeConfig.json>
```

The file named `join_vdms_cluster.sh` will be created in <mark>kubernetes/</mark> folder, copy/transfer that to the <mark>kubernetes/</mark> folder at the Worker nodes. If you see a `cri-socket error` add the `cri-socket` argument at the end of the `kubeadm join` command in the `join_vdms_cluster.sh` file. Modify the cri-socket value to be same as the installation config.

```bash
--cri-socket=unix:///var/run/containerd/containerd.sock
```

On the Worker Node execute the following command
```bash
./global_vdms_setup_script.sh -m remote -k yes
```

Final step, On the Control Plane (Primary) Node execute the following command
```bash
./global_vdms_setup_script.sh -m primary -k yes -j <path to kubeConfig.json>
```

Use ipconfig/ip addr to get the IP address of the Control plane.
