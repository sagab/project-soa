/*
 * Ash File System
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "ash.h"


/*
 * Lists a part of the entries in a directory, starting from filp->f_pos entry
 * and by using the filldir function
 *
 * filldir(dirent, name, name_len, pos, ino, flags)
 */
int ash_readdir (struct file *filp, void *dirent, filldir_t filldir) {
	int cpos, done, i;
	struct dentry *de;
	struct ash_raw_file *rf, *entry;
	struct inode *dir;
	struct super_block *sb;
	uint32_t lB, lO, blocks, maxoff;
	char *point;
	
	// current position
	cpos = filp->f_pos;

	// get dentry for the current dir
	de = filp->f_path.dentry;
	
	// test if we passed the . and .. virtual dirs
	if (cpos == 0) {
		
		if (filldir(dirent, ".", 1, cpos, de->d_inode->i_ino, DT_DIR) < 0)
			return 0;
		cpos++;
		filp->f_pos++;
	}

	if (cpos == 1) {
		// getting dentry for the ..
		de = de->d_parent;

		if (filldir(dirent, "..", 2, cpos, de->d_inode->i_ino, DT_DIR) < 0)
			return 0;

		cpos++;
		filp->f_pos++;
	}

	// parses next entries from the directory entry on the disk
	// need the information from the inode
	dir = de->d_inode;
	sb = dir->i_sb;
	rf = (struct ash_raw_file*) dir->i_private;
	
	// find the entry on disk that corresponds to cpos
	blocks = cpos >> (sb->s_blocksize_bits);
	
	// start block of dentry
	lB = BAT_read(sb, rf->startblock);
	
	// must traverse all BAT for the dentry to current position
	for (i = 0; i < blocks; i++)
		lB = BAT_read(sb, lB);
		if (lB < 0)
			return -EIO;
	
	if (cpos > 2)
		lO = cpos & (sb->s_blocksize - 1);
	else
		lO = 0;		// cpos is actually 0, but I needed those 2 virtual files and the state
				// of running filldir on them
	
	done = 0;
	maxoff = sb->s_blocksize - sizeof(struct ash_raw_file);
	
	while (!done) {
	
		// read the dir entry from disk
		point = (char*) block_read (sb, lB);
		
		if (!point) {
			kfree(point);
			return -EIO;
		}
			
		entry = (struct ash_raw_file*) (point + lO);
		
		// parse the entries in the block
		while(lO < maxoff && cpos < rf->size) {
			unsigned type;		// type of dentry, DT_DIR, DT_REG for now
			
			type = DT_UNKNOWN;	// can be anything
			
			// for directory dentry
			if (S_ISDIR(entry->mode))
				type = DT_DIR;
			
			// for normal file dentry
			if (S_ISREG(entry->mode))
				type = DT_REG;
			
			// a dentry can be marked deleted but still be present and accounted for space
			if (entry->ashtype != ASHTYPE_REMDENTRY &&
				filldir(dirent, entry->name, strlen(entry->name), cpos, entry->fno, type) < 0) {
			
				kfree(point);
				return 0;
			}
			
			// next entry on disk
			entry++;
			
			// adjust offsets
			lO += sizeof(struct ash_raw_file);
			cpos += sizeof(struct ash_raw_file);
			filp->f_pos += sizeof(struct ash_raw_file);
		}
		
		// there are a few bytes betwen lO and end of block, which need
		// to be accounted for in cpos, or next time the function runs
		// we won't be able to find the correct logical block in which the
		// filldir stopped
		
		// test if we are finished
		if (cpos == rf->size) {
			kfree(point);
			return 0;
		}
		
		cpos += sb->s_blocksize - lO;
		filp->f_pos += sb->s_blocksize - lO;
		
		// find out next block from BAT
		lB = BAT_read(sb, lB);
		lO = 0;
		
		if (lB<0) {
			kfree(point);
			return -EIO;
		}
		
		// we have reached the end of the direntry list
		if (lB == 0)
			done = 1;
		
		// free the buffer of the read block from disk
		kfree(point);
	}
	
	return 0;
}


struct file_operations ash_dir_operations = {
	.read		=	generic_read_dir,
	.readdir	=	ash_readdir,
};

