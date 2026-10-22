#include "fm1_wire.h"
#include <string.h>

static size_t pack7(const uint8_t *raw, size_t n, uint8_t *out)
{
    uint32_t acc = 0; unsigned bits = 0; size_t count = 0;
    for (size_t i=0; i<n; i++) {
        acc |= (uint32_t)raw[i] << bits; bits += 8;
        while (bits >= 7) { out[count++] = acc & 127; acc >>= 7; bits -= 7; }
    }
    if (bits) out[count++] = acc;
    return count;
}

size_t fm1_identity_request(uint8_t *out, size_t capacity)
{
    const uint8_t query[] = {0xf0,0,0x32,0x45,0,0,0,0x40,0x7f,0xf7};
    if (capacity < sizeof(query)) return 0;
    memcpy(out, query, sizeof(query)); return sizeof(query);
}

size_t fm1_encode(uint8_t command,const uint8_t *payload,size_t size,uint8_t *out,size_t capacity)
{
    if(size>520 || capacity<((size+7)*8+6)/7+2)return 0;
    uint8_t raw[527]={0,0x59,command,(uint8_t)size,(uint8_t)(size>>8),0};
    memcpy(raw+6,payload,size);uint8_t sum=0;
    for(size_t i=0;i<size;i++)sum+=payload[i];
    raw[6+size]=(uint8_t)~sum;out[0]=0xf0;
    size_t n=pack7(raw,size+7,out+1);out[n+1]=0xf7;return n+2;
}

size_t fm1_voice_request(uint8_t *out, size_t capacity)
{
    // Vendor READ (23), voice subcommand 5, offset 0, length 155.
    const uint8_t raw[] = {0,0x59,0x23,8,0,0,5,0,0,0,0,155,0,0,0x5f};
    if (capacity < 20) return 0;
    out[0] = 0xf0;
    size_t n = pack7(raw,sizeof(raw),out+1); out[n+1]=0xf7; return n+2;
}

size_t fm1_parameter_request(uint16_t p, uint8_t v, uint8_t ch, uint8_t *out, size_t capacity)
{
    if (p>=155 || v>127 || ch>15 || capacity<7) return 0;
    out[0]=0xf0; out[1]=0x43; out[2]=0x10|ch;
    out[3]=p>>7; out[4]=p&127; out[5]=v; out[6]=0xf7; return 7;
}

bool fm1_decode(const uint8_t *f, size_t n, uint8_t *cmd, uint8_t *out, size_t *out_n)
{
    if (n<3 || n>FM1_FRAME_MAX || f[0]!=0xf0 || f[n-1]!=0xf7) return false;
    uint8_t raw[FM1_FRAME_MAX]; size_t used=0; unsigned bits=0; uint32_t acc=0;
    for (size_t i=1;i<n-1;i++) {
        if (f[i]>127) return false;
        acc |= (uint32_t)f[i]<<bits; bits+=7;
        if(bits>=8) {raw[used++]=acc&255; acc>>=8; bits-=8;}
    }
    if(used<7 || raw[0]!=0 || raw[1]!=0x59) return false;
    size_t size=(size_t)raw[3]|(size_t)raw[4]<<8|(size_t)raw[5]<<16;
    if(size+7!=used || size>*out_n) return false;
    uint8_t sum=0; for(size_t i=6;i<used-1;i++) sum+=raw[i];
    if ((uint8_t)(sum+raw[used-1])!=255) return false;
    memcpy(out,raw+6,size); *out_n=size; *cmd=raw[2]; return true;
}

size_t fm1_stream_packet(fm1_stream_t *s, const uint8_t p[4])
{
    if(p[0]>>4) return 0; // Other virtual cables must not corrupt cable 0.
    uint8_t cin=p[0]&15;
    if(cin==15 && p[1]>=0xf8) return 0; // MIDI real-time may interrupt SysEx.
    if(cin<4 || cin>7) {s->active=false;s->used=0;return 0;}
    size_t count=cin==4?3:cin-4;
    for(size_t i=1;i<=count;i++) {
        uint8_t b=p[i];
        if(b>=0xf8) continue;
        if(b==0xf0) {s->active=true;s->used=0;}
        else if((b&0x80) && b!=0xf7) {s->active=false;s->used=0;return 0;}
        if(!s->active) continue;
        if(s->used==sizeof(s->data)) {s->active=false;s->used=0;return 0;}
        s->data[s->used++]=b;
        if(b==0xf7) {s->active=false;return s->used;}
    }
    if(cin!=4) {s->active=false;s->used=0;}
    return 0;
}

size_t fm1_usb_packets(const uint8_t *s, size_t n, uint8_t *out, size_t cap)
{
    if(n<2 || s[0]!=0xf0 || s[n-1]!=0xf7 || ((n+2)/3)*4>cap) return 0;
    size_t used=0;
    for(size_t i=0;i<n;) {
        size_t left=n-i, take=left<3?left:3;
        out[used++]=left>3?4:4+take;
        for(size_t k=0;k<3;k++) out[used++]=k<take?s[i+k]:0;
        i+=take;
    }
    return used;
}
