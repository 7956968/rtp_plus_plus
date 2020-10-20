/** @file

MODULE						: FastForwardDctImpl2

TAG								: FFDI2

FILE NAME					: FastForwardDctImpl2.cpp

DESCRIPTION				: A class to implement a fast forward 8x8 2-D dct on the 
										input. It implements the IForwardDct interface. The 
										scaling is designed for use in H.263 codecs.

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2008 all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <string.h>

#include "FastForwardDctImpl2.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define FFDI2_W1	2841				///< sqrt(2).cos(pi/16) << 11
#define FFDI2_W2	2676				///< sqrt(2).cos(2.pi/16) << 11
#define FFDI2_W3	2408				///< sqrt(2).cos(3.pi/16) << 11
#define FFDI2_W5	1609				///< sqrt(2).cos(5.pi/16) << 11
#define FFDI2_W6	1108				///< sqrt(2).cos(6.pi/16) << 11
#define FFDI2_W7	 565				///< sqrt(2).cos(7.pi/16) << 11
#define FFDI2_W10 2276				///< W1 - W7
#define FFDI2_W11 3406				///< W1 + W7
#define FFDI2_W12 4017				///< W3 + W5
#define FFDI2_W13  799				///< W3 - W5
#define FFDI2_W14 1568				///< W2 - W6
#define FFDI2_W15 3784				///< W2 + W6

#define FFDI2_UNROLL_LOOP_AND_INTERLEAVE

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** In-place forward Dct.
The 2-D Dct is performed on the input and replaces it with the coeffs. A 1-D
transform is performed on the rows first and then the cols. 11-point fixed
arithmetic is used except for one 8-point case. Rounding for -ve numbers is
towards zero.
@param ptr	: Data to transform.
@return			:	none.
*/
void FastForwardDctImpl2::dct(void* ptr)
{
	short* block = (short *)ptr;

#ifdef FFDI2_UNROLL_LOOP_AND_INTERLEAVE
	/// Should be obvious for superscalar (interleave) and large code cache 
	/// (unrolled loops) based CPUs.
	register int s0,s1,s2,s3,s4,s5,s6,s7,t5,t6;
	register int u0,u1,u2,u3,u4,u5,u6,u7,v5,v6;

	/// ---------------------------------------------------------------------------------------------
	/// 1-D forward horiz direction.

	s0 = (int)(block[0]	 + block[7]);
	s7 = (int)(block[0]	 - block[7]);
	s1 = (int)(block[1]  + block[6]);
	s6 = (int)(block[1]  - block[6]);
	s2 = (int)(block[2]  + block[5]);
	s5 = (int)(block[2]  - block[5]);
	s3 = (int)(block[3]  + block[4]);
	s4 = (int)(block[3]  - block[4]);
	u0 = (int)(block[8]	 + block[15]);
	u7 = (int)(block[8]	 - block[15]);
	u1 = (int)(block[9]  + block[14]);
	u6 = (int)(block[9]  - block[14]);
	u2 = (int)(block[10] + block[13]);
	u5 = (int)(block[10] - block[13]);
	u3 = (int)(block[11] + block[12]);
	u4 = (int)(block[11] - block[12]);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[0]  = (short)(s0 + s1 + s2 + s3);
	block[1]  = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[2]  = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[3]  = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[4]  = (short)(s0 - s1 - s2 + s3);
	block[5]  = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[6]  = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[7]  = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[8]  = (short)(u0 + u1 + u2 + u3);
	block[9]  = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[10] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[11] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[12] = (short)(u0 - u1 - u2 + u3);
	block[13] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[14] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);
	block[15] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	s0 = (int)(block[16] + block[23]);
	s7 = (int)(block[16] - block[23]);
	s1 = (int)(block[17] + block[22]);
	s6 = (int)(block[17] - block[22]);
	s2 = (int)(block[18] + block[21]);
	s5 = (int)(block[18] - block[21]);
	s3 = (int)(block[19] + block[20]);
	s4 = (int)(block[19] - block[20]);
	u0 = (int)(block[24] + block[31]);
	u7 = (int)(block[24] - block[31]);
	u1 = (int)(block[25] + block[30]);
	u6 = (int)(block[25] - block[30]);
	u2 = (int)(block[26] + block[29]);
	u5 = (int)(block[26] - block[29]);
	u3 = (int)(block[27] + block[28]);
	u4 = (int)(block[27] - block[28]);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[16] = (short)(s0 + s1 + s2 + s3);
	block[17] = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[18] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[19] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[20] = (short)(s0 - s1 - s2 + s3);
	block[21] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[22] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[23] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[24] = (short)(u0 + u1 + u2 + u3);
	block[25] = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[26] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[27] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[28] = (short)(u0 - u1 - u2 + u3);
	block[29] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[30] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);
	block[31] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	s0 = (int)(block[32] + block[39]);
	s7 = (int)(block[32] - block[39]);
	s1 = (int)(block[33] + block[38]);
	s6 = (int)(block[33] - block[38]);
	s2 = (int)(block[34] + block[37]);
	s5 = (int)(block[34] - block[37]);
	s3 = (int)(block[35] + block[36]);
	s4 = (int)(block[35] - block[36]);
	u0 = (int)(block[40] + block[47]);
	u7 = (int)(block[40] - block[47]);
	u1 = (int)(block[41] + block[46]);
	u6 = (int)(block[41] - block[46]);
	u2 = (int)(block[42] + block[45]);
	u5 = (int)(block[42] - block[45]);
	u3 = (int)(block[43] + block[44]);
	u4 = (int)(block[43] - block[44]);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[32] = (short)(s0 + s1 + s2 + s3);
	block[33] = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[34] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[35] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[36] = (short)(s0 - s1 - s2 + s3);
	block[37] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[38] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[39] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[40] = (short)(u0 + u1 + u2 + u3);
	block[41] = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[42] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[43] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[44] = (short)(u0 - u1 - u2 + u3);
	block[45] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[46] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);
	block[47] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	s0 = (int)(block[48] + block[55]);
	s7 = (int)(block[48] - block[55]);
	s1 = (int)(block[49] + block[54]);
	s6 = (int)(block[49] - block[54]);
	s2 = (int)(block[50] + block[53]);
	s5 = (int)(block[50] - block[53]);
	s3 = (int)(block[51] + block[52]);
	s4 = (int)(block[51] - block[52]);
	u0 = (int)(block[56] + block[63]);
	u7 = (int)(block[56] - block[63]);
	u1 = (int)(block[57] + block[62]);
	u6 = (int)(block[57] - block[62]);
	u2 = (int)(block[58] + block[61]);
	u5 = (int)(block[58] - block[61]);
	u3 = (int)(block[59] + block[60]);
	u4 = (int)(block[59] - block[60]);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[48] = (short)(s0 + s1 + s2 + s3);
	block[49] = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[50] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[51] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[52] = (short)(s0 - s1 - s2 + s3);
	block[53] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[54] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[55] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[56] = (short)(u0 + u1 + u2 + u3);
	block[57] = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[58] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[59] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[60] = (short)(u0 - u1 - u2 + u3);
	block[61] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[62] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);
	block[63] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	/// ---------------------------------------------------------------------------------------------
	/// 1-D forward vertical transform.

	s0 = (int)(block[0]	 + block[56]);
	s7 = (int)(block[0]  - block[56]);
	u0 = (int)(block[1]	 + block[57]);
	u7 = (int)(block[1]  - block[57]);
	s1 = (int)(block[8]	 + block[48]);
	s6 = (int)(block[8]  - block[48]);
	u1 = (int)(block[9]	 + block[49]);
	u6 = (int)(block[9]  - block[49]);
	s2 = (int)(block[16] + block[40]);
	s5 = (int)(block[16] - block[40]);
	u2 = (int)(block[17] + block[41]);
	u5 = (int)(block[17] - block[41]);
	s3 = (int)(block[24] + block[32]);
	s4 = (int)(block[24] - block[32]);
	u3 = (int)(block[25] + block[33]);
	u4 = (int)(block[25] - block[33]);

	/// Final stage transform with scaling (dividing) by 8 and rounding. The
	/// sqrt(2) scaling factor is built in to the constants.
	/// scaling = sqrt(2)/8.

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[0]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[1]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[8]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[9]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[16] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[17] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[24] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[25] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[32] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[33] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[40] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[41] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[48] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[49] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);
	block[56] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[57] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);

	s0 = (int)(block[2]	 + block[58]);
	s7 = (int)(block[2]  - block[58]);
	u0 = (int)(block[3]	 + block[59]);
	u7 = (int)(block[3]  - block[59]);
	s1 = (int)(block[10] + block[50]);
	s6 = (int)(block[10] - block[50]);
	u1 = (int)(block[11] + block[51]);
	u6 = (int)(block[11] - block[51]);
	s2 = (int)(block[18] + block[42]);
	s5 = (int)(block[18] - block[42]);
	u2 = (int)(block[19] + block[43]);
	u5 = (int)(block[19] - block[43]);
	s3 = (int)(block[26] + block[34]);
	s4 = (int)(block[26] - block[34]);
	u3 = (int)(block[27] + block[35]);
	u4 = (int)(block[27] - block[35]);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[2]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[3]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[10]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[11]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[18] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[19] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[26] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[27] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[34] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[35] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[42] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[43] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[50] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[51] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);
	block[58] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[59] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);

	s0 = (int)(block[4]	 + block[60]);
	s7 = (int)(block[4]  - block[60]);
	u0 = (int)(block[5]	 + block[61]);
	u7 = (int)(block[5]  - block[61]);
	s1 = (int)(block[12] + block[52]);
	s6 = (int)(block[12] - block[52]);
	u1 = (int)(block[13] + block[53]);
	u6 = (int)(block[13] - block[53]);
	s2 = (int)(block[20] + block[44]);
	s5 = (int)(block[20] - block[44]);
	u2 = (int)(block[21] + block[45]);
	u5 = (int)(block[21] - block[45]);
	s3 = (int)(block[28] + block[36]);
	s4 = (int)(block[28] - block[36]);
	u3 = (int)(block[29] + block[37]);
	u4 = (int)(block[29] - block[37]);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[4]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[5]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[12]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[13]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[20] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[21] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[28] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[29] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[36] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[37] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[44] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[45] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[52] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[53] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);
	block[60] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[61] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);

	s0 = (int)(block[6]	 + block[62]);
	s7 = (int)(block[6]  - block[62]);
	u0 = (int)(block[7]	 + block[63]);
	u7 = (int)(block[7]  - block[63]);
	s1 = (int)(block[14] + block[54]);
	s6 = (int)(block[14] - block[54]);
	u1 = (int)(block[15] + block[55]);
	u6 = (int)(block[15] - block[55]);
	s2 = (int)(block[22] + block[46]);
	s5 = (int)(block[22] - block[46]);
	u2 = (int)(block[23] + block[47]);
	u5 = (int)(block[23] - block[47]);
	s3 = (int)(block[30] + block[38]);
	s4 = (int)(block[30] - block[38]);
	u3 = (int)(block[31] + block[39]);
	u4 = (int)(block[31] - block[39]);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[6]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[7]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[14]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[15]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[22] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[23] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[30] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[31] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[38] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[39] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[46] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[47] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[54] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[55] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);
	block[62] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[63] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);

