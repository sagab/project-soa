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
#include <linux/spinlock.h>
#include <asm/string.h>
#include "ash.h"
#include "crypt.h"


static struct super_operations ash_super_operations = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

extern struct inode * ash_make_inode (struct super_block *, int);
extern struct file_operations ash_dir_operations;
extern struct inode_operations ash_inode_operations;

static int ash_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode * root;
	struct dentry * root_dentry;
	struct buffer_head *bh;
	struct ash_raw_superblock *rsb;
	struct ash_raw_file *rfile;
		
	// read sector 0 -> the superblock sector
	bh = __bread(sb->s_bdev, 0, ASH_SECTORSIZE);
	if (!bh) {
		printk(KERN_ERR "__bread from the device failed\n");
		return -1;
	}
	
	rsb = (struct ash_raw_superblock*)bh->b_data;
	
	// check if it's an Ash filesystem
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
	sb->s_fs_info = rsb;
	
	if (silent != 1)
		printk("Ash vers: %d volname: '%s'\n", rsb->vers, rsb->volname);

	brelse(bh);

	// create the root inode
	// read the root directory entry from the device
	bh = __bread(sb->s_bdev, rsb->datastart << rsb->blockbits >> KERNEL_BLOCKBITS, KERNEL_BLOCKSIZE);
	if (!bh) {
		printk(KERN_ERR "cannot read root directory entry\n");
		return -1;
	}
	
	rfile = (struct ash_raw_file*) (bh->b_data);
	
	// making the root
	root = ash_make_inode(sb, rfile->mode);
	if (! root) {
		brelse(bh);
		return -ENOMEM;
	}

	root->i_op = &ash_inode_operations;
	root->i_fop = &ash_dir_operations;
	
	// set private data
	root->i_private = rfile;

	root_dentry = d_alloc_root(root);
	if (! root_dentry) {
		brelse(bh);
		iput(root);
		return -ENOMEM;
	}
	
	// root has no parent
	root_dentry->d_parent = root_dentry;

	// final superblock init
	sb->s_root = root_dentry;
	sb->s_op = &ash_super_operations;

	brelse(bh);

	return 0;
	
}



/*
 * Reads a block from the drive and returns a buffer of blocksize bytes
 * @return NULL in case of an error, or a pointer to a buffer
 */
void* block_read (struct super_block *sb, uint32_t block)
{
	uint64_t bytes, kB, kO, off;
	int i, n;
	char *buf;
	struct buffer_head *bh;
	
	bytes = block << sb->s_blocksize_bits;		// the real offset on disk
	kB = bytes >> KERNEL_BLOCKBITS;			// kernel block of 4096 bytes
	kO = bytes & (KERNEL_BLOCKSIZE - 1);		// offset in the kernel block

	buf = kmalloc(sb->s_blocksize, GFP_ATOMIC);	// try to get a buffer to read in
	
	if (!buf)
		return NULL;
		
	off = 0;					// offset for copying in the buf
	
	// read start of buffer from kernel block kB
	bh = __bread(sb->s_bdev, kB, KERNEL_BLOCKSIZE);
	
	if (!bh) {
		kfree(buf);
		return NULL;
	}

	// test which one is the min
	if (sb->s_blocksize <= KERNEL_BLOCKSIZE) {
		off = sb->s_blocksize - kO;	// how many bytes we are copying
		memcpy(buf, bh->b_data, off);
	} else {
		off = KERNEL_BLOCKSIZE - kO;	// how many bytes we are copying
		memcpy(buf, bh->b_data, off);
	}
	
	brelse(bh);
	
	// now to copy an integer number of kernel blocks
	n = (sb->s_blocksize - off) >> KERNEL_BLOCKBITS;
	for (i = 0; i < n; i++) {
		bh = __bread(sb->s_bdev, kB + i, KERNEL_BLOCKSIZE);
		
		if (!bh) {
			kfree(buf);
			return NULL;
		}
			
		memcpy(buf+off, bh->b_data, KERNEL_BLOCKSIZE);
		off += KERNEL_BLOCKSIZE;
		
		brelse(bh);
	}
	
	// copy the last part of a kernel block
	// check if there's really a need to read another kernel block
	if (off < sb->s_blocksize) {
		bh = __bread(sb->s_bdev, kB + n, KERNEL_BLOCKSIZE);
		
		if (!bh) {
			kfree(buf);
			return NULL;
		}

		memcpy(buf+off, bh->b_data, sb->s_blocksize - off);
		brelse(bh);
	}
	
	// all ok
	return buf;

}



