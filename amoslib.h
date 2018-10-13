#ifndef AMOSLIB_H
#define AMOSLIB_H 1

#include <stdlib.h>
#include <stdio.h>

#define amos_deek(a) ((((a)[0])<<8)|((a)[1]))
#define amos_leek(a) ((((a)[0])<<24)|(((a)[1])<<16)|(((a)[2])<<8)|((a)[3]))

#define AMOS_EXTENSION_SLOTS (25)
#define AMOS_TOKEN_TABLE_SIZE (16267)  /* should be prime */

struct AMOS_token {
  struct AMOS_token *next;
  int key;
  char text[1];
};

int AMOS_print_source(unsigned char *src, size_t len, FILE *out,
		      struct AMOS_token *table[AMOS_TOKEN_TABLE_SIZE]);

void AMOS_decrypt_procedure(unsigned char *src);

int AMOS_parse_config(unsigned char *src, size_t len,
		      char *slots[AMOS_EXTENSION_SLOTS]);

int AMOS_parse_extension(unsigned char *src, size_t len, int slot, int start,
			 struct AMOS_token *table[AMOS_TOKEN_TABLE_SIZE]);

int AMOS_find_slot(unsigned char *src, size_t len);

void AMOS_free_tokens(struct AMOS_token *table[AMOS_TOKEN_TABLE_SIZE]);

#endif
