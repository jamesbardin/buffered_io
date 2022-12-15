#include "io61.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <climits>
#include <cerrno>

// io61.cc
//    YOUR CODE HERE!


// io61_file
//    Data structure for io61 file wrappers. Add your own stuff.

struct io61_file {
    int fd = -1;     // file descriptor
    int mode;
    // from section notes
    static constexpr off_t bufsize = 4096; // block size for this cache
    unsigned char cbuf[bufsize];
    // These “tags” are addresses—file offsets—that describe the cache’s contents.
    off_t tag;      // file offset of first byte in cache (0 when file is opened)
    off_t end_tag;  // file offset one past last valid byte in cache
    off_t pos_tag;  // file offset of next char to read in cache
};


// io61_fdopen(fd, mode)
//    Returns a new io61_file for file descriptor `fd`. `mode` is either
//    O_RDONLY for a read-only file or O_WRONLY for a write-only file.
//    You need not support read/write files.

io61_file* io61_fdopen(int fd, int mode) {
    assert(fd >= 0);
    io61_file* f = new io61_file;
    f->fd = fd;
    f->mode = mode;
    return f;
}


// io61_close(f)
//    Closes the io61_file `f` and releases all its resources.

int io61_close(io61_file* f) {
    io61_flush(f);
    int r = close(f->fd);
    delete f;
    return r;
}

// (Based on fill from section)
    // Fill the read cache with new data, starting from file offset `end_tag`.
    // Only called for read caches.

int io61_fill(io61_file* f) {
    // cache cleared
    f->tag = f->end_tag;
    f->pos_tag = f->end_tag;

    if (f->mode != O_RDONLY) {
        return -1;
    }

    ssize_t nr = read(f->fd, f->cbuf, f->bufsize);
    if (nr >= 0) {
        f->end_tag = f->tag + nr; 
    }
    return nr;
}


// io61_readc(f)
//    Reads a single (unsigned) byte from `f` and returns it. Returns EOF,
//    which equals -1, on end of file or error.


int io61_readc(io61_file* f) {
    // Check invariants.
    assert(f->tag <= f->pos_tag && f->pos_tag <= f->end_tag);
    assert(f->end_tag - f->pos_tag <= f->bufsize);

    if (f->pos_tag == f->end_tag) {
        if (io61_fill(f) == -1 || f->pos_tag == f->end_tag) {
            return -1;
        }
    }
    unsigned char c = f->cbuf[f->pos_tag - f->tag];
    ++f->pos_tag;
    return c;
}


// io61_read(f, buf, sz)
//    Reads up to `sz` bytes from `f` into `buf`. Returns the number of
//    bytes read on success. Returns 0 if end-of-file is encountered before
//    any bytes are read, and -1 if an error is encountered before any
//    bytes are read.
//
//    Note that the return value might be positive, but less than `sz`,
//    if end-of-file or error is encountered before all `sz` bytes are read.
//    This is called a “short read.”

ssize_t io61_read(io61_file* f, unsigned char* buf, size_t sz) {
    // Check invariants.
    assert(f->tag <= f->pos_tag && f->pos_tag <= f->end_tag);
    assert(f->end_tag - f->pos_tag <= f->bufsize);
    // number bytes read
    int nr = 0;
    // count for memcpy
    int cp_ct;
    while (nr < (int) sz) {
        // if cache full
        if (f->pos_tag == f->end_tag) {
            int check = io61_fill(f);
            if (check == 0) {
                break;
            }
            if (check == -1) {
                return -1;
            }
        }
        // check how many bytes there are left to be read
        if ((sz - nr) >= (size_t) (f->end_tag - f->pos_tag)) {
            cp_ct = f->end_tag - f->pos_tag;
        }
        else {
            cp_ct = sz - nr;
        }

        memcpy(&buf[nr], &f->cbuf[f->pos_tag - f->tag], cp_ct);
        nr += cp_ct;
        f->pos_tag += cp_ct;
    }
    return nr;  
}

// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success and
//    -1 on error.

int io61_writec(io61_file* f, int ch) {
    // Check invariants.
    assert(f->tag <= f->pos_tag && f->pos_tag <= f->end_tag);
    assert(f->end_tag - f->pos_tag <= f->bufsize);
    // flush if cache full
    if (f->end_tag - f->tag == f->bufsize){
        int r = io61_flush(f);
        if (r < 0) {
            return -1;
        }
    }
    f->cbuf[f->pos_tag - f->tag] = ch;
    ++f->pos_tag;
    f->end_tag = f->pos_tag;
    return 0;
}

