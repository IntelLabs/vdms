# Backup and Restore VDMS

VDMS added the feature of backing up the DB folder using the following options.

These instructions assume you already install `tar 1.28` and `bsdtar` for copying sparse folders.

```bash
sudo apt-get install bsdtar
```

## Backup DB
### Setup in configuration file (config-vdms.json)
1. The variable `autoreplicate_interval` specifies the time interval the application wants to backup the DB folder. This value should be greater than 0.
2. The variable `unit` specifies the unit of the `autoreplicate_interval` variable. This value should be one of the following options: `h` (hour), `m` (minute), or `s` (seconds). We are planning to add the date option to trigger the back_up at a specified date.
3. The flag `backup_flag` specifies whether to use the auto-replication thread. By default, this value is false.
4. The variable `backup_path` specifies the path to store the backup DB. By default, if not specified, this value is the DB path (`db_root_path`).

#### Example
In this example, the backup will be triggered every three hours of the vdms-server life.
The default location of the backup DB is the db path that was specified in the vdms_config.json file. However,  this can be changed using the `backup_path` variable in the config file.
```json
autoreplicate_interval: 3
unit: "h"
```

### Naming of the backup DB
Currently, the code produces the following output:
1. `<filename>.tar.gz` file that represents the actual DB folder after compression. `<filename>` represents the datetime of the back_up (`Www Mmm dd hh:mm:ss yyyy`), without spaces, to show the version/history of the backup. For example, `WedMay1816:37:122022.tar.gz` means that the backup occurred on Wednesday, May 18, 2022 at 16:37:12. This filename will be used by the configuration file in order to help restore it when needed.
2. `<filename>.json` file that saves the configuration of the DB at the moment of backing it up. The `<filename>` is the same as the compressed backup file.


### Manual Archiving
To manually copy/archive a VDMS database ***only***, please see [Archive a VDMS Database](../Archive-a-VDMS-Database.md).
<br>


## Restoring DB
In order to restore a version named `WedMay1816:37:122022.tar.gz`, for example, apply the following steps:
1. Locate the backup file of interest, i.e. `<backup_path>/WedMay1816:37:122022.tar.gz`.
1. Use the `-restore` flag when launching VDMS to retrieve the configuration file of for the backup file, extract backup data, and mount it to VDMS.
```bash
./vdms -restore <backup_path>/WedMay1816:37:122022.tar.gz
```