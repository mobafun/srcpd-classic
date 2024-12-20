/* $Id: srcp-server.h 1458 2010-03-01 13:07:56Z mtrute $ */

/*
 * Vorliegende Software unterliegt der General Public License,
 * Version 2, 1991. (c) Matthias Trute, 2000-2001.
 *
 */

#ifndef _SRCP_SERVER_H
#define _SRCP_SERVER_H

#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>

#include "config-srcpd.h" /*for bus_t*/
#include "config.h"       /*for VERSION*/

/* SRCP server welcome message */
static const char WELCOME_MSG[] =
    "srcpd V" VERSION "; SRCP 0.8.4; SRCPOTHER 0.8.3\n";

typedef struct _SERVER_DATA {
    unsigned short int TCPPORT;
    char *listenip;
    char *username;
    char *groupname;
} SERVER_DATA;

typedef enum {
    ssInitializing = 0,
    ssRunning,
    ssTerminating,
    ssResetting
} server_state_t;

extern void set_server_state(server_state_t);
extern server_state_t get_server_state();

extern int readconfig_server(xmlDocPtr doc, xmlNodePtr node, bus_t busnumber);
extern int startup_SERVER(void);
extern int init_bus_server(bus_t);
extern void server_reset(void);
extern void server_shutdown(void);
extern int describeSERVER(bus_t bus, int addr, char *reply);
extern int infoSERVER(char *msg);

#endif
