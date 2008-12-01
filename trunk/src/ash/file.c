/*
 * Ash File System
 *
 * Created by:
 * 			   Daniel Baluta  <daniel.baluta@gmail.com> 
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/dcache.h>

#define BUFFSIZE 10

int ash_file_open (struct inode *inode, struct file *filp)
{
	// at the moment, I only need to pass the private data from the inode to the file structure
	filp->private_data = inode->i_private;
	return 0;
}


ssize_t ash_file_read (struct file *filp, char *buf, size_t count, loff_t *offset)
{
	atomic_t *counter;
	int val;
	size_t len;
	char out[BUFFSIZE];

	// get counter value and increment it
	counter = (atomic_t*) filp->private_data;

	val = atomic_read(counter);

	// transform to string
	len = snprintf(out, BUFFSIZE, "%d", val);

	// seek
	if (*offset > len)
		return 0;

	len -= *offset;
	if (count > len)
		count = len;

	if (copy_to_user(buf, out, count))
		return -EFAULT;

	// adjust offset
	*offset += count;
	return count;
}


ssize_t ash_file_write (struct file *filp, const char *buf, size_t count, loff_t *offset)
{
	atomic_t *counter;
	char in[BUFFSIZE];

	if (count > BUFFSIZE)
		return -EINVAL;

	// we want the file always to start at 0
	if (*offset != 0)
		return -EINVAL;

	memset(in, 0, BUFFSIZE);
	if (copy_from_user(in, buf, count))
		return -EFAULT;

	counter = (atomic_t*) filp->private_data;
	atomic_set(counter, simple_strtol(in, NULL, 10));
	return count;
}


static struct file_operations ash_file_operations = {
	.open	= ash_file_open,
	.read	= ash_file_read,
	.write	= ash_file_write,
};


extern struct inode * ash_make_inode (struct super_block *, int);

/**
 *	Creates a new file in the filesystem
 * 
 *	@sb: superblock
 *	@dir: dentry for parent
 *	@name: file name
 *	@private: pointer to data that can be attached to the inode
 *
 */
struct dentry * ash_create_file (struct super_block *sb, struct dentry *dir, const char *name, void *private)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;
	
	// get the quick string info for the filename
	qname.name = name;
	qname.len = strlen(name);
	qname.hash = full_name_hash(name, qname.len);

	// create dentry
	dentry = d_alloc(dir, &qname);
	if (! dentry)
		return NULL;

	// create an inode
	inode = ash_make_inode(sb, S_IFREG | 0644);
	if (! inode) {
		dput(dentry);
		return NULL;
	}
		
	inode->i_fop = &ash_file_operations;
	inode->i_private = private;

	// add the 2 to the dentry cache
	d_add(dentry, inode);

	return dentry;
}
