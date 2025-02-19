/* +----------------------------------------------------------------------+ */
/* | DDL - Digital Direct for Linux                                       | */
/* +----------------------------------------------------------------------+ */
/* | Copyright (c) 2002 - 2003 Vogt IT                                    | */
/* +----------------------------------------------------------------------+ */
/* | This source file is subject of the GNU general public license 2,     | */
/* | that is bundled with this package in the file COPYING, and is        | */
/* | available at through the world-wide-web at                           | */
/* | http://www.gnu.org/licenses/gpl.txt                                  | */
/* | If you did not receive a copy of the PHP license and are unable to   | */
/* | obtain it through the world-wide-web, please send a note to          | */
/* | gpl-license@vogt-it.com so we can mail you a copy immediately.       | */
/* +----------------------------------------------------------------------+ */
/* | Authors: Torsten Vogt vogt@vogt-it.com                               | */
/* |          For more read ChangeLog file                                | */
/* +----------------------------------------------------------------------+ */

/***************************************************************/
/* erddcd - Electric Railroad Direct Digital Command Daemon    */
/*    generates without any other hardware digital commands    */
/*    to control electric model railroads                      */
/*                                                             */
/* file: nmra.c                                                */
/* job : implements routines to compute data for the           */
/*       NMRA-DCC protocol and send this data to               */
/*       the serial device.                                    */
/*                                                             */
/* Torsten Vogt, may 1999                                      */
/*                                                             */
/* last changes: Torsten Vogt, September 2000                  */
/*               Torsten Vogt, January 2001                    */
/*               Torsten Vogt, April 2001                      */
/*               For more read ChangeLog file                  */
/***************************************************************/

/**********************************************************************
 data format:

 look at the srcp specification

 protocol formats:

 N: NMRA multi function decoder with 7/14-bit address, 14/28/128 speed steps
 (implemented)

 NA: accessory digital decoders
 (implemented)

 service mode instruction packets for direct mode
 (verify cv, write cv, cv bit manipulation)
 (implemented)

 service mode instruction packets for PoM mode
 (verify cv, write cv, cv bit manipulation)
 (only write implemented)

 service mode instruction packets for address-only mode
 (verify address contents, write address contents)
 (implemented)

 service mode instruction packets for physical register addressing
 (verify register contents, write register contents)
 (implemented)

 service mode instruction packets paged cv addressing
 (verify/write register/cv contents)
 (implemented)

 general notes:

   configuration of the serial port:

      start bit: 1
      stop bit : 1
      data bits: 8
      baud rate: 19200

      ==> one serial bit takes 52.08 usec.

      ==> NMRA-1-Bit: 01         (52 usec low and 52 usec high)
          NMRA-0-Bit: 0011       (at least 100 usec low and high)

      serial stream (only start/stop bits):

      0_______10_______10_______10_______10_______10_______10___ ...

      problem: how to place the NMRA-0- and NMRA-1-Bits in the serial stream

      examples:

      0          0xF0     _____-----
      00         0xC6     __--___---
      01         0x78     ____----_-
      10         0xE1     _-____----
      001        0x66     __--__--_-
      010        0x96     __--_-__--
      011        0x5C     ___---_-_-
      100        0x99     _-__--__--
      101        0x71     _-___---_-
      110        0xC5     _-_-___---
      0111       0x56     __--_-_-_-
      1011       0x59     _-__--_-_-
      1101       0x65     _-_-__--_-
      1110       0x95     _-_-_-__--
      11111      0x55     _-_-_-_-_-
                          ^        ^
                          start-   stop-
                          bit      bit

   known bugs (of version 1 of the NMRA dcc translation routine):
   (i hope version 2 don't have these bugs ;-) )

      following packets are not translatable:

        N1 031 1 06 0 0 0 0 0
        N1 047 0 07 0 0 0 0 0

        N2 031 0 091 0 0 0 0 0
        N2 031 1 085 0 0 0 0 0
        N2 031 1 095 0 0 0 0 0
        N2 047 0 107 0 0 0 0 0
        N2 047 1 103 0 0 0 0 0
        N2 047 1 111 0 0 0 0 0
        N2 048 1 112 0 0 0 0 0
        N2 051 1 115 0 0 0 0 0
        N2 053 1 117 0 0 0 0 0
        N2 056 0 124 0 0 0 0 0
        N2 057 1 113 0 0 0 0 0
        N2 058 1 114 0 0 0 0 0
        N2 059 1 115 0 0 0 0 0
        N2 060 1 116 0 0 0 0 0
        N2 061 1 117 0 0 0 0 0
        N2 062 1 118 0 0 0 0 0

     I think, that these are not really problems. The only consequence is
     e.g. that some addresses has 127 speed steps instead of 128. That's
     life, don't worry.

     New: completely new algorithm to generate the NMRA packet stream
     (i call it 'version 3' of the translate routines)

     The idea in this approach to generate NMRA patterns is, to split the
     starting and ending bit in each pattern and share it with the next
     pattern. Therefore the patterns, which could be generated, are coded with
     h, H, l and L, lowercase describing the half bits. The longest possible
     pattern contains two half h bits and four H bits. For the access into
     the coding table, the index of course doesn't differentiate between half
     and full bits, because the first one is always half and the last one
     determined by this table. This table shows, which bit pattern will be
     replaced by which pattern on the serial line. There is only one pattern
     left, which could not be directly translated. This pattern starts with
     an h, so we have to look at the patterns, which end with an h, if we want
     to avoid this pattern. All of the patterns, we access in the first try,
     contain an l or an L, which could be enlarged, to get rid of at least on
     bit in this pattern and don't have our problem in the next pattern. With
     the only exception of hHHHHh, but this pattern simply moves our problem
     into one byte before or up to the beginning of the sequence. And there, we
     could always add a bit. So we are sure, to be able, to translate any
     given sequence of bits.

    Because only the case of hHHHHh really requires 6 bits, the translation table
    could be left at 32 entries. The other case, which has the same first five
    bits is our problem, so we have to handle it separately anyway.

    Of course the resulting sequence is not dc free. But if this is required,
    one could replace the bytes in the TranslateData by an index in another
    table which holds for each data containing at least an L or two l
    replacements with different dc components. This way one could get a dc
    free signal or at will a signal with a given dc part.

    #define ll          0xf0  _____-----

    #define lLl         0xcc  ___--__---    000000  000001  000010 000011 000100 000101 000110  000111

    #define lHl         0xe8  ____-_----

    #define lHLl        0x9a  __-_--__--    010000  010001  010010  010011

    #define lLHl        0xa6  __--__-_--    001000  001001  001010  001011

    #define lHHl        0xd4  ___-_-_---    011000  011001  011010  011011

    #define lHHHl       0xaa  __-_-_-_--



    #define lh          0x00  _________-

    #define lLh         0x1c  ___---___-

    #define lHh         0x40  _______-_-

    #define lLHh        0x4c  ___--__-_-    001100  001101  001110  001111

    #define lHLh        0x34  ___-_--__-    010100  010101  010110  010111

    #define lHHh        0x50  _____-_-_-    011100  011101

    #define lHHHh       0x54  ___-_-_-_-    011110  011111



    #define hLh         0x0f  _----____-

    #define hHLh        0x1d  _-_---___-    110100  110101

    #define hLHh        0x47  _---___-_-    101100  101101

    #define hHHLh       0x35  _-_-_--__-    111010  111011

    #define hHLHh       0x4d  _-_--__-_-    110110  110111

    #define hLHHh       0x53  _--__-_-_-      101110  101111

    #define hHHHHh      0x55  _-_-_-_-_-    111111



    #define hl          0xff  _---------

    #define hLl         0xc7  _---___---    100000  100001  100010  100011  100100  100101  100110  100111

    #define hHl         0xfd  _-_-------

    #define hHLl        0xcd  _-_--__---    110000  110001  110010  110011

    #define hLHl        0xD3        _--__-_---      101000  101001  101010  101011

    #define hHHl        0xF5        _-_-_-----      111000  111001

    #define hHHHl       0xd5  _-_-_-_---      111100  111101

    not directly translatable     111110

****************************************************************/

