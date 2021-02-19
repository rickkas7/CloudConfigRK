# CloudConfigRK

Store configuration settings locally with the ability to set from the cloud


## Storage Methods

The storage methods provide a way to store the data locally so it's available immediately after restart, and to avoid having to get the data from the cloud as frequently. You can set an adjustable refresh period, if desired, or only fetch once.

### Retained Memory

Retained memory is preserved:

- Across reboot
- Across sleep modes including HIBERNATE

It is not preserved:

- When removing power (including use the Gen 3 EN pin)
- Most code flashes
- Most Device OS upgrades

There is around 3K of retained memory on most devices.

### EEPROM

Emulated EEPROM is a good choice because it's preserved:

- Across reboot
- Across sleep modes including HIBERNATE
- When removing power (including use the Gen 3 EN pin)
- Most code flashes
- Most Device OS upgrades

There is around 3K of retained memory on most devices. On Gen 3 devices, it may make more sense to use the flash file system file option instead. On the Argon, Boron, B Series SoM, and Tracker SoM, emulated EEPROM is just a file on the flash file system, so there is no efficiency advantage to using EEPROM over files.


### Flash File System File

On Gen 3 devices (Argon, Boron, B Series SoM, and Tracker SoM) running Device OS 2.0.0 or later, you can store the data in a file on the flash file system.

The file system is 2 MB, except on the Tracker SoM, where it's 4 MB. 

### Static Data in Code

This option does not allow for updating from the cloud, but does provide a way to store the configuration as a string constant in the program flash. This is convenient when you want to be able to use a similar code base with both hardcoded and cloud-based configuration settings.

## Data Update Methods

The data update methods allow the data to be updated from the cloud.

### Function



### Subscription

### Static Data

### Device Notes


### Google Sheets

