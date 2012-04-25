/*+ TsipParser.cpp
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
 *    This file implements the CTsipParser class.
 *            
 * Revision History:
 *    05-18-2005    Mike Priven
 *                  Written
 *
 * Notes:
 *
-*/

/*---------------------------------------------------------------------------*\
 |                         I N C L U D E   F I L E S
\*---------------------------------------------------------------------------*/
#include "TsipParser.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

using namespace std;


/*---------------------------------------------------------------------------*\
 |                 T S I P   P R O C E S S O R   R O U T I N E S 
\*---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Function:       ReceivePkt

Description:    Receives a complete TSIP packet from a specified serial port.
                The entire packet (the starting DLE, packet ID, packet data, 
                and trailing DLE and ETX) is placed in the buffer ucPkt. The 
                entire packet size is stored in pPktLen.

                NOTE: This function returns as soon as a valid TSIP packet was
                received on the specified serial port. It will block the 
                calling thread until a valid packet has been received.

Parameters:     ptSerialPort - a pointer to the serial port object from which
                               data can be received.
                ucPkt        - a memory buffer where the entire TSIP packet 
                               will be stored.
                pPktLen      - a pointer to a variable to be updated with the
                               packet size (which includes the packet header
                               and trailing bytes).

Return Value:   None
-----------------------------------------------------------------------------*/
void CTsipParser::ReceivePkt (unsigned char raw_data[],
                              int raw_pkt_len)
{
    unsigned char ucByte;
    int           nPktLen, i;
    int           nParseState = MSG_IN_COMPLETE;
    unsigned char ucPkt[MAX_TSIP_PKT_LEN];
    printf("len: %d\n",raw_pkt_len );

    // This function runs in a permanent loop until a valid TSIP packet has
    // been received on the specified serial port.
    for(i = 0; i < raw_pkt_len; i++)
    {
//printf("in for loop i=%d\n", i);
        // The TSIP packet is received in a local state machine.
        ucByte = raw_data[i];
        switch (nParseState) 
        {
            case MSG_IN_COMPLETE:               
                // This is the initial state in which we look for the start
                // of the TSIP packet. We can also end up in this state if
                // we received too many data bytes from the serial port but
                // did not find a valid TSIP packet in that data stream.
                // 
                // While in this state, we look for a DLE character. If we
                // are in this state and the DLE is received, we initialize
                // the packet buffer and transition to the next state.
//printf("MSG_IN_COMPLETE i=%d\n", i);
                if (ucByte == DLE) 
                {
                    nParseState      = TSIP_DLE;
                    nPktLen          = 0;
                    ucPkt[nPktLen++] = ucByte;
                }
                break;
 
            case TSIP_DLE:
                // The parser transitions to this state if a previosly
                // received character was DLE. Receipt of this character
                // indicates that we may be in one of three situations:
                //
                //     Case 1: DLE ETX  = end of a TSIP packet
                //     Case 2: DLE <id> = start of a TSIP packet <id>
                //     Case 3: DLE DLE  = a DLE byte inside the packet
                //                        (stuffed DLE byte which is a part
                //                        of the TSIP data)
                // 
                // To distinguish amongst these, we look at the next
                // character (currently in read into ucByte). 
                // 
                // If the next character is ETX (Case 1), it's the end the 
                // TSIP packet. At this point, we either have a complete TSIP
                // packet in ucPkt or an empty packet. If we have a complete
                // packet, return it to the caller. Otherwise, go back
                // to the intial state and look for a valid packet again.
                //
                // If the next character is anything other than ETX, we
                // add the character to the packet buffer and transition to
                // next state to distinguish between Cases 2 and 3.
//printf("TSIP_DLE i=%d\n", i);
                if (ucByte == ETX) 
                {
                    if (nPktLen > 1)
                    {
                        ucPkt[nPktLen++] = DLE;
                        ucPkt[nPktLen++] = ETX;

                        printf(" a complete packet, len:%d\n", nPktLen);
                        ParsePkt(ucPkt, nPktLen);
                        nPktLen = 0;
                        memset(ucPkt, 0, sizeof(ucPkt));
                        
                        continue;
                    }
                    else
                    {
                        nParseState = MSG_IN_COMPLETE;
                    }
                }
                else  
                {
                    nParseState = TSIP_IN_PARTIAL;
                    ucPkt[nPktLen++] = ucByte;
                }
                break;

            case TSIP_IN_PARTIAL:
//printf("TSIP_IN_PARTIAL i=%d\n", i);
                // The parser is in this state if a previous character was
                // a part of the TSIP data. As noted above, a DLE character
                // can be a part of the TSIP data in which case another DLE
                // character is present in the data stream. So, here we look 
                // at the next character in the stream (currently loaded in 
                // ucByte). If it is a DLE character, we just encountered
                // a stuffed DLE byte. In that case, we ignore this byte
                // and go back to the TSIP_DLE state. That way, we will log
                // only one DLE byte which was a part of the TSIP data.
                //
                // All other non-DLE characters are placed in the TSIP packet
                // buffere.
                if (ucByte == DLE) 
                {
                    nParseState = TSIP_DLE;
                }
                else 
                {
                    ucPkt[nPktLen++] = ucByte;
                }
                break;

            default:
                // We should never be in this state. This is just for a good
                // programming style.
                nParseState = MSG_IN_COMPLETE;
                break;
        }

        // We get to this point after reading each byte from the serial port.
        // Because it is not the end of the valid TSIP packet yet, we check for 
        // the buffer overflow. If it overflows we assume something went wrong 
        // with the end of message character(s) since no input message should
        // be bigger than MAX_TSIP_PKT_LEN. We ignore this message and wait till 
        // the next message starts.                                    
        if (nPktLen >= MAX_TSIP_PKT_LEN) 
        {
            nParseState = MSG_IN_COMPLETE;
            nPktLen = 0;
        }
    }
}

