/*
 * AshFS Testing routine
 *
 * Created by:
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


/*
 * Subtracts the t1 timeval from t2 timeval and puts the result into res
 * used to find out how much time in seconds and microseconds has passed from t1 until t2.
 */
void ElapsedTime (res, t1, t2)
	struct timeval *res, *t1, *t2;
{
	res->tv_sec = t2->tv_sec - t1->tv_sec;
	
	if (t1->tv_usec > t2->tv_usec) {
		res->tv_usec = 1000000 + t2->tv_usec - t1->tv_usec;
		res->tv_sec -= 1;
	} else
		res->tv_usec = t2->tv_usec - t1->tv_usec;
}


// used to hold time results for a speed test: average write time
// and average read time
struct timeresult {
	double avg_write;
	double avg_read;
};



/*
 * Runs a series of times tests, each test:
 * - creates a file					<---  timestamp
 * - writes size bytes to it
 * - closes the file				<--- timestamp
 *
 * - opens the file					<--- timestamp
 * - reads all of it
 * - closes the file				<--- timestamp
 
 * - erases the file
 *
 * The average time for write operation and read operation is returned in struct timeresult
 *
 */
struct timeresult SpeedTest (char *filename, size_t size, int times) {
	struct timeval start, stop, time;
	struct timeresult tr;
	int f, i;
	size_t s, l;
	
	// making a filled buffer
	char buf[512];
	
	for (i = 0; i < 512; i+=8)
		strcpy(buf+i, "deadtash");
		
	// init results to 0
	tr.avg_write = 0;
	tr.avg_read = 0;
	
	double tests = times;	// need to divide each test's time by times to get average

	for (i = 0; i < times; i++) {

// WRITE TEST
		// timestamp of start test
		gettimeofday(&start, NULL);
		
		f = open (filename, O_CREAT | O_RDWR);
		if (f<0) {
			printf("open error for '%s'\n", filename);
			return;
		}
		
		// writing to the file
		for (s = 0; s < size; s+=512) {
			l = write (f, buf, 512);
			
			if (l<0) {
				printf("write failed @ %d\n", s);
				return;
			}
		}
		
		// last piece
		if (s < size) {
			l = write (f, buf, size - s);
			
			if (l<0) {
				printf("write failed @ %d\n", s);
				return;
			}
		}
		
		// makesure the file is synced (forcing writes)
		fsync(f);
		
		// close file
		close(f);

		// timestamp of end test
		gettimeofday(&stop, NULL);

		// compute difference
		ElapsedTime(&time, &start, &stop);
		
		// add results to timeresults
		tr.avg_write += time.tv_sec * 1000000 / tests;		// adds the seconds
		tr.avg_write += time.tv_usec / tests;				// adds the microseconds
		
// READ TEST
		// timestamp of start test
		gettimeofday(&start, NULL);
		
		f = open (filename, O_RDONLY);
		if (f<0) {
			printf("open error for %s\n", filename);
			return;
		}
		
		// reading from the file
		for (s = 0; s < size; s+=512) {
			l = read (f, buf, 512);
			
			if (l<0) {
				printf("read failed @ %d\n", s);
				return;
			}
		}
		
		// last piece
		if (s < size) {
			l = read (f, buf, size - s);
			
			if (l<0) {
				printf("write failed @ %d\n", s);
				return;
			}
		}
		
		// close file
		close(f);

		// timestamp of end test
		gettimeofday(&stop, NULL);

		// compute difference
		ElapsedTime(&time, &start, &stop);
		
		// add results to timeresults
		tr.avg_read += time.tv_sec * 1000000 / tests;		// adds the seconds
		tr.avg_read += time.tv_usec / tests;				// adds the microseconds


		// try erasing the file
		if (unlink(filename) < 0) {
			printf("unlink error for %s\n", filename);
			return;
		}
		
		printf(".");
		fflush(stdout);
	}
	
	return tr;
}


int main(int argc, char **argv) {
	
	// testing arguments
	if (argc != 3) {
		printf("\n\tTASH - AshFS Testing utility\n\n");
		printf("\t./tash <inputfile> <outputfile>\n\n");
	
		return 1;
	}

	// open the input file and output file
	FILE* fin = fopen(argv[1], "r");
	FILE* fout = fopen(argv[2], "w");
	
	// test if opening went ok
	if (!fin || !fout) {
		printf("bad filename arguments\n");
		return 1;
	}
	
	// read the input file
	char *filename = NULL;
	size_t n = 0;
	ssize_t len;
	
	// first line of inputfile should have a temp file name on it (with the path to the file on the device, too)
	// this file will be used to write to/read from , in order to test the filesystem speed on the device
	len = getline(&filename, &n, fin);
	filename[len-1]='\0';

	int tests = -1, i;
	
	// next line contains number of lines that will follow next, each line having a number and a size
	// valid input lines have <number of measurements> <size of the test file>, separated by a space.
	fscanf(fin, "%d", &tests);
	
	if (tests < 0) {
		printf("input file has wrong format\n");
		return 1;
	}
	
	// run each test
	for (i = 0; i < tests; i++) {
		struct timeresult tr;
	
		size_t size = -1;
		int number = -1;
		
		// read from the input file a line for this test
		fscanf(fin, "%d %d", &number, &size);
		
		if (size < 0 || number < 0) {
			printf("input file has wrong format\n");
			return 1;
		}
		
		printf("test #%d size: %d  times %d", i+1, size, number);
		fflush(stdout);
		
		// run the test
		tr = SpeedTest(filename, size, number);

		// write to console
		printf("\nwrite= %.0f read= %.0f\n\n", tr.avg_write, tr.avg_read);

		// write to output file
		fprintf(fout, "%.0f %.0f\n", tr.avg_write, tr.avg_read);
	}
	
	fclose(fin);
	fclose(fout);
	
	return 0;
}
