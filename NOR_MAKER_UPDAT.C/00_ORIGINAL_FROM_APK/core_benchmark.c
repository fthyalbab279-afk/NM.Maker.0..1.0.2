#include "gml_frontend.h"
#include "gml_vm.h"
#include "gm82_gmk_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

static double now_sec(void) { return (double)clock() / (double)CLOCKS_PER_SEC; }
static unsigned char *read_file(const char *path, size_t *size) {
    FILE *f=fopen(path,"rb"); if(!f)return NULL; fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
    if(n<0){fclose(f);return NULL;} unsigned char *p=(unsigned char*)malloc((size_t)n); if(!p){fclose(f);return NULL;}
    if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;} fclose(f); *size=(size_t)n; return p;
}
int main(int argc, char **argv) {
    const int iterations=1000; const char *fixture=argc>1?argv[1]:"build_test/gmk_payload_fixture.gmk";
    gml_ast *root=NULL; char error[160]={0};
    if(!gml_parse_program("x=0; for(i=0; i<100; i+=1) { x += i; }",&root,error,sizeof error)){fprintf(stderr,"parse: %s\n",error);return 2;}
    double t0=now_sec(); double checksum=0;
    for(int k=0;k<iterations;k++){gml_vm vm;gml_vm_init(&vm);if(!gml_vm_execute(&vm,root)){fprintf(stderr,"vm: %s\n",vm.error);gml_ast_free(root);return 3;}gml_value v=gml_vm_get(&vm,"x");checksum+=v.real;gml_value_free(&v);gml_value_free(&vm.return_value);}
    double vm_sec=now_sec()-t0; gml_ast_free(root);
    size_t n=0; unsigned char *bytes=read_file(fixture,&n); if(!bytes){perror(fixture);return 4;}
    t0=now_sec(); size_t json_bytes=0;
    for(int k=0;k<iterations;k++){char *json=gm82_gmk_resource_manifest_json(bytes,n);if(!json){free(bytes);return 5;}json_bytes+=strlen(json);free(json);}
    double gmk_sec=now_sec()-t0; free(bytes);
    printf("CORE_BENCHMARK_PASS iterations=%d vm_seconds=%.6f gmk_seconds=%.6f checksum=%.0f json_bytes=%zu\n",iterations,vm_sec,gmk_sec,checksum,json_bytes);
    return 0;
}
