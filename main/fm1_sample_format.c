#include "fm1_sample_format.h"
#include <string.h>
uint32_t fm1_sample_u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
void fm1_sample_put32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=n>>(8*i);}
bool fm1_sample_name(const char *s)
{
    size_t n=strlen(s);if(n<6||n>63||strcmp(s+n-5,".fm1s")||strstr(s,".."))return false;
    for(size_t i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='_'||s[i]=='-'||s[i]=='.'))return false;
    return true;
}
bool fm1_sample_validate(const uint8_t *p,size_t n)
{
    if(!p||n<512||n>FM1_SAMPLE_CAPACITY||fm1_sample_u32(p)!=0x53314d46||fm1_sample_u32(p+4)!=1||fm1_sample_u32(p+8)!=n||fm1_sample_u32(p+16)||fm1_sample_u32(p+20)!=32000||fm1_sample_u32(p+24)||fm1_sample_u32(p+28))return false;
    uint32_t crc=~0u;
    for(size_t i=0;i<n;i++){crc^=(i>=12&&i<20)?0:p[i];for(unsigned j=0;j<8;j++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);}
    if((~crc)!=fm1_sample_u32(p+12))return false;
    for(unsigned i=480;i<512;i++)if(p[i])return false;
    size_t pos=512;
    for(unsigned i=0;i<16;i++) {
        const uint8_t *e=p+32+i*28;uint32_t frames=fm1_sample_u32(e+4),bytes=fm1_sample_u32(e+8);
        if(fm1_sample_u32(e)!=pos||frames>((FM1_SAMPLE_CAPACITY-512)/228)*320||bytes!=((frames+319)/320)*228||bytes>n-pos||e[27]||(i>=8&&frames))return false;
        for(unsigned j=12;j<28;j++)if((e[j]&&(e[j]<32||e[j]>126))||(i>=8&&e[j]))return false;
        for(size_t at=pos;at<pos+bytes;at+=228)if(memcmp(p+at,"\x01\x45\x14\x5a",4))return false;
        pos+=bytes;
    }
    return pos==n;
}
unsigned fm1_sample_encode(uint8_t *out,unsigned cmd,unsigned token,const uint8_t *data,unsigned size)
{
    if(size>132||token>16383||cmd>63)return 0;
    memcpy(out,"\xf0\x7d" "FM\x02",5);out[5]=cmd;out[6]=token&127;out[7]=token>>7;
    unsigned value=0,bits=0,n=8,sum=0;
    for(unsigned i=0;i<size;i++){value|=(unsigned)data[i]<<bits;bits+=8;while(bits>=7){out[n++]=value&127;value>>=7;bits-=7;}}
    if(bits)out[n++]=value&127;
    for(unsigned i=1;i<n;i++)sum+=out[i];
    out[n++]=(-sum)&127;out[n++]=0xf7;return n;
}
int fm1_sample_decode(const uint8_t *p,unsigned n,unsigned cmd,unsigned token,uint8_t out[65])
{
    if(n<12||memcmp(p,"\xf0\x7d" "FM\x02",5)||p[n-1]!=0xf7)return -1;
    unsigned sum=0;for(unsigned i=1;i<n-1;i++){if(p[i]&128)return -1;sum+=p[i];}if(sum&127)return -1;
    if(p[5]!=(cmd|64)||(p[6]|(unsigned)p[7]<<7)!=token)return 0;
    unsigned value=0,bits=0,k=0;
    for(unsigned i=8;i<n-2;i++){value|=(unsigned)p[i]<<bits;bits+=7;if(bits>=8){if(k==65)return -1;out[k++]=value&255;value>>=8;bits-=8;}}
    return value||bits>=7||!k?-1:(int)k;
}