#define ll          0xf0
#define lLl         0xcc
#define lHl         0xe8
#define lHLl        0x9a
#define lLHl        0xa6
#define lHHl        0xd4
#define lHHHl       0xaa
#define lh          0x00
#define lLh         0x1c
#define lHh         0x40
#define lLHh        0x4c
#define lHLh        0x34
#define lHHh        0x50
#define lHHHh       0x54
#define hLh         0x0f
#define hHLh        0x1d
#define hLHh        0x47
#define hHHLh       0x35
#define hHLHh       0x4d
#define hLHHh       0x53
#define hHHHHh      0x55
#define hl          0xff
#define hLl         0xc7
#define hHl         0xfd
#define hHLl        0xcd
#define hLHl        0xD3
#define hHHl        0xF5
#define hHHHl       0xd5

#include <time.h>

#include "ddl.h"
#include "ddl_nmra.h"
#include "syslogmessage.h"


typedef struct {
    char *pattern;
    int patternlength;
    int value;
} tTranslateData;

typedef struct {
    int value;
    int patternlength;
} tTranslateData_v3;

static const tTranslateData TranslateData[] = {
    {"0", 1, 0xF0},
    {"00", 2, 0xC6},
    {"01", 2, 0x78},
    {"10", 2, 0xE1},
    {"001", 3, 0x66},
    {"010", 3, 0x96},
    {"011", 3, 0x5C},
    {"100", 3, 0x99},
    {"101", 3, 0x71},
    {"110", 3, 0xC5},
    {"0111", 4, 0x56},
    {"1011", 4, 0x59},
    {"1101", 4, 0x65},
    {"1110", 4, 0x95},
    {"11111", 5, 0x55}
};

/* number of translatable patterns */
static int DataCnt = sizeof(TranslateData) / sizeof(TranslateData[0]);

static const tTranslateData_v3 TranslateData_v3[32][2] = {
    {{lLl, 2}, {ll, 1}},
    {{lLl, 2}, {ll, 1}},
    {{lLl, 2}, {ll, 1}},
    {{lLl, 2}, {ll, 1}},
    {{lLHl, 3}, {lLh, 2}},
    {{lLHl, 3}, {lLh, 2}},
    {{lLHh, 3}, {lLh, 2}},
    {{lLHh, 3}, {lLh, 2}},
    {{lHLl, 3}, {lHl, 2}},
    {{lHLl, 3}, {lHl, 2}},
    {{lHLh, 3}, {lHl, 2}},
    {{lHLh, 3}, {lHl, 2}},
    {{lHHl, 3}, {lHh, 2}},
    {{lHHl, 3}, {lHh, 2}},
    {{lHHh, 3}, {lHh, 2}},
    {{lHHHh, 4}, {lHHh, 3}},
    {{hLl, 2}, {hl, 1}},
    {{hLl, 2}, {hl, 1}},
    {{hLl, 2}, {hl, 1}},
    {{hLl, 2}, {hl, 1}},
    {{hLHl, 3}, {hLh, 2}},
    {{hLHl, 3}, {hLh, 2}},
    {{hLHh, 3}, {hLh, 2}},
    {{hLHHh, 4}, {hLHh, 3}},
    {{hHLl, 3}, {hHl, 2}},
    {{hHLl, 3}, {hHl, 2}},
    {{hHLh, 3}, {hHl, 2}},
    {{hHLHh, 4}, {hHLh, 3}},
    {{hHHl, 3}, {hHHl, 3}},
    {{hHHLh, 4}, {hHHl, 3}},
    {{hHHHl, 4}, {hHHHl, 4}},
    {{hHHHHh, 5}, {hHHHHh, 5}}
};

static char *preamble = "111111111111111";
static const int NMRA_STACKSIZE = 200;

/* 230 is needed for all functions F0-F28 */
static const unsigned int BUFFERSIZE = 1000;

/* internal offset of the long addresses */
static const unsigned int ADDR14BIT_OFFSET = 128;

/* the result is only an index, no warranty */
static int translateabel(char *bs)
{
    int i;
    size_t size;
    char *pbs;
    size = strlen(bs);
    for (i = (DataCnt - 1); i >= 0; i--) {
        pbs = bs + (size - TranslateData[i].patternlength);
        if (strcmp(pbs, TranslateData[i].pattern) == 0)
            return 1;
    }
    return 0;
}

static int read_next_six_bits(char *Bitstream)
{
    int i, bits = 0;
    for (i = 0; i < 6; i++)
        bits = (bits << 1) | (*Bitstream++ == '0' ? 0 : 1);
    return bits;
}

static int translateBitstream2Packetstream_v1(char *Bitstream,
                                              char *Packetstream,
                                              int force_translation)
{

    char Buffer[BUFFERSIZE];
    char *pBs = Buffer;
    int i;                      /* decision of each recursion level          */
    int j = 0;                  /* index of Packetstream, level of recursion */
    int found;                  /* flag                                      */
    int stack[NMRA_STACKSIZE];  /* stack for the i's                         */
    int pstack = 0;             /* stack pointer                             */
    int correction = 0;
    size_t bufsize = 0;
    int highest_level = 0;      /* highest recursion level reached during algo. */
    const int max_level_delta = 7;      /* additional recursion base, speeds up */

    pBs = strncpy(Buffer, Bitstream, BUFFERSIZE - 1);
    memset(Packetstream, 0, PKTSIZE);
    i = DataCnt - 1;
    if (!translateabel(Buffer)) {
        /* The last bit of the bitstream is always '1'. */
        pBs[strlen(pBs) - 1] = 0;
        correction = 1;
    }
    bufsize = strlen(Buffer);
    while (*pBs) {
        found = 0;
        while (i >= 0) {
            if (strncmp(pBs, TranslateData[i].pattern,
                        TranslateData[i].patternlength) == 0) {
                found = 1;
                break;
            }
            i--;
        }
        if (!found) {           /* now backtracking    */
            pstack--;           /* back to last level  */
            if (pstack >= 0) {  /* last level avail.?  */
                i = stack[pstack];
                pBs -= TranslateData[i].patternlength;  /* corrections */
                j--;
                if (((highest_level - j) >= max_level_delta) &&
                    (!force_translation))
                    j = 0;
                i--;            /* next try */
            }
        }
        else {
            Packetstream[j] = (unsigned char) TranslateData[i].value;
            j++;
            if (j > highest_level)
                highest_level = j;
            pBs += TranslateData[i].patternlength;
            stack[pstack] = i;
            pstack++;
            i = DataCnt - 1;
        }
        if (j >= PKTSIZE - 1 || bufsize == BUFFERSIZE) {
            syslog(LOG_INFO,
                   "cannot translate bitstream '%s' to NMRA packet",
                   Bitstream);
            return 0;
        }
        if (j <= 0 || pstack < 0 || pstack > NMRA_STACKSIZE - 1) {
            /* it's nasty, but a try: */
            /* leading 1's don't make problems */
            strcat(Buffer, "1");
            bufsize++;
            pBs = Buffer;
            j = 0;
            pstack = 0;
            i = DataCnt - 1;
            correction = 0;     /* correction also done */
            memset(Packetstream, 0, PKTSIZE);
        }
    }
    if (correction) {
        Packetstream[j] = (unsigned char) 0x99; /* Now the handling of the */
        j++;                    /* final '1'. See above. */
    }
    return j + 1;               /* return number of bytes in packetstream */
}