/*-----------------------------------------------------------------------------
Function:       ParsePkt

Description:    This function extracts the data values from a TSIP packets
                and returns an ASCII-formatted string with the values.

Parameters:     ucPkt   - a memory buffer with the entire TSIP packet 
                nPktLen - size of the packet (including the header and 
                          trailing bytes).

Return Value:   A string with TSIP data values extracted and formatted as 
                ASCII text.
-----------------------------------------------------------------------------*/
void CTsipParser::ParsePkt (unsigned char ucPkt[], int nPktLen)
{
    // The CTsipParser object is non-rentrant, and ParsePkt can only
    // be called from one thread because we use a global string to
    // store the parsed TSIP data.


    // ucPkt contains the entire TSIP packet including the leading
    // DLE (0x10) and the trailing DLE and ETX (0x03). 
    
    // Here, based on the TSIP packet ID (indicated by the second
    // byte of the packet buffer, we pass the pointer and size
    // of the actual packet data to an appropriate parser. Note that
    // we only pass the address of the first actual data byte and
    // only the size of data (which excludes the first DLE, packet
    // ID byte, and trailing DLE and ETX.

    switch (ucPkt[1])
    {
        /*case 0x41: Parse0x41 (&ucPkt[2], nPktLen-4); break;
        case 0x42: Parse0x42 (&ucPkt[2], nPktLen-4); break;
        case 0x43: Parse0x43 (&ucPkt[2], nPktLen-4); break;
        case 0x45: Parse0x45 (&ucPkt[2], nPktLen-4); break;
        case 0x46: Parse0x46 (&ucPkt[2], nPktLen-4); break;
        case 0x4A: Parse0x4A (&ucPkt[2], nPktLen-4); break;
        case 0x4B: Parse0x4B (&ucPkt[2], nPktLen-4); break;
        case 0x55: Parse0x55 (&ucPkt[2], nPktLen-4); break;
        case 0x56: Parse0x56 (&ucPkt[2], nPktLen-4); break;
        case 0x6D: Parse0x6D (&ucPkt[2], nPktLen-4); break;
        case 0x82: Parse0x82 (&ucPkt[2], nPktLen-4); break;
        case 0x83: Parse0x83 (&ucPkt[2], nPktLen-4); break;
        case 0x84: Parse0x84 (&ucPkt[2], nPktLen-4); break;*/
        case 0x8F: Parse0x8F (&ucPkt[2], nPktLen-4); break;
        default:                                     break;
    }

    // Each of the individual packet parsers formats m_str with the parsed
    // TSIP data values represented as an ASCII text string.
    return ;
}



