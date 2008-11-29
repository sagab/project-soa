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

static struct super_operations ash_super_operations = {
	//.alloc_inode = ash_alloc_inode,
	//more operations to come here
};

static int ash_get_sb(struct file_system_type *fs,
		int flags, const char *dev_name,
		void *data, struct vfsmount *mnt)
{
	return 0;
}


static void ash_kill_sb(struct super_block *sb)
{

}


static struct file_system_type ash_fs_type = {
	.owner 		= THIS_MODULE,
	.name  		= "ash",
	.get_sb 	= ash_get_sb,
	.kill_sb	= ash_kill_sb,
};


static int __init init_ash_fs(void)
{
	return 0;
}

static void __exit exit_ash_fs(void)
{

}

module_init(init_ash_fs);
module_exit(exit_ash_fs);
	
MODULE_DESCRIPTION("Ash File System");
MODULE_AUTHOR("DB.GS");
MODULE_LICENSE("MIT");
