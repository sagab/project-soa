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
	struct ash_raw_superblock *rsb;
	mode_t def;

	// read sector 0 -> the superblock sector
	bh = __bread(sb->s_bdev, 0, ASH_SECTORSIZE);
	rsb = (struct ash_raw_superblock*)bh->b_data;
	
	// check if it's a ash filesystem
	if (rsb->magic != ASH_MAGIC) {
		printk(KERN_ERR "incorrect magic number\n");
		brelse(bh);
		return -1;
	}
	
	// fill in superblock fields by using the superblock read from disk
	sb->s_blocksize = rsb->blocksize;
	sb->s_blocksize_bits = rsb->blockbits;
	sb->s_magic = rsb->magic;
	
	// setting time granularity at 1 second (it is in ns)
	sb->s_time_gran = 1000000000;
	
	// private filesystem info
	sb->s_fs_info = rsb;	// now, I wonder what happens to rsb after I brelse(bh) ^^.
	
	
	// sanity check
	printk("vers: %d volname: '%s'\n", rsb->vers, rsb->volname);


	// create the root inode
	
	// normal file permissions: owner can read/write/execute, the group and
	// others can only r/w
	def = S_IRWXU | (S_IRGRP|S_IWGRP) | (S_IROTH|S_IWOTH);
	
	// root is also a directory, so S_IFDIR
	root = ash_make_inode(sb, S_IFDIR | def);
	if (! root) {
		brelse(bh);
		return -ENOMEM;
	}

	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	root_dentry = d_alloc_root(root);
	if (! root_dentry) {
		brelse(bh);
		iput(root);
		return -ENOMEM;
	}

	// final superblock init
	sb->s_root = root_dentry;
	sb->s_op = &ash_super_operations;

	brelse(bh);
	
	if (rsb != NULL)
		printk("omg, a ramas si %d\n", rsb->vers);
	else
		printk("oops.\n");

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
	.owner		= THIS_MODULE,
	.name 		= "ash",
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