/*-----------------------------------------------------------------------------
Function:       Parse0x8F

Description:    Parses TSIP superpacket 0x8F-xx.

Parameters:     ucData - a pointer to the start of the TSIP data values buffer
                nLen   - number of TSIP data bytes in the data buffer ucData

Return Value:   none
-----------------------------------------------------------------------------*/
void CTsipParser::Parse0x8F (unsigned char ucData[], int nLen)
{
    // Extract the super-packet identifier contained in the first byte of the
    // TSIP data stream and dispatch to an appropriate parser.
    switch (ucData[0])
    {
        case 0x20: Parse0x8F20 (ucData, nLen); break;
        case 0xAB: Parse0x8FAB (ucData, nLen); break;
        case 0xAC: Parse0x8FAC (ucData, nLen); break;
        default:                               break;
    }
}

/*-----------------------------------------------------------------------------
Function:       Parse0x8F20

Description:    Extracts the data values from the TSIP packet and fills the
                global m_str variable with ASCII representations of the data
                values.

Parameters:     ucData - a pointer to the start of the TSIP data values buffer
                nLen   - number of TSIP data bytes in the data buffer ucData

Return Value:   none
-----------------------------------------------------------------------------*/
void CTsipParser::Parse0x8F20 (unsigned char ucData[], int nLen)
{
    DBL  dblLat, dblLon, dblAlt, fltTimeOfFix, dblLonDeg, dblLatDeg;
    DBL  dblEnuVel[3], dblVelScale;
    U32  ulTemp;
    S32  lTemp;
    S16  sWeekNum, ucSvIODE[32];
    U8   ucSubpacketID, ucInfo, ucNumSVs, ucSvPrn[32], i, ucPrn, ucMaxSVs;
    S8   cDatumIdx;
    char strDatum[20];

    // Check the length of the data string
    if (nLen == 56)
    {
        ucMaxSVs = 8;
    }
    else if (nLen == 64)
    {
        ucMaxSVs = 12;
    }
    else
    {
        return;
    }

    // Extract values from the data string
    ucSubpacketID = ucData[0];
    dblVelScale   = (ucData[24] & 1) ? 0.020 : 0.005;
    dblEnuVel[0]  = GetShort (&ucData[2]) * dblVelScale;
    dblEnuVel[1]  = GetShort (&ucData[4]) * dblVelScale;
    dblEnuVel[2]  = GetShort (&ucData[6]) * dblVelScale;
    fltTimeOfFix  = GetULong (&ucData[8]) * 0.001;

    lTemp  = GetLong (&ucData[12]);
    dblLat = lTemp*(GPS_PI/MAX_LONG);

    ulTemp = GetULong (&ucData[16]);
    dblLon = ulTemp*(GPS_PI/MAX_LONG);
    if (dblLon > GPS_PI)
    {
        dblLon -= 2.0*GPS_PI;
    }

    dblAlt = GetLong (&ucData[20])*.001;

    /* 25 blank; 29 = UTC */
    cDatumIdx = ucData[26];
    cDatumIdx--;

    ucInfo   = ucData[27];
    ucNumSVs = ucData[28];
    sWeekNum = GetShort (&ucData[30]);

    for (i=0; i<ucMaxSVs; i++) 
    {
        ucPrn       = ucData[32+2*i];
        ucSvPrn[i]  = (U8)(ucPrn & 0x3F);
        ucSvIODE[i] = (U16)(ucData[33+2*i] + 4*((S16)ucPrn-(S16)ucSvPrn[i]));
    }

    // Format the output string
    printf ("Fix at: %04d:%3s:%02d:%02d:%06.3f GPS (=UTC+%2ds)  FixType: %s%s%s",
                      sWeekNum, gstrDayName[(S16)(fltTimeOfFix/86400.0)],
                      (S16)fmod(fltTimeOfFix/3600., 24.), 
                      (S16)fmod(fltTimeOfFix/60., 60.),
                      fmod(fltTimeOfFix, 60.), 
                      (char)ucData[29],
                      ((ucInfo & INFO_DGPS) ? "Diff" : ""),
                      ((ucInfo & INFO_2D) ? "2D" : "3D"),
                      ((ucInfo & INFO_FILTERED) ? "-Filtrd" : ""));
    

    if (cDatumIdx > 0)
    {
        sprintf(strDatum, "Datum%3d", cDatumIdx);
    }
    else if (cDatumIdx)
    {
        sprintf(strDatum, "Unknown ");
    }
    else
    {
        sprintf(strDatum, "WGS-84");
    }

    /* convert from radians to degrees */
    dblLatDeg = R2D * fabs(dblLat);
    dblLonDeg = R2D * fabs(dblLon);

    printf ("\r\n   Pos: %4d:%09.6f %c %5d:%09.6f %c %10.2f m HAE (%s)",
                      (S16)dblLatDeg, fmod(dblLatDeg, 1.)*60.0, (dblLat<0.0)?'S':'N',
                      (S16)dblLonDeg, fmod(dblLonDeg, 1.)*60.0, (dblLon<0.0)?'W':'E',
                      dblAlt, strDatum);
    

    printf ("\r\n   Vel:    %9.3f E       %9.3f N      %9.3f U   (m/sec)",
                      dblEnuVel[0], dblEnuVel[1], dblEnuVel[2]);
    

    printf ("\r\n   SVs: ");
    

    for (i=0; i<ucNumSVs; i++)
    {
        printf (" %02d", ucSvPrn[i]);
        
    }

    printf ("     (IODEs:");
    

    for (i=0; i<ucNumSVs; i++)
    {
        printf (" %02X", ucSvIODE[i] & 0xFF);
        
    }

    printf(")");
    
}

