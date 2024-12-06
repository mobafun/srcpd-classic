/* $Id: loopback.h 1176 2008-01-29 22:42:17Z schmischi $ */

#ifndef _LOOPBACK_H
#define _LOOPBACK_H

#include <libxml/tree.h> /*xmlDocPtr, xmlNodePtr*/

typedef struct _LOOPBACK_DATA {
    int number_ga;
    int number_gl;
    int number_fb;
    ga_state_t tga[50];
} LOOPBACK_DATA;

extern int readconfig_LOOPBACK(xmlDocPtr doc, xmlNodePtr node, bus_t busnumber);
extern int init_bus_LOOPBACK(bus_t);
extern int init_gl_LOOPBACK(gl_state_t *);
extern int init_ga_LOOPBACK(ga_state_t *);
extern int getDescription_LOOPBACK(char *reply);
extern void *thr_sendrec_LOOPBACK(void *);

#endif
