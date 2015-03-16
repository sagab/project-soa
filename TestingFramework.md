# Introduction #

We need a special in-house system for testing an USB flash file system, therefore I will not be using any existing tools, but rather design our own one to handle typical tests that should make us aware of whether the system works or not. For now, speed is not the main reason of concern.

Also, another reason for not beeing able to use automated tests suites is the fact that we are designing code which runs in both kernel and user space. Any bug or mistake will probably have a negative effect on the kernel and shut down everything or block the system. Therefore, a different means of testing is necessary.

# Functionality Tests #
There should be a lot of functionality tests involved, but because there is always a risk involved in crashing the linux kernel, tests will not be devised to run automatically. Rather, TASH will be a command line tool that will receive enough parameters and the type of test to run. If this is achieved, we can then script it through bash and redirect output in a file used to collect results.

TASH will require the linux modules to be loaded, but it yet unknown whether I will design it for automatic loading, or to assume everything is ready for testing. Probably I will choose to load everything, just in case we need to automate running TASH.

Functionality tests include all the components of AshFS, therefore, it will have tests for:
  * mounting file system
  * reading files
  * writing files
  * deleting files
  * making links to files
  * cripting files
  * compressing files
  * garbage collecting
  * fixing a _broken_ file.

Probably the most complex tests are the last 3 ones. The main idea of the tests is to check every step of the code to assure it does what the specifications ask for it.

The error fixing part will be using a CRC32 and Hamming distance algorithm, and will simulate reading a file with errors from the filesystem. Whether this will actually be the case, or we will have to engineer the data loss in the file system ourselves, it remains to be seen. For now, it should be enough just to test the routines that handle that stage.

# Speed Tests #
Speed testing should be designed after we have the complete design and implementation of the filesystem, because we will know exactly what hasn't been implemented as best as possible and be able to make some tests that exploit that weakness.

At this stage, though, speed testing should try to exploit the file size: I propose a test that will determine the time lapse for random generated files of these sizes, obviously a geometrical progression, but they model the usual encountered file sizes in a normal user environment pretty well:
  * 1 KB
  * 10 KB
  * 100 KB
  * 1 MB
  * 10 MB

The test would be based on cycles of:
  * write operation
  * read operation
  * delete operation

Each cycle would have 100 operations of each type, as a round looking figure. This method of testing should be scripted and have all outputs collected in a simple text file that looks like a matrix. From that, results can be ported into Excel and charts generated.

After knowing the details of the design, I can implement more advanced speed tests, that can exploit the garbage collecting properties, or a certain pattern of file writes and deletes that should maximise the time needed to clean up redundant data. Because an USB has a rather constant read/write access speed, the speed of file access will depend on the algorithms used.

Lastly, speed will definetely be put to a hard test in compressing hard to process information such as JPEGs, MPGs and other formats which are already compressed to the maximum, and ZIP compression algorithms will have a hard time.

The hardest test would be activating both compression and crypting on a file large enough to stress the system.

# Results Presentations #
The results of these tests will be a:
  * YES/NO result as in yes, it didn't crash the kernel - so something works... or no, it crashed the kernel, so it is still wrong (this is used mainly for our internal debug work)
  * speed measured in miliseconds or nanoseconds (if that's the case) for the length of the test
  * Excel charts with the evolution of speed when changing different characteristics
  * comparison between results of these tests using AshFS and running the same tests on a different filesystem, already implemented.
  * it may or may not be possible to test burning out an USB stick through repeated use. If so, we will pick a rather unexpensive model and run the same tests through our filesystem and another filesystem, observing how many cycles it took to destroy that part of the memory through write cycles.