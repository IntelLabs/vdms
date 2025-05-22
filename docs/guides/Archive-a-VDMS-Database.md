# Archive a VDMS Database

Since VDMS uses PMGD (which uses mmap to a large virtual memory space but maps physical pages
only when needed) the size on disk shown for the database by some utilities can be quite large.
Similarly, copying a database can take a long time, so we use a different method of copying/archiving a VDMS database.

Assume a database called db at `/home/user/vdms/`:
```bash
sudo apt-get install bsdtar     # this is used for doing the sparse copy
cd /home/user/vdms/
bsdtar cvfz vdms_archive.tar.gz db
tar -xvSf vdms_archive.tar.gz
```


Tested on Ubuntu 16.04: [more info here](https://stackoverflow.com/questions/13252682/copying-a-1tb-sparse-file)
