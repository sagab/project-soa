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
#include <linux/dcache.h>

struct inode * ash_make_inode (struct super_block *sb, int mode)
{
	struct inode *inode;

	inode = new_inode(sb);
	if (inode) {
		inode->i_mode = mode;
		inode->i_atime = CURRENT_TIME;
		inode->i_mtime = inode->i_ctime = inode->i_atime;
	}

	return inode;
}
