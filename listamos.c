/* listamos: prints AMOS source code as ASCII */
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include "fileio.c"
#include "amoslib.c"

struct AMOS_token *table[AMOS_TOKEN_TABLE_SIZE] = { NULL };
char extensions_loaded[AMOS_EXTENSION_SLOTS + 1] = { 0 };

void read_extension_file(char *filename, int slot) {
    uint8_t *buf;
    size_t len;

    if ((buf = read_file(filename, &len))) {
        if (slot == -1) {
            slot = AMOS_find_slot(buf, len);
        }
        if (slot == -1) {
            fprintf(stderr, "%s: can't determine extension slot"
                            " (use -eNN=filename to set slot NN)\n", filename);
        }
        else if (extensions_loaded[slot]) {
            fprintf(stderr, "%s: extension already loaded in slot %d\n", filename, slot);
        }
        else if (AMOS_parse_extension(buf, len, slot, 6, table)) {
            fprintf(stderr, "%s: not an AMOS extension\n", filename);
        }
        else {
            fprintf(stderr, "%s: loaded into slot %d\n", filename, slot);
            extensions_loaded[slot] = 1;
        }
        free(buf);
    }
}

int read_extensions(int argc, char *argv[]) {
    int i, slot;

    /* look for -e options */
    for (i = 1; i < argc; i++) {
        if (*argv[i] != '-') break; /* end of arg processing */
        if (!strcmp(argv[i], "-e")) {
            read_extension_file(argv[++i], -1);
        }
        else if (sscanf(argv[i], "-e%d=", &slot) == 2) {
            if (slot < 1 || slot > 25) {
                fprintf(stderr, "invalid extension slot %d (must be 1-25)\n", slot);
            }
            else {
                read_extension_file(strchr(argv[i], '=') + 1, slot);
            }
        }
        else if (argv[i][1] == 'c' || argv[i][1] == 'd') {
            i++; /* skip arg */
        }
    }
    return i;
}

/* extensions to ignore in the config file, because we already loaded them */
#define NUM_DEFAULT_EXTENSIONS 9
char *default_extensions[NUM_DEFAULT_EXTENSIONS] = {
    "Music.Lib",   "AMOSPro_Music.Lib",
    "Compact.Lib", "AMOSPro_Compact.Lib",
    "Request.Lib", "AMOSPro_Request.Lib",
    "Serial.Lib",  "AMOSPro_IOPorts.Lib",
    "" /* also ignore blank entries */
};

void read_config_extensions(int argc, char *argv[]) {
    char *dir = ".", *config = NULL, *fullname = NULL, *fp;
    char *slots[AMOS_EXTENSION_SLOTS];
    uint8_t *buf = NULL;
    size_t len;
    int i, j;

    DIR *d = NULL;
    struct dirent *de;

    /* look for -c and -d options */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') break; /* end of arg processing */
        if (argv[i][1] == 'c') config = argv[++i];
        else if (argv[i][1] == 'd') dir = argv[++i];
        else if (argv[i][1] == 'e' && !argv[i][2]) i++;
    }

    /* if no '-c' flag, do nothing */
    if (!config) goto error_handler;

    /* open '-d' directory */
    if (!(d = opendir(dir))) {
        perror(dir);
        goto error_handler;
    }

    /* read '-c' config file */
    if (!(buf = read_file(config, &len))) {
        goto error_handler;
    }
    if (AMOS_parse_config(buf, len, slots)) {
        fprintf(stderr, "%s: not an AMOS config file\n", config);
        goto error_handler;
    }

    /* reduce the extension names to just NULL or a filename,
     * and remove default extensions */
    for (i = 0; i < AMOS_EXTENSION_SLOTS; i++) {
        if (slots[i] && *slots[i]) {
            char *fname = strrchr(slots[i], '/');
            if (fname) slots[i] = fname + 1;
            /* ignore extensions loaded by default */
            for (j = 0; j < NUM_DEFAULT_EXTENSIONS; j++) {
                if (!strcasecmp(slots[i], default_extensions[j])) {
                    slots[i] = NULL;
                    break;
                }
            }
        }
        else {
            slots[i] = NULL;
        }
    }

    /* allocate space for dir + "/" + fname, pre-fill dir + "/" */
    fullname = malloc(strlen(dir) + 257);
    if (!fullname) goto error_handler;
    strcpy(fullname, dir);
    fp = &fullname[strlen(dir)];
    *fp++ = '/';

    /* find files in the directory that are listed as extensions */
    while ((de = readdir(d))) {
        /* compare each extension's filename to the current dir entry */
        for (i = 0; i < AMOS_EXTENSION_SLOTS; i++) {
            if (!slots[i] || strcasecmp(de->d_name, slots[i])) continue;
            /* build the full path and load extension */
            strncpy(fp, de->d_name, 256);
            read_extension_file(fullname, i + 1);
            slots[i] = NULL;
        }
    }

    /* complain about any listed extensions not loaded */
    for (i = 0; i < AMOS_EXTENSION_SLOTS; i++) {
        if (slots[i]) fprintf(stderr, "%s/%s: no such file\n", dir, slots[i]);
    }

 error_handler:
    if (d) closedir(d);
    free(buf);
    free(fullname);
}

/* read tokens for base language and default extensions */
#include "extensions/00_base.h"
#include "extensions/01_music.h"
#include "extensions/02_compact.h"
#include "extensions/03_request.h"
#include "extensions/06_ioports.h"
void read_default_extensions() {
    AMOS_parse_extension(ext00_base, sizeof(ext00_base), 0, -194, table);
    if (!extensions_loaded[1]) {
        AMOS_parse_extension(ext01_music, sizeof(ext01_music), 1, 6, table);
    }
    if (!extensions_loaded[2]) {
        AMOS_parse_extension(ext02_compact, sizeof(ext02_compact), 2, 6, table);
    }
    if (!extensions_loaded[3]) {
        AMOS_parse_extension(ext03_request, sizeof(ext03_request), 3, 6, table);
    }
    if (!extensions_loaded[6]) {
        AMOS_parse_extension(ext06_ioports, sizeof(ext06_ioports), 6, 6, table);
    }
}


int main(int argc, char *argv[]) {
    uint8_t *buf;
    size_t len;
    int i;

    if (argc < 2) {
	printf("List AMOS source code as plain text.\n");
        printf("Usage: %s [options] <Filename.AMOS>\n", argv[0]);
        printf("Options:\n"
               "  -e extension     use this extension in its default slot\n"
               "  -eN=extension    use this extension in slot N\n"
               "  -c config        read extension filenames from config\n"
               "  -d dir           ...and extension files are in this dir\n");
        return 0;
    }

    i = read_extensions(argc, argv);
    read_config_extensions(argc, argv);
    read_default_extensions();

    /* list source file(s) */
    for (; i < argc; i++) {
        if ((buf = read_file(argv[i], &len))) {
            if (len >= 34 && amos_leek(buf) == 0x414D4F53) {
                uint32_t srclen = amos_leek(&buf[16]);
                if (srclen > (len - 20)) srclen = len - 20;
                int err = AMOS_print_source(&buf[20], srclen, stdout, table);
                if (err & 4) {
                    fprintf(stderr, "%s: missing extension instruction(s) - "
                            "are the correct extensions loaded?\n", argv[i]);
                }
            }
            else {
                fprintf(stderr, "%s: not an AMOS source file\n", argv[i]);
            }
            free(buf);
        }
    }
        
    AMOS_free_tokens(table);
    return 0;
}