// io61_write(f, buf, sz)
//    Writes `sz` characters from `buf` to `f`. Returns `sz` on success.
//    Can write fewer than `sz` characters when there is an error, such as
//    a drive running out of space. In this case io61_write returns the
//    number of characters written, or -1 if no characters were written
//    before the error occurred.

ssize_t io61_write(io61_file* f, const unsigned char* buf, size_t sz) {
    // Check invariants.
    assert(f->tag <= f->pos_tag && f->pos_tag <= f->end_tag);
    assert(f->end_tag - f->pos_tag <= f->bufsize);
    assert(f->pos_tag == f->end_tag);
    // number bytes written
    int nwr = 0;
    // count for memcpy
    int cp_ct;
    while (nwr < (int) sz) {
        // flush if cache full
        if (f->pos_tag - f->tag == f->bufsize) {
            io61_flush(f);
        }
        // check how many bytes are left to be written
        if ( (sz - (size_t) nwr) >= (size_t) (f->tag + f->bufsize - f->pos_tag) ) {
            cp_ct = f->tag + f->bufsize - f->pos_tag;
        }
        else {
            cp_ct = sz - nwr;
        }

        memcpy(&f->cbuf[f->pos_tag - f->tag], &buf[nwr], cp_ct);
        nwr += cp_ct; 
        f->end_tag += cp_ct;
        f->pos_tag += cp_ct;
    }
    return nwr;
}

// io61_flush(f)
//    Forces a write of any cached data written to `f`. Returns 0 on
//    success. Returns -1 if an error is encountered before all cached
//    data was written.
//
//    If `f` was opened read-only, `io61_flush(f)` returns 0. If may also
//    drop any data cached for reading.

int io61_flush(io61_file* f) {
    if (f->mode == O_WRONLY) {
        // Check invariants.
        assert(f->tag <= f->pos_tag && f->pos_tag <= f->end_tag);
        assert(f->end_tag - f->pos_tag <= f->bufsize);
        assert(f->pos_tag == f->end_tag);

        ssize_t c = write(f->fd, f->cbuf, f->pos_tag - f->tag);
        f->tag = f->pos_tag;
        if (c == -1) {
            return -1;
        }
        else {
            return 0;
        }
    }
    else if (f->mode == O_RDONLY) {
        return 0;
    }
    else {
        return -1;
    }
}

// io61_seek(f, pos)
//    Changes the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.

int io61_seek(io61_file* f, off_t pos) {
    if (f->mode == O_WRONLY) {
        // if data can't be written
        if (io61_flush(f) == -1) {
            return -1;
        }
        off_t r = lseek(f->fd, pos, SEEK_SET);
        if (r == pos) {
            f->pos_tag = pos;
            f->end_tag = pos;
            f->tag = pos;
            return 0; 
        }
        else {
            return -1;
        }        
    }
    else if (f->mode == O_RDONLY) {
        // if in cache no need to call lseek
        if (pos < f->end_tag && pos >= f->tag) {
            f->pos_tag = pos;
            return 0;
        }
        else {
            // calc start position
            off_t st_pos = (pos / f->bufsize) * f->bufsize;
            off_t s = lseek(f->fd, st_pos, SEEK_SET);
            if (s == st_pos) {
                f->pos_tag = st_pos;
                f->end_tag = st_pos;
                f->tag = st_pos;
                io61_fill(f); 
                f->pos_tag = pos;
                return 0; 
            }
            else {
                return -1;
            }
        }
    }
    else {
        return -1;
    }
}

// You shouldn't need to change these functions.

// io61_open_check(filename, mode)
//    Opens the file corresponding to `filename` and returns its io61_file.
//    If `!filename`, returns either the standard input or the
//    standard output, depending on `mode`. Exits with an error message if
//    `filename != nullptr` and the named file cannot be opened.

io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    if (filename) {
        fd = open(filename, mode, 0666);
    } else if ((mode & O_ACCMODE) == O_RDONLY) {
        fd = STDIN_FILENO;
    } else {
        fd = STDOUT_FILENO;
    }
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}


// io61_fileno(f)
//    Returns the file descriptor associated with `f`.

int io61_fileno(io61_file* f) {
    return f->fd;
}


// io61_filesize(f)
//    Returns the size of `f` in bytes. Returns -1 if `f` does not have a
//    well-defined size (for instance, if it is a pipe).

off_t io61_filesize(io61_file* f) {
    struct stat s;
    int r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode)) {
        return s.st_size;
    } else {
        return -1;
    }
}
