#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const char *inname = (argc>1)? argv[1] : "example.file";
    const char *outname = (argc>2)? argv[2] : "/dev/cu.usbmodem42_dave1";

    int fin = open(inname, O_RDONLY);
    if (fin <= 0 ) {
        perror( inname );
        return 1;
    }

    int fout = open(outname, O_RDWR);
    if (fout <= 0 ) {
        perror( outname );
        return 1;
    }

    struct termios theTermios;
    memset(&theTermios, 0, sizeof(struct termios));
    cfmakeraw(&theTermios);
    cfsetspeed(&theTermios, 115200);
    theTermios.c_cflag = CREAD | CLOCAL; // turn on READ and ignore modem control lines
    theTermios.c_cflag |= CS8;
    theTermios.c_cc[VMIN] = 0;
    theTermios.c_cc[VTIME] = 10; // 1 sec timeout
    ioctl(fout, TIOCSETA, &theTermios);

    char buff[64];

    int total = 0;
    int loops;
    for (loops=0; loops<5000;loops++) {

        int got = read( fin, buff, 32 );
        if (got == 0) break;
        if (got < 0) {
            perror("read");
            break;
        }
        int sent = write( fout, buff, got);
        total += sent;
    }
    printf("Overall sent %d\n", total);
    if (loops == 5000) {
        printf("Hmm, runaway?\n");
        return 1;
    }
    printf("Finished\n");



    return 0;
}
