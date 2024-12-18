/* $Id: srcp-ga.h 1529 2011-10-06 16:32:39Z andre_schenk $ */

#ifndef _SRCP_GA_H
#define _SRCP_GA_H

#include <stdbool.h>
#include <sys/time.h>

#include "config-srcpd.h"
#include "srcp-session.h"

#define MAXGAPORT 8

/* accessory decoder */
typedef struct _GASTATE {
    int state;                  /* 0==dead, 1==living, 2==terminating */
    char protocol;              /* Protocol Id */
    int id;                     /* Identification */
    int port;                   /* Port number     */
    int action;                 /* 0, 1, 2, 3...  */
    long activetime;            /* Aktivierungszeit in msec bis das 
                                   automatische AUS kommen soll */
    struct timeval inittime;
    struct timeval tv[MAXGAPORT];   /* time of last activation, each port 
                                       gets its own time value */
    struct timeval t;           /* switch off time */
    struct timeval locktime;
    sessionid_t locked_by;      /* who has the LOCK? */
    long int lockduration;
} ga_state_t;

typedef struct _GA {
    int numberOfGa;
    ga_state_t* gastate;
} GA;

extern int startup_GA(void);
extern int init_GA(bus_t busnumber, int number);
extern int get_number_ga(bus_t busnumber);

extern int enqueueGA(bus_t busnumber, int addr, int port, int action,
            long int activetime);
extern int dequeueNextGA(bus_t busnumber, ga_state_t *);
extern int queue_GA_isempty(bus_t busnumber);

extern int getGA(bus_t busnumber, int addr, ga_state_t *a);
extern int setGA(bus_t busnumber, int addr, ga_state_t a);
extern int initGA(bus_t busnumber, int addr, const char protocol);
extern int termGA(bus_t busnumber, int addr);
extern int describeGA(bus_t busnumber, int addr, char *msg);
extern int infoGA(bus_t busnumber, int addr, int port, char *msg);
extern int cmpGA(ga_state_t a, ga_state_t b);
extern bool isInitializedGA(bus_t busnumber, int addr);

extern int lockGA(bus_t busnumber, int addr, long int duration,
           sessionid_t sessionid);
extern int getlockGA(bus_t busnumber, int addr, sessionid_t * sessionid);
extern int unlockGA(bus_t busnumber, int addr, sessionid_t sessionid);
extern void unlock_ga_bysessionid(sessionid_t);
extern void unlock_ga_bytime(void);
extern int describeLOCKGA(bus_t bus, int addr, char *reply);
#endif