/*-----------------------------------------------------------------------------
Function:       Parse0x8FAB

Description:    Extracts the data values from the TSIP packet and fills the
                global m_str variable with ASCII representations of the data
                values.

Parameters:     ucData - a pointer to the start of the TSIP data values buffer
                nLen   - number of TSIP data bytes in the data buffer ucData

Return Value:   none
-----------------------------------------------------------------------------*/
void CTsipParser::Parse0x8FAB (unsigned char ucData[], int nLen)
{
    U32 ulTimeOfWeek;
    U16 usWeekNumber, usYear;
    S16 sUtcOffset;
    U8  ucTimingFlag, ucSecond, ucMinute, ucHour, ucDay, ucMonth;    

    // Check the length of the data string
    if (nLen != 17) 
    {
        return;
    }

    // Extract values from the data string
    ulTimeOfWeek = GetULong (&ucData[1]);
    usWeekNumber = GetUShort (&ucData[5]);
    sUtcOffset   = GetShort (&ucData[7]);
    ucTimingFlag = ucData[9];
    ucSecond     = ucData[10];
    ucMinute     = ucData[11];
    ucHour       = ucData[12];
    ucDay        = ucData[13];
    ucMonth      = ucData[14];
    usYear       = GetUShort (&ucData[15]);

    // Format the output string
    printf ("8FAB: TOW: %06d  WN: %04d", ulTimeOfWeek, usWeekNumber);
    

    printf ("\r\n      %04d/%02d/%02d  %02d:%02d:%02d",
                      usYear, ucMonth, ucDay, ucHour, ucMinute, ucSecond);
    

    printf ("\r\n      UTC Offset: %d s   Timing flag: 000%d%d%d%d%d",
                      sUtcOffset,
                      ((ucTimingFlag>>4) & 1),
                      ((ucTimingFlag>>3) & 1),
                      ((ucTimingFlag>>2) & 1),
                      ((ucTimingFlag>>1) & 1),
                      ((ucTimingFlag   ) & 1));
    
}

