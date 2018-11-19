/* dumpamos: extracts AMOS source files and AMOS bank files */
#include "fileio.c"
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>

/* read 16-bit big-endian word from uint8_t[] */
#define amos_deek(a) ((((a)[0])<<8)|((a)[1]))

/* read 32-bit big-endian word from uint8_t[] */
#define amos_leek(a) ((((a)[0])<<24)|(((a)[1])<<16)|(((a)[2])<<8)|((a)[3]))

/* write 32-bit big-endian word into uint8_t[] */
#define amos_doke(a, v) do { \
    (a)[0] = ((v) >> 8) & 0xFF; \
    (a)[1] = (v) & 0xFF; \
} while (0)

/* write 32-bit big-endian word into uint8_t[] */
#define amos_loke(a, v) do { \
    (a)[0] = ((v) >> 24) & 0xFF; \
    (a)[1] = ((v) >> 16) & 0xFF; \
    (a)[2] = ((v) >>  8) & 0xFF; \
    (a)[3] = ((v)      ) & 0xFF; \
} while (0)

#define ID_AMOS (0x414D4F53)
#define ID_AmBs (0x416D4273)
#define ID_AmBk (0x416D426B)
#define ID_AmSp (0x416D5370)
#define ID_AmIc (0x416D4963)

#define IFF_ILBM_HEADER_LEN (0xb0)
static const uint8_t iff_ilbm_header[IFF_ILBM_HEADER_LEN] = {
  'F', 'O', 'R', 'M',    /* 00 FORM                        */
   0,   0,   0,   0,     /* 04   form length               */
  'I', 'L', 'B', 'M',    /* 08   ILBM                      */
  'B', 'M', 'H', 'D',    /* 0c   BMHD                      */
   0,   0,   0,   20,    /* 10     bmhd chunk length (20)  */
   0,   0,               /* 14     width                   */
   0,   0,               /* 16     height                  */
   0,   0,               /* 18     x offset (0)            */
   0,   0,               /* 1a     y offset (0)            */
   0,                    /* 1c     number of bitplanes     */
   0,                    /* 1d     masking (0)             */
   0,                    /* 1e     compression (0)         */
   0,                    /* 1e     reserved1 (0)           */
   0,   0,               /* 20     transparent colour (0)  */
   1,                    /* 22     x aspect (1)            */
   1,                    /* 23     x aspect (1)            */
   0,   0,               /* 24     page width              */
   0,   0,               /* 26     page height             */
  'C', 'A', 'M', 'G',    /* 28   CAMG                      */
   0,   0,   0,   4,     /* 2c     camg chunk length (4)   */
   0,   0,   0,   0,     /* 30     viewmode                */
  'G', 'R', 'A', 'B',    /* 34   GRAB                      */
   0,   0,   0,   4,     /* 38     grab chunk length (4)   */
   0,   0,               /* 3C     x hotspot               */
   0,   0,               /* 3E     y hotspot               */
  'C', 'M', 'A', 'P',    /* 40   CMAP                      */
   0,   0,   0,   96     /* 44     cmap chunk length (96)  */
                         /* 48     {UBYTE r,g,b}[32]       */
                         /* a8   BODY                      */
                         /* ac     body chunk length       */
                         /* b0     unpacked raw bitplanes  */
};

#define IFF_8SVX_HEADER_LEN (0x30)
static const uint8_t iff_8svx_header[IFF_8SVX_HEADER_LEN] = {
  'F', 'O', 'R', 'M',    /* 00 FORM                        */
   0,   0,   0,   0,     /* 04   form length               */
  '8', 'S', 'V', 'X',    /* 08   8SVX                      */
  'V', 'H', 'D', 'R',    /* 0c   VHDR                      */
   0,   0,   0,   20,    /* 10     vhdr chunk length (20)  */
   0,   0,   0,   0,     /* 14     one-shot length         */
   0,   0,   0,   0,     /* 18     repeat length (0)       */
   0,   0,   0,   0,     /* 1c     average rate (0)        */
   0,   0,               /* 20     frequency in hz         */
   1,                    /* 22     number of octaves (1)   */
   0,                    /* 23     compression mode (0)    */
   0,   1,   0,   0,     /* 24     volume (0x10000)        */
  'B', 'O', 'D', 'Y',    /* 28   BODY                      */
   0,   0,   0,   0      /* 2c     body length             */
                         /* 30     raw sample data         */
};

int use_prefix = 0;

