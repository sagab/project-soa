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
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/backing-dev.h>

extern struct file_operations ash_file_operations;
extern struct address_space_operations ash_aops;

struct inode_operations ash_file_inode_operations;
struct inode_operations ash_dir_inode_operations;

struct backing_dev_info ash_backing_dev_info = {
	.ra_pages		= 0,	// no readahead
	.capabilities	=  BDI_CAP_NO_ACCT_DIRTY | BDI_CAP_NO_WRITEBACK |
                       BDI_CAP_MAP_DIRECT | BDI_CAP_MAP_COPY |
                       BDI_CAP_READ_MAP | BDI_CAP_WRITE_MAP | BDI_CAP_EXEC_MAP,
};


struct inode* ash_get_inode (struct super_block *sb, int mode)
{
	struct inode *inode = new_inode(sb);
	
	if (!inode)
		return NULL;
		
	inode->i_mode = mode;
	inode->i_uid = current->fsuid;
	inode->i_gid = current->fsgid;
	inode->i_blocks = 0;
	inode->i_mapping->a_ops = &ash_aops;
	inode->i_mapping->backing_dev_info = &ash_backing_dev_info;
	mapping_set_gfp_mask (inode->i_mapping, GFP_HIGHUSER);
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		
	if (S_ISREG(mode)) {
		inode->i_op = &ash_file_inode_operations;
		inode->i_fop = &ash_file_operations;
	} else if (S_ISDIR(mode)) {
		inode->i_op = &ash_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;
				
		inc_nlink(inode);	// for "." reference
	}
	
	return inode;
}



int ash_mknod (struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
	struct inode *inode;
	
	inode = ash_get_inode (dir->i_sb, mode);
	
	if (!inode)
		return -ENOSPC;
	
	if (dir->i_mode & S_ISGID) {
		inode->i_gid = dir->i_gid;
		
		if (S_ISDIR(mode))
			inode->i_mode |= S_ISGID;
	}
	
	d_instantiate (dentry, inode);
	dget (dentry);
	
	dir->i_mtime = dir->i_ctime = CURRENT_TIME;
		
	return 0;
}


int ash_mkdir (struct inode * dir, struct dentry * dentry, int mode)
{
	int ret;
	ret = ash_mknod (dir, dentry, mode | S_IFDIR, 0);
	if (!ret)
		inc_nlink(dir);
	return ret;
}


int ash_create (struct inode *dir, struct dentry *dentry, int mode, struct nameidata *nd)
{
	return ash_mknod (dir, dentry, mode | S_IFREG, 0);
}


struct inode_operations ash_dir_inode_operations = {
	.create		= ash_create,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.mkdir 		= ash_mkdir,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
};


struct inode_operations ash_file_inode_operations = {
	.getattr	= simple_getattr,
};

