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

#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/tcp_impl.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "ipv4/ip_addr.h"


unsigned char debug_flags = LWIP_DBG_OFF;

#define SEND_STR "brown lazy"
#define SEND_LEN 10

static void
brownlazy_main(void *arg)
{
    printf("brownlazy_main(): hello\n");
    int listenfd;
    struct sockaddr_in brownlazy_saddr, brownlazy_caddr;
    LWIP_UNUSED_ARG(arg);

    /* First acquire our socket for listening for connections */
    listenfd = lwip_socket(AF_INET, SOCK_STREAM, 0);

    LWIP_ASSERT("brownlazy_main(): socket create failed.", listenfd >= 0);
    memset(&brownlazy_saddr, 0, sizeof(brownlazy_saddr));
    brownlazy_saddr.sin_family = AF_INET;
    brownlazy_saddr.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
    brownlazy_saddr.sin_port = htons(19);     /* Chargen server port */

    if (lwip_bind(listenfd, (struct sockaddr *) &brownlazy_saddr, sizeof(brownlazy_saddr)) == -1)
        LWIP_ASSERT("brownlazy_main(): socket bind failed.", 0);

    /* Put socket into listening mode */
    if (lwip_listen(listenfd, 3) == -1)
        LWIP_ASSERT("brownlazy_main(): Listen failed.", 0);

    /* Wait forever for network input: This could be connections or data */
    for (;;) {
        unsigned clilen = sizeof(brownlazy_caddr);
        int newsockfd = lwip_accept(listenfd, (struct sockaddr *)&brownlazy_caddr, &clilen);
        printf("brownlazy_main(): got conn from port %d\n",
                ntohs(brownlazy_caddr.sin_port));
        if (newsockfd < 0) {
            perror("brownlazy_main(): error on accept");
            continue;
        }
        int err = lwip_send(newsockfd,SEND_STR,SEND_LEN,0);
        lwip_close(newsockfd);
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
    LWIP_UNUSED_ARG(arg);
    int connfd;
    struct sockaddr_in brownlazy_saddr;

    connfd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    LWIP_ASSERT("brownlazy_guest(): socket failed", connfd >= 0);
    memset(&brownlazy_saddr, 0, sizeof(brownlazy_saddr));
    brownlazy_saddr.sin_family = AF_INET;
    brownlazy_saddr.sin_addr.s_addr = PP_HTONL(INADDR_LOOPBACK);
    brownlazy_saddr.sin_port = htons(19);     /* Chargen server port */

    int n_tries = 5;
    while (lwip_connect(connfd, &brownlazy_saddr, sizeof(brownlazy_saddr)) < 0) {
        perror("brownlazy_guest(): connect failed\n");
        if (n_tries-- == 0) break;
    }
    if (n_tries < 0) {
        printf("brownlazy_guest(): failed for too many times\n");
    } else {
        printf("brownlazy_guest(): connected to server\n");
        char recvbuf[15];
        int n_recv = lwip_recv(connfd, recvbuf, sizeof(recvbuf)-1, 0);
        if (n_recv < 0)
            perror("brownlazy_guest(): receive failed\n");
        recvbuf[n_recv] = 0;
        printf("brownlazy_guest(): got: %s\n", recvbuf);
        printf("brownlazy_guest(): guest bye\n");
    }
}


int
main(int argc, char **argv)
{
    tcpip_init(NULL, NULL);
    printf("TCP/IP initialized.\n");

    sys_thread_new("brownlazy", brownlazy_main, 0, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
    sys_thread_new("guest", brownlazy_guest, 0, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);

    // spin
    while (1) ;
}
/*-----------------------------------------------------------------------------------*/
