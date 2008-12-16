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
#include <linux/spinlock.h>
#include "ash.h"


/*
 * Lists a part of the entries in a directory, starting from filp->f_pos entry
 * and by using the filldir function
 *
 * filldir(dirent, name, name_len, pos, ino, flags)
 */
int ash_readdir (struct file *filp, void *dirent, filldir_t filldir) {
	int cpos;
	struct dentry *de;
	struct ash_raw_file *rf, *entry;
	struct inode *dir;
	char *point;
	
	// current position
	cpos = filp->f_pos;

	// get dentry for the current dir
	de = filp->f_path.dentry;

	// test if we passed the . and .. positions in a dir
	if (cpos == 0) {
		if (filldir(dirent, ".", 1, cpos, de->d_inode->i_ino, DT_DIR) < 0)
			goto out;
		cpos++;
		filp->f_pos++;
	}

	if (cpos == 1) {
		// getting dentry for the ..
		de = de->d_parent;

		if (filldir(dirent, "..", 2, cpos, de->d_inode->i_ino, DT_DIR) < 0)
			goto out;

		cpos++;
		filp->f_pos++;
	}

	// parses next entries from the directory entry on the disk
	// need the information from the inode
	dir = filp->f_path.dentry->d_inode;
	rf = (struct ash_raw_file*) dir->i_private;
	entry = (struct ash_raw_file*) block_read (dir->i_sb, rf->startblock);
	
	printk("block: %d, size: %d\n", rf->startblock ,dir->i_sb->s_blocksize);
	
out:	
	return 0;
}


struct file_operations ash_dir_operations = {
	.read		=	generic_read_dir,
	.readdir	=	ash_readdir,
};

