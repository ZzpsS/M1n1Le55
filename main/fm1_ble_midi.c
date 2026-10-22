#include "fm1_ble_midi.h"
#include <string.h>

size_t fm1_ble_sysex_packet(uint32_t ms,const uint8_t *m,size_t n,size_t *offset,
                            uint8_t *out,size_t cap)
{
    if(!m||!out||!offset||n<2||n>FM1_BLE_SYSEX_MAX||*offset>=n||cap<4||m[0]!=0xf0||m[n-1]!=0xf7)return 0;
    for(size_t i=1;i+1<n;i++)if(m[i]&0x80)return 0;
    size_t at=*offset,k=0;out[k++]=0x80|((ms>>7)&0x3f);
    if(!at)out[k++]=0x80|(ms&127);
    while(at+1<n&&k<cap)out[k++]=m[at++];
    if(at==n-1&&k+2<=cap){out[k++]=0x80|(ms&127);out[k++]=m[at++];}
    *offset=at;return k;
}

bool fm1_ble_sysex_receive(fm1_ble_sysex_rx_t *r,const uint8_t *p,size_t n,
                           fm1_ble_sysex_cb cb,void *ctx)
{
    if(!r||!p||!cb)return false;
    if(n<2||(p[0]&0xc0)!=0x80){r->active=false;r->size=0;return false;}
    size_t i=1;bool ok=true;
    while(i<n) {
        uint8_t b=p[i++];
        if(b&0x80) { // Timestamp preceding a MIDI status or running-status data.
            if(i==n){ok=false;break;}
            b=p[i++];
            if(b>=0xf8)continue; // Real-time messages do not interrupt SysEx.
            if(b==0xf0){r->active=true;r->size=0;}
            else if(b==0xf7) {
                if(r->active&&r->size<sizeof(r->bytes)) {
                    r->bytes[r->size++]=b;cb(r->bytes,r->size,ctx);
                } else ok=false;
                r->active=false;r->size=0;continue;
            } else if(b&0x80){r->active=false;r->size=0;continue;}
        }
        if(r->active) {
            if(r->size==sizeof(r->bytes)){ok=false;r->active=false;r->size=0;}
            else r->bytes[r->size++]=b;
        }
    }
    if(!ok){r->active=false;r->size=0;}
    return ok;
}

size_t fm1_ble_midi_packet(uint32_t ms,const uint8_t *m,size_t n,uint8_t *out,size_t cap)
{
    if(!m||!out||!n||m[0]<0x80||m[0]>=0xf0)return 0;
    size_t expected=(m[0]>>4==0xc||m[0]>>4==0xd)?2:3;
    if(n!=expected||cap<n+2)return 0;
    for(size_t i=1;i<n;i++)if(m[i]&0x80)return 0;
    out[0]=0x80|((ms>>7)&0x3f);out[1]=0x80|(ms&0x7f);
    memcpy(out+2,m,n);return n+2;
}

bool fm1_ble_advert(const uint8_t *data,size_t size,bool *midi,char name[32])
{
    static const uint8_t uuid[]={0x00,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,
        0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,0x03};
    if(!data||!midi||!name)return false;
    bool found=false;char label[32]={0};
    for(size_t i=0;i<size;) {
        unsigned n=data[i++];if(!n)break;
        if(n>size-i)return false;
        uint8_t type=data[i];const uint8_t *p=data+i+1;unsigned count=n-1;
        if(type==6||type==7) {
            if(count%16)return false;
            for(unsigned j=0;j<count;j+=16)if(!memcmp(p+j,uuid,16))found=true;
        }
        if(type==8||type==9) {
            unsigned len=count<31?count:31;
            for(unsigned j=0;j<len;j++)label[j]=p[j]>=32&&p[j]<=126?p[j]:'?';
            label[len]=0;
        }
        i+=n;
    }
    *midi=found;memcpy(name,label,32);return true;
}
