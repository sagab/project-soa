/*
 * Ash File System
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#ifndef __ASH_H__
#define __ASH_H__

#include <linux/types.h>
 
#define ASH_MAGIC		0x451
#define ASH_VERSION		10
#define ASH_SECTORSIZE 		512

// states for the filesystem
#define ASH_FAST_FORMAT		1
#define ASH_DEEP_FORMAT		2


/*
 * Represents the ASH superblock on the physical USB drive
 *
 */  
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
 


/*
 * Represents a file/directory entry on the physical USB drive
 *
 */ 
struct ash_raw_file {
	__u16	mode;			// file type and access rights
	__u32	uid;			// owner ID
	__u32	gid;			// group ID
	__u64	size;			// file length in bytes
	__u32	atime;			// last accessed
	__u32	wtime;			// last written
	__u32	ctime;			// created
	__u32	startblock;		// reference to both BAT and actual data block where file's data is stored
	__u16	namelength;		// how long is the filename
};

#endif /* ash.h */
