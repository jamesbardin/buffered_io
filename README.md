Our simple IO61 library performs I/O on files.


- cat61 reads a file sequentially one byte at a time and writes it to standard output.
- blockcat61 is like cat61, but reads the file sequentially one block at a time. Its -b BLOCKSIZE argument sets the block size.
- randblockcat61 is like blockcat61, but it chooses a random block size for each read and write. It still reads and writes sequentially (only the block sizes are random). The -b argument is the maximum possible block size. This may very well stress your caching solution.
- reverse61 reads a file a byte at a time in reverse and writes the result to standard output.
- stride61 reads its input file using a strided access pattern, writing the data it reads sequentially. By strided, we mean that it begins at offset 0, reads BLOCKSIZE bytes, seeks to position STRIDE, reads another block, seeks to position (2 * STRIDE), etc. When it gets to the end of the file, it jumps back to the first bytes not yet read and repeats the pattern. The -b and -s arguments set the block size and stride length, respectively.
reordercat61 reads its input file one block at a time in random order and writes each block to the output file at the correct position. The result is a copy of the input file, but created non-sequentially.
- scattergather61 can take two or more input files and two or more output files. It reads blocks (or lines) from the input files in round-robin order, and writes them to the output files in round-robin order. This will test your ability to handle multiple input and output files.
