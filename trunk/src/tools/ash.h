/*
 * Ash File System Tools Header
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */
 
#define ASH_MAGIC		0x451
#define ASH_SECTORSIZE 		512
#define ASH_BLOCKSIZE		4096

#include <stdint.h>  

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
	uint16_t	vers;			// ASH version
	
	char		volname[16];		// volume name
	
	uint16_t	BATsize;		// size of Block Allocation Table in blocks
	uint16_t	BATsectors;		// size of BAT in sectors
	uint16_t	datastart;		// number of first data sector (it's an offset)
	uint16_t	rootentry;		// block number of the root directory entry
};
