/*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*/
/* conference client */

//Anthony Caligure: CS428

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#include <stdlib.h>

#define MAXMSGLEN  1024
#define STDIN 0

extern char * recvtext(int sd);
extern int sendtext(int sd, char *msg);

extern int hooktoserver(char *servhost, ushort servport);
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
int main(int argc, char *argv[]) {

    int sock;

    fd_set rfds;
    int retval;

    /* check usage */
    if (argc != 3) {
        fprintf(stderr, "usage : %s <servhost> <servport>\n", argv[0]);
        exit(1);
    }

    /* get hooked on to the server */
    sock = hooktoserver(argv[1], atoi(argv[2]));
    if (sock == -1) {
        perror("Error: ");
        exit(1);
    }

    FD_ZERO(&rfds);

    FD_SET(0, &rfds);
    FD_SET(sock, &rfds);

    /* keep talking */
    while (1) {

         FD_ZERO(&rfds);

         FD_SET(0, &rfds);
         FD_SET(sock, &rfds);

         int messageReady = select(sock + 1, &rfds, NULL, NULL, NULL);

         if(FD_ISSET(sock, &rfds) && messageReady > 0){
           char *msg;
           msg = recvtext(sock);
           if (!msg) {
                /* server killed, exit */
                fprintf(stderr, "error: server died\n");
                exit(1);
            }

            /* display the message */
            printf(">>> %s", msg);
            /* free the message */
            free(msg);

            messageReady--;

        }

        if (FD_ISSET(STDIN, &rfds) && messageReady > 0) {
          char msg[MAXMSGLEN];
          if (!fgets(msg, MAXMSGLEN, stdin))
            exit(0);
            sendtext(sock, msg);
            messageReady--;
        }

        printf(">");
        fflush(stdout);
    }
    return 0;
}
/*--------------------------------------------------------------------*/