static int translateBitstream2Packetstream_v2(char *Bitstream,
                                              char *Packetstream)
{

    int i = DataCnt - 1;        /* decision of each recursion level          */
    int j = 0;                  /* index of Packetstream, level of recursion */
    int found;                  /* flag                                      */
    int stack[PKTSIZE];         /* stack for the i's                         */

    memset(Packetstream, 0, PKTSIZE);
    while (*Bitstream) {
        /* Check if the end of the buffer contains only 1s */
        if (strlen(Bitstream) <= 5) {
            if (strncmp(Bitstream, "11111", strlen(Bitstream)) == 0) {
                /* This is the end */
                Packetstream[j++] = (unsigned char) 0x55;
                return j + 1;
            }
        }

        found = 0;
        while (i >= 0) {
            if (strncmp
                (Bitstream, TranslateData[i].pattern,
                 TranslateData[i].patternlength) == 0) {
                found = 1;
                break;
            }
            i--;
        }
        if (!found) {           /* now backtracking   */
            if (j > 0) {        /* last level avail.? */
                j--;            /* go back            */
                i = stack[j];
                Bitstream -= TranslateData[i].patternlength;    /* corrections */
                i--;

            }
            else {
                syslog(LOG_INFO,
                       "cannot translate bitstream '%s' to NMRA packet",
                       Bitstream);
                return 0;
            }
        }
        else {
            Packetstream[j] = (unsigned char) TranslateData[i].value;
            Bitstream += TranslateData[i].patternlength;
            stack[j] = i;
            j++;
            i = DataCnt - 1;
        }
        /* Check buffer size */
        if (j >= PKTSIZE) {
            syslog(LOG_INFO, "Oops buffer too small - bitstream '%s'",
                   Bitstream);
            return 0;
        }
    }
    return j + 1;               /* return number of bytes in packetstream */
}

static int translateBitstream2Packetstream_v3(char *Bitstream,
                                              char *Packetstream)
{

    /* This routine assumes, that any Bitstream starts with a 1 Bit. */
    /* This could be changed, if necessary */

    /* keep room for additional pre and postamble */
    char Buffer[BUFFERSIZE + 20];

    /* here the real sequence starts */
    char *read_ptr = Buffer + 1;

    /* one more 1 in the beginning for successful restart */
    char *restart_read = Buffer;

    /* this necessary, only to verify our assumptions */
    char *last_restart = Buffer - 1;

    char *buf_end;

    int restart_packet = 0;
    int generate_packet = 0;

    int second_try = false;
    int act_six;

    read_ptr = strcpy(Buffer, "11");

    /* one bit, to start with a half-bit, so we have to put in the left half */
    /* one bit, to be able, to back up one bit, if we run into a 111110 pattern */

    strncat(Buffer, Bitstream, BUFFERSIZE - 1);

    /* for simply testing, whether our job is done */
    buf_end = Buffer + strlen(Buffer);

    strcat(Buffer, "111111");

    /* at most six trailing bits are possibly necessary */

    memset(Packetstream, 0, PKTSIZE);

    while (generate_packet < PKTSIZE && read_ptr < buf_end) {
        act_six = read_next_six_bits(read_ptr);
        if (act_six == 0x3e /* 111110 */ ) {
            /*did we reach an untranslatable value */
            /* try again from last position, where a shorter translation */
            /* could be chosen                                          */
            second_try = true;
            generate_packet = restart_packet;
            if (restart_read == last_restart)
                syslog(LOG_INFO, "Sorry, restart algorithm doesn't "
                       "work as expected for NMRA-Packet %s", Bitstream);
            last_restart = restart_read;
            read_ptr = restart_read;
            act_six = read_next_six_bits(read_ptr);
        }

        Packetstream[generate_packet] =
            TranslateData_v3[act_six >> 1][second_try ? 1 : 0].value;

        if (act_six < 0x3e /* 111110 */ ) {
            /* is translation fixed up to here ? */
            restart_packet = generate_packet;
            restart_read = read_ptr;
        }
        read_ptr +=
            TranslateData_v3[act_six >> 1][second_try ? 1 : 0].
            patternlength;
        generate_packet++;
        second_try = false;
    }

    return generate_packet;     /* return number of bytes in packetstream */
}


int translateBitstream2Packetstream(bus_t busnumber, char *Bitstream,
                                    char *Packetstream,
                                    int force_translation)
{
    DDL_DATA *DDL = ((DDL_DATA *) buses[busnumber].driverdata);
    int NMRADCC_TR_V = DDL->NMRADCC_TR_V;
    switch (NMRADCC_TR_V) {
        case 1:
            return translateBitstream2Packetstream_v1(Bitstream,
                                                      Packetstream,
                                                      force_translation);
        case 2:
            return translateBitstream2Packetstream_v2(Bitstream,
                                                      Packetstream);
        case 3:
            return translateBitstream2Packetstream_v3(Bitstream,
                                                      Packetstream);
        default:
            return 0;
    }
}

/*** Some useful functions to calculate NMRA-DCC bytes (char arrays) ***/

/**
  Transform the lower 8 bit of the input into a "bitstream" byte
  @par Input: int value
  @par Output: char* byte
*/
static void calc_single_byte(char *byte, int value)
{
    int i;
    int bit = 0x1;

    strncpy(byte, "00000000", 8);
    byte[8] = 0;

    for (i = 7; i >= 0; i--) {
        if (value & bit)
            byte[i] = '1';
        bit <<= 1;
    }
}

/* calculating address bytes: 11AAAAAA AAAAAAAA */
static void calc_14bit_address_byte(char *byte1, char *byte2, int address)
{
    calc_single_byte(byte2, address);
    calc_single_byte(byte1, 0xc0 | (address >> 8));
}

/* calculating speed byte2: 01DUSSSS  */
static void calc_baseline_speed_byte(char *byte, int direction, int speed,
                                     int func)
{
    calc_single_byte(byte, 0x40 | (direction << 5) | speed);
    if (func & 1)
        byte[3] = '1';
}

/* calculating speed byte: 01DSSSSS */
static void calc_28spst_speed_byte(char *byte, int direction, int speed)
{
    /* last significant speed bit is at pos 3 */

    if (speed > 1) {
        speed += 2;
        calc_single_byte(byte, 0x40 | (direction << 5) | (speed >> 1));
        if (speed & 1) {
            byte[3] = '1';
        }
    }
    else {
        calc_single_byte(byte, 0x40 | (direction << 5) | speed);
    }
}

/* calculating function byte: 100DDDDD */
static void calc_function_group_one_byte(char *byte, int func)
{
    /* mask out lower 5 function bits */
    func &= 0x1f;
    calc_single_byte(byte, 0x80 | (func >> 1));

    /* F0 or FL is out of order */
    if (func & 1)
        byte[3] = '1';
}

/* calculating function byte: 101SDDDD */
static void calc_function_group_two_byte(char *byte, int func, int lower)
{
    if (lower) {
        /* shift func bits to lower 4 bits and mask them */
        func = (func >> 5) & 0xf;
        /* set command for F5 to F8 */
        func |= 0xb0;
    }
    else {
        func = (func >> 9) & 0xf;
        func |= 0xa0;
    }
    calc_single_byte(byte, func);
}

static void calc_128spst_adv_op_bytes(char *byte1, char *byte2,
                                      int direction, int speed)
{
    strcpy(byte1, "00111111");
    calc_single_byte(byte2, speed);
    if (direction == 1)
        byte2[0] = '1';
}

static void xor_two_bytes(char *byte, char *byte1, char *byte2)
{

    int i;

    for (i = 0; i < 8; i++) {
        if (byte1[i] == byte2[i])
            byte[i] = '0';
        else
            byte[i] = '1';
    }
    byte[8] = 0;
}

/*** functions to generate NMRA-DCC data packets ***/

