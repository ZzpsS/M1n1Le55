/* SPDX-License-Identifier: MIT
 * SEA decoding adapted from M-VAVE's portable decoder (c) 2026 M-VAVE,
 * based on SEA (c) 2025 Dani Biro. See licenses/sea-waveform.txt.
 * The peak aggregation and bank bounds are specific to M1n1Le55. */
#include "m1_waveform.h"
#include "fm1_sample_format.h"
#include <string.h>
#include "m1_sea_table.inc"
static int32_t clamp16(int32_t v){return v>32767?32767:v<-32768?-32768:v;}
bool m1_sample_peaks(const uint8_t *bank,size_t bytes,uint8_t peaks[8][64],uint32_t frames[8])
{
    if(!fm1_sample_validate(bank,bytes))return false;
    memset(peaks,0,8*64);
    for(unsigned slot=0;slot<8;slot++){
        const uint8_t *entry=bank+32+slot*28;uint32_t count=fm1_sample_u32(entry+4),start=fm1_sample_u32(entry);
        frames[slot]=count;
        for(unsigned base=0;base<count;base+=320){
            const uint8_t *chunk=bank+start+(base/320)*228;int32_t state[8];
            for(unsigned i=0;i<8;i++){unsigned v=chunk[4+i*2]|(unsigned)chunk[5+i*2]<<8;state[i]=v<32768?(int32_t)v:(int32_t)v-65536;}
            for(unsigned i=0;i<320&&base+i<count;i++){
                unsigned g=i/20,scale=(chunk[20+g/2]>>((g&1)?0:4))&15,code=0,at=i*5;
                for(unsigned b=0;b<5;b++)code=(code<<1)|((chunk[28+(at+b)/8]>>(7-(at+b)%8))&1);
                int64_t sum=0;for(unsigned j=0;j<4;j++)sum+=(int64_t)state[j]*state[4+j];
                int32_t residual=sample_dqt[scale*32+code],value=clamp16((int32_t)(sum>>13)+residual),delta=residual>>4;
                for(unsigned j=0;j<4;j++)state[4+j]=clamp16(state[4+j]+(state[j]<0?-delta:delta));
                for(unsigned j=0;j<3;j++)state[j]=state[j+1];
                state[3]=value;
                unsigned bin=(uint64_t)(base+i)*64/count,amp=(value<0?-value:value)*255u/32768u;
                if(amp>peaks[slot][bin])peaks[slot][bin]=amp;
            }
        }
    }
    return true;
}
