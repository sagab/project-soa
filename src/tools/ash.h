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
#define ASH_SECTORBITS		9

// these are just default values and can be changed at format
#define ASH_BLOCKSIZE		4096
#define ASH_BLOCKBITS		12

// states for the filesystem
#define ASH_UMOUNT		1
#define ASH_MOUNTED		2

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
	uint8_t		blockbits;		// how many bits to represent blocksize
	
	uint16_t	mnt_count;		// how many times it was mounted
	uint16_t	max_mnt_count;		// max times before checking it
	uint32_t	mount_time;		// last mount time
	uint32_t	write_time;		// last write time
	uint32_t	last_check;		// time of the last check
	uint32_t	max_check_time;		// max time between checks

	uint8_t		state;			// state of the filesystem	
	uint16_t	magic;			// contains ASH_MAGIC for valid superblock
	uint16_t	vers;			// divide by 128 to display ASH version
	
	char		volname[16];		// volume name
	
	uint16_t	UBBblocks;		// size of the Used Blocks Bitmap in blocks
	uint16_t	BATblocks;		// size of Block Allocation Table in blocks
	uint16_t	UBBstart;		// block where UBB starts
	uint16_t	BATstart;		// block where BAT starts
	uint16_t	datastart;		// block where data starts
	uint64_t	fnogen;			// number of generated files. used to get a unique number for new files
};



// values defined for ash_raw_file.ashtype field
#define ASHTYPE_NORMAL		1
#define ASHTYPE_CRYPT		2
#define ASHTYPE_COMP		3
#define ASHTYPE_CRYPTCOMP	4
#define ASHTYPE_COMPCRYPT	5
#define ASHTYPE_HASHEDDIR	6


/*
 * Represents a file/directory entry on the physical USB drive
 *
 */
struct ash_raw_file {
	uint16_t	mode;			// Linux file type and access rights
	uint8_t		ashtype;		// special Ash type
	uint32_t	uid;			// owner ID
	uint32_t	gid;			// group ID
	uint64_t	size;			// file length in bytes
	uint32_t	atime;			// last accessed
	uint32_t	wtime;			// last written
	uint32_t	ctime;			// created
	uint32_t	startblock;		// reference to both BAT and actual data block where file's data is stored
	uint64_t	fno;			// file number reference. should be unique in the fs.
	char		name[256];		// filename
};

#endif /* ash.h */