int comp_nmra_accessory(bus_t busnumber, int nr, int output, int activate,
                        int offset)
{
    /* command: NA <nr [0001-2044]> <outp [0,1]> <activate [0,1]>
       example: NA 0012 0 1  */

    char byte1[9];
    char byte2[9];
    char byte3[9];
    char bitstream[BUFFERSIZE];
    char packetstream[PKTSIZE];
    char *p_packetstream;

    int address = 0;            /* of the decoder                */
    int pairnr = 0;             /* decoders have pair of outputs */

    int j;

    syslog_bus(busnumber, DBG_DEBUG,
               "command for NMRA protocol for accessory decoders "
               "(NA) received");

    /* no special error handling, it's job of the clients */
    if (nr < 1 || nr > 2044 || output < 0 || output > 1 ||
        activate < 0 || activate > 1)
        return 1;

    /* packet is not available */
    p_packetstream = packetstream;

    /* calculate the real address of the decoder and the pair number 
     * of the switch */
    /* valid decoder addresses: 0..511 */
    address = (nr + offset) / 4;
    pairnr = (nr + offset) % 4;

    /* address byte: 10AAAAAA (lower 6 bits) */
    calc_single_byte(byte1, 0x80 | (address & 0x3f));

    /* address and data 1AAACDDO upper 3 address bits are inverted */
    /* C =  activate, DD = pairnr */
    calc_single_byte(byte2,
                     0x80 | ((~address) & 0x1c0) >> 2 | activate << 3 |
                     pairnr << 1 | output);
    xor_two_bytes(byte3, byte2, byte1);

    /* putting all together in a 'bitstream' (char array) */
    memset(bitstream, 0, BUFFERSIZE);
    strcat(bitstream, preamble);
    strcat(bitstream, "0");
    strcat(bitstream, byte1);
    strcat(bitstream, "0");
    strcat(bitstream, byte2);
    strcat(bitstream, "0");
    strcat(bitstream, byte3);
    strcat(bitstream, "1");

    j = translateBitstream2Packetstream(busnumber, bitstream,
                                        packetstream, true);
    if (j > 0) {
        queue_add(busnumber, address, p_packetstream, QNBACCPKT, j);
#if 0                           /* GA Packet Cache */
        updateNMRAGaPacketPool(nr, output, activate, p_packetstream, j);
#endif
        return 0;
    }

    return 1;
}
/**
  Calculate up to 4 command sequences depending on the number of possible 
  functions (taken from INIT <BUS> GL ...) for up to 28 Functions
  due to the long bitstream in case of 28 functions the bitstream is
  split into to parts.
  @par Input: char *addrerrbyte Error detection code for address bytes(s)
              char *addrstream "bitstream" for preamble and the address byte(s)
              int func function bits
              int nfuncs number of functions
  @par Output: char* bitstream the resulting "bitstream"
  @par Output: char* bitstream2 the second  "bitstream"
*/
static void calc_function_stream(char *bitstream, char *bitstream2,
                                 char *addrerrbyte, char *addrstream,
                                 int func, int nfuncs)
{
    char funcbyte[9];
    char errdbyte[9];
    /* between two commands to the same address there has to be a gap
       of at least 5ms. 5ms are 10 packet bytes. one packet byte can take
       5 logical "1" (0x55) ==> 50 logical "1" are needed between 2 packets.
       The preamble holds 15 "1"s, so only 35 additional are needed
     */
    char wait[40] = "11111111111111111111111111111111111";

    calc_function_group_one_byte(funcbyte, func);
    xor_two_bytes(errdbyte, addrerrbyte, funcbyte);

    /* putting all together in a 'bitstream' (char array) (functions) */
    memset(bitstream2, 0, BUFFERSIZE);
    strcat(bitstream2, addrstream);
    strcat(bitstream2, funcbyte);
    strcat(bitstream2, "0");
    strcat(bitstream2, errdbyte);
    strcat(bitstream2, "1");
    if (nfuncs > 5) {
        calc_function_group_two_byte(funcbyte, func, true);
        xor_two_bytes(errdbyte, addrerrbyte, funcbyte);
        strcat(bitstream, wait);
        strcat(bitstream, addrstream);
        strcat(bitstream, funcbyte);
        strcat(bitstream, "0");
        strcat(bitstream, errdbyte);
        strcat(bitstream, "1");

        if (nfuncs > 8) {
            calc_function_group_two_byte(funcbyte, func, false);
            xor_two_bytes(errdbyte, addrerrbyte, funcbyte);
            strcat(bitstream2, wait);
            strcat(bitstream2, addrstream);
            strcat(bitstream2, funcbyte);
            strcat(bitstream2, "0");
            strcat(bitstream2, errdbyte);
            strcat(bitstream2, "1");
            if (nfuncs > 12) {
                char funcbyte2[9];
                strncpy(funcbyte2, "11011110", 8);
                funcbyte2[8] = 0;
                calc_single_byte(funcbyte, func >> 13);
                xor_two_bytes(errdbyte, addrerrbyte, funcbyte2);
                xor_two_bytes(errdbyte, errdbyte, funcbyte);
                strcat(bitstream, wait);
                strcat(bitstream, addrstream);
                strcat(bitstream, funcbyte2);
                strcat(bitstream, "0");
                strcat(bitstream, funcbyte);
                strcat(bitstream, "0");
                strcat(bitstream, errdbyte);
                strcat(bitstream, "1");
                if (nfuncs > 20) {
                    funcbyte2[7] = '1';
                    xor_two_bytes(errdbyte, addrerrbyte, funcbyte2);
                    calc_single_byte(funcbyte, func >> 21);
                    xor_two_bytes(errdbyte, errdbyte, funcbyte);
                    strcat(bitstream2, wait);
                    strcat(bitstream2, addrstream);
                    strcat(bitstream2, funcbyte2);
                    strcat(bitstream2, "0");
                    strcat(bitstream2, funcbyte);
                    strcat(bitstream2, "0");
                    strcat(bitstream2, errdbyte);
                    strcat(bitstream2, "1");
                }
            }
        }
    }
}

/**
  Calculate the "bitstream" for the cv programming sequence
  @par Input: char *addrerrbyte error detection byte of the address byte(s)
              int cv
              int value
              int verify
              int pom  if true generate PoM command
  @par Output: char* progstream the resulting "bitstream" for the
               program sequence
*/
static void calc_byte_program_stream(char *progstream, char *addrerrbyte,
                                     int cv, int value, int verify,
                                     int pom)
{
    char byte2[9];
    char byte3[9];
    char byte4[9];
    char byte5[9];

    memset(progstream, 0, BUFFERSIZE);
    /* calculating byte3: AAAAAAAA (rest of CV#) */
    calc_single_byte(byte3, cv);

    if (pom) {
        /* calculating byte2: 1110C1AA (instruction byte1) */
        calc_single_byte(byte2, 0xec | (cv >> 8));
    }
    else {
        /* calculating byte2: 011110AA (instruction byte1) */
        calc_single_byte(byte2, 0x7c | (cv >> 8));
    }
    if (verify) {
        byte2[4] = '0';
    }

    /* calculating byte4: DDDDDDDD (data) */
    calc_single_byte(byte4, value);

    /* calculating byte5: EEEEEEEE (error detection byte) */
    xor_two_bytes(byte5, addrerrbyte, byte2);
    xor_two_bytes(byte5, byte5, byte3);
    xor_two_bytes(byte5, byte5, byte4);

    strcat(progstream, byte2);
    strcat(progstream, "0");
    strcat(progstream, byte3);
    strcat(progstream, "0");
    strcat(progstream, byte4);
    strcat(progstream, "0");
    strcat(progstream, byte5);
    strcat(progstream, "1");
}

