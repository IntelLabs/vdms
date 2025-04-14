#!/bin/bash

helpFunction()
{
   echo ""
   echo "Usage: $0 -m machinetype -i install -s setup_the_node -k k8s_setup -c clear_the_node -j local_kubeConfig"
   echo "\t-m Input the machine type either 'remote' or 'master'"
   echo "\t-i Input the installation task as 'yes' or 'no'"
   echo "\t-s Input the status for the Node setup as - 'yes' or 'no'"
   echo "\t-k Input the status for the Kubernetes setup on Node as - 'yes' or 'no'"
   echo "\t-c Input to clear the node of the Kubernetes setup on Node as - 'yes' or 'no'"
   echo "\t-j Path to the master node kubeConfig.json"
   exit 1 # Exit script after printing help
}

jsonparserFunction()
{
   echo "Now parsing the file KubeConfig.json to get worker node info"
}
remoteSetupFunction()
{
   echo "Setup the docker images and registries will be created on the remote machine"
   sudo docker image load < remote_segment.tar
   sudo docker run -d -p 5000:5000 --name registry registry:2
   sudo docker tag rudf:latest  localhost:5000/remote-udf-1
   sudo docker push localhost:5000/remote-udf-1
}
remoteInstallFunction()
{
   echo "Dependency Installations will now be done on the remote machine"

   ##install containerd
   curl -L https://github.com/containerd/containerd/releases/download/v1.6.2/containerd-1.6.2-linux-amd64.tar.gz -o containerd-1.6.2-linux-amd64.tar.gz
   sudo tar Cxzvf /usr/local containerd-1.6.2-linux-amd64.tar.gz
   curl -L https://github.com/opencontainers/runc/releases/download/v1.1.3/runc.amd64 -o runc.amd64
   sudo install -m 755 runc.amd64 /usr/local/sbin/runc
   sudo mkdir -p /etc/containerd
   containerd config default | sudo tee /etc/containerd/config.toml
   sudo sed -i 's/SystemdCgroup \= false/SystemdCgroup \= true/g' /etc/containerd/config.toml
   sudo curl -L https://raw.githubusercontent.com/containerd/containerd/main/containerd.service -o /etc/systemd/system/containerd.service
   sudo systemctl daemon-reload
   sudo systemctl enable --now containerd

   #install docker engine
   # Add Docker's official GPG key:
   sudo apt-get update
   sudo apt-get install ca-certificates curl jq
   sudo install -m 0755 -d /etc/apt/keyrings
   sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
   sudo chmod a+r /etc/apt/keyrings/docker.asc

   # Add the repository to Apt sources:
   echo \
   "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
   $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
   sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
   sudo apt-get update

   sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
   sudo apt-get install conntrack
   ## install kubeadm, kubelet, kubectl
   CNI_PLUGINS_VERSION="v1.3.0"
   ARCH="amd64"
   DEST="/opt/cni/bin"
   sudo mkdir -p "$DEST"
   curl -L "https://github.com/containernetworking/plugins/releases/download/${CNI_PLUGINS_VERSION}/cni-plugins-linux-${ARCH}-${CNI_PLUGINS_VERSION}.tgz" | sudo tar -C "$DEST" -xz
   DOWNLOAD_DIR="/usr/local/bin"
   sudo mkdir -p "$DOWNLOAD_DIR"
   CRICTL_VERSION="v1.31.0"
   curl -L "https://github.com/kubernetes-sigs/cri-tools/releases/download/${CRICTL_VERSION}/crictl-${CRICTL_VERSION}-linux-${ARCH}.tar.gz" | sudo tar -C $DOWNLOAD_DIR -xz
   RELEASE="$(curl -sSL https://dl.k8s.io/release/stable.txt)"
   CDIR=$(pwd)
   cd $DOWNLOAD_DIR
   sudo curl -L --remote-name-all https://dl.k8s.io/release/${RELEASE}/bin/linux/${ARCH}/{kubeadm,kubelet}
   sudo chmod +x {kubeadm,kubelet}
   RELEASE_VERSION="v0.16.2"
   curl -sSL "https://raw.githubusercontent.com/kubernetes/release/${RELEASE_VERSION}/cmd/krel/templates/latest/kubelet/kubelet.service" | sed "s:/usr/bin:${DOWNLOAD_DIR}:g" | sudo tee /usr/lib/systemd/system/kubelet.service
   sudo mkdir -p /usr/lib/systemd/system/kubelet.service.d
   curl -sSL "https://raw.githubusercontent.com/kubernetes/release/${RELEASE_VERSION}/cmd/krel/templates/latest/kubeadm/10-kubeadm.conf" | sed "s:/usr/bin:${DOWNLOAD_DIR}:g" | sudo tee /usr/lib/systemd/system/kubelet.service.d/10-kubeadm.conf
   sudo systemctl enable --now kubelet
   cd $CDIR
}



