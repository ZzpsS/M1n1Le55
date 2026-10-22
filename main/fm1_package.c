#include "fm1_package.h"
#include <string.h>
#include "fm1_ota.h"

bool fm1_package_name(const char *s)
{
    size_t n=strlen(s);
    if(n<6 || n>63 || strcmp(s+n-5,".fwsc") || s[0]=='.')return false;
    for(size_t i=0;i<n;i++)
        if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||
             (s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='_'||s[i]=='.'))return false;
    return strstr(s,"..")==NULL;
}

// The 20 marker bytes encode only product/version, not package integrity.
bool fm1_package_identity(const uint8_t *h,size_t size,char identity[21])
{
    identity[0]=0;if(size<960)return false;
    unsigned used=0;
    for(unsigned i=0;i<20;i++) {
        uint8_t m=h[i*48+47];if(m==0x7d)continue;
        uint8_t c=(uint8_t)(m-i-1);
        if(c<32 || c>126)return false;
        identity[used++]=(char)c;
    }
    identity[used]=0;
    return fm1_ota_identity_valid(identity)&&!fm1_ota_loader(identity);
}
static int hex(char c)
{
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
size_t fm1_hex_decode(const char *s,uint8_t *out,size_t cap)
{
    size_t n=strlen(s);
    if(!n||n%2||n/2>cap)return 0;
    for(size_t i=0;i<n/2;i++) {
        int a=hex(s[i*2]),b=hex(s[i*2+1]);if(a<0||b<0)return 0;
        out[i]=(uint8_t)(a*16+b);
    }
    return n/2;
}
bool fm1_hash_parse(const char *s,uint8_t out[32])
{return fm1_hex_decode(s,out,32)==32;}
void fm1_hash_text(const uint8_t h[32],char s[65])
{
    const char *digits="0123456789abcdef";
    for(unsigned i=0;i<32;i++){s[i*2]=digits[h[i]>>4];s[i*2+1]=digits[h[i]&15];}s[64]=0;
}

static const fm1_release_t releases[]={
    {"M-VAVE official V15","FM-1_015","db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a",699956,false,false},
    {"Felucca 1.6.1","FM-1_916","dff3a38f7e5b9833e15aba322558c9485e9e422e3ff813c616c91b3584a2d909",610090,true,true},
    {"SLOOP 2.7","FM-1_900","194a254cc4df80e27b52efee58821c304391c1c0c4df8d61fa23c2c246f8c9b3",610035,true,true},
    {"FM-1+VA 101","FM-1_101","b43d6fcdf5618b83e3294b84d0b177f490956b903024452d021cd7667a61f9b4",859700,true,false},
    {"FoMni 0.4","FM-1_8000400","71fa8304b91bff89078bfd27e981ed28ca4f0d3c5f406e8fdb5a2ab240fbdbb5",609661,true,true},
    {"X0X 1.1.0","FM-1_9010100","a0c29deb930c65f1d40b5691b7d673f861418b438b7e65027cb9c671e6f44f5b",609684,true,true},
    {"Jangada 1.0","FM-1_910","d850c74b76b79e2c5918de251c74e3bd3ec65c55dfb42952cc74ce8e80c1d663",610133,true,true},
    // Open packages also expose the legacy ota-FM1C_0000000000000000 RAM loader.
    {"FM1 Open V12","FM-1_8000012","2dd4f2d2f020851f8797752371704a9a541aa747f4e20419d629de9aa2c66f43",475700,true,false},
    {"BLE RAM probe V912 (not OTA)","FM-1_8000912","866d5a157ae5bca7f9774626949dd4e6d47b8f18cdf72e90099880caada9d75e",475700,true,false},
    {"BLE diagnostics V913 (not OTA)","FM-1_8000913","38f09ce30f45300a816985f3234a3a302d014b8d93eb03744c1c6952ca37d1bf",479796,true,false}
};
const fm1_release_t *fm1_release_find(const char *hash,size_t size,const char *identity)
{
    for(unsigned i=0;i<sizeof(releases)/sizeof(*releases);i++)
        if(size==releases[i].size&&!strcmp(hash,releases[i].sha256)&&!strcmp(identity,releases[i].identity))return &releases[i];
    return NULL;
}
static const fm1_release_t *by_identity(const char *id)
{
    for(unsigned i=0;i<sizeof(releases)/sizeof(*releases);i++)
        if(!strcmp(id,releases[i].identity))return &releases[i];
    return NULL;
}
bool fm1_release_loader_matches(const char *target,const char *loader)
{
    const fm1_release_t *r=by_identity(target);
    if(!r||!fm1_ota_loader(loader))return false;
    if(fm1_ota_open_loader(loader))
        return !strcmp(target,"FM-1_015")||!strcmp(target,"FM-1_8000012")||
            !strcmp(target,"FM-1_8000912")||!strcmp(target,"FM-1_8000913");
    // These pinned fork packages all contain the app-only loader identifying as 900.
    // The stock loader writes a different layout: never resume it with a fork image.
    if(r->app_only_loader)return !strcmp(loader,"ota-FM-1_900");
    const char *id=loader+4;
    return (strlen(id)==8&&!strncmp(id,"FM-1_0",6)&&id[6]<='1'&&(id[6]!='1'||id[7]<='5'))||!strcmp(id,"FM-1_101");
}
static bool open_release(const fm1_release_t *r)
{
    return r&&(!strcmp(r->identity,"FM-1_8000012")||!strcmp(r->identity,"FM-1_8000912")||!strcmp(r->identity,"FM-1_8000913"));
}
bool fm1_release_needs_bridge(const fm1_release_t *r,const char *source)
{
    if(!r||!r->custom||!fm1_ota_identity_valid(source))return false;
    if(fm1_ota_loader(source))return !fm1_release_loader_matches(r->identity,source)&&fm1_release_loader_matches("FM-1_015",source);
    const fm1_release_t *from=by_identity(source);
    bool stock=strlen(source)==8&&!strncmp(source,"FM-1_0",6)&&source[6]<='1'&&(source[6]!='1'||source[7]<='5');
    // Fork updaters require the original boot header. Open and fork images
    // have different headers/loaders; preserve the verified V15 migration route.
    if(open_release(r)&&from&&from->app_only_loader)return true;
    if(stock&&strcmp(source,"FM-1_015"))return true;
    if(r->app_only_loader&&from&&from->custom&&!from->app_only_loader)return true;
    // Open's legacy RAM loader is verified for Open and stock recovery only.
    return open_release(from)&&!strcmp(r->identity,"FM-1_101");
}
const char *fm1_release_block(const fm1_release_t *r,const char *source)
{
    if(!r)return "Package has no supported install profile";
    if(!fm1_ota_identity_valid(source))return "Connect an identified FM-1 first";
    if(fm1_ota_loader(source))return (fm1_release_loader_matches(r->identity,source)||fm1_release_needs_bridge(r,source))?NULL:
        "Loader differs: finish with its matching firmware package";
    const fm1_release_t *from=by_identity(source);
    bool stock=strlen(source)==8&&!strncmp(source,"FM-1_0",6)&&source[6]<='1'&&(source[6]!='1'||source[7]<='5');
    if(!from&&!stock)return "Current firmware has no supported install path";
    if(!r->custom && !strcmp(source,"FM-1_015"))return "Official V15 is already running";
    return NULL;
}
size_t fm1_package_logical(uint8_t *raw,size_t n)
{
    if(n<960)return 0;
    for(size_t i=0;i<20;i++)memmove(raw+i*47,raw+i*48,47);
    memmove(raw+940,raw+960,n-960);return n-20;
}