char *output_fname(char *fname, char *fmt, ...) {
    size_t namelen = strlen(fname), i;
    char *name = malloc(namelen + 202);
    va_list args;

    if (!name) {
	perror(fname);
	return NULL;
    }

    *name = 0;
    if (use_prefix) {
	/* prefix input filename, with .amos/.abk/.abs stripped off */
	char *extensions[] = {".amos", ".abk", ".abs"};
	strcpy(name, fname);
	for (i = 0; i < sizeof(extensions)/sizeof(*extensions); i++) {
	    size_t elen = strlen(extensions[i]);
	    if (namelen > elen && !strcasecmp(&name[namelen - elen], extensions[i])) {
		name[namelen - elen] = 0;
		break;
	    }
	}
	/* append "." */
	strcat(name, ".");
    }
    va_start(args, fmt);
    vsnprintf(&name[strlen(name)], 200, fmt, args);
    va_end(args);
    return name;
}

uint32_t bank_length(uint8_t *src, size_t len) {
    if (len >= 6 && (amos_leek(src) == ID_AmSp || amos_leek(src) == ID_AmIc)) {
	int num_sprites = amos_deek(&src[4]);
	uint32_t pos = 6, w, h, d;
	while (num_sprites--) {
	    if (pos + 10 > len) return 0;
 	    w = amos_deek(&src[pos+0]) * 2;
 	    h = amos_deek(&src[pos+2]);
 	    d = amos_deek(&src[pos+4]);
	    pos += 10 +  w * h * d;
	}
	pos += 64; /* include palette */
	return pos > len ? 0 : pos;
    }
    else if (len >= 20 && amos_leek(src) == ID_AmBk) {
	uint32_t bank_len = (amos_leek(&src[8]) & 0x0FFFFFFF) + 20 - 8;
        return bank_len > len ? 0 : bank_len;
    }
    return 0;
}

uint32_t bank_number(uint8_t *src, size_t len) {
    if (len >= 6 && amos_leek(src) == ID_AmSp) {
	return 1; /* Sprites always bank 1 */
    }
    if (len >= 6 && amos_leek(src) == ID_AmIc) {
	return 2; /* Icons always bank 2 */
    }
    if (len >= 20 && amos_leek(src) == ID_AmBk) {
	return amos_deek(&src[4]);
    }
    return 0;
}

char *bank_type(uint8_t *src, size_t len) {
    if (len >= 6 && amos_leek(src) == ID_AmSp) {
	return "Sprites";
    }
    if (len >= 6 && amos_leek(src) == ID_AmIc) {
	return "Icons";
    }
    if (len >= 20 && amos_leek(src) == ID_AmBk) {
	/* copy bank type, remove space padding */
	static char type[9];
	char *tp = &type[8];
	memcpy(type, &src[12], 8);
	do { *tp-- = 0; } while (*tp == ' ' && tp > &type[0]);
	return type;
    }
    return NULL;
}

/* https://www.exotica.org.uk/wiki/AMOS_file_formats#AMOS_Sprite_and_Icon_bank_formats */
void decode_sprites(char *fname, uint8_t *src, size_t len, char *prefix) {
    uint32_t num_sprites = amos_deek(&src[4]);
    uint8_t *sp = &src[6], *pal = &src[len - 64];

    /* create an IFF ILBM file for each sprite/icon */
    for (uint32_t i = 0; i < num_sprites; i++) {
	uint32_t w, h, d, sp_len, ilbm_len, line, plane;
	uint8_t *ilbm, *body;

	w = amos_deek(&sp[0]) * 2;
	h = amos_deek(&sp[2]);
	d = amos_deek(&sp[4]);

	sp_len = w * h * d;
	ilbm_len = IFF_ILBM_HEADER_LEN + sp_len;
	if ((ilbm = malloc(ilbm_len))) {
	    char *outname;
            memcpy(ilbm, iff_ilbm_header, IFF_ILBM_HEADER_LEN);
	    amos_loke(&ilbm[0x04], ilbm_len - 8);
	    amos_doke(&ilbm[0x14], w * 8);  /* width */
	    amos_doke(&ilbm[0x16], h);      /* height */
	    ilbm[0x1c] = d & 0xFF;          /* number of bitplanes */
	    amos_doke(&ilbm[0x24], w * 8);  /* page width */
	    amos_doke(&ilbm[0x26], h);      /* page height */
	    memcpy(&ilbm[0x3C], &sp[6], 4); /* x/y hotspot */
	    memcpy(&ilbm[0xA8], "BODY", 4);
	    amos_loke(&ilbm[0xAC], sp_len); /* body length */

	    /* convert palette from 0x0RGB to 0xRR 0xGG 0xBB */
	    for (int j = 0; j < 32; j++) {
		uint32_t c = amos_deek(&pal[j * 2]);
		ilbm[0x48 + (j * 3) + 0] = ((c >> 8) & 0xF) * 0x11;
		ilbm[0x48 + (j * 3) + 1] = ((c >> 4) & 0xF) * 0x11;
		ilbm[0x48 + (j * 3) + 2] = ((c     ) & 0xF) * 0x11;
	    }
	    
	    /* interleave the sprite data (all lines of plane 0, plane 1, ...)
	     * into ILBM format (line 0 plane 0, line 0 plane 1, ...) */
	    body = &ilbm[IFF_ILBM_HEADER_LEN];
	    for (line = 0; line < h; line++) {
		for (plane = 0; plane < d; plane++) {
		    memcpy(body, &sp[10 + line*w + plane*w*h], w);
		    body += w;
		}
	    }

	    /* save IFF ILBM */
	    if ((outname = output_fname(fname, "%s%02X.iff", prefix, i + 1))) {
		printf("%s: converting %s %d to ILBM image %s (%d bytes)\n",
		       fname, prefix, i+1, outname, ilbm_len);
		write_file(outname, ilbm, ilbm_len);
		free(outname);
	    }
	    else {
		perror(fname);
	    }

	    free(ilbm);
	}
	sp += 10 + sp_len;
    }
}