/*
 * Writes a block to the drive. The block's data is an array of blocksize bytes
 * @return 0 on success
 */
int block_write (struct super_block *sb, void *data, uint32_t block)
{
	uint64_t bytes, kB, kO, off;
	int i, n;
	struct buffer_head *bh;
	
	bytes = block << sb->s_blocksize_bits;		// the real offset on disk
	kB = bytes >> KERNEL_BLOCKBITS;			// kernel block of 4096 bytes
	kO = bytes & (KERNEL_BLOCKSIZE - 1);		// offset in the kernel block

	off = 0;					// offset for copying from data buffer
	
	// read start of buffer from kernel block kB
	bh = __getblk(sb->s_bdev, kB, KERNEL_BLOCKSIZE);
	
	if (!bh)
		return -1;
	
	// test which one is the min
	if (sb->s_blocksize <= KERNEL_BLOCKSIZE) {
		off = sb->s_blocksize - kO;		// how many bytes we are copying
		memcpy(bh->b_data, data, off);
	} else {
		off = KERNEL_BLOCKSIZE - kO;		// how many bytes we are copying
		memcpy(bh->b_data, data, off);
	}
	
	// make a request
	mark_buffer_dirty(bh);
	brelse(bh);
	
	// now to copy an integer number of kernel blocks
	n = ((uint64_t)sb->s_blocksize - off) >> KERNEL_BLOCKBITS;
	
	for (i = 0; i < n; i++) {
		bh = __getblk(sb->s_bdev, kB + i, KERNEL_BLOCKSIZE);
		
		if (!bh)
			return -1;
			
		memcpy(bh->b_data, data + off, KERNEL_BLOCKSIZE);
		off += KERNEL_BLOCKSIZE;
		
		mark_buffer_dirty(bh);
		brelse(bh);
	}

	// copy the last part of a kernel block
	// check if there's really a need to read another kernel block
	if (off < sb->s_blocksize) {
		bh = __getblk(sb->s_bdev, kB + n, KERNEL_BLOCKSIZE);
		
		if (!bh)
			return -1;
			
		memcpy(bh->b_data, data + off, sb->s_blocksize - off);
		
		mark_buffer_dirty(bh);
		brelse(bh);
	}
	
	// all ok
	return 0;
}



/*
 * Returns the first block that is available for use
 * @return block index, or -1 in case of error
 */
int block_first_free(struct super_block *sb)
{
	struct ash_raw_superblock *rsb;
	uint32_t kB, kO;
	uint64_t off, size;
	int done, i;
	struct buffer_head *bh;
	uint8_t *ubb, byte;
	
	// obtain a point to the ash_raw_superblock structure
	rsb = sb->s_fs_info;
	
	// get the kernel block and offset for UBBstart
	off = rsb->UBBstart << sb->s_blocksize_bits;
	kB = off >> KERNEL_BLOCKBITS;
	kO = off & (KERNEL_BLOCKSIZE - 1);
	
	size = rsb->UBBblocks << sb->s_blocksize_bits;
	off = 0;
	done = 0;
	byte = 0;
	
	// read all UBB
	while (!done && off < size) {
		
		// read a new block from UBB
		bh = __bread(sb->s_bdev, kB, sb->s_blocksize);
		
		if (!bh)
			return -1;
		
		// get address to KERNEL_BLOCKSIZE bytes from UBB
		ubb = (uint8_t*)bh->b_data + kO;
		
		for (i=0; i < KERNEL_BLOCKSIZE && !done; i++)
			if (ubb[i] != 0xFF) {
				done = 1;
				off += i;
				byte = ubb[i];
				break;
			}
			
		if (!done) {
			off += KERNEL_BLOCKSIZE;
			kO = 0;		// only first kernel block doesn't have data start from byte 0
		}
		
		brelse(bh);
	}

	if (off >= size)
		return 0;
	
	// find first 0 bit from byte
	i = 7;
	while (byte & (1<<i))
		i--;
		
	return off * 8 + (8 - i);
}



