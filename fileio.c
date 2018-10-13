#include <stdio.h>
#include <stdlib.h>

/* reads a binary file into memory */
void *read_file(char *name, size_t *length) {
    void *mem = NULL;
    long len = 0;
    FILE *fh;

    if ((fh = fopen(name, "rb"))) {
        if (!fseek(fh, 0, SEEK_END) && (len = ftell(fh)) > 0 && len < 1<<30 &&
            !fseek(fh, 0, SEEK_SET) && (mem = malloc(len)))
        {
            if (fread(mem, 1, len, fh) == (size_t) len) {
                if (length) *length = len;
            }
            else {
                free(mem);
                mem = NULL;
            }
        }
        fclose(fh);
    }
    if (!mem) {
	if (!len) {
	    fprintf(stderr, "%s: empty file\n", name);
	}
	else if (len > 1<<30) {
	    /* linux: fopen() succeeds, fseek() succeeds, ftell() succeeds and
	     * returns the largest positive integer because name is a directory */
	    fprintf(stderr, "%s: is a directory\n", name);
	}
	else {
	    perror(name);
	}
    }
    return mem;
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
