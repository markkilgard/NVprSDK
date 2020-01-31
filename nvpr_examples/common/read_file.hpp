
// read_file.hpp - portable routines for reading a file into memory

#ifndef __read_file_hpp__
#define __read_file_hpp__

extern char *read_text_file(const char *filename);
extern char *read_binary_file(const char *filename, long *file_size);

#endif  // __read_file_hpp__
