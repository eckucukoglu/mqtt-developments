#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

int main (int argc, char* argv[]) {
    struct timeval tv;
    long long int ts_usec_start, ts_usec_end;

    gettimeofday(&tv, NULL);
    ts_usec_start = ((long long int) tv.tv_sec) * 1000000ll +
        (long long int) tv.tv_usec;
    printf("s.%f\t", (double)ts_usec_start/1000000);

    return 0;
}
