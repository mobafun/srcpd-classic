/**
 * C++ Interface: portio
 *
 * Description:
 *
 *
 * Author: Ing. Gerard van der Sel
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#ifndef _PORTIO_H
#define _PORTIO_H

#include "config-srcpd.h"

extern int open_port(bus_t bus);
extern void close_port(bus_t bus);
extern void write_port(bus_t bus, unsigned char b);
extern int check_port(bus_t bus);
extern unsigned int read_port(bus_t bus);

#endif
