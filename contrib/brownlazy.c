/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>

#include <lwip/def.h>
#include <lwip/tcpip.h>
#include <lwip/sockets.h>

#include <unistd.h>
#include <sys/syscall.h>

#define SEND_STR "brown lazy"
#define SEND_LEN 10

int ready = 0;
int sent = 0;

static void
brownlazy_main(void *arg)
{
    printf("brownlazy_main(): hello\n");
    int listenfd;
    struct sockaddr_in brownlazy_saddr, brownlazy_caddr;

    /* First acquire our socket for listening for connections */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0) {
        printf("brownlazy_main(): socket create failed.");
        return;
    }
    memset(&brownlazy_saddr, 0, sizeof(brownlazy_saddr));
    brownlazy_saddr.sin_family = AF_INET;
    brownlazy_saddr.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
    brownlazy_saddr.sin_port = PP_HTONS(19);     /* Chargen server port */

    if (bind(listenfd, (struct sockaddr *) &brownlazy_saddr, sizeof(brownlazy_saddr)) == -1) {
        printf("brownlazy_main(): socket bind failed.");
        return;
    }

    /* Put socket into listening mode */
    if (listen(listenfd, 3) == -1) {
        printf("brownlazy_main(): Listen failed.");
        return;
    }

    /* Wait forever for network input: This could be connections or data */
    ready = 1;
    for (;;) {
        unsigned clilen = sizeof(brownlazy_caddr);
        int newsockfd = accept(listenfd, (struct sockaddr *)&brownlazy_caddr, &clilen);
        printf("brownlazy_main(): got conn from port %d\n",
                PP_NTOHS(brownlazy_caddr.sin_port));
        if (newsockfd < 0) {
            perror("brownlazy_main(): error on accept");
            continue;
        }
        int err = syscall(334,newsockfd,SEND_STR,SEND_LEN,0);
        // syscall(333,newsockfd);
        sent = 1;
        if (err < 0) {
            perror("brownlazy_main(): error writing to socket");
        }
        break;  // only deal with one client
    }
    printf("brownlazy_main(): server bye\n");
}

static void
brownlazy_guest(void* arg)
{
    printf("brownlazy_guest(): hello\n");
    int connfd;
    struct sockaddr_in brownlazy_saddr;

    connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd < 0) {
        printf("brownlazy_guest(): socket failed");
        return;
    }
    memset(&brownlazy_saddr, 0, sizeof(brownlazy_saddr));
    brownlazy_saddr.sin_family = AF_INET;
    brownlazy_saddr.sin_addr.s_addr = PP_HTONL(INADDR_LOOPBACK);
    brownlazy_saddr.sin_port = PP_HTONS(19);     /* Chargen server port */

    int n_tries = 5;
    while (syscall(SYS_connect, connfd, &brownlazy_saddr, sizeof(brownlazy_saddr)) < 0) {
        perror("brownlazy_guest(): connect failed\n");
        if (n_tries-- == 0) break;
    }
    if (n_tries < 0) {
        printf("brownlazy_guest(): failed for too many times\n");
    } else {
        printf("brownlazy_guest(): connected to server\n");
        char recvbuf[15];
        while (!sent);
        int n_recv = syscall(335, connfd, recvbuf, sizeof(recvbuf)-1, 0);
        if (n_recv < 0)
            perror("brownlazy_guest(): receive failed\n");
        recvbuf[n_recv] = 0;
        printf("brownlazy_guest(): got: %s\n", recvbuf);
        printf("brownlazy_guest(): guest bye\n");
    }
    // syscall(333,connfd);
}


int
main()
{
    pthread_t pa, pb;
    pthread_create(&pa, NULL,
            brownlazy_main, NULL);
    while (!ready);
    pthread_create(&pb, NULL,
            brownlazy_guest, NULL);

    // spin
    while (1) ;
}
/*-----------------------------------------------------------------------------------*/
