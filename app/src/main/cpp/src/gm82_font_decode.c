#define _POSIX_C_SOURCE 200809L
#include "gm82_font_decode.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static int32_t rd_i32(const uint8_t *p) {
    return (int32_t)(p[0]|p[1]<<8|p[2]<<16|p[3]<<24);
}
static uint8_t *inflate_at(const uint8_t *src, size_t n, size_t *ol) {
    *ol=0; z_stream strm; memset(&strm,0,sizeof(strm));
    if (inflateInit2(&strm,15)!=Z_OK) return NULL;
    size_t cap=n*8+256; uint8_t *dst=malloc(cap);
    if(!dst){inflateEnd(&strm);return NULL;}
    strm.next_in=(Bytef*)src; strm.avail_in=(uInt)n;
    strm.next_out=dst; strm.avail_out=(uInt)cap;
    int ret;
    while((ret=inflate(&strm,Z_NO_FLUSH))==Z_OK){
        if(strm.avail_out==0){size_t used=cap;cap*=2;uint8_t*nd=realloc(dst,cap);
            if(!nd){free(dst);inflateEnd(&strm);return NULL;}
            dst=nd;strm.next_out=dst+used;strm.avail_out=(uInt)(cap-used);}
    }
    if(ret!=Z_STREAM_END){free(dst);inflateEnd(&strm);return NULL;}
    *ol=strm.total_out;inflateEnd(&strm);return dst;
}
void gm82_decoded_font_list_free(gm82_decoded_font_list *L) {
    if(!L)return; free(L->items); memset(L,0,sizeof(*L));
}
int gm82_decode_fonts_from_gmk(const uint8_t *data, size_t size, gm82_decoded_font_list *out) {
    memset(out,0,sizeof(*out));
    if(!data||size<12) return -1;
    gm82_decoded_font tmp[32]; int n=0;
    for(size_t i=12;i+2<size;i++){
        if(!(data[i]==0x78&&(data[i+1]==0x9c||data[i+1]==0xda||data[i+1]==0x01||data[i+1]==0x5e))) continue;
        size_t ol=0; uint8_t *d=inflate_at(data+i,size-i,&ol); i+=16;
        if(!d||ol<40){free(d);continue;}
        if(rd_i32(d)!=1){free(d);continue;}
        int32_t slen=rd_i32(d+4);
        if(slen<3||slen>48||8+(size_t)slen+20>ol){free(d);continue;}
        char name[64];memcpy(name,d+8,(size_t)slen);name[slen]=0;
        int ok=1;for(int k=0;k<slen;k++)if(name[k]<32||name[k]>126)ok=0;
        if(!ok||(strncmp(name,"font",4)!=0&&strncmp(name,"fnt_",4)!=0)){free(d);continue;}
        if(n>=32){free(d);break;}
        gm82_decoded_font *f=&tmp[n++];
        memset(f,0,sizeof(*f));
        strncpy(f->name,name,sizeof(f->name)-1);
        size_t off=8+(size_t)slen+8+4;
        if(off+4<=ol){
            int32_t nlen=rd_i32(d+off); off+=4;
            if(nlen>0&&nlen<63&&off+(size_t)nlen<=ol){
                memcpy(f->font_name,d+off,(size_t)nlen); off+=(size_t)nlen;
            }
            if(off+12<=ol){
                f->size=rd_i32(d+off); off+=4;
                f->bold=rd_i32(d+off); off+=4;
                f->italic=rd_i32(d+off);
            }
        }
        free(d);
    }
    out->count=n;
    if(n>0){out->items=malloc((size_t)n*sizeof(*out->items));
        if(out->items)memcpy(out->items,tmp,(size_t)n*sizeof(*tmp)); else out->count=0;}
    return out->count;
}
