/*
 * Ash File System
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */
 
 #include <linux/types.h>
 
 #define ASH_MAGIC		0x451
 #define ASH_BLOCKSIZE		512
 #define ASH_BSIZE_BITS		9
 
 /*
  * This is the representation of a node in the
  * physical structure of the USB flash disk
  *
  */
 struct ash_raw_node {
	__u16	magic;	// contains ASH_MAGIC for valid nodes
	__u8	type;	// node type
	__u8	tcomp;	// compression type
	__u64	size;	// total size in bytes
	__u64	next;	// offset in bytes for the next node
	void*	data;	// node's data
 };
 
 // defines for node types
 #define ASH_NODE_HEAD	1
 #define ASH_NODE_DATA	2
 #define ASH_NODE_END	3
 
 // defines for compression types
 #define ASH_COMP_NONE	1
 #define ASH_COMP_ZIP	2
 
 /*
  * Represents the ASH superblock on the physical USB drive
  *
  */  
 struct ash_raw_superblock {
	__u16	blocksize;		// size of a physical block of device
	__u64	totalblocks;		// number of total physical blocks
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
	
	__u64	blockmap;		// block number for the block bitmap
	__u64	firstnode;		// block number of the first ash_raw_node
 };
