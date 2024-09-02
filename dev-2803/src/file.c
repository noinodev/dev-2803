#include "file.h"
#include <stdio.h>
#include <sys/stat.h>
#include <ftw.h>
#include <stdlib.h>

int fcheck(const char* src){
    struct stat buffer;
    return stat(src,&buffer) == 0;
}

int fcpy(const char* source, const char* destination) {
    FILE *src, *dest;
    char ch;

    if(fcheck(source) == 0){
        perror("something wrong with the source file. probably doesn't exist");
        return -1;
    }
    src = fopen(source, "rb");
    if (src == NULL) {
        perror("different error with the source file");
        return -1;
    }

    dest = fopen(destination, "wb");
    if (dest == NULL) {
        perror("different error with destination file");
        fclose(src);
        return -1;
    }

    while ((ch = fgetc(src)) != EOF) fputc(ch, dest);
    fclose(src);
    fclose(dest);
    return 0;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    return remove(fpath);
}

/*int fclear(const char* dir){ // orange you glad i didnt use this ! :)
    char command[64];
    snprintf(command, sizeof(command), "rm -rf %s", dir);
    system(command);
    return 0;
}*/