/* https://www.exotica.org.uk/wiki/AMOS_Pac.Pic._format */
void decode_pacpic(char *fname, uint8_t *src, size_t len) {
    uint8_t *end = &src[len], *s, *pal, *picdata, *rledata,
	*points, *ilbm;
    uint32_t i, j, k, l, bplcon0, width, ilbm_width, lumps,
	lump_height, ilbm_height, bitplanes, ilbm_len, ilbm_line,
	rledata_offset, points_offset;

    /* check if screen header present */
    if (len < 24) return;
    i = amos_leek(&src[20]);
    if (len > 134 && (i == 0x12031990 || i == 0x00031990 || i == 0x12030090)) {
	bplcon0 = amos_deek(&src[20 + 20]); /* screen mode */
	pal = &src[20 + 26]; /* palette */
	s = &src[110]; /* picture header */
    }
    else if (len > 44 && (i == 0x06071963 || i == 0x06070063)) {
	bplcon0 = amos_deek(&src[20 + 14]) << 12 | 0x200; /* guess BPLCON0 */
	pal = NULL; /* no palette */
	s = &src[20]; /* picture header */
    }
    else {
	fprintf(stderr, "%s: unknown Pic.Pac. header $%08X\n", fname, i);
	return;
    }
    
    width = amos_deek(&s[8]);
    lumps            = amos_deek(&s[10]);
    lump_height      = amos_deek(&s[12]);
    bitplanes        = amos_deek(&s[14]);
    rledata_offset   = amos_leek(&s[16]);
    points_offset    = amos_leek(&s[20]);

    if (rledata_offset > (len - (s - src)) ||
	points_offset > (len - (s - src)))
    {
	return;
    }

    /* IFF ILBM requires an even width */
    ilbm_width = width;
    if (ilbm_width & 1) ilbm_width++;

    /* number of bytes jump to get to the next line in IFF */
    ilbm_line = ilbm_width * bitplanes;
    ilbm_height = lumps * lump_height;

    ilbm_len = IFF_ILBM_HEADER_LEN + ilbm_line * ilbm_height;
    if ((ilbm = malloc(ilbm_len))) {
	uint8_t *plane, rbit, rrbit, picbyte, rlebyte;
	char *outname;

	memcpy(ilbm, iff_ilbm_header, IFF_ILBM_HEADER_LEN);
	amos_loke(&ilbm[0x04], ilbm_len - 8);
	amos_doke(&ilbm[0x14], ilbm_width * 8);  /* width */
	amos_doke(&ilbm[0x16], ilbm_height);     /* height */
	ilbm[0x1c] = bitplanes & 0xFF;           /* number of bitplanes */
	amos_doke(&ilbm[0x24], ilbm_width * 8);  /* page width */
	amos_doke(&ilbm[0x26], ilbm_height);     /* page height */
	amos_loke(&ilbm[0x30], bplcon0);         /* viewmode */
	memcpy(&ilbm[0xA8], "BODY", 4);
	amos_loke(&ilbm[0xAC], ilbm_len - IFF_ILBM_HEADER_LEN); /* body length */
	/* convert palette from 0x0RGB to 0xRR 0xGG 0xBB */
	for (int i = 0; i < 32; i++) {
	    uint32_t c = pal ? amos_deek(&pal[i*2]) : (i & 0x0F) * 0x111;
	    ilbm[0x48 + (i * 3) + 0] = ((c >> 8) & 0xF) * 0x11;
	    ilbm[0x48 + (i * 3) + 1] = ((c >> 4) & 0xF) * 0x11;
	    ilbm[0x48 + (i * 3) + 2] = ((c     ) & 0xF) * 0x11;
	}

	/* decrunch RLE streams */
	rbit = 7;
	rrbit = 6;
	picdata = &s[24];
	rledata = &s[rledata_offset];
	points  = &s[points_offset];
	picbyte = *picdata++;
	rlebyte = *rledata++;
	if (*points & 0x80) rlebyte = *rledata++;
	plane = &ilbm[IFF_ILBM_HEADER_LEN];
	for (i = 0; i < bitplanes; i++) {
	    uint8_t *lump_start = plane;
	    for (j = 0; j < lumps; j++) {
		uint8_t *lump_offset = lump_start;
		for (k = 0; k < width; k++) {
		    uint8_t *d = lump_offset;
		    for (l = 0; l < lump_height; l++) {
			/* if the current RLE bit is set to 1, read in a new picture byte */
			if (rlebyte & (1 << rbit--)) {
			    picbyte = *picdata++;
			    if (picdata > end) {free(ilbm); return;}
			}
			
			/* write picture byte and move down by one line in the picture */
			*d = picbyte; d += ilbm_line;
			
			/* if we've run out of RLE bits, check the POINTS bits to see if
			 * a new RLE byte is needed */
			if (rbit > 7) {
			    rbit = 7;
			    if (*points & (1 << rrbit--)) {
				rlebyte = *rledata++;
				if (rledata > end) {free(ilbm); return;}

			    }
			    if (rrbit > 7) {
				rrbit = 7, points++;
				if (points > end) {free(ilbm); return;}
			    }
			}
		    }
		    lump_offset++;
		}
		lump_start += ilbm_line * lump_height;
	    }
	    plane += ilbm_width; /* ILBM interleaved bitplanes */
	}

	/* save IFF ILBM */
	if ((outname = output_fname(fname, "pic%02X.iff", bank_number(src, len)))) {
	    printf("%s: unpacking to ILBM image %s (%d bytes)\n", fname, outname, ilbm_len);
	    write_file(outname, ilbm, ilbm_len);
	    free(outname);
	}
	else {
	    perror(fname);
	}

	
	free(ilbm);
    }
}