/*-----------------------------------------------------------------------------
Function:       Parse0x8FAC

Description:    Extracts the data values from the TSIP packet and fills the
                global m_str variable with ASCII representations of the data
                values.

Parameters:     ucData - a pointer to the start of the TSIP data values buffer
                nLen   - number of TSIP data bytes in the data buffer ucData

Return Value:   none
-----------------------------------------------------------------------------*/
void CTsipParser::Parse0x8FAC (unsigned char ucData[], int nLen)
{
    U8  ucReceiverMode, ucDiscipliningMode, ucSelfSurveyProgress;
    U8  ucGPSDecodingStatus, ucDiscipliningActivity,ucSpareStatus1;
    U8  ucSpareStatus2;
    U16 usCriticalAlarms, usMinorAlarms;
    FLT fltPPSQuality, fltTenMHzQuality, fltDACVoltage, fltTemperature;
    U32 ulHoldoverDuration, ulDACValue;
    DBL dblLatitude, dblLongitude, dblAltitude, dblLatDeg, dblLonDeg;

    // Check the length of the data string
    if (nLen != 68) 
    {
        return;
    }

    // Extract values from the data string
    ucReceiverMode         = ucData[1];
    ucDiscipliningMode     = ucData[2];
    ucSelfSurveyProgress   = ucData[3];
    ulHoldoverDuration     = GetULong(&ucData[4]);
    usCriticalAlarms       = GetUShort(&ucData[8]);
    usMinorAlarms          = GetUShort(&ucData[10]);
    ucGPSDecodingStatus    = ucData[12];
    ucDiscipliningActivity = ucData[13];
    ucSpareStatus1         = ucData[14];
    ucSpareStatus2         = ucData[15];
    fltPPSQuality          = GetSingle(&ucData[16]);
    fltTenMHzQuality       = GetSingle(&ucData[20]);
    ulDACValue             = GetULong(&ucData[24]);
    fltDACVoltage          = GetSingle(&ucData[28]);
    fltTemperature         = GetSingle(&ucData[32]);
    dblLatitude            = GetDouble(&ucData[36]);
    dblLongitude           = GetDouble(&ucData[44]);
    dblAltitude            = GetDouble(&ucData[52]);

    // These text descriptions are used for formatting below.
    const char *strOprtngDim[8] = 
    {
        "Automatic (2D/3D)",
        "Single Satellite (Time)",
        "unknown",
        "Horizontal (2D)",
        "Full Position (3D)",
        "DGPR Reference",
        "Clock Hold (2D)",
        "Overdetermined Clock"
    };

    // Format the output string
    printf ("8FAC: RecvMode: %s   DiscMode: %d   SelfSurv: %d   Holdover: %d s",
                      strOprtngDim[ucReceiverMode%7], ucDiscipliningMode, 
                      ucSelfSurveyProgress, ulHoldoverDuration);
    

    printf ("\r\n      Crit: %d%d%d%d.%d%d%d%d   Minr: %d%d%d%d.%d%d%d%d",
                      ((usCriticalAlarms >> 7) & 1),
                      ((usCriticalAlarms >> 6) & 1),
                      ((usCriticalAlarms >> 5) & 1),
                      ((usCriticalAlarms >> 4) & 1),
                      ((usCriticalAlarms >> 3) & 1),
                      ((usCriticalAlarms >> 7) & 1),
                      ((usCriticalAlarms >> 1) & 1),
                      ((usCriticalAlarms     ) & 1),
                      ((usMinorAlarms >> 7) & 1),
                      ((usMinorAlarms >> 6) & 1),
                      ((usMinorAlarms >> 5) & 1),
                      ((usMinorAlarms >> 4) & 1),
                      ((usMinorAlarms >> 3) & 1),
                      ((usMinorAlarms >> 7) & 1),
                      ((usMinorAlarms >> 1) & 1),
                      ((usMinorAlarms     ) & 1));
    

    printf ("\r\n      GPS Status: %d   Discpln Act: %d   Spare Status: %d %d",
                      ucGPSDecodingStatus, ucDiscipliningActivity, 
                      ucSpareStatus1, ucSpareStatus2);
    

    printf ("\r\n      Qual:  PPS: %.1f ns   Freq: %.3f PPB",
                      fltPPSQuality, fltTenMHzQuality);
    

    printf ("\r\n      DAC:  Value: %d   Voltage: %f   Temp: %f deg C",
                      ulDACValue, fltDACVoltage, fltTemperature);
    

    /* convert from radians to degrees */
    dblLatDeg = R2D * fabs(dblLatitude);
    dblLonDeg = R2D * fabs(dblLongitude);

    printf ("\r\n      Pos:  %d:%09.6f %c   %d:%09.6f %c   %.2f m ",
                      (short)dblLatDeg, fmod (dblLatDeg, 1.)*60.0,
                      (dblLatitude<0.0)?'S':'N',
                      (short)dblLonDeg, fmod (dblLonDeg, 1.)*60.0,
                      (dblLongitude<0.0)?'W':'E',
                      dblAltitude);    
    
}

/*---------------------------------------------------------------------------*\
 |            D A T A   V A L U E   E X T R A C T   R O U T I N E S
\*---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Function:       GetShort

Description:    Extracts an S16 value from the data buffer pointed to by pucBuf.

Parameters:     pucBuf - a pointer to the data from which to extract the value

Return Value:   Extracted value
-----------------------------------------------------------------------------*/
S16 CTsipParser::GetShort (U8 *pucBuf)
{
    U8 ucBuf[2];

    ucBuf[0] = pucBuf[1];
    ucBuf[1] = pucBuf[0];

    return (*((S16*)ucBuf));
}

