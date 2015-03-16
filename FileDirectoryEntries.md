# Storing Data #
Data will be organised in files and directories for the time beeing, with no support for symlinks. The rootentry field in the superblock will point to the first actual block of data, which contains the root directory (check Directories section for info on what it looks like). From there, we will get an hierarchic tree.

# Files #
  * a file is represented by this data, written from the sector offset where the previous file entry ended.
  * this structure is followed by namelength bytes, which is the ASCII encoding of the filename.

  * startblock points to the block where the file's content is stored, and by using the BAT + size field, we can read the contents of the whole file.

```
struct ash_raw_file {
	__u16	mode;			// file type and access rights
	__u64	size;			// file length in bytes
	__u32	atime;			// last accessed
	__u32	wtime;			// last written
	__u32	ctime;			// created
	__u32	startblock;		// reference to both BAT and actual data block where file's data is stored
	__u16	namelength;		// how long is the filename
};
```


# Directories #

Directories will be a bit different than files: they will start with
the same header, but instead of file data, at the startblock we will find additional ash\_raw\_file entries with the files in the directory.

**the field filesize will be used to figure out how many files there are in the directory's data block(s).**

There is a special thing to consider, though. Say we have 1000 files in the root, one of them a directory "spells" with 1000 files. In "spells", there's a subdirectory called "fire". A command "cd /spells/fire" will have to go through all 1000 entries in root directory to find "spells", then go through all 1000 entries in spells to find "fire" to view it's contents and create the dentries in memory. This is a problem which can be solved by using a small 512-entries hashtable for a directory which contains more than 100 files in it.

Each hashtable entry contains a reference to the first file entry with that hash value, and the reference can be given as a block number and block offset in bytes, which is 32 + 12 bits = 6 bytes of data per entry.

**For now, the hashtable design remains Future Work implementation**