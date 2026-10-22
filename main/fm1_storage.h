#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define FM1_LIBRARY_MAX 24
typedef struct {
    char name[64],identity[21],sha256[65],check[48];
    uint32_t size;
    bool model_ok,hash_match;
} fm1_package_t;
typedef struct {
    bool mounted,busy;
    uint32_t revision,capacity_mb,count,backups,omitted;
    char message[120],last_backup[64],history[6][120];
    fm1_package_t files[FM1_LIBRARY_MAX];
} fm1_storage_state_t;
void fm1_storage_start(void);
void fm1_storage_snapshot(fm1_storage_state_t *out);
bool fm1_storage_request(bool backup);
bool fm1_storage_read_backup(const char *name,char *out,size_t capacity);
bool fm1_storage_upload_begin(const char *name,uint32_t size,const char *sha);
bool fm1_storage_upload_data(uint32_t offset,const uint8_t *data,size_t size);
bool fm1_storage_upload_end(void);
void fm1_storage_upload_abort(void);
bool fm1_storage_load_package(const char *name,uint8_t **bytes,size_t *size,fm1_package_t *metadata);
bool fm1_storage_backup_now(char name[64]);
bool fm1_storage_update_record(const char *name,const char *phase,const char *detail);
unsigned fm1_storage_stack_min(void);

bool fm1_storage_external_begin(void);
void fm1_storage_external_end(void);
bool fm1_storage_sample_upload_begin(const char *name,uint32_t size,const char *sha);
bool fm1_storage_exchange_upload_begin(const char *name,uint32_t size,const char *sha);
