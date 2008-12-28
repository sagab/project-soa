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

int ash_mkdir (struct inode *inode, struct dentry *dentry, int mode)
{
	//struct ash_raw_file arf;
	// verify if dentry exists in the inode on disk
	
	// create it
	/*arf.mode = mode;
	arf.ashtype = ;
	arf.uid, gid
	arf.size = 0;
	atime, wtime, ctime,
	startblock
	fno
	name
*/
	return 0;
}


struct inode_operations ash_inode_operations = {
	.mkdir = ash_mkdir,
	.lookup = simple_lookup,
};
