#
# Makefile for the Ash File System
# uses an aditional Kbuild file for building the ash kernel module
#

KDIR=/lib/modules/`uname -r`/build

kbuild:
	make -C $(KDIR) M=`pwd`
	
test:
	insmod ash.ko
	mount -t ash /dev/sdb1 /mnt
	umount /mnt
	rmmod ash


clean:
	make -C $(KDIR) M=`pwd` clean