/* https://www.exotica.org.uk/wiki/AMOS_Samples_Bank_format */
void decode_samples(char *fname, uint8_t *src, size_t len) {
    uint32_t num_samples;

    if (len < 22) return;
    num_samples = amos_deek(&src[20]);
    if (len < 22 + (num_samples * 4)) return;
    
    /* create an IFF 8SVX file for each sample */
    for (uint32_t i = 0; i < num_samples; i++) {
	uint8_t *svx;
	uint32_t offset = amos_leek(&src[22 + i*4]) + 22;
	uint32_t freq, sam_len, svx_len;
	uint8_t name[7];
	
	if (offset > len) return;

	/* copy sample name, change non-alnum to "_" */
	memcpy(name, &src[offset], 6);
	for (int i = 0; i < 6; i++) if (!isalnum(name[i])) name[i] = '_';
	name[6] = 0;
	
	freq    = amos_deek(&src[offset + 8]);
	sam_len = amos_leek(&src[offset + 10]);

	if ((offset + 14 + sam_len) > len) {
	    sam_len = len - (offset + 14);
	}
	
	svx_len = IFF_8SVX_HEADER_LEN + sam_len;
	if ((svx = malloc(svx_len))) {
	    char *outname;
            memcpy(svx, iff_8svx_header, IFF_8SVX_HEADER_LEN);
	    amos_loke(&svx[0x04], svx_len - 8);
	    amos_doke(&svx[0x20], freq);
	    amos_loke(&svx[0x2C], sam_len);
	    memcpy(&svx[0x30], &src[offset + 14], sam_len);

	    /* save IFF 8SVX */
	    if ((outname = output_fname(fname, "sample%02X.%s.8svx", i + 1, name))) {
		printf("%s: extracting sample %d to 8SVX sample %s (%d bytes)\n",
		       fname, i+1, outname, svx_len);
		write_file(outname, svx, svx_len);
		free(outname);
	    }
	    else {
		perror(fname);
	    }

	    free(svx);
	}
    }
    
}

