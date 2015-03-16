# Ash File System #

Ash FS is a file system for flash memory devices. It tries to reduce wear levelling of the flash drive by storing such large quantity of data in an uniform way with the help of a block manager.


# Features #

Ash FS offers:

  * compression , implies running a simple algorithm over the data
  * garbage collector
  * error detection , using CRC
  * security using symmetric key cryptography

# Design #

AshFS project contains 3 main modules:

  * **ash**  , contains the actual file system implementation
  * **ftl**  , emulates a block device on the top of flash hardware
  * **tash** , offers a testbench for evaluating file system's performances.

# See also #

  * [Flash Memory](http://en.wikipedia.org/wiki/Flash_memory)
  * [List of flash file systems](http://www.en.wikipedia.org)
  * [Memory Technology Device](http://www.linux-mtd.infradead.org/)
  * [Flash Filesystems for Embedded Linux Systems](http://www.linuxdevices.com/articles/AT7478621147.html)