/**
  Calculate the "bitstream" for the cvbit programming sequence
  @par Input: char *addrerrbyte error detection byte of the address byte(s)
              int cv
              int value
              int verify
              int pom  if true generrate PoM command
  @par Output: char* progstream the resulting "bitstream" for the
               program sequence
*/
static void calc_bit_program_stream(char *progstream, char *addrerrbyte,
                                    int cv, int bit, int value, int verify,
                                    int pom)
{
    char byte2[9];
    char byte3[9];
    char byte4[9];
    char byte5[9];

    memset(progstream, 0, BUFFERSIZE);
    /* calculating byte3: AAAAAAAA (rest of CV#) */
    calc_single_byte(byte3, cv);

    if (pom) {
        /* calculating byte2: 111010AA (instruction byte1) */
        calc_single_byte(byte2, 0xe8 | (cv >> 8));
    }
    else {
        /* calculating byte2: 011110AA (instruction byte1) */
        calc_single_byte(byte2, 0x78 | (cv >> 8));
    }

    /* calculating byte4: 111CDBBB (data) */
    calc_single_byte(byte4, 0xf0 | (value << 3) | bit);
    if (verify) {
        byte4[3] = '0';
    }

    /* calculating byte5: EEEEEEEE (error detection byte) */
    xor_two_bytes(byte5, addrerrbyte, byte2);
    xor_two_bytes(byte5, byte5, byte3);
    xor_two_bytes(byte5, byte5, byte4);

    /* putting all together in a 'bitstream' (char array) */
    strcat(progstream, byte2);
    strcat(progstream, "0");
    strcat(progstream, byte3);
    strcat(progstream, "0");
    strcat(progstream, byte4);
    strcat(progstream, "0");
    strcat(progstream, byte5);
    strcat(progstream, "1");
}

/**
  Calculate the "bitstream" for 7 and 14 bit addresses
  @par Input: int address
              int mode  1 = 7 bit, 2 = 14 bit
  
  @par Output: char* addrstream the resulting "bitstream" for address byte(s)
               char* addrerrbyte the "bitstream" for error detection byte
*/
static void calc_address_stream(char *addrstream, char *addrerrbyte,
                                int address, int mode)
{
    char addrbyte[9];
    char addrbyte2[9];
    // if (mode == 1 || mode == 2 || mode == 5) { // Torsten Vogt 2012-01-15
    if (mode == 1) { // Torsten Vogt 2012-11-10
        /* calc 7 bit address - leading bit is zero */
        calc_single_byte(addrbyte, address & 0x7f);
        /* no second byte => error detection byte = addressbyte */
        memcpy(addrerrbyte, addrbyte, 9);

    }
    else {
        calc_14bit_address_byte(addrbyte, addrbyte2, address);
        xor_two_bytes(addrerrbyte, addrbyte, addrbyte2);
    }

    /* putting all together in a 'bitstream' (char array) (speed & direction) */
    memset(addrstream, 0, BUFFERSIZE);
    strcat(addrstream, preamble);
    strcat(addrstream, "0");
    strcat(addrstream, addrbyte);
    strcat(addrstream, "0");
    if (mode == 2) { // Torsten Vogt 2012-11-10
        strcat(addrstream, addrbyte2);
        strcat(addrstream, "0");
    }
}

/**
  Generate the packet for NMRA (multi)-function-decoder with 7-bit or 14-bit
  address and 14/28 or 128 speed steps and up to 28 functions
  @par Input: bus_t busnumber
              int address GL address
              int direction
              int speed
              int func function bits
              int nspeed number of speeds for this decoder
              int nfuncs number of functions
              int mode 1 == short address, 2 == long (2byte) address
  @return 0 == OK, 1 == Error
  
*/
int comp_nmra_multi_func(bus_t busnumber, int address, int direction,
                         int speed, int func, int nspeed, int nfuncs,
                         int mode)
{

    char spdrbyte[9];
    char spdrbyte2[9];
    char errdbyte[9];
    char addrerrbyte[9];
    char addrstream[BUFFERSIZE];
    char bitstream[BUFFERSIZE];
    char bitstream2[BUFFERSIZE];
    char packetstream[PKTSIZE];
    char packetstream2_buf[PKTSIZE];
    char *packetstream2 = packetstream2_buf;

    int adr = 0;
    int j, jj;

    syslog_bus(busnumber, DBG_DEBUG,
               "command for NMRA protocol (N%d) received \naddr:%d "
               "dir:%d speed:%d nspeeds:%d nfunc:%d",
               mode, address, direction, speed, nspeed, nfuncs);

    adr = address; // adr is an additional identifier, only used to store
                   // the pakets in the internal queue!!!!!

    // Torsten Vogt, 2012-11-10
    if (mode == 2) {
        adr += ADDR14BIT_OFFSET;
    }
    /* no special error handling, it's job of the clients */
    // Torsten Vogt, 2012-11-10
    if (address < 1 || address > 10239 || direction < 0 || direction > 1 ||
        speed < 0 || speed > (nspeed + 1) || (address > 127 && mode == 1))
        return 1;

    if (speed > 127) {
        speed = 127;
    }

    calc_address_stream(addrstream, addrerrbyte, address, mode);

    if (speed < 2 || nspeed < 15) {
        /* commands for stop and emergency stop are identical for
           14 and 28 speed steps. All decoders supporting 128
           speed steps also have to support 14/28 speed step commands
           ==> results in shorter refresh cycle if there are many 128
           speed steps decoders with speed=0, and a slightly faster 
           emergency stop for these decoders.
         */
        calc_baseline_speed_byte(spdrbyte, direction, speed, func);
    }
    else {
        if (nspeed > 29) {
            calc_128spst_adv_op_bytes(spdrbyte, spdrbyte2, direction,
                                      speed);
        }
        else {
            calc_28spst_speed_byte(spdrbyte, direction, speed);
        }
    }

    xor_two_bytes(errdbyte, addrerrbyte, spdrbyte);

    memset(bitstream, 0, BUFFERSIZE);
    strcat(bitstream, addrstream);
    strcat(bitstream, spdrbyte);
    strcat(bitstream, "0");
    if (nspeed > 29 && speed > 1) {
        strcat(bitstream, spdrbyte2);
        strcat(bitstream, "0");
        xor_two_bytes(errdbyte, errdbyte, spdrbyte2);
    }
    strcat(bitstream, errdbyte);
    strcat(bitstream, "1");

    if (nfuncs && (nspeed > 14)) {
        calc_function_stream(bitstream, bitstream2, addrerrbyte,
                             addrstream, func, nfuncs);
        j = translateBitstream2Packetstream(busnumber, bitstream,
                                            packetstream, false);
        jj = translateBitstream2Packetstream(busnumber, bitstream2,
                                             packetstream2, false);
    }
    else {
        j = translateBitstream2Packetstream(busnumber, bitstream,
                                            packetstream, false);
        packetstream2 = packetstream;
        jj = j;
    }
    if (j > 0 && jj > 0) {
        update_NMRAPacketPool(busnumber, adr, packetstream, j,
                              packetstream2, jj);
        queue_add(busnumber, adr, packetstream, QNBLOCOPKT, j);
        if (nfuncs && (nspeed > 14)) {
            queue_add(busnumber, adr, packetstream2, QNBLOCOPKT, jj);
        }

        return 0;
    }

    return 1;
}

