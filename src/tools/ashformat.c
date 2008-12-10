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
#include <math.h>

#include "ash.h"


/**
 * Get the size in bytes of a device
 *
 * @device name of the device in the /dev system
 * @return the size of the device in bytes. 0 on error.
 */
uint64_t getsize(char *device)
{
	int k = strlen(device);
	char path[k+20];

	// device will contain /dev/sdb1 or equivalent.
	// we need to build path to point to this file: /sys/block/sdb/sdb1/size
	strcpy(path, "/sys/block/");
	char *devpos = strrchr(device,'/');
	
	if (! devpos || strlen(devpos) < 5) {
		printf("'%s' is not a device path\n", device);
		return 0;
	}
	
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
		return 0;
	}
	
	uint64_t rez;
	int sw = fscanf(f,"%d", &rez);
	
	// close
	fclose(f);
	
	if (sw != 1) {
		printf("unknown file format for '%s'\n", path);
		return 0;
	}
	
	// multiply the size in sectors by sectorsize (512 = 2^9)
	rez <<= ASH_SECTORBITS;
	
	return rez;
}



/**
 * Format the device with AshFS
 * @device The name of the device in /dev
 * @bsize block size in bytes for Ash
 * @size device capacity in bytes
 * @name volume name
 * @return 0 on ok.
 */
int format(char *device, uint16_t bsize, uint64_t size, char *volname)
{
	// fill in a superblock structure
	struct ash_raw_superblock s;
	
	// set everything to 0, just in case we forget to init fields
	memset(&s, 0, sizeof(s));
	
	s.sectorsize = ASH_SECTORSIZE;
	s.blocksize = bsize;
	s.blockbits = (uint16_t) (ceil(log(bsize)/log(2)));
	
	// formula on the wiki
	// http://code.google.com/p/project-soa/wiki/PhysicalStructure
	long long mb = (long long)size / 4096;
	printf("size %d, max block %d\n ", size, mb);
	s.maxblocks = size >> s.blockbits;
	s.UBBsize = s.maxblocks >> 3;
	s.BATsize = s.maxblocks << 2;
	
	uint16_t total = sizeof(s) + s.UBBsize + s.BATsize;
	s.datastart = total >> s.blockbits;
	
	// in case it occupies more, take the additional space up to next block
	if (total > (s.datastart << s.blockbits))
		s.datastart ++;
	
	s.mnt_count = 0;
	s.max_mnt_count = ASH_MAX_MOUNTS;
	
	time_t now = time(0);
	
	s.mount_time = now;
	s.write_time = now;
	s.last_check = now;
	s.max_check_time = ASH_MAX_CHECK;
	
	s.state = ASH_UMOUNT;
	s.magic = ASH_MAGIC;
	s.vers = ASH_VERSION;
	
	strcpy(s.volname, volname);
	
	// filling in the root directory entry
	struct ash_raw_file rentry;
	
	// clear struct
	memset(&rentry, 0, sizeof(rentry));
	
	// sets bits for directory, r/w/x for owner, r/w group & others
	rentry.mode = S_IFDIR | S_IRWXU | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH);
	rentry.ashtype = ASHTYPE_NORMAL;
	rentry.uid = rentry.gid = 1;
	rentry.size = 0;
	rentry.atime = rentry.wtime = rentry.ctime = now;
	rentry.startblock = s.datastart;
	rentry.namelength = 0;		// root dir doesn't have a name
	
	// trying to open the device file
	FILE *fd = fopen("/root/test.log", "w+");
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
	
	// writing the used block bitmap
	uint8_t buf[512];
	
	// bytes representing used blocks -> therefore = FF
	uint16_t bytes = s.datastart >> 3;
	
	// number of unused blocks in the last byte
	uint16_t bits = 8 - s.UBBsize & 7;
	
	// this will insert zeroes in the octet
	uint16_t last = (0xFF >> bits) << bits;
	
	// unless we're somehow talking about a 22 TB flash drive,
	// there's no way for bytes >= 512, so they all fit in buf.
	memset(buf, 0, 512);		// clear all
	memset(buf, 0xFF, bytes);	// set bytes of them to 0xFF
	buf[bytes] = last;		// set the last partially used one
	
	printf("UBB used bytes: %d, bits: %d, last= %x\n", bytes, bits, last);
	
	// write first bytes + 1 bytes.
	sw = fwrite(&buf, sizeof(uint8_t), bytes + 1, fd);
	if (sw != bytes + 1) {
		printf("error while writing UBB\n");
		return 1;
	}
	
	// write last part of big 0-es
	uint16_t left = s.UBBsize - bytes - 1;		// bytes left to copy
	int n = left >> 9;		// chunks of 512 bytes from left
	memset(buf, 0, 512);
	
	// mass copy
	int i;
	for (i = 0; i < n; i++) {
		sw = fwrite(&buf, sizeof(uint8_t), 512, fd);
		if (sw != 512) {
			printf("error while writing UBB\n");
			return 1;
		}
		
		left -= 512;	// adjust number of bytes left
	}
	
	// small size copy
	sw = fwrite(&buf, sizeof(uint8_t), left, fd);
	if (sw != left) {
		printf("error while writing UBB\n");
		return 1;
	}

	printf("UBB left=%d n=%d\n", left, n);
	
	// writing BAT table with zeroes
	memset(buf, 0, 512);

	left = s.BATsize;
	n = left >> 9;
	for (i = 0; i < n; i++) {
		sw = fwrite(&buf, sizeof(uint8_t), 512, fd);
		if (sw != 512) {
			printf("error while writing BAT\n");
			return 1;
		}
		
		left -= 512;
	}
	
	sw = fwrite(&buf, sizeof(uint8_t), left, fd);
	if (sw != left) {
		printf("error while writing UBB\n");
		return 1;
	}
	
	printf("BAT left=%d n=%d\n", left, n);
	
	// seek to the root directory entry location
	if (fseek(fd, rentry.startblock * s.blocksize, SEEK_SET) != 0) {
		printf("error while trying to seek on the device file\n");
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
	printf("formatted with Ash vers. %4.2f\n", (float)s.vers/128);
	printf("block size: %d bytes\n", s.blocksize);
	printf("block bits: %d\n", s.blockbits);
	printf("max blocks: %d\n", s.maxblocks);
	printf("UBBsize: %d bytes\n", s.UBBsize);
	printf("BATsize: %d bytes\n", s.BATsize);
	printf("datastart @ block: %d\n\n", s.datastart);
	
	return 0;
}




