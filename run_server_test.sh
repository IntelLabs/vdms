#! /bin/bash

PMGD_PATH="/home/ragaad/vdms-dependenices/pmgd/tools/"

rm log.log
rm -r /opt/retail/vdms/$1
mkdir /opt/retail/vdms/$1
mkdir /opt/retail/vdms/$1/images
mkdir /opt/retail/vdms/$1/images/pngs
mkdir /opt/retail/vdms/$1/images/jpgs
mkdir /opt/retail/vdms/$1/videos

# mkgraph <graph name> <node/edge> <tag/class in VDMS case> <property key> <property type>
$PMGD_PATH/mkgraph /opt/retail/vdms/$1/graph node "Person" "Id" string node "Area" "Name" string node "Visit" "Id" string node "Visit" "starting_time" time  node "Visit" "ending_time" time

CONFIG=$1"_config.json"

echo "// VDMS Config File
// This is the run-time config file
// Sets database paths and other parameters
{
  // Network
  \"port\": 55555, // Default is 55555
  \"max_simultaneous_clients\": 20, // Default is 500

  // Database paths
  \"pmgd_path\": \"/opt/retail/vdms/$1/graph\", // This will be an IP address in the future
  \"png_path\": \"/opt/retail/vdms/$1/images/pngs/\",
  \"jpg_path\": \"/opt/retail/vdms/$1/images/jpgs/\",
  \"tdb_path\": \"/opt/retail/vdms/$1/images/tiledb/tdb/\",
 \"video_path\": \"/opt/retail/vdms/$1/videos/\",

  \"support_info\": \"a-team@intel.com\",
  \"support_phone\": \"1-800-A-TEAM\"
}" > $CONFIG

./vdms -cfg $CONFIG 2> log.log
