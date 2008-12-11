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

int ash_readdir (struct file *filp, void *dirent, filldir_t filldir) {
	return 0;
}


struct file_operations ash_dir_operations = {
	.readdir	=	ash_readdir,
};

