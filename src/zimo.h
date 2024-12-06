/* $Id: zimo.h 983 2007-12-16 09:06:42Z gscholz $ */

#ifndef _ZIMO_H
#define _ZIMO_H

#include <libxml/tree.h> /*xmlDocPtr, xmlNodePtr*/

typedef struct _zimo_DATA {
    int number_ga;
    int number_gl;
    int number_fb;
} zimo_DATA;

extern int readconfig_ZIMO(xmlDocPtr doc, xmlNodePtr node, bus_t busnumber);
extern int init_bus_ZIMO(bus_t);
extern int getDescription_ZIMO(char *reply);
extern void *thr_sendrec_ZIMO(void *);

#endif