/*
 * Prints instructions
 *
 */
void instructions()
{
	printf("\n\tAshFS Disk Format Utility\n\n");
	printf("usage:\t");
	printf("./ashformat <dev> [-b <bsize>] [-n <volname>]\n");
	printf("<dev>: name of the device to format (ex: /dev/sdb1)\n\n");
	printf("<bsize>: size of logical block. Must be a multiple of 512. Default 4096\n");
	printf("<volname>: 15 alfanum for name. Default 'usbstick'\n\n");
}



/**
 * The main function of the formatter
 * @arc the number of given arguments
 * @argv the list of arguments of the program. argv[0]=program's name.
 * @return 0 on success, the rest are error codes.
 */
int main(int argc, char **argv)
{
	if (argc < 2) {
		instructions();
		return 1;
	}
	
	char volname[16];
	uint64_t size;
	int bsize;
	int devicearg;

	// do some inits
	devicearg = -1;
	strcpy(volname, "usbstick");
	bsize = ASH_BLOCKSIZE;

	// start parsing the parameters
	int p = 1;
	
	while(p<argc) {
		if (strcmp(argv[p],"-n") == 0) {
			
			if (p+1 >= argc) {
				printf("you need to specify a string if you use -n flag\n");
				return 1;			
			}
			
			int len = strlen(argv[p+1]);
			
			// test
			if (len < 16) {
			
				// check each letter
				int l, test = 1;
				
				for (l = 0; l < len; l++)
					if (! isalnum(argv[p+1][l]) )
						test = 0;
						
				if (!test) {
					printf("volname contains illegal chars\n");
					return 1;
				}
				
				// all ok, copy name
				strcpy(volname, argv[p+1]);
				p+=2;
			
			} else {
				printf("volname bigger than 15 letters\n");
				return 1;
			}
			
		} else
			if (strcmp(argv[p],"-b") == 0) {
				if (p+1 >= argc) {
					printf("you are missing the blocksize parameter\n");
					return 1;
				}
			
				int r = sscanf(argv[p+1], "%d", &bsize);
				
				if (bsize < 512 || bsize > 8096) {
					printf("blocksize must be between 512 and 8096.\n");
					return 1;
				}
				
				if (r == 0 || (bsize != bsize >> 9 << 9)) {
					printf("blocksize is not a valid multiple of 512.\n");
					return 1;
				}
				
				p+=2;
		
			} else
				devicearg = p++;
	
	}
	
	if (devicearg < 1) {
		printf("you must specify a device to format\n");
		return 1;
	}
	
	
	// get device info
	size = getsize(argv[devicearg]);
	if (size == 0) {
		printf("the device '%s' isn't a valid block device!\n", argv[devicearg]);
		return 2;
	}
	
	// format the device media
	int sw = format(argv[devicearg], bsize, size, volname);
	if (sw == 0) {
		printf("Formatting OK.\n");
	} else
		printf("Error occured during formatting: %d\n", sw);
		
	return sw;
}
