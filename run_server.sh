rm log.log
rm -r db

mkdir db
/home/luisremi/vcs/pmgd/tools/mkgraph db/graph \
        node VD:VID name string
./vdms 2> log.log

# If problems with initialization, try deleting db folder.
