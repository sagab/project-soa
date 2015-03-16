# Design #

From the design of the EXT2 & FAT filesystems, the superblock should look like this:

```
 struct ash_raw_superblock {
	__u16	sectorsize;		// size of a physical unit of storage in bytes
	__u16	blocksize;		// size of a logical unit of storage in bytes
	__u32	maxblocks;		// number of blocks
	__u64	maxsectors;		// number of sectors
	__u8	blockbits;		// how many bits to represent blocksize
	
	__u16	mnt_count;		// how many times it was mounted
	__u16	max_mnt_count;		// max times before checking it
	__u32	mount_time;		// last mount time
	__u32	write_time;		// last write time
	__u32	last_check;		// time of the last check
	__u32	max_check_time;		// max time between checks

	__u8	state;			// state of the filesystem	
	__u16	magic;			// contains ASH_MAGIC for valid superblock
	__u16	vers;			// ASH version
	
	char	volname[16];		// volume name
	
	__u16	BATsize;		// size of Block Allocation Table in blocks
	__u16	BATsectors;		// size of BAT in sectors
	__u16	datastart;		// number of first data sector (it's an offset)
	__u16	rootentry;		// block number of the root directory entry
 };
```

The superblock resides on sector 0 of the drive and this is all the information that the sector will retain, just in case we need to insert more data into the superblock later on.