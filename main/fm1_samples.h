#pragma once
#include <stdbool.h>
#include <stdint.h>
#define FM1_SAMPLE_FILES 24
typedef enum { SAMPLE_IDLE,SAMPLE_LOADING,SAMPLE_PREVIEW,SAMPLE_PREPARED,SAMPLE_QUERY,
    SAMPLE_BACKUP,SAMPLE_WRITING,SAMPLE_VERIFYING,SAMPLE_DONE,SAMPLE_ERROR,SAMPLE_PLAYING } fm1_sample_phase_t;
typedef struct {
    uint32_t revision,token,bytes,done,total,elapsed_ms,write_ms,verify_ms,retries;
    unsigned count,omitted,operation;
    bool busy,prepared,verified,device_known,confirmation_valid,result_verified;
    /* Completion can use the device commit/status reply; verified means full readback. */
    bool completed,result_completed,device_verified,full_readback,recovery,result_backup_current;
    fm1_sample_phase_t phase,failed_phase;
    uint32_t connection_id,device_connection_id,result_backup_connection_id,device_bytes,frames[8],device_frames[8];
    uint8_t peaks[8][64],device_peaks[8][64];
    char message[128],name[64],backup[64],slots[8][16];
    char peer[32],device_name[64],device_slots[8][16],result[128],result_backup[64];
    char files[FM1_SAMPLE_FILES][64];
} fm1_samples_state_t;
void fm1_samples_start(void);
void fm1_samples_snapshot(fm1_samples_state_t *out);
bool fm1_samples_scan(void);
bool fm1_samples_prepare(const char *name);
/* Explicitly selected recovery source: validates the SD bank and binds confirmation
 * to the current BLE session, including an interrupted/error destination bank. */
bool fm1_samples_prepare_restore(const char *name);
/* Shortcut only: select the internally recorded backup for this exact original
 * connection. A stale UI snapshot must not rebind an old backup to a new peer. */
bool fm1_samples_prepare_recent_restore(uint32_t expected_connection);
bool fm1_samples_preview(const char *name);
bool fm1_samples_confirm(uint32_t token);
/* The diagnostic confirm above keeps full readback; UI may choose the faster commit check. */
bool fm1_samples_confirm_with_readback(uint32_t token,bool full_readback);
bool fm1_samples_cancel(uint32_t token);
bool fm1_samples_backup(void);
bool fm1_samples_play(unsigned slot);