masterInstallFunction()
{
   echo "Dependency Installation will now be done on the VDMS Master node"
   ##install containerd
   curl -L https://github.com/containerd/containerd/releases/download/v1.6.2/containerd-1.6.2-linux-amd64.tar.gz -o containerd-1.6.2-linux-amd64.tar.gz
   sudo tar Cxzvf /usr/local containerd-1.6.2-linux-amd64.tar.gz
   curl -L https://github.com/opencontainers/runc/releases/download/v1.1.3/runc.amd64 -o runc.amd64
   sudo install -m 755 runc.amd64 /usr/local/sbin/runc
   sudo mkdir /etc/containerd
   containerd config default | sudo tee /etc/containerd/config.toml
   sudo sed -i 's/SystemdCgroup \= false/SystemdCgroup \= true/g' /etc/containerd/config.toml
   sudo curl -L https://raw.githubusercontent.com/containerd/containerd/main/containerd.service -o /etc/systemd/system/containerd.service
   sudo systemctl daemon-reload
   sudo systemctl enable --now containerd

   #install docker engine
   # Add Docker's official GPG key:
   sudo apt-get update
   sudo apt-get install ca-certificates curl
   sudo install -m 0755 -d /etc/apt/keyrings
   sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
   sudo chmod a+r /etc/apt/keyrings/docker.asc

   # Add the repository to Apt sources:
   echo \
   "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
   $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
   sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
   sudo apt-get update

   sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
   sudo apt-get install conntrack

   ## install kubeadm, kubelet, kubectl
   CNI_PLUGINS_VERSION="v1.3.0"
   ARCH="amd64"
   DEST="/opt/cni/bin"
   sudo mkdir -p "$DEST"
   curl -L "https://github.com/containernetworking/plugins/releases/download/${CNI_PLUGINS_VERSION}/cni-plugins-linux-${ARCH}-${CNI_PLUGINS_VERSION}.tgz" | sudo tar -C "$DEST" -xz

   DOWNLOAD_DIR="/usr/local/bin"
   sudo mkdir -p "$DOWNLOAD_DIR"

   CRICTL_VERSION="v1.31.0"
   curl -L "https://github.com/kubernetes-sigs/cri-tools/releases/download/${CRICTL_VERSION}/crictl-${CRICTL_VERSION}-linux-${ARCH}.tar.gz" | sudo tar -C $DOWNLOAD_DIR -xz

   RELEASE="$(curl -sSL https://dl.k8s.io/release/stable.txt)"
   CDIR=$(pwd)
   cd $DOWNLOAD_DIR
   sudo curl -L --remote-name-all https://dl.k8s.io/release/${RELEASE}/bin/linux/${ARCH}/{kubeadm,kubelet}
   sudo chmod +x {kubeadm,kubelet}

   RELEASE_VERSION="v0.16.2"
   curl -sSL "https://raw.githubusercontent.com/kubernetes/release/${RELEASE_VERSION}/cmd/krel/templates/latest/kubelet/kubelet.service" | sed "s:/usr/bin:${DOWNLOAD_DIR}:g" | sudo tee /usr/lib/systemd/system/kubelet.service
   sudo mkdir -p /usr/lib/systemd/system/kubelet.service.d
   curl -sSL "https://raw.githubusercontent.com/kubernetes/release/${RELEASE_VERSION}/cmd/krel/templates/latest/kubeadm/10-kubeadm.conf" | sed "s:/usr/bin:${DOWNLOAD_DIR}:g" | sudo tee /usr/lib/systemd/system/kubelet.service.d/10-kubeadm.conf
   sudo systemctl enable --now kubelet

   #Install Cillium
   cd $CDIR
   CILIUM_CLI_VERSION=$(curl -s https://raw.githubusercontent.com/cilium/cilium-cli/main/stable.txt)
   CLI_ARCH=amd64
   if [ "$(uname -m)" = "aarch64" ]; then CLI_ARCH=arm64; fi
   curl -L --fail --remote-name-all https://github.com/cilium/cilium-cli/releases/download/${CILIUM_CLI_VERSION}/cilium-linux-${CLI_ARCH}.tar.gz{,.sha256sum}
   sha256sum --check cilium-linux-${CLI_ARCH}.tar.gz.sha256sum
   sudo tar xzvfC cilium-linux-${CLI_ARCH}.tar.gz /usr/local/bin
   rm cilium-linux-${CLI_ARCH}.tar.gz{,.sha256sum}

   sudo docker image load < vdms.tar
   sudo docker run -d -p 5000:5000 --name registry registry:2
   sudo docker tag vdms localhost:5000/vdms
   sudo docker push localhost:5000/vdms


}

masterSetupFunction()
{
   sudo kubeadm reset -f --cri-socket=unix:///var/run/cri-dockerd.sock
   sudo rm -rf $HOME/.kube
   sudo rm -rf /etc/cni/net.d
   sudo kubeadm init --cri-socket=unix:///var/run/cri-dockerd.sock

   mkdir -p $HOME/.kube
   export KUBECONFIG=$HOME/.kube/config
   sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
   sudo chown $(id -u):$(id -g) $HOME/.kube/config

   cilium install --version 1.16.0
}

jsonparserFunction_remote()
{
   json_data=$(cat $1)
   workers=$(echo $json_data | jq ".WorkerNodeDetail")
   num_workers=$(echo $workers | jq length)
   one=1
   count=$(($num_workers-$one))
   for i in $(seq 0 $count);
   do
      node=$(echo $workers | jq -r ".[$i]")
      dict_string="${node#\{}"
      dict_string="${dict_string%\}}"
      key=$(echo "$dict_string" | grep -o '[^:,]*:' | tr -d ':' | tr ',' '\n')
      value=$(echo "$dict_string" | grep -o ':[^:,]*' | tr -d ':' | tr ',' '\n')
      key=$(echo "$key" | sed 's/"//g')
      value=$(echo "$value" | sed 's/"//g')
      key=${key// /}
      value=${value// /}
      kubectl label --overwrite node ${key} nodeSelector=${key}
   done
}

jsonparserFunction_setup()
{
   json_data=$(cat $1)
   workers=$(echo $json_data | jq ".WorkerNodeDetail")
   num_workers=$(echo $workers | jq length)
   one=1
   count=$(($num_workers-$one))
   for i in $(seq 0 $count);
   do
      node=$(echo $workers | jq -r ".[$i]")
      dict_string="${node#\{}"
      dict_string="${dict_string%\}}"
      key=$(echo "$dict_string" | grep -o '[^:,]*:' | tr -d ':' | tr ',' '\n')
      value=$(echo "$dict_string" | grep -o ':[^:,]*' | tr -d ':' | tr ',' '\n')
      key=$(echo "$key" | sed 's/"//g')
      value=$(echo "$value" | sed 's/"//g')
      key=${key// /}
      value=${value// /}
      sudo -- sh -c -e "echo '${value}    ${key}' >> /etc/hosts";
   done
}

jsonparserFunction_master()
{
   json_data=$(cat $1)
   masternode=$(echo $json_data | jq ".MasterNodeDetail")
   dict_string="${masternode#\{}"
   dict_string="${dict_string%\}}"
   MASTER=$(echo "$dict_string" | grep -o '[^:,]*:' | tr -d ':' | tr ',' '\n')
   MASTER_IP=$(echo "$dict_string" | grep -o ':[^:,]*' | tr -d ':' | tr ',' '\n')
   MASTER=$(echo "$MASTER" | sed 's/"//g')
   MASTER_IP=$(echo "$MASTER_IP" | sed 's/"//g')
   MASTER=${MASTER// /}
   MASTER_IP=${MASTER_IP// /}
}

masterVDMSk8setupFunction()
{
   echo "Setup the VDMS on the master node and generate the keys"
   ## use the json parser here
   jsonparserFunction_master $1
   kubectl label node ${MASTER} vdmstype=vdmsmaster
   kubectl create clusterrolebinding serviceaccounts-cluster-admin \
   --clusterrole=cluster-admin \
   --group=system:serviceaccounts
   kubectl create configmap node-map --from-file=kubeConfig.json
   kubectl taint node ${MASTER} node-role.kubernetes.io/control-plane:NoSchedule-
   kubectl apply -f vdms-config.yaml
   kubectl apply -f service-config.yaml
   jsonparserFunction_remote $1
}



OPTSTRING=":m:i:s:k:c:j:p"

while getopts ${OPTSTRING} opt; do
  case ${opt} in
    m)
      echo "The type of machine is - ${OPTARG}"
      machinetype="$OPTARG"
      machinetype=${machinetype// /}
      ;;
    i)
      install_arg="$OPTARG"
      echo "Are we going to install the dependencies - ${OPTARG}"
      install_arg=${install_arg// /}
      ;;
    s)
      setup_arg="$OPTARG"
      echo "Do we setup the  $machinetype ? - ${OPTARG}"
      setup_arg=${setup_arg// /}
      ;;
    k)
      k8s_setup_arg="$OPTARG"
      echo "Do we configure the k8s cluster on $machinetype ? - ${OPTARG}"
      k8s_setup_arg=${k8s_setup_arg// /}
      ;;
    c)
      clean_up="$OPTARG"
      clean_up=${clean_up// /}
      echo "Do we clean up the k8s cluster? - ${OPTARG}"
      ;;
    j)
      config_path="$OPTARG"
      config_path=${config_path// /}
      echo "Path to the kubeConfig.json - ${OPTARG}"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      usage
      ;;
  esac
done

if [ -z "$machinetype" ];
then
   echo "Please mention the type of machine - either remote or master";
   helpFunction
fi

if [ "$install_arg" == "yes" ]; then
   if [ "$machinetype" == "remote" ]; then
      echo "Installing Dependencies on the remote Node"
      remoteInstallFunction
   fi
   if [ "$machinetype" == "master" ]; then
      echo "Installing Dependencies on the master Node"
      masterInstallFunction
   fi
fi

if [ "$setup_arg" == "yes" ]; then
   if [ "$machinetype" == "remote" ]; then
      echo "setup the remote Node"
      remoteSetupFunction
   fi
   if [ "$machinetype" == "master" ]; then
      echo "setup the master Node"
      masterSetupFunction
      jsonparserFunction_setup $config_path
      echo "sudo $(kubeadm token create --print-join-command)" > join_vdms_cluster.sh
   fi
fi

if [ "$k8s_setup_arg" == "yes" ]; then
   if [ "$machinetype" == "remote" ]; then
      echo "setup the k8s on remote Node"
      chmod +x join_vdms_cluster.sh
      ./join_vdms_cluster.sh
   fi
   if [ "$machinetype" == "master" ]; then
      echo "setup the k8s on master Node"
      masterVDMSk8setupFunction $config_path
   fi
fi

if [ "$clean_up" == "yes" ]; then
   sudo kubeadm reset -f
   if [ "$machinetype" == "master" ]; then
      sudo rm -rf $HOME/.kube
      sudo rm -f join_vdms_cluster.sh
   else
      sudo rm -f join_vdms_cluster.sh
   fi
fi
