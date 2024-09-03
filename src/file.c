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

    src = fopen(source, "rb");
    if (src == NULL){
        perror("source file failed to open");
        return -1;
    }

    dest = fopen(destination, "wb");
    if (dest == NULL){
        perror("destination file failed to open");
        fclose(src);
        return -1;
    }

    while((ch = fgetc(src)) != EOF) fputc(ch, dest);
    fclose(src);
    fclose(dest);
    return 0;
}

int fcat(const char* src){
    char buffer[BUFFER_LINE_MAX];
    FILE* file = fopen(src,"r");
	int lines = 0;
	if(file == NULL){
		perror("error opening file");
		return -1;
	}
	while(fgets(buffer,sizeof(buffer),file)){
		printf("%i : %s",lines,buffer);
		lines++;
		if (lines%30 == 0) {
            printf("Continue? (y/n) ");
            int c = getchar();
            while ((c != '\n') && (getchar() != '\n')); // clear input buffer
            if (c == 'n' || c == 'N') break;
        }
	}
	printf("\n");
	fclose(file);
    return 0;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    return remove(fpath);
}

// orange you glad i didnt use this ! :) :)

/*int fclear(const char* dir){
    char command[64];
    snprintf(command, sizeof(command), "rm -rf %s", dir);
    system(command);
    return 0;
}*/