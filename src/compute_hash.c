#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>


uint8_t yy_prof_compute_hash(char * str) {
    uint64_t h = 5381;
    uint32_t i = 0;
    uint8_t res = 0;

    while (*str) {
        h += (h << 5); 
        h ^= (ulong) *str++;
    }   

    for (i = 0; i < sizeof(ulong); i++) {
        res += ((uint8_t*)&h)[i];
    }   
    return res;
}


int main(int argc, char ** argv) {
    if(argc <= 1) {
        printf("Usage: %s <string>\n", *argv);
        return 0;
    }

    uint8_t ret = yy_prof_compute_hash((char *) *(argv + 1));

    printf("Hash of \"%s\" is %d\n", *(argv + 1), ret);


    return 0;
}
