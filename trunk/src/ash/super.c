/*
 * Ash File System
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENCE'
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#define ASH_MAGIC	0x123456


static int ash_fill_super(struct super_block *sb, void *data, int silent)
{
	static struct tree_descr files[] = {{""}};
	return simple_fill_super(sb, ASH_MAGIC, files);
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
	.fs_flags	= FS_REQUIRES_DEV,
	.get_sb 	= ash_get_sb,
	.kill_sb	= ash_kill_sb,
};

static struct super_operations ash_super_operations = {
	/*
	.alloc_inode		= ash_alloc_inode,
	.destroy_inode	= ash_destroy_inode,
	.dirty_inode		= ash_dirty_inode,
	.write_inode		= ash_write_inode,
	.delete_inode		= ash_delete_inode,
	.put_super		= ash_put_super,
	.write_super		= ash_write_super,
	.sync_fs		= ash_sync_fs,
	.statfs		= ash_statfs,
	.remount_fs		= ash_remount_fs,
	*/
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
