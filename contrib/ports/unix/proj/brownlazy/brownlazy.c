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
#include "lwip/inet_chksum.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "netif/tapif.h"
#include "netif/tunif.h"
#include "netif/unixif.h"
#include "netif/dropif.h"
#include "netif/pcapif.h"
#include "netif/tcpdump.h"
#include "lwip/ip_addr.h"
#include "httpd.h"


/* (manual) host IP configuration */
static ip_addr_t ipaddr, netmask, gw;

unsigned char debug_flags = LWIP_DBG_OFF;

struct netif netif;


static void
init_netifs(void)
{
// no dhcp
  netif_set_default(netif_add(&netif,&ipaddr, &netmask, &gw, NULL, tapif_init,
                  tcpip_input));
  netif_set_up(&netif);

  httpd_init();
}


int
main(int argc, char **argv)
{
  IP4_ADDR(&gw, 192,168,0,1);
  IP4_ADDR(&netmask, 255,255,255,0);
  IP4_ADDR(&ipaddr, 192,168,0,2);

  printf("H 192.168.0.2;   M 255.255.255.0;   G 192.168.0.1\n");

  printf("System initialized.\n");

  tcpip_init(init_netifs, NULL);
  printf("TCP/IP initialized.\n");

  // spin
  while (1) ;
}
/*-----------------------------------------------------------------------------------*/

