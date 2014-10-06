/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   version.h
*/
#ifndef __VERSION_H
#define __VERSION_H

/*
 * Version definitions
 */
#define VERSION_MAJOR           4
#define VERSION_MINOR           0
#define EDITION_LEVEL           0
#define BUILD_NUMBER            0

#define VERSION_2_1_10_0	0x02010a00ul
#define VERSION_2_1_10_1	0x02010a01ul
#define VERSION_2_1_10_7	0x02010a07ul
#define VERSION_2_1_10_9	0x02010a09ul
#define VERSION_2_1_10_16	0x02010a10ul
#define VERSION_2_1_10_17	0x02010a11ul
#define VERSION_2_1_10_24	0x02010a18ul
#define VERSION_2_2_0_1	   0x02020001ul
#define VERSION_2_3_0_5	   0x02030005ul
#define VERSION_2_5_0_1	   0x02050001ul
#define VERSION_3_0_1_1	   0x03000101ul
#define VERSION_3_3_1_5	   0x03030105ul
#define VERSION_3_4_1_1    0x03040101ul
#define VERSION_3_5_1_1    0x03050101ul
#define VERSION_3_8_0_1    0x03080001ul
#define VERSION_3_8_0_4    0x03080004ul
#define VERSION_3_10_3_2   0x030A0302ul

/*
 * Version macros
 */
#define VERSION_BASE		((VERSION_MAJOR << 8) | VERSION_MINOR)
#define VERSION			(((VERSION_BASE << 16) | (EDITION_LEVEL << 8)) | BUILD_NUMBER)

#define MAKE_STRING(value)				#value
#define MAKE_STRING_EX(value)			MAKE_STRING(value)

#define MAKE_EDITION_LEVEL(editlvl)	MAKE_STRING(editlvl)
#define MAKE_BUILD_NUMBER(buildnum)	MAKE_STRING(buildnum)
#define MAKE_VERSION(major, minor)	MAKE_STRING(major) "." MAKE_STRING(minor)

#define VER_PART(version, part) ((u_int) (((version) >> (part * 8)) & 0xFF))

#endif // __VERSION_H

