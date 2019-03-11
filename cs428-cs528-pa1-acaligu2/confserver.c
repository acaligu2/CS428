/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* conference server */

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

extern char * recvtext(int sd);
extern int sendtext(int sd, char *msg);

extern int startserver();
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
int fd_isset(int fd, fd_set *fsp) {
    return FD_ISSET(fd, fsp);
}
/* main routine */
int main(int argc, char *argv[]) {

    char* msg;

    int servsock; /* server socket descriptor */

    fd_set livesdset; /* set of live client sockets */
    int livesdmax; /* largest live client socket descriptor */

    /* check usage */
    if (argc != 1) {
        fprintf(stderr, "usage : %s\n", argv[0]);
        exit(1);
    }

    /* get ready to receive requests */
    servsock = startserver();
    if (servsock == -1) {
        perror("Error on starting server: ");
        exit(1);
    }

    struct timeval tv;

    FD_ZERO(&livesdset);

    FD_SET(servsock, &livesdset);
    livesdmax = servsock;

    /* receive requests and process them */
    while (1) {

        int frsock; /* loop variable */
        fd_set localReadyCopy;
        FD_ZERO(&localReadyCopy);

        memcpy((void*)&localReadyCopy, (void*)&livesdset, sizeof(fd_set));

        select(livesdmax+1, &localReadyCopy, NULL, NULL, NULL);

        /* look for messages from live clients */
        for (frsock = 3; frsock <= livesdmax; frsock++) {	
            /* skip the listen socket */
            /* this case is covered separately */
            if (frsock == servsock)
                continue;

            if (fd_isset(frsock, &localReadyCopy)) {
                char* clienthost; /* host name of the client */
                ushort clientport; /* port number of the client */

                struct sockaddr_in address1;
                socklen_t addressLength = sizeof(address1);
                struct hostent *host;

                getpeername(frsock, (struct sockaddr*)&address1, &addressLength);

                host = gethostbyaddr(&(address1.sin_addr), sizeof(addressLength), AF_INET);

                clientport = ntohs(address1.sin_port);
                clienthost = host->h_name;

                /* read the message */
                msg = recvtext(frsock);
                if (!msg) {
                    /* disconnect from client */
                    printf("admin: disconnect from '%s(%hu)'\n", clienthost,
                            clientport);

                     FD_CLR(frsock, &livesdset);

                     if(frsock == livesdmax){

                       while(!fd_isset(--livesdmax, &livesdset));

                     }

                    /* close the socket */
                    close(frsock);
                } else {

                    //Loop through all active sockets to send message
                    for(int activeSock = 3; activeSock <= livesdmax; activeSock++){

                      //Ignore client sending the message and server socket
                      if(activeSock == frsock || activeSock == servsock){ continue; }

                      //Send the message to all other clients
                      else if(FD_ISSET(activeSock, &livesdset)){ sendtext(activeSock, msg); }

                    }

                    /* display the message */
                    printf("%s(%hu): %s", clienthost, clientport, msg);

                    /* free the message */
                    free(msg);
                }
            }
        }

        /* look for connect requests */
        if(fd_isset(servsock, &localReadyCopy)) {

            struct hostent *h;

            struct sockaddr_in incomingAddress;
            socklen_t incomingSize = sizeof(incomingAddress);
            //Accepting new socket request
            int csd = accept(servsock, (struct sockaddr*)&incomingAddress, &incomingSize);

            //Add new socket to the fd_set
            FD_SET(csd, &livesdset);

            //Update max socket value if necessary
            if(csd > livesdmax){ livesdmax = csd;}

            /* if accept is fine? */
            if (csd != -1) {
                char* clienthost; /* host name of the client */
                ushort clientport; /* port number of the client */

                 h = gethostbyaddr(&(incomingAddress.sin_addr), sizeof(incomingAddress), AF_INET);

                 clientport = ntohs(incomingAddress.sin_port);
                 clienthost = h->h_name;

                printf("admin: connect from '%s' at '%hu'\n", clienthost,
                        clientport);

                 FD_SET(csd, &livesdset);

            } else {
                perror("Error with Accept");
                exit(0);
            }

        }
    }

    return 0;
}
/*--------------------------------------------------------------------*/