/*
	s0 = (int)(block[0]	 + block[7]);
	s1 = (int)(block[1] + block[6]);
	s2 = (int)(block[2] + block[5]);
	s3 = (int)(block[3] + block[4]);
	s4 = (int)(block[3] - block[4]);
	s5 = (int)(block[2] - block[5]);
	s6 = (int)(block[1] - block[6]);
	s7 = (int)(block[0]	- block[7]);

	u0 = (int)(block[8]	 + block[15]);
	u1 = (int)(block[9]  + block[14]);
	u2 = (int)(block[10] + block[13]);
	u3 = (int)(block[11] + block[12]);
	u4 = (int)(block[11] - block[12]);
	u5 = (int)(block[10] - block[13]);
	u6 = (int)(block[9]  - block[14]);
	u7 = (int)(block[8]	 - block[15]);

	block[0]  = (short)(s0 + s1 + s2 + s3);
	block[8]  = (short)(u0 + u1 + u2 + u3);
	block[2]  = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[10] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[4]  = (short)(s0 - s1 - s2 + s3);
	block[12] = (short)(u0 - u1 - u2 + u3);
	block[6]  = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[14] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[1]  = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[9]  = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[3]  = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[11] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[5]  = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[13] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[7]  = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[15] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	s0 = (int)(block[16] + block[23]);
	s1 = (int)(block[17] + block[22]);
	s2 = (int)(block[18] + block[21]);
	s3 = (int)(block[19] + block[20]);
	s4 = (int)(block[19] - block[20]);
	s5 = (int)(block[18] - block[21]);
	s6 = (int)(block[17] - block[22]);
	s7 = (int)(block[16] - block[23]);

	u0 = (int)(block[24] + block[31]);
	u1 = (int)(block[25] + block[30]);
	u2 = (int)(block[26] + block[29]);
	u3 = (int)(block[27] + block[28]);
	u4 = (int)(block[27] - block[28]);
	u5 = (int)(block[26] - block[29]);
	u6 = (int)(block[25] - block[30]);
	u7 = (int)(block[24] - block[31]);

	block[16] = (short)(s0 + s1 + s2 + s3);
	block[24] = (short)(u0 + u1 + u2 + u3);
	block[18] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[26] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[20] = (short)(s0 - s1 - s2 + s3);
	block[28] = (short)(u0 - u1 - u2 + u3);
	block[22] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[30] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[17] = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[25] = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[19] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[27] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[21] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[29] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[23] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[31] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	s0 = (int)(block[32] + block[39]);
	s1 = (int)(block[33] + block[38]);
	s2 = (int)(block[34] + block[37]);
	s3 = (int)(block[35] + block[36]);
	s4 = (int)(block[35] - block[36]);
	s5 = (int)(block[34] - block[37]);
	s6 = (int)(block[33] - block[38]);
	s7 = (int)(block[32] - block[39]);

	u0 = (int)(block[40] + block[47]);
	u1 = (int)(block[41] + block[46]);
	u2 = (int)(block[42] + block[45]);
	u3 = (int)(block[43] + block[44]);
	u4 = (int)(block[43] - block[44]);
	u5 = (int)(block[42] - block[45]);
	u6 = (int)(block[41] - block[46]);
	u7 = (int)(block[40] - block[47]);

	block[32] = (short)(s0 + s1 + s2 + s3);
	block[40] = (short)(u0 + u1 + u2 + u3);
	block[34] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[42] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[36] = (short)(s0 - s1 - s2 + s3);
	block[44] = (short)(u0 - u1 - u2 + u3);
	block[38] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[46] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[33] = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[41] = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[35] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[43] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[37] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[45] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[39] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[47] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	s0 = (int)(block[48] + block[55]);
	s1 = (int)(block[49] + block[54]);
	s2 = (int)(block[50] + block[53]);
	s3 = (int)(block[51] + block[52]);
	s4 = (int)(block[51] - block[52]);
	s5 = (int)(block[50] - block[53]);
	s6 = (int)(block[49] - block[54]);
	s7 = (int)(block[48] - block[55]);

	u0 = (int)(block[56] + block[63]);
	u1 = (int)(block[57] + block[62]);
	u2 = (int)(block[58] + block[61]);
	u3 = (int)(block[59] + block[60]);
	u4 = (int)(block[59] - block[60]);
	u5 = (int)(block[58] - block[61]);
	u6 = (int)(block[57] - block[62]);
	u7 = (int)(block[56] - block[63]);

	block[48] = (short)(s0 + s1 + s2 + s3);
	block[56] = (short)(u0 + u1 + u2 + u3);
	block[50] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
	block[58] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 1024) >> 11);
	block[52] = (short)(s0 - s1 - s2 + s3);
	block[60] = (short)(u0 - u1 - u2 + u3);
	block[54] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);
	block[62] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 1024) >> 11);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[49] = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
	block[57] = (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 1024) >> 11);
	block[51] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
	block[59] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 1024) >> 11);
	block[53] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
	block[61] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 1024) >> 11);
	block[55] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);
	block[63] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 1024) >> 11);

	/// 1-D forward vertical transform.

	s0 = (int)(block[0]	 + block[56]);
	s1 = (int)(block[8]	 + block[48]);
	s2 = (int)(block[16] + block[40]);
	s3 = (int)(block[24] + block[32]);
	s4 = (int)(block[24] - block[32]);
	s5 = (int)(block[16] - block[40]);
	s6 = (int)(block[8]  - block[48]);
	s7 = (int)(block[0]  - block[56]);

	u0 = (int)(block[1]	 + block[57]);
	u1 = (int)(block[9]	 + block[49]);
	u2 = (int)(block[17] + block[41]);
	u3 = (int)(block[25] + block[33]);
	u4 = (int)(block[25] - block[33]);
	u5 = (int)(block[17] - block[41]);
	u6 = (int)(block[9]  - block[49]);
	u7 = (int)(block[1]  - block[57]);

	/// Final stage transform with scaling (dividing) by 8 and rounding. The
	/// sqrt(2) scaling factor is built in to the constants.
	/// scaling = sqrt(2)/8.
	block[0]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[1]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[16] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[17] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[32] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[33] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[48] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[49] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[8]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[9]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[24] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[25] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[40] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[41] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[56] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[57] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);

	s0 = (int)(block[2]	 + block[58]);
	s1 = (int)(block[10] + block[50]);
	s2 = (int)(block[18] + block[42]);
	s3 = (int)(block[26] + block[34]);
	s4 = (int)(block[26] - block[34]);
	s5 = (int)(block[18] - block[42]);
	s6 = (int)(block[10] - block[50]);
	s7 = (int)(block[2]  - block[58]);

	u0 = (int)(block[3]	 + block[59]);
	u1 = (int)(block[11] + block[51]);
	u2 = (int)(block[19] + block[43]);
	u3 = (int)(block[27] + block[35]);
	u4 = (int)(block[27] - block[35]);
	u5 = (int)(block[19] - block[43]);
	u6 = (int)(block[11] - block[51]);
	u7 = (int)(block[3]  - block[59]);

	block[2]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[3]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[18] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[19] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[34] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[35] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[50] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[51] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[10]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[11]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[26] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[27] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[42] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[43] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[58] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[59] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);

	s0 = (int)(block[4]	 + block[60]);
	s1 = (int)(block[12] + block[52]);
	s2 = (int)(block[20] + block[44]);
	s3 = (int)(block[28] + block[36]);
	s4 = (int)(block[28] - block[36]);
	s5 = (int)(block[20] - block[44]);
	s6 = (int)(block[12] - block[52]);
	s7 = (int)(block[4]  - block[60]);

	u0 = (int)(block[5]	 + block[61]);
	u1 = (int)(block[13] + block[53]);
	u2 = (int)(block[21] + block[45]);
	u3 = (int)(block[29] + block[37]);
	u4 = (int)(block[29] - block[37]);
	u5 = (int)(block[21] - block[45]);
	u6 = (int)(block[13] - block[53]);
	u7 = (int)(block[5]  - block[61]);

	block[4]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[5]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[20] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[21] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[36] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[37] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[52] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[53] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[12]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[13]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[28] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[29] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[44] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[45] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[60] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[61] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);

	s0 = (int)(block[6]	 + block[62]);
	s1 = (int)(block[14] + block[54]);
	s2 = (int)(block[22] + block[46]);
	s3 = (int)(block[30] + block[38]);
	s4 = (int)(block[30] - block[38]);
	s5 = (int)(block[22] - block[46]);
	s6 = (int)(block[14] - block[54]);
	s7 = (int)(block[6]  - block[62]);

	u0 = (int)(block[7]	 + block[63]);
	u1 = (int)(block[15] + block[55]);
	u2 = (int)(block[23] + block[47]);
	u3 = (int)(block[31] + block[39]);
	u4 = (int)(block[31] - block[39]);
	u5 = (int)(block[23] - block[47]);
	u6 = (int)(block[15] - block[55]);
	u7 = (int)(block[7]  - block[63]);

	block[6]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
	block[7]	= (short)((u0 + u1 + u2 + u3 + 4) >> 3);
	block[22] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
	block[23] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) + ((u0 - u3) * FFDI2_W14) + 8192) >> 14);
	block[38] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
	block[39] = (short)((u0 - u1 - u2 + u3 + 4) >> 3);
	block[54] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);
	block[55] = (short)((((u0 + u1 - u2 - u3) * FFDI2_W6) - ((u1 - u2) * FFDI2_W15) + 8192) >> 14);

	t5 = (((s6 - s5) * 181) + 128) >> 8;
	t6 = (((s6 + s5) * 181) + 128) >> 8;
	v5 = (((u6 - u5) * 181) + 128) >> 8;
	v6 = (((u6 + u5) * 181) + 128) >> 8;

	block[14]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
	block[15]	= (short)((((u4 + u7 + v5 + v6) * FFDI2_W1) - ((u4 + v5) * FFDI2_W10) + 8192) >> 14);
	block[30] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
	block[31] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u4 - v5) * FFDI2_W12) + 8192) >> 14);
	block[46] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
	block[47] = (short)((((u4 + u7 - v5 - v6) * FFDI2_W3) - ((u7 - v6) * FFDI2_W13) + 8192) >> 14);
	block[62] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	block[63] = (short)((((u7 + v6) * FFDI2_W11) - ((u4 + u7 + v5 + v6) * FFDI2_W1) + 8192) >> 14);
*/
#else

	int j;

	/// 1-D forward horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// Loop 0
		int s0 = (int)(block[j]	  + block[j+7]);
		int s1 = (int)(block[j+1] + block[j+6]);
		int s2 = (int)(block[j+2] + block[j+5]);
		int s3 = (int)(block[j+3] + block[j+4]);
		int s4 = (int)(block[j+3] - block[j+4]);
		int s5 = (int)(block[j+2] - block[j+5]);
		int s6 = (int)(block[j+1] - block[j+6]);
		int s7 = (int)(block[j]		- block[j+7]);

		block[j]	 = (short)(s0 + s1 + s2 + s3);
		block[2+j] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 1024) >> 11);
		block[4+j] = (short)(s0 - s1 - s2 + s3);
		block[6+j] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 1024) >> 11);

		int t5 = (((s6 - s5) * 181) + 128) >> 8;
		int t6 = (((s6 + s5) * 181) + 128) >> 8;

		block[1+j] = (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 1024) >> 11);
		block[3+j] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 1024) >> 11);
		block[5+j] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 1024) >> 11);
		block[7+j] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 1024) >> 11);

	}//end for j...

	/// 1-D forward vert direction.
	for(j = 0; j < 8; j++)
	{
		int s0 = (int)(block[j]			+ block[j+56]);
		int s1 = (int)(block[j+8]		+ block[j+48]);
		int s2 = (int)(block[j+16]	+ block[j+40]);
		int s3 = (int)(block[j+24]	+ block[j+32]);
		int s4 = (int)(block[j+24]	- block[j+32]);
		int s5 = (int)(block[j+16]	- block[j+40]);
		int s6 = (int)(block[j+8]		- block[j+48]);
		int s7 = (int)(block[j]			- block[j+56]);

		// Final stage transform with scaling (dividing) by 8 and rounding. The
		// sqrt(2) scaling factor is built in to the constants.
		// scaling = sqrt(2)/8.
		block[0+j]	= (short)((s0 + s1 + s2 + s3 + 4) >> 3);
		block[16+j] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) + ((s0 - s3) * FFDI2_W14) + 8192) >> 14);
		block[32+j] = (short)((s0 - s1 - s2 + s3 + 4) >> 3);
		block[48+j] = (short)((((s0 + s1 - s2 - s3) * FFDI2_W6) - ((s1 - s2) * FFDI2_W15) + 8192) >> 14);

		int t5 = (((s6 - s5) * 181) + 128) >> 8;
		int t6 = (((s6 + s5) * 181) + 128) >> 8;

		block[8+j]	= (short)((((s4 + s7 + t5 + t6) * FFDI2_W1) - ((s4 + t5) * FFDI2_W10) + 8192) >> 14);
		block[24+j] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s4 - t5) * FFDI2_W12) + 8192) >> 14);
		block[40+j] = (short)((((s4 + s7 - t5 - t6) * FFDI2_W3) - ((s7 - t6) * FFDI2_W13) + 8192) >> 14);
		block[56+j] = (short)((((s7 + t6) * FFDI2_W11) - ((s4 + s7 + t5 + t6) * FFDI2_W1) + 8192) >> 14);
	}//end for j...

#endif
}//end dct.

/** Transfer forward Dct.
The 2-D Dct is performed on the input and the coeffs are written to 
the output. A 1-D transform is performed on the rows first and then the 
cols. 11-point fixed arithmetic is used except for one 8-point case. 
Rounding for -ve numbers is towards zero.
@param pIn		: Input data.
@param pCoeff	: Output coeffs.
@return				:	none.
*/
void FastForwardDctImpl2::dct(void* pIn, void* pCoeff)
{
	/// Copy to output and then do in-place inverse transform.
	memcpy(pCoeff, pIn, sizeof(short) * 64);
	dct(pCoeff);
}//end dct.


