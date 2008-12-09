/*
 * Ash File System
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/buffer_head.h>
#include <asm/string.h>
#include "ash.h"

static atomic_t testcount;

static struct super_operations ash_super_operations = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

extern struct dentry * ash_create_file (struct super_block  *, struct dentry *, const char *, void *);
extern struct inode * ash_make_inode (struct super_block *, int);

static void ash_create_testfile (struct super_block *sb, struct dentry *root)
{
	atomic_set(&testcount, 0);

	// create a testfile, passing the testcount as private data pointer
	ash_create_file(sb, root, "counter" , &testcount);
} 

static int ash_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode * root;
	struct dentry * root_dentry;
	struct buffer_head * bh;

	// init the superblock fields
	sb->s_magic = ASH_MAGIC;
	sb->s_blocksize = 4096;
	sb->s_op = &ash_super_operations;

	printk("phase 1 mounted \n");
	
	// read sector 0 -> the superblock sector
	bh = __bread(sb->s_bdev, 0, ASH_SECTORSIZE);

	// create the root inode
	root = ash_make_inode(sb, S_IFDIR | 0755);
	if (! root)
		return -ENOMEM;

	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	root_dentry = d_alloc_root(root);
	if (! root_dentry) {
		iput(root);
		return -ENOMEM;
	}
	
	sb->s_root = root_dentry;

	printk("all done: %d", bh->b_size);

	brelse(bh);

	return 0;
}

static int ash_get_sb(struct file_system_type *fs,
		int flags, const char *dev_name,
		void *data, struct vfsmount *mnt)
{
	return get_sb_bdev(fs, flags, dev_name, data, ash_fill_super, mnt);
}


static void ash_kill_sb(struct super_block *sb)
{
	kill_block_super(sb);
}


static struct file_system_type ash_fs_type = {
	.owner	= THIS_MODULE,
	.name 	= "ash",
	.get_sb 	= ash_get_sb,
	.kill_sb	= ash_kill_sb,
};


static int __init init_ash_fs(void)
{
	return register_filesystem(&ash_fs_type);
}

static void __exit exit_ash_fs(void)
{
	unregister_filesystem(&ash_fs_type);
}

module_init(init_ash_fs);
module_exit(exit_ash_fs);
	
MODULE_DESCRIPTION("Ash File System");
MODULE_AUTHOR("DB.GS");
MODULE_LICENSE("MIT");
