# Introduction #

This document will discuss coding styles and offer information about the project relating the processes involved in coding.

# Technologies #
We will be using the **C language** as defined in GNU standards http://www.gnu.org/software/libc/ and compile it with **GCC 4.2.3** under Linux Kernel 2.6.24-19-generic.

I will be developing under Kubuntu 8.04.1 i386 which is built on that kernel.

# Coding Style #
Best choice of the moment is **Linux kernel coding style**, found at http://lxr.linux.no/linux/Documentation/CodingStyle.

There are a few small changes to it which I would like to experiment with:
  1. avoid using any macros apart from constants: macros tend to complicate the readability of the code.
  1. this coding style suggests 8 whitespace tabs, while others suggest using whitespace instead of tabbing. From my experience, 8 whitespace make it hell to read code in a virtual machine window. I propose to try for 4 whitespaces tabs. Need to work on this as we code.

### Naming Conventions ###
This will be very important but at this stage we still cannot give a list of filenames and function signatures. As naming conventions, I will have to set some basic guidance as to what the project will look like:
  * module and file names have to be short, lowercase, without separators such as underscore, and be suggestive of its function/use. Good example `blockman.c`. Bad example `Block_Man.c`.
  * function names can adopt either a Java style convention or a C convention, but never a C++ one. Good example: `check_error()`, `checkError()`. Bad example: `CheckError()`
  * do not insert numbers into function names unless it's part of the algorithm name (such as MD5, CRC16, etc.)
  * variable names should be short, but not too cryptic. Good example: `int tmp2`. Bad example: `int tgy_pvf`. If it cannot be avoided, insert more letters into the name.

### Spacing and constructs ###
Functions will have paranthesis according to this style:
```
int function()
{
...
}
```

Special constructs will compress those lines between keywords, like this:
```
if (condition)
{
...
} else {
...
}
```

The rest of the spacing conventions should be according to kernel coding style.

### Commenting ###
Apart from usual documentation system used for functions, parameters, etc, we will also use internal comments where it is needed, to explain what the next block of code does. Since those comments should not be over-detailing the code, multiple line commenting style is not needed, and comments should look like this, if they do not fit in only 1 line:
```
int big_function() {
...
      // size of array
      // will change, every time a new line is read
      int sz;
...
}
```


# Folder Hierarchy #
Each module should reside in his own separate folder, for now, as we are not aware of how many source files there will be present in each one:

| 1. | Block Manager: | /src/block |
|:---|:---------------|:-----------|
| 2. | Garbage Collector: | /src/garb |
| 3. | Wear Leveler: | /src/wear |
| 4. | Error Fixer: | /src/errfix |
| 5. | Compression System: | /src/zipper |
| 6. | Security System: | /src/sec |
| 7. | Testbench for Ash File System: | /src/tash |

The basic documentation will contain several files, all in the same folder /svn/trunk/docs (http://project-soa.googlecode.com/svn/trunk/docs/). The basic docs contain the project specs, article and project phases that result in document deliverables. Apart from those, the docs folder will also have the source code docs, which will be regarded as a separate hierarchy, because it will be generated in an automatic way, from sources, rather than beeing edited like the rest of these documents.

If a module will be reduced to having just 1 source file, it will be taken into /src and the folder removed. For now, we are allowing a certain degree of complexity for the modules, therefore assigning each of them a separate folder.

# Documenting Sources #
For documenting source code, best option seems to use **doxygen** system, after comparing it to the other available options: http://en.wikipedia.org/wiki/Comparison_of_documentation_generators.

We will be using the latest doxygen release version 1.5.7.1, from 5 October 2008.

Each module will be documented separately by its author in the source code, then the documentation will be generated in form of online HTML pages, stored in separate folders into /svn/trunk/docs bearing the same name as the module folder.