/**
  Write a configuration variable (cv) on the Main (operations mode
  programming). This is very similar to the service mode programming with
  the extension, that the decoder address is used. I.e. only the cv of
  the selected decoder is updated not all decoders.
  @par Input: bus_t busnumber
              int address
              int cv
              int value
              int mode 1 == short address, 2 == long (2byte) address
  @return ack 
*/
int protocol_nmra_sm_write_cvbyte_pom(bus_t busnumber, int address, int cv,
                                      int value, int mode)
{
    char addrerrbyte[9];
    char addrstream[BUFFERSIZE];
    char progstream[BUFFERSIZE];
    char bitstream[BUFFERSIZE];
    char packetstream[PKTSIZE];
    int j;
    int adr = 0;

    syslog_bus(busnumber, DBG_DEBUG,
               "command for PoM %d received \naddr:%d CV:%d value:%d",
               mode, address, cv, value);
    cv -= 1;

    if (address < 1 || address > 10239 || cv < 0 || cv > 1023 ||
        value < 0 || value > 255 || (address > 127 && mode == 1))
        return 1;

    adr = address; // adr is an additional identifier, only used to store
                   // the pakets in the internal queue!!!!!

    // Torsten Vogt, 2012-11-10
    if (mode == 2) {
    // if (mode > 2 && mode < 5) {
        adr += ADDR14BIT_OFFSET;
    }

    calc_address_stream(addrstream, addrerrbyte, address, mode);
    calc_byte_program_stream(progstream, addrerrbyte, cv, value, false,
                             true);
    memset(bitstream, 0, BUFFERSIZE);
    strcat(bitstream, addrstream);
    strcat(bitstream, progstream);

    j = translateBitstream2Packetstream(busnumber, bitstream, packetstream,
                                        false);

    if (j > 0) {
        queue_add(busnumber, adr, packetstream, QNBLOCOPKT, j);
        return 0;
    }
    return 1;
}

/**
  Write a single bit of a configuration variable (cv) on the Main.
  This is very similar to the service mode programming with the extension,
  that the decoder address is used. I.e. only the cv of the selected 
  decoder is updated not all decoders.
  @par Input: bus_t busnumber
              int address
              int cv
              int value
              int mode 1 == short address, 2 == long (2byte) address
  @return ack 
*/
int protocol_nmra_sm_write_cvbit_pom(bus_t busnumber, int address, int cv,
                                     int bit, int value, int mode)
{
    char addrerrbyte[9];
    char addrstream[BUFFERSIZE];
    char progstream[BUFFERSIZE];
    char bitstream[BUFFERSIZE];
    char packetstream[PKTSIZE];
    int j;
    int adr = 0;

    syslog_bus(busnumber, DBG_DEBUG,
               "command for PoM %d received \naddr:%d CV:%d value:%d",
               mode, address, cv, value);
    cv -= 1;
    /* do not allow to change the address on the main ==> cv 1 is disabled */
    if (address < 1 || address > 10239 || cv < 1 || cv > 1023
        || bit < 0 || bit > 7 || value < 0 || value > 1
        || (address > 127 && mode == 1))
        return 1;

    adr = address; // adr is an additional identifier, only used to store
                   // the pakets in the internal queue!!!!!

    // Torsten Vogt, 2012-11-10
    if (mode == 2) {
    // if (mode > 2 && mode < 5) {
        adr += ADDR14BIT_OFFSET;
    }

    calc_address_stream(addrstream, addrerrbyte, address, mode);
    calc_bit_program_stream(progstream, addrerrbyte, cv, bit, value, false,
                            true);
    memset(bitstream, 0, BUFFERSIZE);
    strcat(bitstream, addrstream);
    strcat(bitstream, progstream);

    j = translateBitstream2Packetstream(busnumber, bitstream, packetstream,
                                        false);

    if (j > 0) {
        queue_add(busnumber, adr, packetstream, QNBLOCOPKT, j);
        return 0;
    }
    return 1;
}

/**
 * The following function(s) supports the implementation of NMRA-
 * programmers. It is recommended to use a programming track to  
 * program your locos. In every case it is useful to stop the   
 * refresh-cycle on the track when using one of the following    
 * service mode functions.
 **/

static int sm_initialized = false;

static char resetstream[PKTSIZE];
static int rs_size = 0;
static char idlestream[PKTSIZE];
static int is_size = 0;
static char pagepresetstream[PKTSIZE];
static int ps_size = 0;

static char *longpreamble = "111111111111111111111111111111";
static char reset_packet[] =
    "1111111111111111111111111111110000000000000000000000000001";
static char page_preset_packet[] =
    "1111111111111111111111111111110011111010000000010011111001";
static char idle_packet[] =
    "1111111111111111111111111111110111111110000000000111111111";

static void sm_init(bus_t busnumber)
{
    memset(resetstream, 0, PKTSIZE);
    rs_size =
        translateBitstream2Packetstream(busnumber, reset_packet,
                                        resetstream, false);
    memset(idlestream, 0, PKTSIZE);
    is_size =
        translateBitstream2Packetstream(busnumber, idle_packet, idlestream,
                                        false);
    memset(pagepresetstream, 0, PKTSIZE);
    ps_size =
        translateBitstream2Packetstream(busnumber, page_preset_packet,
                                        pagepresetstream, true);
    sm_initialized = true;
}

/**
  Check the serial line for an acknowledgment (ack)
  @par Input: bus_t busnumber
  @return ack 
*/
static int scanACK(bus_t busnumber)
{
    int result, arg;
    result = ioctl(buses[busnumber].device.file.fd, TIOCMGET, &arg);
    if (result == -1) {
        syslog_bus(busnumber, DBG_ERROR,
                   "ioctl() failed: %s (errno = %d)\n",
                   strerror(errno), errno);
    }

    if ((result >= 0) && (!(arg & TIOCM_RI)))
        return 1;
    return 0;
}

/**
  Wait for the UART to write the packetstream
  check in the meantime if an ack is set
  @par Input: bus_t busnumber
  @return ack 
*/
static int waitUARTempty_scanACK(bus_t busnumber)
{
    int value = 0;
    int result = 0;
    int ack = 0;
    clock_t start = clock();
    do {                        /* wait until UART is empty */
        if (scanACK(busnumber))
            ack = 1;            /* scan ACK */
        /* prevent endless loop in case somone turned the power on */
        if (buses[busnumber].power_state == 1) {
            clock_t now = clock();
            float diff = (now - start);
            diff /= CLOCKS_PER_SEC;
            if (ack) {
                value = 1;
                waitUARTempty(busnumber);
            }
            /* wait 300ms */
            if (diff > 0.3) {
                waitUARTempty(busnumber);
                value = 1;
            }
        }
        else {
#if linux
            result =
                ioctl(buses[busnumber].device.file.fd, TIOCSERGETLSR,
                      &value);
#else
            result =
                ioctl(buses[busnumber].device.file.fd, TCSADRAIN, &value);
#endif
        }
        if (result == -1) {
            syslog_bus(busnumber, DBG_ERROR,
                       "ioctl() failed: %s (errno = %d)\n",
                       strerror(errno), errno);
        }
    } while (!value);

    /* if power is on do not turn it off */
    if (!buses[busnumber].power_state)
        tcflow(buses[busnumber].device.file.fd, TCOOFF);
    return ack;
}

/**
  Check if a given ack is valid 
  an ack is invalid if the line is permanently active
  @par Input: bus_t busnumber
              int ack
  @return ack if valid, 0 otherwise
*/
static int handleACK(bus_t busnumber, int ack)
{
    if (usleep(5000) == -1) {
        syslog_bus(busnumber, DBG_ERROR,
                   "usleep() failed: %s (errno = %d)\n",
                   strerror(errno), errno);
    }
    /* ack not supported */
    if ((ack == 1) && (scanACK(busnumber) == 1)) {
        return 0;
    }
    /* ack supported ==> send to client */
    else
        return ack;
}

/**
  Generate command stream to access a physical register
  @par Input: bus_t bus
              int reg
              int value
              int verify
  @par Output:char *packetstream
  @return length of packetstream
*/
static int calc_reg_stream(bus_t bus, int reg, int value, int verify,
                           char *packetstream)
{

    char byte1[9];
    char byte2[9];
    char byte3[9];
    char bitstream[BUFFERSIZE];
    int j;

    /* calculating byte1: 0111CRRR (instruction and number of register) */
    calc_single_byte(byte1, 0x78 | reg);
    if (verify) {
        byte1[4] = '0';
    }

    /* calculating byte2: DDDDDDDD (data) */
    calc_single_byte(byte2, value);

    /* calculating byte3 (error detection byte) */
    xor_two_bytes(byte3, byte1, byte2);

    /* putting all together in a 'bitstream' (char array) */
    memset(bitstream, 0, BUFFERSIZE);
    strcat(bitstream, longpreamble);
    strcat(bitstream, "0");
    strcat(bitstream, byte1);
    strcat(bitstream, "0");
    strcat(bitstream, byte2);
    strcat(bitstream, "0");
    strcat(bitstream, byte3);
    strcat(bitstream, "1");

    memset(packetstream, 0, PKTSIZE);
    j = translateBitstream2Packetstream(bus, bitstream, packetstream,
                                        true);
    return j;
}