/*-----------------------------------------------------------------------------
Function:       GetUShort

Description:    Extracts a U16 value from the data buffer pointed to by pucBuf.

Parameters:     pucBuf - a pointer to the data from which to extract the value

Return Value:   Extracted value
-----------------------------------------------------------------------------*/
U16 CTsipParser::GetUShort (U8 *pucBuf)
{
    U8 ucBuf[2];

    ucBuf[0] = pucBuf[1];
    ucBuf[1] = pucBuf[0];

    return (*((U16*)ucBuf));
}

/*-----------------------------------------------------------------------------
Function:       GetLong

Description:    Extracts an S32 value from the data buffer pointed to by pucBuf.

Parameters:     pucBuf - a pointer to the data from which to extract the value

Return Value:   Extracted value
-----------------------------------------------------------------------------*/
S32 CTsipParser::GetLong (U8 *pucBuf)
{
    U8 ucBuf[4];

    ucBuf[0] = pucBuf[3];
    ucBuf[1] = pucBuf[2];
    ucBuf[2] = pucBuf[1];
    ucBuf[3] = pucBuf[0];

    return (*((S32*)ucBuf));
}

/*-----------------------------------------------------------------------------
Function:       GetULong

Description:    Extracts a U32 value from the data buffer pointed to by pucBuf.

Parameters:     pucBuf - a pointer to the data from which to extract the value

Return Value:   Extracted value
-----------------------------------------------------------------------------*/
U32 CTsipParser::GetULong (U8 *pucBuf)
{
    U8 ucBuf[4];

    ucBuf[0] = pucBuf[3];
    ucBuf[1] = pucBuf[2];
    ucBuf[2] = pucBuf[1];
    ucBuf[3] = pucBuf[0];

    return (*((U32 *)ucBuf));
}

/*-----------------------------------------------------------------------------
Function:       GetSingle

Description:    Extracts a FLT value from the data buffer pointed to by pucBuf.

Parameters:     pucBuf - a pointer to the data from which to extract the value

Return Value:   Extracted value
-----------------------------------------------------------------------------*/
FLT CTsipParser::GetSingle (U8 *pucBuf)
{
    U8 ucBuf[4];

    ucBuf[0] = pucBuf[3];
    ucBuf[1] = pucBuf[2];
    ucBuf[2] = pucBuf[1];
    ucBuf[3] = pucBuf[0];

    return (*((FLT *)ucBuf));
}

/*-----------------------------------------------------------------------------
Function:       GetDouble

Description:    Extracts a DBL value from the data buffer pointed to by pucBuf.

Parameters:     pucBuf - a pointer to the data from which to extract the value

Return Value:   Extracted value
-----------------------------------------------------------------------------*/
DBL CTsipParser::GetDouble (U8* pucBuf)
{
    U8 ucBuf[8];

    ucBuf[0] = pucBuf[7];
    ucBuf[1] = pucBuf[6];
    ucBuf[2] = pucBuf[5];
    ucBuf[3] = pucBuf[4];
    ucBuf[4] = pucBuf[3];
    ucBuf[5] = pucBuf[2];
    ucBuf[6] = pucBuf[1];
    ucBuf[7] = pucBuf[0];

    return (*((DBL *)ucBuf));
}

/*---------------------------------------------------------------------------*\
 |                       H E L P E R   R O U T I N E S
\*---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Function:       TsipShowTime

Description:    Convert time of week into day-hour-minute-second and print

Parameters:     fltTimeOfWeek - time of week

Return Value:   A pointer to the formatted time string
-----------------------------------------------------------------------------*/
void CTsipParser::ShowTime (FLT fltTimeOfWeek)
{
    S16     sDay, sHour, sMinute;
    FLT     fltSecond;
    DBL     dblTimeOfWeek;
    

    if (fltTimeOfWeek == -1.0)
    {
        printf("   <No time yet>   ");
    }
    else if ((fltTimeOfWeek >= 604800.0) || (fltTimeOfWeek < 0.0))
    {
        printf("     <Bad time>     ");
    }
    else
    {
        if (fltTimeOfWeek < 604799.9) 
        {
            dblTimeOfWeek = fltTimeOfWeek + .00000001;
        }

        fltSecond = (FLT)fmod(dblTimeOfWeek, 60.);
        sMinute   =  (S16) fmod(dblTimeOfWeek/60., 60.);
        sHour     = (S16)fmod(dblTimeOfWeek / 3600., 24.);
        sDay      = (S16)(dblTimeOfWeek / 86400.0);

         printf(" %s %02d:%02d:%05.2f   ",
                        gstrDayName[sDay], sHour, sMinute, fltSecond);
    }

    return ;
}
