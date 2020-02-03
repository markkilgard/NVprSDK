
// read_file.cpp - portable routines for reading a file into memory

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "read_file.hpp"

static char *read_file(const char *filename, const char *open_mode, long *file_size)
{
#if defined(_WIN32)
    struct _stat f_stat;
    #define statfunc _stat
#else
    struct stat f_stat;
    #define statfunc stat
#endif

    if (!filename) 
        return 0;

    if (statfunc(filename, &f_stat) == 0) {
        FILE *fp = 0;
        if (!(fp = fopen(filename, open_mode))) {
            fprintf(stderr,"Cannot open \"%s\" for read!\n", filename);
            return 0;
        }

        long size = f_stat.st_size;
        char * buf = new char[size+1];

        size_t bytes;
        bytes = fread(buf, 1, size, fp);

        buf[bytes] = 0;

        fclose(fp);
        *file_size = size;
        return buf;
    }

    fprintf(stderr,"Cannot open \"%s\" for stat read!\n", filename);

    return 0;
}

char *read_text_file(const char *filename)
{
  long ignored;
  return read_file(filename, "r", &ignored);
}

char *read_binary_file(const char *filename, long *file_size)
{
  return read_file(filename, "rb", file_size);
}