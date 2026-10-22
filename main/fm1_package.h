#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define FM1_PACKAGE_LIMIT (2u * 1024u * 1024u)
#define FM1_UPLOAD_CHUNK 1024u
size_t fm1_hex_decode(const char *text,uint8_t *bytes,size_t capacity);
bool fm1_package_name(const char *name);
bool fm1_package_identity(const uint8_t *header, size_t size, char identity[21]);
bool fm1_hash_parse(const char *text, uint8_t hash[32]);
void fm1_hash_text(const uint8_t hash[32], char text[65]);
typedef struct {const char *title,*identity,*sha256;size_t size;bool custom,app_only_loader;} fm1_release_t;
const fm1_release_t *fm1_release_find(const char *hash,size_t size,const char *identity);
const char *fm1_release_block(const fm1_release_t *release,const char *source);
// True only for a supported route that must first install the pinned V15 package.
bool fm1_release_needs_bridge(const fm1_release_t *release,const char *source);
bool fm1_release_loader_matches(const char *target,const char *loader);
size_t fm1_package_logical(uint8_t *raw,size_t size);
