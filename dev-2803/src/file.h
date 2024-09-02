#include <ftw.h>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int fcheck(const char* src);
int fcpy(const char* source, const char* destination);
int fclear(const char* dir);