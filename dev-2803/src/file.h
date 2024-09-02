#ifndef FILE_H
#define FILE_H

#include <ftw.h>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int fcheck(const char* src);
int fcpy(const char* source, const char* destination);
int fcat(const char* src);
int fclear(const char* dir);

#endif