/*
 * Reads what value a block has in the Used Blocks Bitmap
 * @return 0 (unused), 1 (used) or -1 in case of error
 */
int UBB_read (struct super_block *sb, uint32_t block)
{
	struct ash_raw_superblock *rsb;
	uint32_t lB, lO, kB, kO;
	uint64_t off;
	uint8_t bit, byte, rez;
	struct buffer_head *bh;
	uint8_t *ubb;
	
	// obtain a point to the ash_raw_superblock structure
	rsb = sb->s_fs_info;
	
	// get the logical block in which the byte containing the bit
	// for block parameter is stored :) and logical offset
	lB = rsb->UBBstart + (block >> (3 + sb->s_blocksize_bits));
	lO = (block >> 3) & (sb->s_blocksize - 1);
	
	// kernel block to read in order to get the byte from lB, lO
	off = ( lB << sb->s_blocksize_bits ) + lO;
	kB = off >> KERNEL_BLOCKBITS;
	kO = off & (KERNEL_BLOCKSIZE - 1);
	
	bit = 8 - (block & 7);	// the bit that needs to be read
	
	// read the block from disk
	bh = __bread(sb->s_bdev, kB, KERNEL_BLOCKSIZE);
	
	// error occured
	if (!bh)
		return -1;
	
	ubb = (uint8_t*) bh->b_data;
	byte = ubb[kO];
	
	rez = (byte & (1 << bit) ) >> bit;
	
	brelse(bh);
	
	return rez;
}



/*
 * Writes the value val for a block in Used Blocks Bitmap zone
 * @return 0 on success
 */
int UBB_write (struct super_block *sb, uint32_t block, uint8_t val)
{
	struct ash_raw_superblock *rsb;
	uint32_t lB, lO, kB, kO;
	uint64_t off;
	uint8_t bit, mask;
	struct buffer_head *bh;
	uint8_t *ubb;
	
	// obtain a point to the ash_raw_superblock structure
	rsb = sb->s_fs_info;
	
	// get the logical block in which the byte containing the bit
	// for block parameter is stored :) and logical offset
	lB = rsb->UBBstart + (block >> (3 + sb->s_blocksize_bits));
	lO = (block >> 3) & (sb->s_blocksize - 1);
	
	// kernel block to read in order to get the byte from lB, lO
	off = ( lB << sb->s_blocksize_bits ) + lO;
	kB = off >> KERNEL_BLOCKBITS;
	kO = off & (KERNEL_BLOCKSIZE - 1);
	
	bit = 8 - (block & 7);	// the bit that needs to be modified
	
	// get the buffer_head from disk
	bh = __getblk(sb->s_bdev, kB, KERNEL_BLOCKSIZE);
	
	// error occured
	if (!bh)
		return -1;
	
	ubb = (uint8_t*) bh->b_data;
	mask = 1 << bit;
	
	if (val == 0)	
		ubb[kO] = ubb[kO] & (~mask);
	else
		ubb[kO] = ubb[kO] | mask;
	
	mark_buffer_dirty(bh);
	brelse(bh);
	
	return 0;
}



/*
 * Get the number of the next block of data following block from the Block Allocation Table
 * @return uint32_t number of block or -1 on error
 */
