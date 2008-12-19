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
#include <linux/fs.h>
 
#define ASH_MAGIC		0x451
#define ASH_VERSION		10

#define ASH_SECTORSIZE 		512
#define ASH_SECTORBITS		9

// these values need to be used for reading/writing,
// unless we want to block the kernel :P
#define KERNEL_BLOCKSIZE	4096
#define KERNEL_BLOCKBITS	12


// states for the filesystem
#define ASH_UMOUNT		1
#define ASH_MOUNTED		2


/*
 * Represents the ASH superblock on the physical USB drive
 *
 */  
struct ash_raw_superblock {
	__u16	sectorsize;		// size of a physical unit of storage in bytes
	__u16	blocksize;		// size of a logical unit of storage in bytes
	__u32	maxblocks;		// number of blocks
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
	
	__u16	UBBblocks;		// size of the Used Blocks Bitmap in blocks
	__u16	BATblocks;		// size of Block Allocation Table in blocks
	__u16	UBBstart;		// block where UBB starts
	__u16	BATstart;		// block where BAT starts
	__u16	datastart;		// block where data starts
	__u64	fnogen;			// number of generated files. used to get a unique number for new files
};
 


// values defined for ash_raw_file.ashtype field
#define ASHTYPE_NORMAL		1
#define ASHTYPE_CRYPT		2
#define ASHTYPE_COMP		3
#define ASHTYPE_CRYPTCOMP	4
#define ASHTYPE_COMPCRYPT	5
#define ASHTYPE_REMDENTRY	6


/*
 * Represents a file/directory entry on the physical USB drive
 *
 */
struct ash_raw_file {
	__u16	mode;			// Linux file type and access rights
	__u8	ashtype;		// special Ash type
	__u32	uid;			// owner ID
	__u32	gid;			// group ID
	__u64	size;			// file length in bytes
	__u32	atime;			// last accessed
	__u32	wtime;			// last written
	__u32	ctime;			// created
	__u32	startblock;		// reference to both BAT and actual data block where file's data is stored
	__u64	fno;			// file number reference. should be unique in the fs.
	char	name[256];		// filename
};


// Reads a block from the drive and returns a buffer of blocksize bytes
// or NULL in case of an error
extern void* block_read (struct super_block *sb, uint32_t block);

// Writes a block to the drive. The block's data is an array of blocksize bytes
// returns 0 on success
extern int block_write (struct super_block *sb, void *data, uint32_t block);


// Returns the number of the first available block
// -1 on error
extern int block_first_free(struct super_block *sb);

// Reads what value a block has in the Used Blocks Bitmap
// returns 0, 1 or -1 in case of error
extern int UBB_read (struct super_block *sb, uint32_t block);

// Writes the value val for a block in Used Blocks Bitmap zone
// returns 0 on success
extern int UBB_write (struct super_block *sb, uint32_t block, uint8_t val);

// Get the number of the next block of data following block from the Block Allocation Table
extern int BAT_read (struct super_block *sb, uint32_t block);

// Write the number of the entry for the block in the BAT
// returns 0 on success
extern int BAT_write (struct super_block *sb, uint32_t block, uint32_t entry);


#endif /* ash.h */
