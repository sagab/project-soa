#
# make tash 
#

tash: tash.o
	$(CC) -o tash tash.o
tash.o: tash.c
	$(CC) -c tash.c 

clean:
	rm -rf *.o tash