/**
  Write or verify a byte of a physical register
  special case: a write to register 1 is the address only command
  0111C000 0AAAAAAA
  @par Input: bus_t bus
              int reg
              int value
              int verify
  @return 1 if successful, 0 otherwise
*/
static int protocol_nmra_sm_phregister(bus_t bus, int reg, int value,
                                       int verify)
{
    /* physical register addressing */

    char packetstream[PKTSIZE];
    char SendStream[4096];

    int j, l, y, ack;

    syslog_bus(bus, DBG_DEBUG,
               "command for NMRA service mode instruction (SMPRA) received"
               " REG %d, Value %d", reg, value);

    /* no special error handling, it's job of the clients */
    if (reg < 1 || reg > 8 || value < 0 || value > 255)
        return -1;

    if (!sm_initialized)
        sm_init(bus);

    reg -= 1;
    j = calc_reg_stream(bus, reg, value, verify, packetstream);

    memset(SendStream, 0, 4096);

    if (!verify) {
        /* power-on cycle, at least 20 valid packets */
        for (l = 0; l < 50; l++)
            strcat(SendStream, idlestream);
        /* 3 or more reset packets */
        for (l = 0; l < 6; l++)
            strcat(SendStream, resetstream);
        /* 5 or more page preset packets */
        for (l = 0; l < 10; l++)
            strcat(SendStream, pagepresetstream);
        /* 6 or more reset packets */
        for (l = 0; l < 12; l++)
            strcat(SendStream, resetstream);
        /* 3 or more reset packets */
        for (l = 0; l < 12; l++)
            strcat(SendStream, resetstream);
        /* 5 or more write packets */
        for (l = 0; l < 20; l++)
            strcat(SendStream, packetstream);
        /* 6 or more reset or identical write packets */
        for (l = 0; l < 24; l++)
            strcat(SendStream, packetstream);
        /* 3 or more reset packets */
        for (l = 0; l < 24; l++)
            strcat(SendStream, resetstream);

        y = 50 * is_size + 54 * rs_size + 44 * j + 10 * ps_size;
    }
    else {
        /* power-on cycle, at least 20 valid packets */
        for (l = 0; l < 30; l++)
            strcat(SendStream, idlestream);
        /* 3 or more reset packets */
        for (l = 0; l < 5; l++)
            strcat(SendStream, resetstream);
        /* 7 or more verify packets */
        for (l = 0; l < 20; l++)
            strcat(SendStream, packetstream);
        y = 30 * is_size + 5 * rs_size + 20 * j + 0;
    }

    setSerialMode(bus, SDM_NMRA);
    tcflow(buses[bus].device.file.fd, TCOON);

    l = write(buses[bus].device.file.fd, SendStream, y);
    if (l == -1) {
        syslog_bus(bus, DBG_ERROR,
                   "write() failed: %s (errno = %d)\n",
                   strerror(errno), errno);
    }

    ack = waitUARTempty_scanACK(bus);
    setSerialMode(bus, SDM_DEFAULT);

    return handleACK(bus, ack);
}

/**
  Write or verify a byte of a physical register using paged mode addressing
  @par Input: bus_t bus
              int page
              int reg
              int value
              int verify
  @return 1 if successful, 0 otherwise
*/
static int protocol_nmra_sm_page(bus_t bus, int page, int reg, int value,
                                 int verify)
{
    /* physical register addressing */

    char packetstream[PKTSIZE];
    char packetstream_page[PKTSIZE];
    char SendStream[4096];

    int j, k, l, y, ack;

    syslog_bus(bus, DBG_DEBUG,
               "command for NMRA service mode instruction (SMPRA) received"
               " PAGE %d, REG %d, Value %d", page, reg, value);

    /* no special error handling, it's job of the clients */
    if (reg < 0 || reg > 3 || page < 0 || page > 255 || value < 0
        || value > 255)
        return -1;

    if (!sm_initialized)
        sm_init(bus);

    j = calc_reg_stream(bus, reg, value, verify, packetstream);
    /* set page to register 6 (number 5) */
    k = calc_reg_stream(bus, 5, page, false, packetstream_page);

    memset(SendStream, 0, 4096);

    /* power-on cycle, at least 20 valid packets */
    for (l = 0; l < 50; l++)
        strcat(SendStream, idlestream);
    /* 3 or more reset packets */
    for (l = 0; l < 6; l++)
        strcat(SendStream, resetstream);
    /* 5 or more writes to the page register */
    for (l = 0; l < 10; l++)
        strcat(SendStream, packetstream_page);
    /* 6 or more reset packets */
    for (l = 0; l < 12; l++)
        strcat(SendStream, resetstream);
    /* 3 or more reset packets */
    for (l = 0; l < 12; l++)
        strcat(SendStream, resetstream);
    /* 5 or more write packets */
    for (l = 0; l < 15; l++)
        strcat(SendStream, packetstream);
    /* 6 or more reset or identical write packets */
    for (l = 0; l < 15; l++)
        strcat(SendStream, packetstream);
    /* 3 or more reset packets */
    for (l = 0; l < 24; l++)
        strcat(SendStream, resetstream);

    y = 50 * is_size + 54 * rs_size + 30 * j + 10 * k;

    setSerialMode(bus, SDM_NMRA);
    tcflow(buses[bus].device.file.fd, TCOON);

    l = write(buses[bus].device.file.fd, SendStream, y);
    if (l == -1) {
        syslog_bus(bus, DBG_ERROR,
                   "write() failed: %s (errno = %d)\n",
                   strerror(errno), errno);
    }

    ack = waitUARTempty_scanACK(bus);
    setSerialMode(bus, SDM_DEFAULT);

    return handleACK(bus, ack);
}

/**
  Write a byte to a physical register
  @par Input: bus_t bus
              int reg
              int value
  @return 1 if successful, 0 otherwise
*/
int protocol_nmra_sm_write_phregister(bus_t bus, int reg, int value)
{
    return protocol_nmra_sm_phregister(bus, reg, value, false);
}

/**
  Check the contens of a physical register
  @par Input: bus_t bus
              int reg
              int value
  @return 1 if the contens of the register equals reg, 0 otherwise
*/
int protocol_nmra_sm_verify_phregister(bus_t bus, int reg, int value)
{
    return protocol_nmra_sm_phregister(bus, reg, value, true);
}

/**
  Get the contens of a physical register
  @par Input: bus_t bus
              int reg
  @return the value of the register
*/
int protocol_nmra_sm_get_phregister(bus_t bus, int reg)
{
    int rc;
    int i;
    for (i = 0; i < 256; i++) {
        rc = protocol_nmra_sm_phregister(bus, reg, i, true);
        if (rc)
            break;
    }
    return i;
}

