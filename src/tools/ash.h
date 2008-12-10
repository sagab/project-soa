/*
 * Ash File System Tools Header
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#ifndef 	__ASH_H__
#define __ASH_H__

#include <stdint.h>  
 
#define ASH_MAGIC		0x451
#define ASH_VERSION		10
#define ASH_SECTORSIZE 		512
#define ASH_BLOCKSIZE		4096
#define ASH_BLOCKBITS		12

// states for the filesystem
#define ASH_FAST_FORMAT		1
#define ASH_DEEP_FORMAT		2

#define ASH_MAX_MOUNTS		30

#define	MINUTE			60
#define HOUR			60*MINUTE
#define DAY			24*HOUR
#define ASH_MAX_CHECK		30*DAY

/*
 * Represents the ASH superblock on the physical USB drive
 *
 */  
struct ash_raw_superblock {
	uint16_t	sectorsize;		// size of a physical unit of storage in bytes
	uint16_t	blocksize;		// size of a logical unit of storage in bytes
	uint32_t	maxblocks;		// number of blocks
	uint64_t	maxsectors;		// number of sectors
	uint8_t		blockbits;		// how many bits to represent blocksize
	
	uint16_t	mnt_count;		// how many times it was mounted
	uint16_t	max_mnt_count;		// max times before checking it
	uint32_t	mount_time;		// last mount time
	uint32_t	write_time;		// last write time
	uint32_t	last_check;		// time of the last check
	uint32_t	max_check_time;		// max time between checks

	uint8_t		state;			// state of the filesystem	
	uint16_t	magic;			// contains ASH_MAGIC for valid superblock
	uint16_t	vers;			// divide by 100 to display ASH version
	
	char		volname[16];		// volume name
	
	uint16_t	BATsize;		// size of Block Allocation Table in blocks
	uint16_t	BATsectors;		// size of BAT in sectors
	uint16_t	datastart;		// number of first data sector (it's an offset)
	uint16_t	rootentry;		// block number of the root directory entry
};


/*
 * Represents a file/directory entry on the physical USB drive
 *
 */
struct ash_raw_file {
	uint16_t	mode;			// file type and access rights
	uint32_t	uid;			// owner ID
	uint32_t	gid;			// group ID
	uint64_t	size;			// file length in bytes
	uint32_t	atime;			// last accessed
	uint32_t	wtime;			// last written
	uint32_t	ctime;			// created
	uint32_t	startblock;		// reference to both BAT and actual data block where file's data is stored
	uint16_t	namelength;		// how long is the filename
};

#endif /* ash.h */
