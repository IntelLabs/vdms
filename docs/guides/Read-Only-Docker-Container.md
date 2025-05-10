## Read-Only Docker Image

To run VDMS in a read-only container, first create a volume \
$ docker volume create vdms_db \
Now create a VDMS container that uses this volume as the storage location for db's \
$ docker run -d -p 55555:55555 --read-only --mount source=vdms_db,destination=/vdms/build/db vdms:latest \
