#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/* reads a binary file into memory */
void *read_file(char *name, size_t *length) {
    void *mem = NULL;
    FILE *fh = NULL;
    struct stat s;
    size_t len;

    if (!stat(name, &s)) {
        if (S_ISDIR(s.st_mode)) {
            errno = EISDIR;
        }
        else {
            len = (size_t) s.st_size;
            if ((mem = malloc(len)) &&
                (fh = fopen(name, "rb")) &&
                fread(mem, 1, len, fh) == len)
            {
                if (length) *length = len;
                fclose(fh);
                return mem;
            }
        }
    }

    if (mem) free(mem);
    if (fh) fclose(fh);
    perror(name);
    return NULL;
}

/* writes binary data to disk */
int write_file(char *name, void *mem, size_t length) {
    FILE *fh;
    int ok = 0;
    if ((fh = fopen(name, "wb"))) {
        ok = (fwrite(mem, 1, length, fh) == length);
        fclose(fh);
    }
    if (!ok) {
        perror(name);
    }
    return ok;
}
