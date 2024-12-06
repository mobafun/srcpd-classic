/* $Id: dccar.c 2012-02-19 15:00 achim_marikar $ */

#ifndef _DCCAR_H
#define _DCCAR_H

#include <libxml/tree.h>

#define DCCAR 0
#define INFRACAR 1

typedef struct _DCCAR_DATA {
    int mode;
    int number_gl;
    unsigned int pause_between_cmd;
} DCCAR_DATA;

extern int readconfig_DCCAR(xmlDocPtr doc, xmlNodePtr node, bus_t busnumber);
extern int init_bus_DCCAR(bus_t);
extern int init_gl_DCCAR(gl_state_t *);
extern int getDescription_DCCAR(char *reply);
extern void *thr_sendrec_DCCAR(void *);
extern void set_address_bytes(unsigned char[3], int);
extern void write_command(unsigned char *, int, int, bus_t);
extern int fd;
extern int mode;

#endif
