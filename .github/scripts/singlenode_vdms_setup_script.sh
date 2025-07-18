#!/bin/bash

set -e

helpFunction()
{
   echo ""
   # echo "Usage: $0 -m machinetype -i install -s setup_the_node -k k8s_setup -c clear_the_node -j local_kubeConfig"
   echo "Usage: $0 -i install -s setup_the_node -k k8s_setup -j local_kubeConfig"
   # echo "\t-m Input the machine type either 'remote' or 'primary' for worker or control plane nodes, respectively"
   echo "\t-i Input the installation task as 'yes' or 'no'"
   echo "\t-s Input the status for the Node setup as - 'yes' or 'no'"
   echo "\t-k Input the status for the Kubernetes setup on Node as - 'yes' or 'no'"
   # echo "\t-c Input to clear the node of the Kubernetes setup on Node as - 'yes' or 'no'"
   echo "\t-j Path to the control plane (primary) node kubeConfig.json"
   exit 1 # Exit script after printing help
}

jsonparserFunction()
{
   echo "Now parsing the file KubeConfig.json to get worker node info"
}

remoteSetupFunction()
{
   json_data=`cat ../../kubernetes/installConfig.json`
   rudf_tar=$(echo $json_data | jq -r ".remote_udf_tar")
   echo "Setup the docker images and registries will be created on the remote machine"
   sudo docker image load < ../../kubernetes/$rudf_tar
   #sudo docker run -d -p 5000:5000 --name registry registry:2
   sudo docker tag rudf:latest  localhost:5000/remote-udf-1
   sudo docker push localhost:5000/remote-udf-1
}

controlplaneInstallFunction()
{
   echo "Dependency Installation will now be done on the VDMS Control Plane (primary) node"

   sudo apt-get update
   sudo apt-get install ca-certificates curl jq

   json_data=`cat ../../kubernetes/installConfig.json`

   ARCH=$(echo $json_data | jq -r ".ARCH")
   DOWNLOAD_DIR=$(echo $json_data | jq -r ".DOWNLOAD_DIR")

   #install minikube
   if ! command -v minikube &> /dev/null; then
      curl -LO https://github.com/kubernetes/minikube/releases/latest/download/minikube-linux-amd64
      sudo install minikube-linux-amd64 /usr/local/bin/minikube && rm minikube-linux-amd64
   else
      echo "Minikube is already installed."
   fi

   #install docker engine
   if ! command -v docker &> /dev/null; then
      # Add Docker's official GPG key:
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
   else
      echo "Docker is already installed."
   fi
   sudo apt-get install conntrack

   ## install kubectl
   if ! command -v kubectl &> /dev/null; then
      RELEASE="$(curl -sSL https://dl.k8s.io/release/stable.txt)"
      curl -LO "https://dl.k8s.io/release/${RELEASE}/bin/linux/${ARCH}/kubectl"
      sudo install -o root -g root -m 0755 kubectl ${DOWNLOAD_DIR}/kubectl
   else
      echo "Kubectl is already installed."
   fi

   vdms_tar=$(echo $json_data | jq -r ".vdms_tar")
   sudo docker image load < ../../kubernetes/$vdms_tar
   sudo docker run -d -p 5000:5000 --name registry registry:2
   sudo docker tag vdms localhost:5000/vdms
   sudo docker push localhost:5000/vdms


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
      # kubectl label --overwrite node ${key} nodeSelector=${key}
      kubectl label --overwrite node vdms-m02 nodeSelector=vdms-m02
   done
}

jsonparserFunction_controlplane()
{
   json_data=$(cat $1)
   controlplanenode=$(echo $json_data | jq ".ControlPlaneNodeDetail")
   dict_string="${controlplanenode#\{}"
   dict_string="${dict_string%\}}"
   CONTROLPLANE=$(echo "$dict_string" | grep -o '[^:,]*:' | tr -d ':' | tr ',' '\n')
   CONTROLPLANE_IP=$(echo "$dict_string" | grep -o ':[^:,]*' | tr -d ':' | tr ',' '\n')
   CONTROLPLANE=$(echo "$CONTROLPLANE" | sed 's/"//g')
   CONTROLPLANE_IP=$(echo "$CONTROLPLANE_IP" | sed 's/"//g')
   CONTROLPLANE=${CONTROLPLANE// /}
   CONTROLPLANE_IP=${CONTROLPLANE_IP// /}
}

controlplaneVDMSk8setupFunction()
{
   echo "Setup the VDMS on the control plane node and generate the keys"
   json_data=`cat ../../kubernetes/installConfig.json`
   configmap=$(echo $json_data | jq ".configmap")
   ## use the json parser here
   jsonparserFunction_controlplane $1

   # start cluster
   minikube start --nodes 2 -p vdms
   kubectl label node ${CONTROLPLANE} vdmstype=vdmscontrolplane
   # kubectl label node vdms vdmstype=vdmscontrolplane
   kubectl label node ${CONTROLPLANE}-m02 node-role.kubernetes.io/worker=worker
   kubectl create clusterrolebinding serviceaccounts-cluster-admin \
   --clusterrole=cluster-admin \
   --group=system:serviceaccounts
   kubectl create configmap node-map --from-file=kubeConfig.json
   # kubectl taint node ${CONTROLPLANE} node-role.kubernetes.io/control-plane:NoSchedule-
   # kubectl taint node vdms node-role.kubernetes.io/control-plane:NoSchedule-
   kubectl apply -f ../../kubernetes/vdms-config.yaml
   kubectl apply -f ../../kubernetes/service-config.yaml
   jsonparserFunction_remote $1
}



OPTSTRING=":i:s:k:j:p"

while getopts ${OPTSTRING} opt; do
  case ${opt} in
   #  m)
   #    echo "The type of machine is - ${OPTARG}"
   #    machinetype="$OPTARG"
   #    machinetype=${machinetype// /}
   #    ;;
    i)
      install_arg="$OPTARG"
      # echo "Are we going to install the dependencies - ${OPTARG}"
      install_arg=${install_arg// /}
      ;;
    s)
      setup_arg="$OPTARG"
      # echo "Do we setup the  $machinetype ? - ${OPTARG}"
      setup_arg=${setup_arg// /}
      ;;
    k)
      k8s_setup_arg="$OPTARG"
      # echo "Do we configure the k8s cluster on $machinetype ? - ${OPTARG}"
      k8s_setup_arg=${k8s_setup_arg// /}
      ;;
   #  c)
   #    clean_up="$OPTARG"
   #    clean_up=${clean_up// /}
   #    echo "Do we clean up the k8s cluster? - ${OPTARG}"
   #    ;;
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

# if [ -z "$machinetype" ];
# then
#    echo "Please mention the type of machine - either remote or primary";
#    helpFunction
# fi

if [ "$install_arg" == "yes" ]; then
   echo "Installing Dependencies on the Control Plane (Primary) Node"
   controlplaneInstallFunction
fi

if [ "$setup_arg" == "yes" ]; then
   echo "Setup the Remote Node"
   remoteSetupFunction
fi

if [ "$k8s_setup_arg" == "yes" ]; then
   echo "Setup the k8s"
   controlplaneVDMSk8setupFunction $config_path
fi
