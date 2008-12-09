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
#include <time.h>
#include <sys/stat.h>

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
	// fill in a superblock structure
	struct ash_raw_superblock s;
	
	// set everything to 0, just in case we forget to init fields
	memset(&s, 0, sizeof(s));
	
	s.sectorsize = ASH_SECTORSIZE;
	s.blocksize = ASH_BLOCKSIZE;
	s.blockbits = ASH_BLOCKBITS;
	s.maxsectors = size;
	
	// slightly different formula than on the wiki
	// http://code.google.com/p/project-soa/wiki/PhysicalStructureeinstein
	uint32_t tmp = (uint32_t)s.blocksize + (uint32_t)4;
	s.maxblocks = (uint32_t)( size * s.sectorsize - 2 * s.sectorsize ) / tmp;
	
	s.BATsize = s.maxblocks * 4;
	s.BATsectors = s.BATsize / s.sectorsize + 1;
	s.datastart = s.rootentry = 1 + s.BATsectors;
	
	s.mnt_count = 0;
	s.max_mnt_count = ASH_MAX_MOUNTS;
	
	time_t now = time(0);
	
	s.mount_time = now;
	s.write_time = now;
	s.last_check = now;
	s.max_check_time = ASH_MAX_CHECK;
	
	s.state = ASH_FAST_FORMAT;
	s.magic = ASH_MAGIC;
	s.vers = ASH_VERSION;
	
	strcpy(s.volname, "usbstick");
	
	// filling in the root directory entry
	struct ash_raw_file rentry;
	
	// clear struct
	memset(&rentry, 0, sizeof(rentry));
	
	// sets all permissions for all users and sets sticky bit
	rentry.mode = S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX;
	rentry.uid = rentry.gid = 1;
	rentry.size = 0;
	rentry.atime = rentry.wtime = rentry.ctime = now;
	rentry.startblock = s.rootentry;
	rentry.namelength = 0;		// root dir doesn't have a name
	
	// trying to open the device file
	FILE *fd = fopen(device, "w");
	if (!fd) {
		printf("cannot open device for writing\n");
		return 1;	
	}
	
	// writing the superblock structure
	int sw = fwrite(&s, sizeof(s), 1, fd);
	if (sw != 1) {
		printf("error while writing superblock\n");
		return 1;
	}
	
	// writing the root directory entry structure
	sw = fwrite(&rentry, sizeof(rentry), 1, fd);
	if (sw != 1) {
		printf("error while writing root directory entry\n");
		return 1;
	}
	
	// closing device
	fclose(fd);

	// printout verbose info
	printf("\n");
	printf("volume name: '%s'\n", s.volname);
	printf("formatted with Ash vers. %4.2f\n", (float)s.vers/100);
	printf("sector size: %d bytes\n", s.sectorsize);
	printf("block size: %d bytes\n", s.blocksize);
	printf("max sectors: %d\n", s.maxsectors);
	printf("max blocks: %d\n", s.maxblocks);
	printf("BATsize: %d bytes\n", s.BATsize);
	printf("BATsectors: %d\n", s.BATsectors);
	printf("start of data @ sector: %d\n\n", s.rootentry);
	
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
