#ifndef AMOSLIB_H
#define AMOSLIB_H 1

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define amos_deek(a) ((((a)[0])<<8)|((a)[1]))
#define amos_leek(a) ((((a)[0])<<24)|(((a)[1])<<16)|(((a)[2])<<8)|((a)[3]))

#define AMOS_EXTENSION_SLOTS (25)

/* should be prime. 8297 is the first number where the core tokens
 * and default extension tokens all hash to a unique table index */
#define AMOS_TOKEN_TABLE_SIZE (8297)

struct AMOS_token {
  struct AMOS_token *next;
  int key;
  char text[1];
};

/**
 * Print AMOS source code to a file handle
 * @param src pointer to first line of source code
 * @param len length of source code in bytes
 * @param out file handle to write to
 * @param table token table filled using AMOS_parse_extension
 * @return zero for success, non-zero if errors occurred
 */
int AMOS_print_source(uint8_t *src, size_t len, FILE *out,
                      struct AMOS_token *table[AMOS_TOKEN_TABLE_SIZE]);

/**
 * Decrypts (or re-encrypts) an AMOS procedure
 * @param src pointer to start of line with PROCEDURE token
 * @param len maximum length of source code from that line onward
 */
void AMOS_decrypt_procedure(uint8_t *src, size_t len);

/**
 * Parse an AMOS 1.3 or AMOS Pro config file to get extension names
 * @param src pointer to config file
 * @param len length of config file
 * @param slots extension slots, will be filled with extension names
 * @return zero for success, non-zero if file can't be parsed
 */
int AMOS_parse_config(uint8_t *src, size_t len,
                      char *slots[AMOS_EXTENSION_SLOTS]);

/**
 * Parse an AMOS 1.3 or AMOS Pro extension to get extension tokens
 * @param src pointer to extension file
 * @param len length of extension file
 * @param slot to load extension into (0-25)
 * @param start offset from token table to start parsing (use 6)
 * @param table token table to be filled
 * @return zero for succes, non-zero if file can't be parsed
 */
int AMOS_parse_extension(uint8_t *src, size_t len, int slot, int start,
                         struct AMOS_token *table[AMOS_TOKEN_TABLE_SIZE]);

/**
 * Parse an AMOS 1.3 or AMOS Pro extension and find its slot number
 * @param src pointer to extension file
 * @param len length of extension file
 * @return slot number if can be determined, or -1 if indeterminate
 */
int AMOS_find_slot(uint8_t *src, size_t len);

/**
 * Free tokens added to token table by AMOS_parse_extension()
 * @param table token table
 */
void AMOS_free_tokens(struct AMOS_token *table[AMOS_TOKEN_TABLE_SIZE]);

#endif
