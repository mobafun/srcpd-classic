/* $Id: srcp-gl.h 1691 2014-02-20 20:15:53Z gscholz $ */

#ifndef _SRCP_GL_H
#define _SRCP_GL_H

#include <stdbool.h>
#include <sys/time.h>

#include "config-srcpd.h"
#include "srcp-session.h"

/* locomotive decoder */
typedef struct _GLSTATE {
    int state; /* 0==dead, 1==living, 2==terminating */
    char protocol;
    int protocolversion;
    int n_func;         /* number of functions */
    int n_fs;           /* number of speed steps*/
    int id;             /* address  */
    int speed;          /* Sollgeschwindigkeit skal. auf 0..14 */
    int direction;      /* 0/1/2                               */
    unsigned int funcs; /* Fx..F1, F                           */
    struct timeval tv;  /* Last time of change                 */
    struct timeval inittime;
} gl_state_t;

typedef struct _GL {
    int numberOfGl;
    gl_state_t *glstate;
    struct timeval locktime;
    long int lockduration;
    sessionid_t locked_by;
} GL;

extern int startup_GL(void);
extern int init_GL(bus_t busnumber, int number);
extern int getMaxAddrGL(bus_t busnumber);
extern bool isInitializedGL(bus_t busnumber, int addr);
extern int isValidGL(bus_t busnumber, int addr);
extern int enqueueGL(bus_t busnumber, int addr, int dir, int speed,
                     int maxspeed, int f);
extern int queue_GL_isempty(bus_t busnumber);
extern int dequeueNextGL(bus_t busnumber, gl_state_t *l);
extern int cacheGetGL(bus_t busnumber, int addr, gl_state_t *l);
extern int cacheSetGL(bus_t busnumber, int addr, gl_state_t l);
extern int cacheInfoGL(bus_t busnumber, int addr, char *info);
extern int cacheDescribeGL(bus_t busnumber, int addr, char *msg);
extern int cacheInitGL(bus_t busnumber, int addr, const char protocol,
                       int protoversion, int n_fs, int n_func);
extern int cacheTermGL(bus_t busnumber, int addr);
extern int cacheLockGL(bus_t busnumber, int addr, long int duration,
                       sessionid_t sessionid);
extern int cacheGetLockGL(bus_t busnumber, int addr, sessionid_t *sessionid);
extern int cacheUnlockGL(bus_t busnumber, int addr, sessionid_t sessionid);
extern void unlock_gl_bysessionid(sessionid_t sessionid);
extern void unlock_gl_bytime(void);
extern int describeLOCKGL(bus_t bus, int addr, char *reply);
extern void debugGL(bus_t busnumber, int start, int end);

#endif
