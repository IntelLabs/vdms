#This script copies data from ucluster locally to create a sample of the dataset

#local_dest=$1
#if [ -z "$local_dest" ]
#    then
#    echo "You must specify local destination as first param!"
#    exit 1
#fi

# mkdir ../data
# This is the entire dataset, will take a while
#rsync -r forest-app.jf.intel.com:/srv/data/yfcc100m/metadata/original ../data
rsync -r forest-app.jf.intel.com:/srv/data/yfcc100m/metadata/extras ../data
rsync -r forest-app.jf.intel.com:/srv/data/yfcc100m/metadata/yfcc100m_short ../data

#bzip2 -dk ../data/original/yfcc100m_dataset.bz2 &
#bzip2 -dk ../data/original/yfcc100m_autotags.bz2