/**
  Verify/write a byte of a configuration variable (cv) 
  @par Input: bus_t busnumber
              int cv
              int value
              int verify
  @return 1 if successful, 0 otherwise
*/
static int protocol_nmra_sm_direct_cvbyte(bus_t busnumber, int cv,
                                          int value, int verify)
{
    ssize_t result;

    /* direct cv access */
    char bitstream[BUFFERSIZE];
    char progstream[BUFFERSIZE];
    char packetstream[PKTSIZE];
    char SendStream[2048];
    char progerrbyte[9];
    int j, l, ack = 0;

    cv -= 1;
    syslog_bus(busnumber, DBG_DEBUG,
               "command for NMRA service mode instruction (SMDWY) received");

    /* no special error handling, it's job of the clients */
    if (cv < 0 || cv > 1024 || value < 0 || value > 255)
        return -1;

    /* address only mode */
    if (!cv && (value < 128)) {
        return protocol_nmra_sm_phregister(busnumber, 1, value, verify);
    }

    if (!sm_initialized)
        sm_init(busnumber);
    /* putting all together in a 'bitstream' (char array) */
    strncpy(progerrbyte, "00000000", 8);
    progerrbyte[8] = 0;
    calc_byte_program_stream(progstream, progerrbyte, cv, value, verify,
                             false);
    memset(bitstream, 0, BUFFERSIZE);
    strcat(bitstream, longpreamble);
    strcat(bitstream, "0");
    strcat(bitstream, progstream);

    j = translateBitstream2Packetstream(busnumber, bitstream, packetstream,
                                        false);

    memset(SendStream, 0, 2048);

    if (!verify) {
        for (l = 0; l < 30; l++)
            strcat(SendStream, idlestream);
        for (l = 0; l < 15; l++)
            strcat(SendStream, resetstream);
        for (l = 0; l < 20; l++)
            strcat(SendStream, packetstream);
        l = 30 * is_size + 15 * rs_size + 20 * j;
    }
    else {
        for (l = 0; l < 30; l++)
            strcat(SendStream, idlestream);
        for (l = 0; l < 5; l++)
            strcat(SendStream, resetstream);
        for (l = 0; l < 20; l++)
            strcat(SendStream, packetstream);
        l = 30 * is_size + 5 * rs_size + 20 * j;
    }

    setSerialMode(busnumber, SDM_NMRA);
    tcflow(buses[busnumber].device.file.fd, TCOON);
    result = write(buses[busnumber].device.file.fd, SendStream, l);
    if (result == -1) {
        syslog_bus(busnumber, DBG_ERROR,
                   "write() failed: %s (errno = %d)\n",
                   strerror(errno), errno);
    }
    ack = waitUARTempty_scanACK(busnumber);
    setSerialMode(busnumber, SDM_DEFAULT);
    return handleACK(busnumber, ack);
}

/**
  Write a byte to a configuration variable (cv) 
  @par Input: bus_t bus
              int cv
              int value
  @return 1 if successful, 0 otherwise
*/
int protocol_nmra_sm_write_cvbyte(bus_t bus, int cv, int value)
{
    return protocol_nmra_sm_direct_cvbyte(bus, cv, value, false);
}

/**
  Verify the content of a configuration variable (cv) 
  @par Input: bus_t bus
              int cv
              int value
  @return 1 if the value matches, 0 otherwise
*/
int protocol_nmra_sm_verify_cvbyte(bus_t bus, int cv, int value)
{
    return protocol_nmra_sm_direct_cvbyte(bus, cv, value, true);
}

/**
  Verify/write a single bit of a configuration variable (cv) 
  @par Input: bus_t bus
              int cv
              int bit
              int value
              int verify
  @return 1 if successful, 0 otherwise
*/
static int protocol_nmra_sm_direct_cvbit(bus_t bus, int cv, int bit,
                                         int value, int verify)
{
    ssize_t result;

    /* direct cv access */
    char progerrbyte[9];
    char bitstream[BUFFERSIZE];
    char progstream[BUFFERSIZE];
    char packetstream[PKTSIZE];
    char SendStream[2048];

    int j, l, ack;

    cv -= 1;
    syslog_bus(bus, DBG_DEBUG,
               "command for NMRA service mode instruction (SMDWB) received");

    /* no special error handling, it's job of the clients */
    if (cv < 0 || cv > 1023 || bit < 0 || bit > 7 || value < 0
        || value > 1)
        return -1;

    if (!sm_initialized)
        sm_init(bus);

    strncpy(progerrbyte, "00000000", 8);
    progerrbyte[8] = 0;
    calc_bit_program_stream(progstream, progerrbyte, cv, bit, value,
                            verify, false);

    /* putting all together in a 'bitstream' (char array) */
    memset(bitstream, 0, BUFFERSIZE);
    strcat(bitstream, longpreamble);
    strcat(bitstream, "0");
    strcat(bitstream, progstream);

    j = translateBitstream2Packetstream(bus, bitstream, packetstream,
                                        false);

    memset(SendStream, 0, 2048);
    for (l = 0; l < 30; l++)
        strcat(SendStream, idlestream);
    for (l = 0; l < 15; l++)
        strcat(SendStream, resetstream);
    for (l = 0; l < 20; l++)
        strcat(SendStream, packetstream);
    l = 30 * is_size + 15 * rs_size + 20 * j;

    setSerialMode(bus, SDM_NMRA);
    tcflow(buses[bus].device.file.fd, TCOON);
    result = write(buses[bus].device.file.fd, SendStream, l);
    if (result == -1) {
        syslog_bus(bus, DBG_ERROR,
                   "write() failed: %s (errno = %d)\n",
                   strerror(errno), errno);
    }
    ack = waitUARTempty_scanACK(bus);
    setSerialMode(bus, SDM_DEFAULT);

    return handleACK(bus, ack);
}

/**
  Write a single bit of a configuration variable (cv) 
  @par Input: bus_t bus
              int cv
              int bit
              int value
  @return 1 if successful, 0 otherwise
*/
int protocol_nmra_sm_write_cvbit(bus_t bus, int cv, int bit, int value)
{
    return protocol_nmra_sm_direct_cvbit(bus, cv, bit, value, false);
}

/**
  Verify a single bit of a configuration variable (cv) 
  @par Input: bus_t bus
              int cv
              int bit
              int value
  @return 1 if the value matches, 0 otherwise
*/
int protocol_nmra_sm_verify_cvbit(bus_t bus, int cv, int bit, int value)
{
    return protocol_nmra_sm_direct_cvbit(bus, cv, bit, value, true);
}

/**
  Get cvbyte by verifying all bits -> constant runtime
  @par Input: bus_t bus
              int cv
  @return the value of the configuration variable (cv)
*/
int protocol_nmra_sm_get_cvbyte(bus_t busnumber, int cv)
{
    int bit;
    int rc = 0;
    for (bit = 7; bit >= 0; bit--) {
        rc <<= 1;               /* shift bits */
        rc += protocol_nmra_sm_direct_cvbit(busnumber, cv, bit, 1, true);
    }
    return rc;
}

/**
  Calclulate the page number for a given cv
  @par Input: int cv
  @return the page number
*/
static int calc_page(int cv)
{
    int page = (cv - 1) / 4 + 1;
    return page;
}

/**
  Write a configuration variable (cv) using paged mode addressing
  @par Input: bus_t bus
              int cv
              int value
  @return 1 if successful, 0 otherwise
*/
int protocol_nmra_sm_write_page(bus_t busnumber, int cv, int value)
{
    int page = calc_page(cv);
    return protocol_nmra_sm_page(busnumber, page, (cv - 1) & 3, value,
                                 false);
}

/**
  Verify the contens of a cv using paged mode addressing
  @par Input: bus_t bus
              int cv
	      int value
  @return 1 if the value matches, 0 otherwise
*/

int protocol_nmra_sm_verify_page(bus_t busnumber, int cv, int value)
{
    int page = calc_page(cv);
    return protocol_nmra_sm_page(busnumber, page, (cv - 1) & 3, value,
                                 true);
}

/**
  Get the contens of a cv using paged mode addressing
  @par Input: bus_t bus
              int cv
  @return the value of the configuration variable (cv)
*/
int protocol_nmra_sm_get_page(bus_t busnumber, int cv)
{
    int page = calc_page(cv);
    int rc;
    int i;
    for (i = 0; i < 256; i++) {
        rc = protocol_nmra_sm_page(busnumber, page, (cv - 1) & 3, i, true);
        if (rc)
            break;
    }
    return i;
}
