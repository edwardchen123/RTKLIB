/*------------------------------------------------------------------------------
* lexbin.c : qzss lex raw message (MADOCA MT=12 format) dependent functions
*
*          Copyright (C) 2014 by T.TAKASU, All rights reserved.
*
*-----------------------------------------------------------------------------*/
#include "rtklib.h"

#ifdef EXTLEX

#define LEXPR1      0x1A         /* lex peamble 1 */
#define LEXPR2      0xCF         /* lex peamble 2 */
#define LEXPR3      0xFC         /* lex peamble 3 */
#define LEXPR4      0x1D         /* lex peamble 4 */

static const char rcsid[]="$Id:$";

/* decode MADOCA (MT12) raw message ------------------------------------------*/
static int decode_lexbin(raw_t *raw)
{
    int i=32,j,prnmsg,type,alert;
    unsigned char *p=raw->buff;
    double tow;
    int week;

    /* decode lex message header (49 bits) */
    prnmsg=getbitu(p,i, 8); i+= 8; /* prn number */
    type  =getbitu(p,i, 8); i+= 8; /* lex message type */
    alert =getbitu(p,i, 1); i+= 1; /* alert flag */
    
    /* decode MADOCA time information */
    tow   =getbitu(p,i,20); /* tow */
    week  =getbitu(p,i+20,13); /* week */
    
    raw->time=gpst2time(week,tow);
    raw->lexmsg.prn=prnmsg;
    raw->lexmsg.type=type;
    raw->lexmsg.alert=alert;
    sprintf(raw->msgtype,"lexbin: week=%d tow=%10.3f prn=%d type=%d alert=%d",
        week,tow,prnmsg,type,alert);

    /* save data part (1695 bytes) */
    for (j=0;j<212;j++) {
        raw->lexmsg.msg[j]=(unsigned char)getbitu(p,i,8); i+=8;
    }
    raw->lexmsg.msg[211]&=0xFE;
    
    trace(4,"lexmsg: prn=%d type=%d aleart=%d\n",prnmsg,type,alert);
    trace(4,"lexmsg: msg="); traceb(4,raw->lexmsg.msg,212);
    return 31;
}
/* sync code -----------------------------------------------------------------*/
static int sync_lexbin(unsigned char *buff, unsigned char data)
{
    buff[0]=buff[1]; buff[1]=buff[2]; buff[2]=buff[3]; buff[3]=data;
    return buff[0]==LEXPR1&&buff[1]==LEXPR2&&buff[2]==LEXPR3&&buff[3]==LEXPR4;
}
/* input lex (MADOCA,MT=12) raw message from stream ----------------------------
* fetch next lex receiver raw data and input a mesasge from stream
* args   : raw_t *raw   IO     receiver raw data control struct
*          unsigned char data I stream data (1 byte)
* return : status (-1: error message, 0: no message, 1: input observation data,
*                  31: input lex message)
*-----------------------------------------------------------------------------*/
extern int input_lexbin(raw_t *raw, unsigned char data)
{
    trace(5,"input_lexbin: data=%02x\n",data);
    
    /* synchronize frame */
    if (raw->nbyte==0) {
        if (!sync_lexbin(raw->buff,data)) return 0;
        raw->nbyte=4;
        return 0;
    }
    raw->buff[raw->nbyte++]=data;
    if (raw->nbyte<218) return 0; /* 250-32 byte */
    raw->nbyte=0;
    
    /* decode lex receiver raw message */
    return decode_lexbin(raw);
}
/* input lex (MADOCA,MT=12) raw message from file ------------------------------
* fetch next lex raw data and input a message from file
* args   : raw_t  *raw   IO     receiver raw data control struct
*          FILE   *fp    I      file pointer
* return : status(-2: end of file, -1...9: same as above)
*-----------------------------------------------------------------------------*/
extern int input_lexbinf(raw_t *raw, FILE *fp)
{
    int i,data,ret;
    
    trace(4,"input_lexbinf:\n");
    
    for (i=0;i<4096;i++) {
        if ((data=fgetc(fp))==EOF) return -2;
        if ((ret=input_lexbin(raw,(unsigned char)data))) return ret;
    }
    return 0; /* return at every 4k bytes */
}
#endif /* EXTLEX */
