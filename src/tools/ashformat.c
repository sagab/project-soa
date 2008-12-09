/*
 * AshFS Format Utility Program
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "ash.h"


/**
 * Get the size in sectors of a device
 *
 * @device name of the device in the /dev system
 * @return the size as a 64bit unsigned integer value. -1 on error.
 */
uint64_t getsize(char *device)
{
	int k = strlen(device);
	char path[k+20];

	// device will contain /dev/sdb1 or equivalent.
	// we need to build path to point to this file: /sys/block/sdb/sdb1/size
	strcpy(path, "/sys/block/");
	char *devpos = strrchr(device,'/');
	devpos++;
	
	strncat(path, devpos, 3);
	strcat(path,"/");
	strcat(path, devpos);
	strcat(path,"/");
	strcat(path, "size");

	// open the file
	FILE *f = fopen(path, "r");
	if (!f) {
		printf("cannot open '%s'\n", path);
		return -1;
	}
	
	uint64_t rez;
	int sw = fscanf(f,"%d", &rez);
	
	// close
	fclose(f);
	
	if (sw != 1) {
		printf("unknown file format for '%s'\n", path);
		return -1;
	}
	
	return rez;
}



/**
 * Format the device with AshFS and the size in sectors
 * @device The name of the device in /dev
 * @size Size given as the number of sectors
 * @return 0 on ok.
 */
int format(char *device, uint64_t size)
{
	struct ash_raw_superblock s;
	
	s.sectorsize = ASH_SECTORSIZE;
	s.blocksize = ASH_BLOCKSIZE;
	s.maxsectors = size;
	
	s.mnt_count = 0;
	s.max_mnt_count = 30;
	s.mount_time = ;
	s.write_time = ;
	
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
	__u16	rootentry;	
	
	return 0;
}



/**
 * The main function of the formatter
 * @arc the number of given arguments
 * @argv the list of arguments of the program. argv[0]=program's name.
 * @return 0 on success, the rest are error codes.
 */
int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("\n\tAshFS Disk Format Utility\n\n");
		printf("usage:\t");
		printf("./ashformat <dev>\n");
		printf("<dev>: name of the device to format (ex: /dev/sdb1)\n\n");
		return 1;
	}
	
	// get device info
	uint64_t sectors = getsize(argv[1]);
	if (sectors < 0) {
		printf("the device '%s' isn't a valid block device!\n", argv[1]);
		return 2;
	}
	
	
	// format the device media
	int sw = format(argv[1], sectors);
	if (sw == 0) {
		printf("Formatting OK.\n");
	} else
		printf("Error occured during formatting: %d\n", sw);
		
	return sw;
}
