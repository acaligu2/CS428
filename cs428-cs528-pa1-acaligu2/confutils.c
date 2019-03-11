/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* functions to connect clients and server */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#include <stdlib.h>

#define MAXNAMELEN 256
/*--------------------------------------------------------------------*/

/*----------------------------------------------------------------*/
/* prepare server to accept requests
 returns file descriptor of socket
 returns -1 on error
 */
int startserver() {
    int sd; /* socket descriptor */
    struct sockaddr_in sa; /* socket struct */

    char * servhost; /* full name of this host */
    ushort servport; /* port assigned to this server */

    //Initializing socket
    sd = socket(AF_INET, SOCK_STREAM, 0);

    if(sd < 0){
      perror("Failure to Create Socket\n");
      return -1;

    };

    //Binding socket to IP/Port
    sa.sin_family = AF_INET;
    sa.sin_port = 0;
    sa.sin_addr.s_addr = INADDR_ANY;

    if(bind(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0){

      perror("Couldn't bind socket\n");
      return -1;

    }

    /* we are ready to receive connections */
    listen(sd, 5);

    //Retrieve hostname
    servhost = (char*) malloc(128);
    char partialHostName[128];
    partialHostName[127] = '\0';
    gethostname(partialHostName, 127);
    struct hostent *he = gethostbyname(partialHostName);
    memcpy(&sa.sin_addr.s_addr, he->h_addr, he->h_length);

    strcpy(servhost, he->h_name);

    //Retrieve given port with getsockname
     int addrLength = sizeof(sa);
     if(getsockname(sd, (struct sockaddr *) &sa, &addrLength) == -1){

       perror("Issue with getsockname");

     }else{

       servport = ntohs(sa.sin_port);

     }

    /* ready to accept requests */
    printf("admin: started server on '%s' at '%hu'\n", servhost, servport);

    free(servhost);

    return (sd);
}
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
/*
 establishes connection with the server
 returns file descriptor of socket
 returns -1 on error
 */
int hooktoserver(char *servhost, ushort servport) {
    int sd = 0; /* socket descriptor */

    ushort clientport; /* port assigned to this client */

    //Try to create a socket
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd < 0){

      perror("Failure to Create Socket\n");
      return -1;

    };

    struct hostent* h;
    h = gethostbyname(servhost);

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(servport);

    memcpy((void*)&servAddr.sin_addr, h->h_addr_list[0], h->h_length);
    //memset(servAddr.sin_zero, '\0', sizeof(servAddr.sin_zero));

    socklen_t servAddrLength = sizeof(servAddr);

    printf("server address: %s\n", inet_ntoa(servAddr.sin_addr));

    if(connect(sd, (struct sockaddr*) &servAddr, servAddrLength) < 0){

        perror("Couldn't connect client to server\n");
        return -1;

    }

    int status = getsockname(sd, (struct sockaddr*)&servAddr, &servAddrLength);
    if(status == -1){ perror("getsockname error"); exit(1); }

    clientport = ntohs(servAddr.sin_port);

    /* succesful. return socket descriptor */
    printf("admin: connected to server on '%s' at '%hu' thru '%hu'\n", servhost,
            servport, clientport);
    printf(">");
    fflush(stdout);
    return (sd);
}
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
int readn(int sd, char *buf, int n) {
    int toberead;
    char * ptr;

    toberead = n;
    ptr = buf;
    while (toberead > 0) {
        int byteread;

        byteread = read(sd, ptr, toberead);
        if (byteread <= 0) {
            if (byteread == -1)
                perror("read");
            return (0);
        }

        toberead -= byteread;
        ptr += byteread;
    }
    return (1);
}

char *recvtext(int sd) {
    char *msg;
    long len;

    /* read the message length */
    if (!readn(sd, (char *) &len, sizeof(len))) {
        return (NULL);
    }
    len = ntohl(len);

    /* allocate space for message text */
    msg = NULL;
    if (len > 0) {
        msg = (char *) malloc(len);
        if (!msg) {
            fprintf(stderr, "error : unable to malloc\n");
            return (NULL);
        }

        /* read the message text */
        if (!readn(sd, msg, len)) {
            free(msg);
            return (NULL);
        }
    }

    /* done reading */
    return (msg);
}

int sendtext(int sd, char *msg) {
    long len;

    /* write lent */
    len = (msg ? strlen(msg) + 1 : 0);
    len = htonl(len);
    write(sd, (char *) &len, sizeof(len));

    /* write message text */
    len = ntohl(len);
    if (len > 0)
        write(sd, msg, len);
    return (1);
}
/*----------------------------------------------------------------*/
