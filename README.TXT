I took Michael Bate's code and replace all IO and MEMORY requests with OS/2
calls.	Notice how it ain't true C any more.   Most of his comments are left
in tack.  Code has been compiled with MSC 5.1  use should use either the
large memory switch (/AL) when compiling) or something to use far pointers
(i.e. /Alfw).

If you have any questions please feel free to call me at 919-846-2014

						       Joe McVerry



--- this is Michael Bate's read.me file ----------------------------------



This is C source code to read a dBASE III .dbf file.  The program as it stands
(which reads and lists a .dbf file) is not very interesting, but the
source code should be of interest to any C programmer working with dBASE
III files.

I got this off the net.

							Michael Bate