int BAT_read (struct super_block *sb, uint32_t block)
{
	struct ash_raw_superblock *rsb;
	uint32_t lB, lO, kB, kO;
	uint64_t off;
	struct buffer_head *bh;
	uint32_t *bat;
	
	// obtain a point to the ash_raw_superblock structure
	rsb = sb->s_fs_info;
	
	// get the logical block which holds the entry in the BAT
	// got the block parameter
	lB = rsb->BATstart + (block >> (sb->s_blocksize - 2));
	lO = (block << 2) & (sb->s_blocksize - 1);
	
	// kernel block to read in order to get the byte from lB, lO
	off = ( lB << sb->s_blocksize_bits ) + lO;
	kB = off >> KERNEL_BLOCKBITS;
	kO = off & (KERNEL_BLOCKSIZE - 1);
	
	// read the block from disk
	bh = __bread(sb->s_bdev, kB, KERNEL_BLOCKSIZE);
	
	// error occured
	if (!bh)
		return -1;
	
	bat = (uint32_t*)( (uint8_t*) bh->b_data + kO);
	
	brelse(bh);
	
	return bat[0];
}



/*
 * Write the number of the entry for the block in the BAT
 * @return 0 on success
 */
int BAT_write (struct super_block *sb, uint32_t block, uint32_t entry)
{

	struct ash_raw_superblock *rsb;
	uint32_t lB, lO, kB, kO;
	uint64_t off;
	struct buffer_head *bh;
	uint32_t *bat;
	
	// obtain a point to the ash_raw_superblock structure
	rsb = sb->s_fs_info;
	
	// get the logical block which holds the entry in the BAT
	// got the block parameter
	lB = rsb->BATstart + (block >> (sb->s_blocksize - 2));
	lO = (block << 2) & (sb->s_blocksize - 1);
	
	// kernel block to read in order to get the byte from lB, lO
	off = ( lB << sb->s_blocksize_bits ) + lO;
	kB = off >> KERNEL_BLOCKBITS;
	kO = off & (KERNEL_BLOCKSIZE - 1);
	
	// read the block from disk
	bh = __getblk(sb->s_bdev, kB, KERNEL_BLOCKSIZE);
	
	// error occured
	if (!bh)
		return -1;
	
	bat = (uint32_t*)( (uint8_t*) bh->b_data + kO);
	bat[0] = entry;
	
	mark_buffer_dirty(bh);
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
	.owner		= THIS_MODULE,
	.name 		= "ash",
	.get_sb 	= ash_get_sb,
	.kill_sb	= ash_kill_sb,
};


static int __init init_ash_fs(void)
{
	uint8_t *key = (uint8_t*) kmalloc (16, GFP_KERNEL);
	uint8_t *src = (uint8_t*) kmalloc (16, GFP_KERNEL);
	int i;
	
	key[0] = 0x00;
	key[1] = 0x01;
	key[2] = 0x02;
	key[3] = 0x03;
	key[4] = 0x04;
	key[5] = 0x05;
	key[6] = 0x06;
	key[7] = 0x07;
	key[8] = 0x08;
	key[9] = 0x09;
	key[10] = 0x0a;
	key[11] = 0x0b;
	key[12] = 0x0c;
	key[13] = 0x0d;
	key[14] = 0x0e;
	key[15] = 0x0f;
	
	src[0] = 0x00;
	src[1] = 0x11;
	src[2] = 0x22;
	src[3] = 0x33;
	src[4] = 0x44;
	src[5] = 0x55;
	src[6] = 0x66;
	src[7] = 0x77;
	src[8] = 0x88;
	src[9] = 0x99;
	src[10] = 0xaa;
	src[11] = 0xbb;
	src[12] = 0xcc;
	src[13] = 0xdd;
	src[14] = 0xee;
	src[15] = 0xff;
	
	printk("plain: ");
	for (i=0; i<16; i++)
		printk("%02x", src[i]);
	
	AES_crypt(src,src,16,key, 4);

	printk("\ncrypt: ");
	for (i=0; i<16; i++)
		printk("%02x", src[i]);

	AES_decrypt(src,src,16,key,4);
	
	printk("\ndecry: ");
	for (i=0; i<16; i++)
		printk("%02x", src[i]);
		
	printk("\n\n");

	kfree(key);
	kfree(src);
	
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
