// USB-MIDI pull-update protocol; see research sources documented in README.
#include "fm1_ota.h"
#include "fm1_package.h"
#include <string.h>
#include <stdio.h>
#define FINISH_CHECK UINT32_C(0xe0000000)
#define FINISH_WRITE UINT32_C(0xf0000000)
enum { TX_QUERY=1,TX_START,TX_DATA,TX_CHECK_END,TX_WRITE_END };
static bool fast_application(const char *id)
{
    // Only pinned open-source implementations whose ota_read() explicitly pulls each reply.
    const char *ids[]={"FM-1_916","FM-1_900","FM-1_910","FM-1_8000400","FM-1_9010100"};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);i++)if(!strcmp(id,ids[i]))return true;
    return false;
}
unsigned fm1_ota_progress(const fm1_ota_t *o)
{
    if(o->phase==OTA_DONE)return 100;
    if(o->phase==OTA_ERROR)return o->progress;
    if(o->phase==OTA_WAIT_REBOOT)return 96;
    if(o->phase==OTA_WAIT_LOADER)return 10;
    if(o->phase==OTA_START_WRITE)return 12;
    if(o->phase==OTA_WRITE) {
        // Estimated wire workload of the pinned loader; retries may exceed it.
        // Felucca verifies a full pass before the app pass; SLOOP/Jangada use one pass.
        uint32_t total=!strcmp(o->target,"FM-1_916")?1188896:
            (!strcmp(o->target,"FM-1_900")||!strcmp(o->target,"FM-1_910"))?603168:
            (!strcmp(o->target,"FM-1_8000400")||!strcmp(o->target,"FM-1_9010100"))?586784:(uint32_t)o->size;
        unsigned p=total?(unsigned)((uint64_t)o->write_bytes*82/total):0;
        return 12+(p>82?82:p);
    }
    if(o->phase==OTA_CHECK) {
        unsigned p=(unsigned)((uint64_t)o->stage_bytes*8/32768);
        return 2+(p>7?7:p);
    }
    return o->phase==OTA_IDLE?0:1;
}
static bool application_identity(const char *id)
{
    size_t n=strlen(id);
    if((n!=8&&n!=12)||strncmp(id,"FM-1_",5))return false;
    for(size_t i=5;i<n;i++)if(id[i]<'0'||id[i]>'9')return false;
    return n==8||id[5]=='8'||id[5]=='9';
}
bool fm1_ota_open_loader(const char *id)
{
    // Pinned Open V12/Probe/Diag SDK enters its own RAM loader when started
    // through the legacy protocol without an armed build tag. Do not accept
    // arbitrary FM1C tags or treat this as the forks' app-only loader.
    return !strcmp(id,"ota-FM1C_0000000000000000");
}
bool fm1_ota_loader(const char *id)
{return fm1_ota_open_loader(id)||(!strncmp(id,"ota-",4)&&application_identity(id+4));}
bool fm1_ota_identity_valid(const char *id)
{
    if(fm1_ota_loader(id))return true;
    return application_identity(id);
}
bool fm1_ota_active(const fm1_ota_t *o){return o->phase>OTA_IDLE&&o->phase<OTA_DONE;}
void fm1_ota_sample(fm1_ota_t *o,int64_t now)
{
    unsigned progress=fm1_ota_progress(o);if(progress>o->progress)o->progress=progress;
    if(!fm1_ota_active(o)||now<=o->sampled)return;
    uint32_t dt=(uint32_t)(now-o->sampled);o->sampled=now;o->elapsed_ms+=dt;
    if(o->phase==OTA_CHECK)o->check_ms+=dt;
    if(o->phase==OTA_WRITE)o->write_ms+=dt;
}
int64_t fm1_ota_wakeup(const fm1_ota_t *o)
{
    if(!fm1_ota_active(o))return -1;
    int64_t due=o->deadline+1,limit=o->started+900001;
    if(limit<due)due=limit;
    if(!o->attached||o->inflight)return due;
    bool query=o->phase==OTA_HANDSHAKE||o->phase==OTA_WAIT_LOADER||o->phase==OTA_WAIT_REBOOT;
    if(query||o->phase==OTA_START_CHECK||o->phase==OTA_START_WRITE||o->count) {
        int64_t ready=o->not_before;
        if(query&&o->next_query>ready)ready=o->next_query;
        if(ready<due)due=ready;
    }
    return due;
}
void fm1_ota_fail(fm1_ota_t *o,const char *s)
{o->phase=OTA_ERROR;o->count=0;snprintf(o->error,sizeof(o->error),"%s",s);}
void fm1_ota_begin(fm1_ota_t *o,const uint8_t *image,size_t size,const char *source,const char *target,int64_t now)
{
    *o=(fm1_ota_t){.phase=OTA_HANDSHAKE,.image=image,.size=size,.attached=true,.deadline=now+10000,.started=now,.sampled=now};
    snprintf(o->source,sizeof(o->source),"%s",source);snprintf(o->target,sizeof(o->target),"%s",target);
    if(!image||!size||size>2097152||!fm1_ota_identity_valid(source)||!fm1_ota_identity_valid(target)||fm1_ota_loader(target))fm1_ota_fail(o,"Invalid update session");
}
void fm1_ota_identity(fm1_ota_t *o,const char *id,int64_t now)
{
    fm1_ota_sample(o,now);
    if(!fm1_ota_active(o))return;
    if(!fm1_ota_identity_valid(id)){fm1_ota_fail(o,"Unexpected device identity");return;}
    if(o->phase==OTA_HANDSHAKE) {
        if(strcmp(id,o->source)){fm1_ota_fail(o,"Device changed after preparation; prepare again");return;}
        if(fm1_ota_loader(id)&&!fm1_release_loader_matches(o->target,id)){fm1_ota_fail(o,"Update loader does not match this package");return;}
        o->phase=fm1_ota_loader(id)?OTA_START_WRITE:OTA_START_CHECK;o->deadline=now+10000;
        o->fast_transfer=fm1_ota_loader(id)?!strcmp(id,"ota-FM-1_900")&&fast_application(o->target):fast_application(id);
    } else if(o->phase==OTA_WAIT_LOADER && fm1_ota_loader(id)) {
        if(!fm1_release_loader_matches(o->target,id)){fm1_ota_fail(o,"Update loader does not match this package");return;}
        o->phase=OTA_START_WRITE;o->deadline=now+10000;
        o->fast_transfer=!strcmp(id,"ota-FM-1_900")&&fast_application(o->target);
    } else if(o->phase==OTA_WAIT_REBOOT && !fm1_ota_loader(id)) {
        if(strcmp(id,o->target))fm1_ota_fail(o,"Restarted firmware identity does not match target");
        else o->phase=OTA_DONE;
    }
}
void fm1_ota_connection(fm1_ota_t *o,bool attached,int64_t now)
{
    fm1_ota_sample(o,now);
    o->attached=attached;o->next_query=now;
    if(!attached) {
        o->inflight=false;o->count=0;
        if(fm1_ota_active(o)&&o->phase!=OTA_WAIT_LOADER&&o->phase!=OTA_WAIT_REBOOT)
            fm1_ota_fail(o,"USB disconnected before stage completed; retry package");
    }
}
void fm1_ota_request(fm1_ota_t *o,const uint8_t *p,size_t n,int64_t now)
{
    fm1_ota_sample(o,now);
    fm1_ota_phase_t phase=o->phase;
    if(o->inflight&&o->tx_kind==TX_START)phase=phase==OTA_START_CHECK?OTA_CHECK:phase==OTA_START_WRITE?OTA_WRITE:phase;
    if(phase!=OTA_CHECK&&phase!=OTA_WRITE)return;
    if(n!=8){fm1_ota_fail(o,"Malformed update read request");return;}
    fm1_ota_read_t r={p[0],(uint32_t)p[1]|(uint32_t)p[2]<<8|(uint32_t)p[3]<<16|(uint32_t)p[4]<<24,
        (uint32_t)p[5]|(uint32_t)p[6]<<8|(uint32_t)p[7]<<16};
    bool terminal=r.address==FINISH_CHECK||r.address==FINISH_WRITE;
    if(terminal) {
        if(r.length!=8 || r.address!=(phase==OTA_CHECK?FINISH_CHECK:FINISH_WRITE) || !o->stage_requests) {
            fm1_ota_fail(o,"Unexpected update finish request");return;
        }
    } else if(!r.length||r.length>512||r.address>o->size||r.length>o->size-r.address) {
        fm1_ota_fail(o,"Device requested data outside verified image");return;
    }
    if(o->count==4){fm1_ota_fail(o,"Update request queue overflow");return;}
    o->queue[(o->head+o->count++)%4]=r;o->deadline=now+(phase==OTA_CHECK?10000:180000);
}
size_t fm1_ota_next(fm1_ota_t *o,uint8_t *f,size_t cap,int64_t now)
{
    fm1_ota_sample(o,now);
    if(!fm1_ota_active(o))return 0;
    if(now>o->deadline||now-o->started>900000){fm1_ota_fail(o,"Update timed out; keep power and retry package");return 0;}
    if(!o->attached||o->inflight||now<o->not_before)return 0;
    size_t n=0;
    if(o->phase==OTA_HANDSHAKE||o->phase==OTA_WAIT_LOADER||o->phase==OTA_WAIT_REBOOT) {
        if(now<o->next_query)return 0;
        n=fm1_identity_request(f,cap);o->tx_kind=TX_QUERY;o->next_query=now+(o->fast_transfer?250:1000);
    } else if(o->phase==OTA_START_CHECK||o->phase==OTA_START_WRITE) {
        const uint8_t start[]={0xf0,0x22,0x24,0x35,0x7f,0xf7};
        if(cap<sizeof(start))return 0;
        memcpy(f,start,sizeof(start));n=sizeof(start);o->tx_kind=TX_START;
    } else if(o->count) {
        o->sending=o->queue[o->head];o->head=(o->head+1)%4;o->count--;
        fm1_ota_read_t r=o->sending;uint8_t body[520];size_t len=r.length;
        body[0]=r.flags;
        for(unsigned i=0;i<4;i++)body[1+i]=r.address>>(i*8);
        body[5]=len;body[6]=len>>8;body[7]=0;
        o->tx_kind=TX_DATA;
        if(r.address==FINISH_CHECK||r.address==FINISH_WRITE) {
            body[0]=0;memcpy(body+8,"success",8);o->tx_kind=r.address==FINISH_CHECK?TX_CHECK_END:TX_WRITE_END;
        } else memcpy(body+8,o->image+r.address,len);
        n=fm1_encode(0x30,body,len+8,f,cap);
    }
    if(n)o->inflight=true;
    return n;
}
void fm1_ota_sent(fm1_ota_t *o,bool ok,int64_t now)
{
    fm1_ota_sample(o,now);
    if(!o->inflight)return;
    o->inflight=false;
    if(!fm1_ota_active(o))return;
    if(!ok){fm1_ota_fail(o,"USB update transfer failed; retry package");return;}
    o->not_before=now+(o->fast_transfer?2:10);
    if(o->tx_kind==TX_START) {
        o->phase=o->phase==OTA_START_CHECK?OTA_CHECK:OTA_WRITE;o->stage_requests=0;o->stage_bytes=0;
        // A queued read is the open firmware's ready signal; no blind 2-second delay.
        o->not_before=now+(o->fast_transfer?2:2000);o->deadline=now+(o->phase==OTA_CHECK?10000:180000);
    } else if(o->tx_kind==TX_DATA) {
        o->requests++;o->stage_requests++;o->bytes+=o->sending.length;o->stage_bytes+=o->sending.length;
        if(o->phase==OTA_WRITE)o->write_bytes+=o->sending.length;
    }
    else if(o->tx_kind==TX_CHECK_END||o->tx_kind==TX_WRITE_END) {
        o->phase=o->tx_kind==TX_CHECK_END?OTA_WAIT_LOADER:OTA_WAIT_REBOOT;
        int delay=o->fast_transfer?250:3000;
        o->count=0;o->not_before=now+delay;o->next_query=now+delay;o->deadline=now+45000;
    }
    unsigned progress=fm1_ota_progress(o);if(progress>o->progress)o->progress=progress;
}
const char *fm1_ota_phase_name(fm1_ota_phase_t p)
{
    const char *names[]={"Idle","Checking device identity","Starting package check","Checking package on FM-1",
        "Waiting for update loader","Starting firmware write","Writing firmware","Waiting for new firmware","Installed and verified","Update incomplete"};
    return p<=OTA_ERROR?names[p]:"Unknown";
}
