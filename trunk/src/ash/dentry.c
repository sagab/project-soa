/*
 * Ash File System
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */


static int ash_fs_link (struct dentry *, struct inode *, struct dentry *);



struct file_operations ash_dir_operations{
	.dir 		=	TODO; 
	.readdir	=	TODO;
};

struct inode_operations ash_dir_inode_operations{
	.create		= ash_fs_create,
	.lookup		= ash_fs_lookup,
	.link		= ash_fs_link,
	.unlink		= ash_fs_unlink,
	//...
};


	
