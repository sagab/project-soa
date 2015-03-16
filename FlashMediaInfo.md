# Flashpens #

We're developing our filesystem to be used on a flash USB Mass Storage device, also known as a flash stick, usb stick, flash drive, flash pen. :P

Currently we're using a 1GB flash made by HP. By 1GB, the storage devices producers mean 1 000 000 000 bytes, not 2^30, as in operating systems terminology, which leads to OS showing less than the available drive capacity. Except this aspect, the drive will ALSO have less capacity than stated in certain cases, due to the filesystem metadata that is stored along with the files (if there are 500 MB available on stick reported by the OS, writing 500 x 1MB files will fail -> best scenario is probably beeing able to write 499 x 1MB files on it).

Additional information:

---

sector size: 512 bytes
sectors: 1953125, close to 2 million, which means we need 64 bits to represent sector number internally if we use it.

Supposedly, there is a limit of 300 000 write cycles on each sector. A filesystem designed for this should take the factor into consideration (it also means using flash drives for RAMboost emulation in Vista will make it break early).