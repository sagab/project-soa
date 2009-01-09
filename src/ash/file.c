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
#include <linux/mm.h>

struct address_space_operations ash_aops = {
	.readpage		= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end		= simple_write_end,
};


struct file_operations ash_file_operations = {
	.read		= do_sync_read,
	.aio_read	= generic_file_aio_read,
	.write		= do_sync_write,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
	.fsync		= simple_sync_file,
	.llseek		= generic_file_llseek,
};