void decode_tracker(char *fname, uint8_t *src, size_t len) {
    uint32_t mod_len = len - 20;
    char *outname = output_fname(fname, "tracker%02X.mod", bank_number(src, len));
    if (outname) {
	printf("%s: extracting tracker mod %s (%d bytes)\n", fname, outname, mod_len);
	write_file(outname, &src[20], mod_len);
	free(outname);
    }
    else {
	perror(fname);
    }
}

void amos_bank(char *fname, uint8_t *src, size_t len) {
    /* get the "real", validated bank length */
    if (!(len = bank_length(src, len))) return;
    
    if (amos_leek(src) == ID_AmSp) {
	decode_sprites(fname, src, len, "sprite");
    }
    else if (amos_leek(src) == ID_AmIc) {
	decode_sprites(fname, src, len, "icon");
    }
    else if (amos_leek(src) == ID_AmBk) {
	if (!memcmp(&src[12], "Pac.Pic.", 8)) {
	    decode_pacpic(fname, src, len);
	}
	else if (!memcmp(&src[12], "Samples ", 8)) {
	    decode_samples(fname, src, len);
	}
	else if (!memcmp(&src[12], "Tracker ", 8)) {
	    decode_tracker(fname, src, len);
	}
    }
}

void amos_banks(char *fname, uint8_t *src, size_t len) {
    if (len < 6) {
	fprintf(stderr, "%s: file too short to be AMOS banks\n", fname);
    }
    else {
	uint32_t num_banks = amos_deek(&src[4]), bank_pos = 6;
	while (num_banks--) {
	    uint32_t bank_len = bank_length(&src[bank_pos], len - bank_pos);
	    uint32_t bank_num = bank_number(&src[bank_pos], bank_len);
	    char *type = bank_type(&src[bank_pos], bank_len), *outname;
	    if (bank_num > 0) {
		if ((outname = output_fname(fname, "bank%02X.abk", bank_num))) {
		    printf("%s: extracting bank %d (%s) as %s (%d bytes)\n",
			   fname, bank_num, type, outname, bank_len);
		    write_file(outname, &src[bank_pos], bank_len);
		    amos_bank(outname, &src[bank_pos], bank_len);
		    free(outname);
		}
		else {
		    perror(fname);
		}
	    }
	    bank_pos += bank_len;
	}
    }
}

void amos_source(char *fname, uint8_t *src, size_t len) {
    if (len < 30) {
	fprintf(stderr, "%s: file too short to be AMOS source code\n", fname);
    }
    else {
	uint32_t src_len = amos_leek(&src[16]) + 20;
	char hdr[17] = {0};
	memcpy(hdr, src, 16);
	printf("%s: AMOS source code with header '%s'\n", fname, hdr);

	/* parse banks after source code */
	if (src_len < len) {
	    amos_banks(fname, &src[src_len], len - src_len);
	}
    }
}

void amos_file(char *fname, uint8_t *src, size_t len) {
    if (len < 4) {
	fprintf(stderr, "%s: file too short to be an AMOS file\n", fname);
    }
    else {
	switch (amos_leek(src)) {
	case ID_AMOS:
	    amos_source(fname, src, len);
	    break;
	    
	case ID_AmBs:
	    printf("%s: AMOS banks\n", fname);
	    amos_banks(fname, src, len);
	    break;
	    
	case ID_AmBk:
	case ID_AmSp:
	case ID_AmIc:
	    printf("%s: AMOS bank (%s)\n", fname, bank_type(src, len));
	    amos_bank(fname, src, len);
	    break;
	    
	default:
	    fprintf(stderr, "%s: not an AMOS source file or bank\n", fname);
	}
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
	printf("Usage: %s [-p] <file.AMOS, file.abk, file.abs, ...>\n", argv[0]);
	return 1;
    }
    
    for (int i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-p")) {
	    use_prefix = 1;
	}
	else {
	    char *fname = argv[i];
	    uint8_t *buf;
	    size_t len;
	    if ((buf = read_file(fname, &len))) {
		amos_file(fname, buf, len);
		free(buf);
	    }
	}
    }
    return 0;
}

