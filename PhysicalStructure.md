# Physical Organization #
We need a plan to store the data on the disk, so after carefully considering Ext2 and FAT filesystem, we chose an approach similar to FAT with a few tweaks.

Consult the http://en.wikipedia.org/wiki/USB_flash_drive to get an idea about what USB flash stick capabilities are.

The disk structure will look like this:
| Super Block | Used Blocks Bitmap | Block Allocation Table | block <sub>datastart</sub> | block <sub>datastart+1</sub> | ... | block <sub>maxblocks-1</sub> |
|:------------|:-------------------|:-----------------------|:---------------------------|:-----------------------------|:----|:-----------------------------|

Each of the starting sections is aligned to a logical disk block size, even if it would occupy less.

  * the Super Block has around only 62 bytes of data (check the structure of ash\_raw\_superblock to get an idea) and takes up the first block.

  * the Used Blocks Bitmap starts from block <sub>1</sub> and contains one bit for every block on the disk, which says whether the block is used for storage or is empty. This zone also takes up a number of blocks on the disk, UBBblocks.

  * the BAT (Block Allocation Table) is similar to the one used by FAT filesystem (http://www.usq.edu.au/users/leis/courses/ELE3305/fat.pdf) and starts with the block after the UBB blocks, having a size of BATblocks on the disk.

Let's imagine a file is stored in several blocks, for example 2, 4, 1, in that order. The file's directory entry structure will contain a reference to the first block used to store the data, which is 2.

To read the next part of the file, the filesystem manager will look in BAT at the index 2, and find the block number 4. It reads the next one third of the file. Then it will look at index 4 in the BAT and find the number 1. Goes and reads block 1 and finishes reading. The BAT will store 0 for the next reference number, indicating it has reached the end.

The BAT is stored as a vector with maxblocks entries and each of them is a reference to an existing logical disk block number between 0 and maxblocks-1. We can mark one of the references unused if we put a 0 value which corresponds to the first block number, which is obviously filled with the superblock, and cannot have actual data for files. Each entry is a 32bit block number, therefore it takes us 4 bytes to store such a reference.

  * the area comprised of actual data blocks is from block <sub>datastart</sub> .. to block <sub>maxblocks-1</sub>



# Logical Disk Blocks, Sectors, Logical Kernel Blocks #

By **sector**, we understand the smallest possible unit in which data can be read from the block device. As a legacy, this value has been maintained to 512 bytes for all known device types over the years, and therefore whenever we are dealing with sector numbers, the sector size will be 512, in all contexts.

**Logical Kernel Blocks** are what the Linux kernel perceives as beeing the unit to read from a block device. Currently, that value is 4096 bytes and attempting to read less (say, a sector) or more from a random location on the device WILL block the kernel. This has been thoroughly tested, so whenever we are making writes or reads by using bread, sb\_bread, etc, we must specify a "sector" number which is actually the number of the kernel logical block and a size of how much to read in the buffer.


**Logical Disk Blocks** are what we need to use to address space on the physical disk. The reason why this must exist is that at some point, the disk becomes fragmented in order to store a new large enough file, and instead of keeping for each file 2 values indicating that it starts on byte x of the drive and ends on byte y, we need an actual linked list with (x,y) pairs for every part of the file. This tends to occupy a lot of space, so a different method is used: we split the disk into larger units, and make a vector that stores linked chains between its elements (this is of course the BAT, but its functions are more than just that).

Assuming a 1 GB capacity for the drive, and block size of 1 sector, we have 2 million blocks. Therefore, a block number needs a 32 bit unsigned integer to be stored, so 4 bytes. We need to have a BAT of 2 million entries, each of them 4 bytes long, so the BAT takes up 8 MB.

Let's assume now a block size of 4096 bytes (4K), which leads to 250K blocks. 250K also needs 32 bits for storing a number, but the table only has 1 MB, which makes it easier to have in memory.

Assume block size of 32768 bytes (32K), which leads to 31K blocks. 31K blocks can be addressed by 16 bits, therefore 2 bytes, which leads to a table of 62K size. Quite a big difference from the 8 MB one, right?

What's the problem with using big block sizes then? Setting up a logical block size is only the first part. Our problem was defragmented files, and storing them as a multiple of blocks was the answer. If a file is a multiple of such a block, one can only start from the start of the block, therefore the last block will have a lot of wasted space, unless all files are perfect multiples of block size. Now, assuming 32K block sizes... and filesizes which leave 16K free from the last block (filesize = 32K x n + 16K), every 100 files we will be wasting 16K x 100 = 1.6 MB. Say those 100 files are just 48K in size, for n=1 and we get a nice waste of 1.6 MB for 4.8MB worth of files, which is **33% wasted space**.

Therefore, we need to choose a good number for block size on the disk, and it can only be a power of 2 because of making it easier to compute relative offsets if we're only multiplying or dividing by powers of 2, which can be shifted around in registers. We can't use normal divides or floating point in Linux kernel space anyway.

**Further problems**
  * Why not use 3 byte numbers for storing block references in BAT, when we get 1024 bytes blocksize, in order to minimize the entry length in the BAT? That is because the registers aren't in lengths of 3 bytes, which will make operations with them a very tedious thing to think about :). Therefore, either 2 byte entries, or 4 bytes ones, no inbetween.

  * We always have to translate these references on the disk to the kernel's 4K block sizes for the read/write operations.

  * The block size will always be a multiple of sectorsize, as a consequence of defining the sector. Therefore, 1 block = 1, 2, 4, 8, 16 sectors usually.

  * Block numbers are on 32 bits, regardless how many sectors a disk can have. Therefore, we get these limits for disk capacities (and filesize, actually):
| Size of blocks (bytes) | Max capacity |
|:-----------------------|:-------------|
| 512 | 2.2 TB |
| 1024 | 4.4 TB |
| 4096 | 17.6 TB |



# Formulas #

We should assume these are variable:
  * capacity = 1 GB for example (the actual size of my 1GB stick is... neither 10<sup>9</sup> nor 2<sup>30</sup>, but for this example, let's consider 1GB = 10<sup>9</sup> to anticipate floating point results when dividing)

  * blocksize = 4096 bytes (must be a multiple of sectorsize)
  * blockbits = 12 (log <sub>2</sub> 4096)

We should assume this is a constant:
  * sectorsize = 512 bytes

This is what we need to find out:
  * maxblocks = number of logical blocks on the device, from our chart
  * UBBblocks = blocks occupied by the UBB
  * BATblocks = blocks occupied by the BAT
  * datastart = first logical block from where we can find data.
  * (dB, dO) -> (kB, kO)? meaning how to translate from disk block, disk offset in the block, to kernel block and kernel offset in that block.

**maxblocks = (uint32\_t) ceil ((double)capacity / blocksize)**

**UBBblocks = (uint32\_t) ceil ( ceil( (double)maxblocks / 8 ) / blocksize )**

**BATblocks =  (uint32\_t) ceil( (double)maxblocks** 4 / blocksize )

**datastart = 1 + UBBblocks + BATblocks**

**byteoffset = dB << blockbits + dO
kB = byteoffset >> 12
kO = byteoffset & 0xFFF**


# Results on 1 GB stick example #
1 GB = 10^9 (in this example) = 111011100110101100101000000000

maxblocks = 111011100110101100 = 244140

UBBsize = 111011100110101 = 30517

BATsize = 1110111001101011 = 61035

datastart = (62 + 30517 + 61035) >> 12 = 10110010111011110 >> 12 = 10110 = block 22.