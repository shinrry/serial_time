/*+ TsipParser.h
 *
 ******************************************************************************
 *
 *                        Trimble Navigation Limited
 *                           645 North Mary Avenue
 *                              P.O. Box 3642
 *                         Sunnyvale, CA 94088-3642
 *
 ******************************************************************************
 *
 *    Copyright � 2005 Trimble Navigation Ltd.
 *    All Rights Reserved
 *
 ******************************************************************************
 *
 * Description:
 *    This file defines the CTsipParser class.
 *            
 * Revision History:
 *    05-18-2005    Mike Priven
 *                  Written
 *
 * Notes:
 *
-*/

#ifndef TSIP_PARSER_H
#define TSIP_PARSER_H


/*---------------------------------------------------------------------------*\
 |                     S I M P L E   D A T A   T Y P E S
\*---------------------------------------------------------------------------*/
typedef unsigned char   S8;              /* Signed 8-bit integer (character) */
typedef unsigned char   U8;              /* Unsigned 8-bit integer (byte)    */
typedef signed short    S16;             /* Signed 16-bit integer (word)     */
typedef unsigned short  U16;             /* Unsigned 16-bit integer (word)   */
typedef signed long     S32;             /* Signed 32-bit integer (long)     */
typedef unsigned long   U32;             /* Unsigned 32-bit integer (long)   */
typedef float           FLT;             /* 4-byte single precision (float)  */
typedef double          DBL;             /* 8-byte double precision (double) */


/*---------------------------------------------------------------------------*\
 |                  C O N S T A N T S   A N D   M A C R O S
\*---------------------------------------------------------------------------*/
#define MSG_IN_COMPLETE  0
#define TSIP_DLE         1
#define TSIP_IN_PARTIAL  2

#define DLE              0x10 // TSIP packet start/end header         
#define ETX              0x03 // TSIP data packet tail                
#define MAX_TSIP_PKT_LEN 300  // max length of a TSIP packet 

#define MAX_SC_MESSAGE   13
#define MAX_EC_MESSAGE   6
#define MAX_AS1_MESSAGE  4

#define GPS_PI           (3.1415926535898)
#define R2D              (180.0/GPS_PI)

#define INFO_DGPS        0x02
#define INFO_2D          0x04
#define INFO_FILTERED    0x10

#define MAX_LONG         (2147483648.)   /* 2**31 */

static const char* gstrDayName[7] = 
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};



/*---------------------------------------------------------------------------*\
 |                      C L A S S   D E F I N I T I O N
\*---------------------------------------------------------------------------*/
class CTsipParser
{

public: //==== P U B L I C   M E T H O D S ===================================/

    CTsipParser() {};
    ~CTsipParser() {};

    void    ReceivePkt (unsigned char raw_data[], int raw_pkt_len);
    void ParsePkt   (unsigned char ucPkt[], int nPktLen);


private: //==== P R I V A T E   M E T H O D S ================================/

    
    void Parse0x8F   (unsigned char ucData[], int nLen);
    void Parse0x8F20 (unsigned char ucData[], int nLen);
    void Parse0x8FAB (unsigned char ucData[], int nLen);
    void Parse0x8FAC (unsigned char ucData[], int nLen);

    void Parse0x4ALong  (unsigned char ucData[], int nLen);
    void Parse0x4AShort (unsigned char ucData[], int nLen);

    S16  GetShort    (U8* pucBuf);
    U16  GetUShort   (U8* pucBuf);
    S32  GetLong     (U8* pucBuf);
    U32  GetULong    (U8* pucBuf);
    FLT  GetSingle   (U8* pucBuf);
    DBL  GetDouble   (U8* pucBuf);

    void ShowTime (FLT fltTimeOfWeek);


private: //==== P R I V A T E   M E M B E R   V A R I A B L E S ==============/

    //CStdString m_str, m_strTemp;

};

#endif
