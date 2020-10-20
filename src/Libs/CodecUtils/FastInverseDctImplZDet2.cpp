/** @file

MODULE						: FastInverseDctImplZDet2

TAG								: FIDIZD2

FILE NAME					: FastInverseDctImplZDet2.cpp

DESCRIPTION				: A class to implement a fast inverse 8x8 2-D dct on the 
										input. Zero coeff detection is performed before the 
										inverse transform to reduce the number of operations. It 
										implements the IInverseDct interface. The scaling is 
										designed for use in H.263 codecs.

COPYRIGHT					: (c)CSIR 2007-2010 all rights resevered

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

#include "FastInverseDctImplZDet2.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define FIDIZD2_W1	2841				///< sqrt(2).cos(pi/16) << 11			or sin(7.pi/16)
#define FIDIZD2_W2	2676				///< sqrt(2).cos(2.pi/16) << 11		or sin(6.pi/16)
#define FIDIZD2_W3	2408				///< sqrt(2).cos(3.pi/16) << 11		or sin(5.pi/16)
#define FIDIZD2_W5	1609				///< sqrt(2).cos(5.pi/16) << 11		or sin(3.pi/16)
#define FIDIZD2_W6	1108				///< sqrt(2).cos(6.pi/16) << 11		or sin(2.pi/16)
#define FIDIZD2_W7	 565				///< sqrt(2).cos(7.pi/16) << 11		or sin(pi/16)
#define FIDIZD2_W10 2276				///< W1 - W7
#define FIDIZD2_W11 3406				///< W1 + W7
#define FIDIZD2_W12 4017				///< W3 + W5
#define FIDIZD2_W13  799				///< W3 - W5
#define FIDIZD2_W14 1568				///< W2 - W6
#define FIDIZD2_W15 3784				///< W2 + W6

#define FIDIZD2_UNROLL_LOOPS_AND_INTERLEAVE

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** In-place inverse Dct.
The 2-D inverse Dct is performed on the input coeffs and replaces them. A 1-D
inverse transform is performed on the rows first and then the cols. 11-point 
fixed arithmetic is used except for one 8-point case. Rounding for -ve numbers 
is towards zero.
@param ptr	: Data to transform.
@return			:	none.
*/
void FastInverseDctImplZDet2::idct(void* ptr)
{
	switch( GetPattern(ptr) )
	{
		case 1:
			Quad123Pattern(ptr);
			break;
		case 2:
			Quad13Pattern(ptr);
			break;
		case 3:
			Quad23Pattern(ptr);
			break;
		case 4:
			Quad3Pattern(ptr);
			break;
		case 0:
		default:
			Quad0123Pattern(ptr);
			break;
	}//end switch GetPattern...

}//end idct.

/** Transfer inverse Dct.
The inverse Dct is performed on the coeffs and are written to 
the output. Not implemented.
@param pCoeff	: Input coeffs.
@param pOut		: Output data.
@return				:	none.
*/
void FastInverseDctImplZDet2::idct(void* pCoeff, void* pOut)
{
	/// Copy to output and then do in-place inverse transform.
	memcpy(pOut, pCoeff, sizeof(short) * 64);
	idct(pOut);
}//end idct.

/*
---------------------------------------------------------------------------
	Private Methods.
---------------------------------------------------------------------------
*/
/** Get the zero pattern in the block.
Each 4x4 block within the higher freq coeffs of the 8x8 block is examined 
for the existence of non-zero coeffs. 
@param ptr	: Coeffs.
@return			: 0 = all non-zero, 1 = Quad123, 2 = Quad13, 3 = Quad23, 4 = Quad3.
*/
int FastInverseDctImplZDet2::GetPattern(void* ptr)
{
	short* block = (short *)ptr;
	int i,j;
	/// Count the zeros in each quadrant.
	int quad1zeros = 0;
	int quad2zeros = 0;
	int quad3zeros = 0;

	for(i = 0; i < 32; i += 8)
		for(j = 4; j < 8; j++)
		{
			if( block[i + j] == 0)
				quad1zeros++;
			else
				goto FIDIZD2_EXIT1;
		}//end for i & j...
	FIDIZD2_EXIT1:

	for(i = 32; i < 64; i += 8)
		for(j = 0; j < 4; j++)
		{
			if( block[i + j] == 0)
				quad2zeros++;
			else
				goto FIDIZD2_EXIT2;
		}//end for i & j...
	FIDIZD2_EXIT2:

	for(i = 32; i < 64; i += 8)
		for(j = 4; j < 8; j++)
		{
			if( block[i + j] == 0)
				quad3zeros++;
			else
				goto FIDIZD2_EXIT3;
		}//end for i & j...
	FIDIZD2_EXIT3:

	/// Most likely cases 1st.
	int ret = 0;	///< All quads non-zero.
	if(quad3zeros == 16)
	{
		if( (quad1zeros == 16)&&(quad2zeros == 16) )			///< All quads except 0 at zero.
			ret = 1;
		else if( (quad1zeros == 16)&&(quad2zeros != 16) )	///< Quads 1 and 3 at zero.
			ret = 2;
		else if( (quad1zeros != 16)&&(quad2zeros == 16) )	///< Quads 2 and 3 at zero.
			ret = 3;
		else																							///< Only quad 3 at zero.
			ret = 4;
	}//end if quad3zeros...

	return(ret);
}//end GetPattern.

#ifdef FIDIZD2_UNROLL_LOOPS_AND_INTERLEAVE

/** Do the IDct assuming no zero coeffs quadrants.
No operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad0123Pattern(void* ptr)
{
	short* block = (short *)ptr;
	register int t0, t1, t2, t3, s5, s6, r4, r5, r6, r7;
	register int v0, v1, v2, v3, x5, x6, u4, u5, u6, u7;

	/// 1-D inverse dct in vert direction.

	/// 1st stage transform with scaling. The sqrt(2) scaling 
	/// factor is built in to the constants. Scaling = sqrt(2).

	/// Loop 0 & 1.
	t0 = (int)(block[0] + block[32]);
	t1 = (int)(block[0] - block[32]);
	v0 = (int)(block[1] + block[33]);
	v1 = (int)(block[1] - block[33]);
	r4 = (((int)(block[8] - block[56]) * FIDIZD2_W1) - ((int)block[8] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[8] - block[56]) * FIDIZD2_W1) + ((int)block[56] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[9] - block[57]) * FIDIZD2_W1) - ((int)block[9] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[9] - block[57]) * FIDIZD2_W1) + ((int)block[57] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[16] + block[48]) * FIDIZD2_W6) - ((int)block[48] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[16] + block[48]) * FIDIZD2_W6) + ((int)block[16] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[17] + block[49]) * FIDIZD2_W6) - ((int)block[49] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[17] + block[49]) * FIDIZD2_W6) + ((int)block[17] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[24] + block[40]) * FIDIZD2_W3) - ((int)block[24] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[24] + block[40]) * FIDIZD2_W3) - ((int)block[40] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[25] + block[41]) * FIDIZD2_W3) - ((int)block[25] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[25] + block[41]) * FIDIZD2_W3) - ((int)block[41] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[0]	= (short)(t0 + t3 + r6 + r7);
	block[1]	= (short)(v0 + v3 + u6 + u7);
	block[8]	= (short)(t1 + t2 + s6);
	block[9]	= (short)(v1 + v2 + x6);
	block[16]	= (short)(t1 - t2 + s5);
	block[17]	= (short)(v1 - v2 + x5);
	block[24]	= (short)(t0 - t3 + r4 + r5);
	block[25]	= (short)(v0 - v3 + u4 + u5);
	block[32]	= (short)(t0 - t3 - r4 - r5);
	block[33]	= (short)(v0 - v3 - u4 - u5);
	block[40]	= (short)(t1 - t2 - s5);
	block[41]	= (short)(v1 - v2 - x5);
	block[48]	= (short)(t1 + t2 - s6);
	block[49]	= (short)(v1 + v2 - x6);
	block[56] = (short)(t0 + t3 - r6 - r7);
	block[57] = (short)(v0 + v3 - u6 - u7);

	/// Loop 2 & 3.
	t0 = (int)(block[2] + block[34]);
	t1 = (int)(block[2] - block[34]);
	v0 = (int)(block[3] + block[35]);
	v1 = (int)(block[3] - block[35]);
	r4 = (((int)(block[10] - block[58]) * FIDIZD2_W1) - ((int)block[10] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[10] - block[58]) * FIDIZD2_W1) + ((int)block[58] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[11] - block[59]) * FIDIZD2_W1) - ((int)block[11] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[11] - block[59]) * FIDIZD2_W1) + ((int)block[59] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[18] + block[50]) * FIDIZD2_W6) - ((int)block[50] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[18] + block[50]) * FIDIZD2_W6) + ((int)block[18] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[19] + block[51]) * FIDIZD2_W6) - ((int)block[51] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[19] + block[51]) * FIDIZD2_W6) + ((int)block[19] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[26] + block[42]) * FIDIZD2_W3) - ((int)block[26] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[26] + block[42]) * FIDIZD2_W3) - ((int)block[42] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[27] + block[43]) * FIDIZD2_W3) - ((int)block[27] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[27] + block[43]) * FIDIZD2_W3) - ((int)block[43] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[2]	= (short)(t0 + t3 + r6 + r7);
	block[3]	= (short)(v0 + v3 + u6 + u7);
	block[10]	= (short)(t1 + t2 + s6);
	block[11]	= (short)(v1 + v2 + x6);
	block[18]	= (short)(t1 - t2 + s5);
	block[19]	= (short)(v1 - v2 + x5);
	block[26]	= (short)(t0 - t3 + r4 + r5);
	block[27]	= (short)(v0 - v3 + u4 + u5);
	block[34]	= (short)(t0 - t3 - r4 - r5);
	block[35]	= (short)(v0 - v3 - u4 - u5);
	block[42]	= (short)(t1 - t2 - s5);
	block[43]	= (short)(v1 - v2 - x5);
	block[50]	= (short)(t1 + t2 - s6);
	block[51]	= (short)(v1 + v2 - x6);
	block[58] = (short)(t0 + t3 - r6 - r7);
	block[59] = (short)(v0 + v3 - u6 - u7);

	/// Loop 4 & 5.
	t0 = (int)(block[4] + block[36]);
	t1 = (int)(block[4] - block[36]);
	v0 = (int)(block[5] + block[37]);
	v1 = (int)(block[5] - block[37]);
	r4 = (((int)(block[12] - block[60]) * FIDIZD2_W1) - ((int)block[12] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[12] - block[60]) * FIDIZD2_W1) + ((int)block[60] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[13] - block[61]) * FIDIZD2_W1) - ((int)block[13] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[13] - block[61]) * FIDIZD2_W1) + ((int)block[61] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[20] + block[52]) * FIDIZD2_W6) - ((int)block[52] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[20] + block[52]) * FIDIZD2_W6) + ((int)block[20] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[21] + block[53]) * FIDIZD2_W6) - ((int)block[53] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[21] + block[53]) * FIDIZD2_W6) + ((int)block[21] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[28] + block[44]) * FIDIZD2_W3) - ((int)block[28] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[28] + block[44]) * FIDIZD2_W3) - ((int)block[44] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[29] + block[45]) * FIDIZD2_W3) - ((int)block[29] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[29] + block[45]) * FIDIZD2_W3) - ((int)block[45] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[4]	= (short)(t0 + t3 + r6 + r7);
	block[5]	= (short)(v0 + v3 + u6 + u7);
	block[12]	= (short)(t1 + t2 + s6);
	block[13]	= (short)(v1 + v2 + x6);
	block[20]	= (short)(t1 - t2 + s5);
	block[21]	= (short)(v1 - v2 + x5);
	block[28]	= (short)(t0 - t3 + r4 + r5);
	block[29]	= (short)(v0 - v3 + u4 + u5);
	block[36]	= (short)(t0 - t3 - r4 - r5);
	block[37]	= (short)(v0 - v3 - u4 - u5);
	block[44]	= (short)(t1 - t2 - s5);
	block[45]	= (short)(v1 - v2 - x5);
	block[52]	= (short)(t1 + t2 - s6);
	block[53]	= (short)(v1 + v2 - x6);
	block[60] = (short)(t0 + t3 - r6 - r7);
	block[61] = (short)(v0 + v3 - u6 - u7);

	/// Loop 6 & 7.
	t0 = (int)(block[6] + block[38]);
	t1 = (int)(block[6] - block[38]);
	v0 = (int)(block[7] + block[39]);
	v1 = (int)(block[7] - block[39]);
	r4 = (((int)(block[14] - block[62]) * FIDIZD2_W1) - ((int)block[14] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[14] - block[62]) * FIDIZD2_W1) + ((int)block[62] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[15] - block[63]) * FIDIZD2_W1) - ((int)block[15] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[15] - block[63]) * FIDIZD2_W1) + ((int)block[63] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[22] + block[54]) * FIDIZD2_W6) - ((int)block[54] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[22] + block[54]) * FIDIZD2_W6) + ((int)block[22] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[23] + block[55]) * FIDIZD2_W6) - ((int)block[55] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[23] + block[55]) * FIDIZD2_W6) + ((int)block[23] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[30] + block[46]) * FIDIZD2_W3) - ((int)block[30] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[30] + block[46]) * FIDIZD2_W3) - ((int)block[46] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[31] + block[47]) * FIDIZD2_W3) - ((int)block[31] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[31] + block[47]) * FIDIZD2_W3) - ((int)block[47] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[6]	= (short)(t0 + t3 + r6 + r7);
	block[7]	= (short)(v0 + v3 + u6 + u7);
	block[14]	= (short)(t1 + t2 + s6);
	block[15]	= (short)(v1 + v2 + x6);
	block[22]	= (short)(t1 - t2 + s5);
	block[23]	= (short)(v1 - v2 + x5);
	block[30]	= (short)(t0 - t3 + r4 + r5);
	block[31]	= (short)(v0 - v3 + u4 + u5);
	block[38]	= (short)(t0 - t3 - r4 - r5);
	block[39]	= (short)(v0 - v3 - u4 - u5);
	block[46]	= (short)(t1 - t2 - s5);
	block[47]	= (short)(v1 - v2 - x5);
	block[54]	= (short)(t1 + t2 - s6);
	block[55]	= (short)(v1 + v2 - x6);
	block[62] = (short)(t0 + t3 - r6 - r7);
	block[63] = (short)(v0 + v3 - u6 - u7);

	/// 1-D inverse dct in horiz direction.

	/// Loop 0 & 8.
	t0 = (int)(block[0] + block[4]);
	t1 = (int)(block[0] - block[4]);
	v0 = (int)(block[8] + block[12]);
	v1 = (int)(block[8] - block[12]);
	r4 = (((int)(block[1]  - block[7]) * FIDIZD2_W1)  - ((int)block[1] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[1]  - block[7]) * FIDIZD2_W1)  + ((int)block[7] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[9]  - block[15]) * FIDIZD2_W1) - ((int)block[9] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[9]  - block[15]) * FIDIZD2_W1) + ((int)block[15] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[2]  + block[6]) * FIDIZD2_W6)  - ((int)block[6] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[2]  + block[6]) * FIDIZD2_W6)  + ((int)block[2] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[10] + block[14]) * FIDIZD2_W6) - ((int)block[14] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[10] + block[14]) * FIDIZD2_W6) + ((int)block[10] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[3]  + block[5]) * FIDIZD2_W3)  - ((int)block[3] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[3]  + block[5]) * FIDIZD2_W3)  - ((int)block[5] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[11] + block[13]) * FIDIZD2_W3) - ((int)block[11] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[11] + block[13]) * FIDIZD2_W3) - ((int)block[13] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[0]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[8]	= (short)((v0 + v3 + u6 + u7 + 4) >> 3);
	block[1]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[9]	= (short)((v1 + v2 + x6 + 4) >> 3);
	block[2]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[10]	= (short)((v1 - v2 + x5 + 4) >> 3);
	block[3]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[11]	= (short)((v0 - v3 + u4 + u5 + 4) >> 3);
	block[4]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[12]	= (short)((v0 - v3 - u4 - u5 + 4) >> 3);
	block[5]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[13]	= (short)((v1 - v2 - x5 + 4) >> 3);
	block[6]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[14]	= (short)((v1 + v2 - x6 + 4) >> 3);
	block[7]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[15]	= (short)((v0 + v3 - u6 - u7 + 4) >> 3);

	/// Loop 16 & 24.
	t0 = (int)(block[16] + block[20]);
	t1 = (int)(block[16] - block[20]);
	v0 = (int)(block[24] + block[28]);
	v1 = (int)(block[24] - block[28]);
	r4 = (((int)(block[17]  - block[23]) * FIDIZD2_W1) - ((int)block[17] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[17]  - block[23]) * FIDIZD2_W1) + ((int)block[23] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[25]  - block[31]) * FIDIZD2_W1) - ((int)block[25] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[25]  - block[31]) * FIDIZD2_W1) + ((int)block[31] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[18]  + block[22]) * FIDIZD2_W6) - ((int)block[22] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[18]  + block[22]) * FIDIZD2_W6) + ((int)block[18] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[26] + block[30]) * FIDIZD2_W6)  - ((int)block[30] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[26] + block[30]) * FIDIZD2_W6)  + ((int)block[26] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[19]  + block[21]) * FIDIZD2_W3) - ((int)block[19] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[19]  + block[21]) * FIDIZD2_W3) - ((int)block[21] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[27] + block[29]) * FIDIZD2_W3)  - ((int)block[27] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[27] + block[29]) * FIDIZD2_W3)  - ((int)block[29] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[16]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[24]	= (short)((v0 + v3 + u6 + u7 + 4) >> 3);
	block[17]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[25]	= (short)((v1 + v2 + x6 + 4) >> 3);
	block[18]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[26]	= (short)((v1 - v2 + x5 + 4) >> 3);
	block[19]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[27]	= (short)((v0 - v3 + u4 + u5 + 4) >> 3);
	block[20]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[28]	= (short)((v0 - v3 - u4 - u5 + 4) >> 3);
	block[21]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[29]	= (short)((v1 - v2 - x5 + 4) >> 3);
	block[22]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[30]	= (short)((v1 + v2 - x6 + 4) >> 3);
	block[23]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[31]	= (short)((v0 + v3 - u6 - u7 + 4) >> 3);

	/// Loop 32 & 40.
	t0 = (int)(block[32] + block[36]);
	t1 = (int)(block[32] - block[36]);
	v0 = (int)(block[40] + block[44]);
	v1 = (int)(block[40] - block[44]);
	r4 = (((int)(block[33]  - block[39]) * FIDIZD2_W1) - ((int)block[33] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[33]  - block[39]) * FIDIZD2_W1) + ((int)block[39] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[41]  - block[47]) * FIDIZD2_W1) - ((int)block[41] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[41]  - block[47]) * FIDIZD2_W1) + ((int)block[47] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[34]  + block[38]) * FIDIZD2_W6) - ((int)block[38] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[34]  + block[38]) * FIDIZD2_W6) + ((int)block[34] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[42] + block[46]) * FIDIZD2_W6)  - ((int)block[46] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[42] + block[46]) * FIDIZD2_W6)  + ((int)block[42] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[35]  + block[37]) * FIDIZD2_W3) - ((int)block[35] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[35]  + block[37]) * FIDIZD2_W3) - ((int)block[37] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[43] + block[45]) * FIDIZD2_W3)  - ((int)block[43] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[43] + block[45]) * FIDIZD2_W3)  - ((int)block[45] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[32]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[40]	= (short)((v0 + v3 + u6 + u7 + 4) >> 3);
	block[33]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[41]	= (short)((v1 + v2 + x6 + 4) >> 3);
	block[34]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[42]	= (short)((v1 - v2 + x5 + 4) >> 3);
	block[35]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[43]	= (short)((v0 - v3 + u4 + u5 + 4) >> 3);
	block[36]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[44]	= (short)((v0 - v3 - u4 - u5 + 4) >> 3);
	block[37]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[45]	= (short)((v1 - v2 - x5 + 4) >> 3);
	block[38]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[46]	= (short)((v1 + v2 - x6 + 4) >> 3);
	block[39]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[47]	= (short)((v0 + v3 - u6 - u7 + 4) >> 3);

	/// Loop 48 & 56.
	t0 = (int)(block[48] + block[52]);
	t1 = (int)(block[48] - block[52]);
	v0 = (int)(block[56] + block[60]);
	v1 = (int)(block[56] - block[60]);
	r4 = (((int)(block[49]  - block[55]) * FIDIZD2_W1) - ((int)block[49] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[49]  - block[55]) * FIDIZD2_W1) + ((int)block[55] * FIDIZD2_W11) + 1024) >> 11;
	u4 = (((int)(block[57]  - block[63]) * FIDIZD2_W1) - ((int)block[57] * FIDIZD2_W10) + 1024) >> 11;
	u7 = (((int)(block[57]  - block[63]) * FIDIZD2_W1) + ((int)block[63] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[50]  + block[54]) * FIDIZD2_W6) - ((int)block[54] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[50]  + block[54]) * FIDIZD2_W6) + ((int)block[50] * FIDIZD2_W14) + 1024) >> 11;
	v2 = (((int)(block[58] + block[62]) * FIDIZD2_W6)  - ((int)block[62] * FIDIZD2_W15) + 1024) >> 11;
	v3 = (((int)(block[58] + block[62]) * FIDIZD2_W6)  + ((int)block[58] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[51]  + block[53]) * FIDIZD2_W3) - ((int)block[51] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[51]  + block[53]) * FIDIZD2_W3) - ((int)block[53] * FIDIZD2_W13) + 1024) >> 11;
	u5 = (((int)(block[59] + block[61]) * FIDIZD2_W3)  - ((int)block[59] * FIDIZD2_W12) + 1024) >> 11;
	u6 = (((int)(block[59] + block[61]) * FIDIZD2_W3)  - ((int)block[61] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	x5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	x6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[48]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[56]	= (short)((v0 + v3 + u6 + u7 + 4) >> 3);
	block[49]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[57]	= (short)((v1 + v2 + x6 + 4) >> 3);
	block[50]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[58]	= (short)((v1 - v2 + x5 + 4) >> 3);
	block[51]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[59]	= (short)((v0 - v3 + u4 + u5 + 4) >> 3);
	block[52]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[60]	= (short)((v0 - v3 - u4 - u5 + 4) >> 3);
	block[53]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[61]	= (short)((v1 - v2 - x5 + 4) >> 3);
	block[54]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[62]	= (short)((v1 + v2 - x6 + 4) >> 3);
	block[55]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[63]	= (short)((v0 + v3 - u6 - u7 + 4) >> 3);
}//end Quad0123Pattern.

/** Do the IDct assuming quad 1, 2 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad123Pattern(void* ptr)
{
	short* block = (short *)ptr;
	register int t, temp1, temp2, r4, r5, r6, r7, s5, s6;
	register int x, temp1x, temp2x, u4, u5, u6, u7, v5, v6;
	
	/// 1-D inverse dct in vert direction.

	/// Loop 0 & 1.

	/// The sqrt(2) scaling factor is built in to the constants. 
	/// Scaling = sqrt(2).
	t				= (int)block[0];
	x				= (int)block[1];
	r4			= (((int)block[8] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[9] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[8] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[9] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[16] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[17] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[16] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[17] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[24] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[25] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[24]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[25]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[0]	= (short)(t + temp1 + r6 + r7);
	block[1]	= (short)(x + temp1x + u6 + u7);
	block[8]	= (short)(t + temp2 + s6);
	block[9]	= (short)(x + temp2x + v6);
	block[16]	= (short)(t - temp2 + s5);
	block[17]	= (short)(x - temp2x + v5);
	block[24]	= (short)(t - temp1 + r4 + r5);
	block[25]	= (short)(x - temp1x + u4 + u5);
	block[32]	= (short)(t - temp1 - r4 - r5);
	block[33]	= (short)(x - temp1x - u4 - u5);
	block[40]	= (short)(t - temp2 - s5);
	block[41]	= (short)(x - temp2x - v5);
	block[48]	= (short)(t + temp2 - s6);
	block[49]	= (short)(x + temp2x - v6);
	block[56] = (short)(t + temp1 - r6 - r7);
	block[57] = (short)(x + temp1x - u6 - u7);

	/// Loop 2 & 3.

	t				= (int)block[2];
	x				= (int)block[3];
	r4			= (((int)block[10] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[11] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[10] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[11] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[18] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[19] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[18] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[19] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[26] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[27] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[26]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[27]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[2]	= (short)(t + temp1 + r6 + r7);
	block[3]	= (short)(x + temp1x + u6 + u7);
	block[10]	= (short)(t + temp2 + s6);
	block[11]	= (short)(x + temp2x + v6);
	block[18]	= (short)(t - temp2 + s5);
	block[19]	= (short)(x - temp2x + v5);
	block[26]	= (short)(t - temp1 + r4 + r5);
	block[27]	= (short)(x - temp1x + u4 + u5);
	block[34]	= (short)(t - temp1 - r4 - r5);
	block[35]	= (short)(x - temp1x - u4 - u5);
	block[42]	= (short)(t - temp2 - s5);
	block[43]	= (short)(x - temp2x - v5);
	block[50]	= (short)(t + temp2 - s6);
	block[51]	= (short)(x + temp2x - v6);
	block[58] = (short)(t + temp1 - r6 - r7);
	block[59] = (short)(x + temp1x - u6 - u7);

	/// 1-D inverse dct in horiz direction.

	/// Loop 0 & 8.

	t				= (int)block[0];
	x				= (int)block[8];
	r4			= (((int)block[1] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[9] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[1] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[9] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[2] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[10] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[2] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[10] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[3] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[11] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[3]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[11]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[0]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[8]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[1]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[9]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[2]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[10]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[3]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[11]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[4]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[12]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[5]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[13]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[6]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[14]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[7]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[15]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

	/// Loop 16 & 24.

	t				= (int)block[16];
	x				= (int)block[24];
	r4			= (((int)block[17] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[25] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[17] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[25] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[18] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[26] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[18] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[26] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[19] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[27] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[19]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[27]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[16]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[24]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[17]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[25]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[18]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[26]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[19]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[27]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[20]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[28]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[21]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[29]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[22]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[30]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[23]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[31]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

	/// Loop 32 & 40.

	t				= (int)block[32];
	x				= (int)block[40];
	r4			= (((int)block[33] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[41] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[33] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[41] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[34] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[42] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[34] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[42] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[35] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[43] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[35]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[43]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[32]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[40]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[33]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[41]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[34]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[42]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[35]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[43]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[36]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[44]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[37]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[45]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[38]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[46]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[39]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[47]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

	/// Loop 48 & 56.

	t				= (int)block[48];
	x				= (int)block[56];
	r4			= (((int)block[49] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[57] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[49] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[57] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[50] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[58] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[50] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[58] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[51] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[59] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[51]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[59]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[48]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[56]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[49]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[57]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[50]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[58]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[51]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[59]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[52]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[60]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[53]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[61]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[54]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[62]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[55]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[63]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

}//end Quad123Pattern.

/** Do the IDct assuming quad 1 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad13Pattern(void* ptr)
{
	short* block = (short *)ptr;
	register int t, t0, t1, t2, t3, temp1, temp2, s5, s6, r4, r5, r6, r7;
	register int x, x0, x1, x2, x3, temp1x, temp2x, v5, v6, u4, u5, u6, u7;
	
	/// 1-D inverse dct in vert direction.

	/// Loop 0 & 1.

	/// 1st stage transform with scaling. The sqrt(2) scaling 
	/// factor is built in to the constants. Scaling = sqrt(2).
	t0 = (int)(block[0]		 + block[32]);
	x0 = (int)(block[1]		 + block[33]);
	t1 = (int)(block[0]		 - block[32]);
	x1 = (int)(block[1]		 - block[33]);
	t2 = (((int)(block[16] + block[48]) * FIDIZD2_W6) - ((int)block[48] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[17] + block[49]) * FIDIZD2_W6) - ((int)block[49] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[16] + block[48]) * FIDIZD2_W6) + ((int)block[16] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[17] + block[49]) * FIDIZD2_W6) + ((int)block[17] * FIDIZD2_W14) + 1024) >> 11;
	r4 = (((int)(block[8]  - block[56]) * FIDIZD2_W1) - ((int)block[8] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[9]  - block[57]) * FIDIZD2_W1) - ((int)block[9] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[8]  - block[56]) * FIDIZD2_W1) + ((int)block[56] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[9]  - block[57]) * FIDIZD2_W1) + ((int)block[57] * FIDIZD2_W11) + 1024) >> 11;
	r5 = (((int)(block[24] + block[40]) * FIDIZD2_W3) - ((int)block[24] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[25] + block[41]) * FIDIZD2_W3) - ((int)block[25] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[24] + block[40]) * FIDIZD2_W3) - ((int)block[40] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[25] + block[41]) * FIDIZD2_W3) - ((int)block[41] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage.
	block[0]	= (short)(t0 + t3 + r6 + r7);
	block[1]	= (short)(x0 + x3 + u6 + u7);
	block[8]	= (short)(t1 + t2 + s6);
	block[9]	= (short)(x1 + x2 + v6);
	block[16]	= (short)(t1 - t2 + s5);
	block[17]	= (short)(x1 - x2 + v5);
	block[24]	= (short)(t0 - t3 + r4 + r5);
	block[25]	= (short)(x0 - x3 + u4 + u5);
	block[32]	= (short)(t0 - t3 - r4 - r5);
	block[33]	= (short)(x0 - x3 - u4 - u5);
	block[40]	= (short)(t1 - t2 - s5);
	block[41]	= (short)(x1 - x2 - v5);
	block[48]	= (short)(t1 + t2 - s6);
	block[49]	= (short)(x1 + x2 - v6);
	block[56] = (short)(t0 + t3 - r6 - r7);
	block[57] = (short)(x0 + x3 - u6 - u7);

	/// Loop 2 & 3.

	t0 = (int)(block[2]		 + block[34]);
	x0 = (int)(block[3]		 + block[35]);
	t1 = (int)(block[2]		 - block[34]);
	x1 = (int)(block[3]		 - block[35]);
	t2 = (((int)(block[18] + block[50]) * FIDIZD2_W6) - ((int)block[50] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[19] + block[51]) * FIDIZD2_W6) - ((int)block[51] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[18] + block[50]) * FIDIZD2_W6) + ((int)block[18] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[19] + block[51]) * FIDIZD2_W6) + ((int)block[19] * FIDIZD2_W14) + 1024) >> 11;
	r4 = (((int)(block[10] - block[58]) * FIDIZD2_W1) - ((int)block[10] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[11] - block[59]) * FIDIZD2_W1) - ((int)block[11] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[10] - block[58]) * FIDIZD2_W1) + ((int)block[58] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[11] - block[59]) * FIDIZD2_W1) + ((int)block[59] * FIDIZD2_W11) + 1024) >> 11;
	r5 = (((int)(block[26] + block[42]) * FIDIZD2_W3) - ((int)block[26] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[27] + block[43]) * FIDIZD2_W3) - ((int)block[27] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[26] + block[42]) * FIDIZD2_W3) - ((int)block[42] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[27] + block[43]) * FIDIZD2_W3) - ((int)block[43] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[2]	= (short)(t0 + t3 + r6 + r7);
	block[3]	= (short)(x0 + x3 + u6 + u7);
	block[10]	= (short)(t1 + t2 + s6);
	block[11]	= (short)(x1 + x2 + v6);
	block[18]	= (short)(t1 - t2 + s5);
	block[19]	= (short)(x1 - x2 + v5);
	block[26]	= (short)(t0 - t3 + r4 + r5);
	block[27]	= (short)(x0 - x3 + u4 + u5);
	block[34]	= (short)(t0 - t3 - r4 - r5);
	block[35]	= (short)(x0 - x3 - u4 - u5);
	block[42]	= (short)(t1 - t2 - s5);
	block[43]	= (short)(x1 - x2 - v5);
	block[50]	= (short)(t1 + t2 - s6);
	block[51]	= (short)(x1 + x2 - v6);
	block[58] = (short)(t0 + t3 - r6 - r7);
	block[59] = (short)(x0 + x3 - u6 - u7);

	/// 1-D inverse dct in horiz direction.

	/// Loop 0 & 8.

	/// 1st stage transform.
	t				= (int)block[0];
	x				= (int)block[8];
	r4			= (((int)block[1] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[9] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[1] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[9] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[2] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[10] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[2] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[10] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[3] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[11] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[3]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[11]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[0]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[8]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[1]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[9]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[2]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[10]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[3]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[11]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[4]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[12]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[5]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[13]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[6]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[14]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[7]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[15]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

	/// Loop 16 & 24.

	/// 1st stage transform.
	t				= (int)block[16];
	x				= (int)block[24];
	r4			= (((int)block[17] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[25] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[17] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[25] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[18] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[26] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[18] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[26] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[19] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[27] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[19]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[27]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[16]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[24]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[17]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[25]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[18]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[26]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[19]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[27]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[20]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[28]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[21]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[29]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[22]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[30]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[23]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[31]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

	/// Loop 32 & 40.

	/// 1st stage transform.
	t				= (int)block[32];
	x				= (int)block[40];
	r4			= (((int)block[33] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[41] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[33] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[41] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[34] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[42] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[34] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[42] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[35] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[43] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[35]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[43]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[32]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[40]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[33]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[41]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[34]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[42]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[35]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[43]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[36]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[44]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[37]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[45]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[38]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[46]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[39]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[47]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

	/// Loop 48 & 56.

	/// 1st stage transform.
	t				= (int)block[48];
	x				= (int)block[56];
	r4			= (((int)block[49] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[57] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[49] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[57] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[50] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[58] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[50] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[58] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[51] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[59] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[51]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[59]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[48]	= (short)((t + temp1 + r6 + r7 + 4) >> 3);
	block[56]	= (short)((x + temp1x + u6 + u7 + 4) >> 3);
	block[49]	= (short)((t + temp2 + s6 + 4) >> 3);
	block[57]	= (short)((x + temp2x + v6 + 4) >> 3);
	block[50]	= (short)((t - temp2 + s5 + 4) >> 3);
	block[58]	= (short)((x - temp2x + v5 + 4) >> 3);
	block[51]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
	block[59]	= (short)((x - temp1x + u4 + u5 + 4) >> 3);
	block[52]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
	block[60]	= (short)((x - temp1x - u4 - u5 + 4) >> 3);
	block[53]	= (short)((t - temp2 - s5 + 4) >> 3);
	block[61]	= (short)((x - temp2x - v5 + 4) >> 3);
	block[54]	= (short)((t + temp2 - s6 + 4) >> 3);
	block[62]	= (short)((x + temp2x - v6 + 4) >> 3);
	block[55]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	block[63]	= (short)((x + temp1x - u6 - u7 + 4) >> 3);

}//end Quad13Pattern.

/** Do the IDct assuming quad 2 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad23Pattern(void* ptr)
{
	short* block = (short *)ptr;
	register int t, t0, t1, t2, t3, temp1, temp2, s5, s6, r4, r5, r6, r7;
	register int x, x0, x1, x2, x3, temp1x, temp2x, v5, v6, u4, u5, u6, u7;
	
	/// 1-D inverse dct in vert direction.

	/// Loop 0 & 1.

	/// The sqrt(2) scaling factor is built in to the constants. 
	/// Scaling = sqrt(2).
	t				= (int)block[0];
	x				= (int)block[1];
	r4			= (((int)block[8] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[9] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[8] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[9] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[16] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[17] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[16] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[17] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[24] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[25] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[24]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[25]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage.
	block[0]	= (short)(t + temp1 + r6 + r7);
	block[1]	= (short)(x + temp1x + u6 + u7);
	block[8]	= (short)(t + temp2 + s6);
	block[9]	= (short)(x + temp2x + v6);
	block[16]	= (short)(t - temp2 + s5);
	block[17]	= (short)(x - temp2x + v5);
	block[24]	= (short)(t - temp1 + r4 + r5);
	block[25]	= (short)(x - temp1x + u4 + u5);
	block[32]	= (short)(t - temp1 - r4 - r5);
	block[33]	= (short)(x - temp1x - u4 - u5);
	block[40]	= (short)(t - temp2 - s5);
	block[41]	= (short)(x - temp2x - v5);
	block[48]	= (short)(t + temp2 - s6);
	block[49]	= (short)(x + temp2x - v6);
	block[56] = (short)(t + temp1 - r6 - r7);
	block[57] = (short)(x + temp1x - u6 - u7);

	/// Loop 2 & 3.

	/// The sqrt(2) scaling factor is built in to the constants. 
	/// Scaling = sqrt(2).
	t				= (int)block[2];
	x				= (int)block[3];
	r4			= (((int)block[10] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[11] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[10] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[11] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[18] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[19] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[18] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[19] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[26] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[27] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[26]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[27]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage.
	block[2]	= (short)(t + temp1 + r6 + r7);
	block[3]	= (short)(x + temp1x + u6 + u7);
	block[10]	= (short)(t + temp2 + s6);
	block[11]	= (short)(x + temp2x + v6);
	block[18]	= (short)(t - temp2 + s5);
	block[19]	= (short)(x - temp2x + v5);
	block[26]	= (short)(t - temp1 + r4 + r5);
	block[27]	= (short)(x - temp1x + u4 + u5);
	block[34]	= (short)(t - temp1 - r4 - r5);
	block[35]	= (short)(x - temp1x - u4 - u5);
	block[42]	= (short)(t - temp2 - s5);
	block[43]	= (short)(x - temp2x - v5);
	block[50]	= (short)(t + temp2 - s6);
	block[51]	= (short)(x + temp2x - v6);
	block[58] = (short)(t + temp1 - r6 - r7);
	block[59] = (short)(x + temp1x - u6 - u7);

	/// Loop 4 & 5.

	t				= (int)block[4];
	x				= (int)block[5];
	r4			= (((int)block[12] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[13] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[12] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[13] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[20] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[21] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[20] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[21] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[28] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[29] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[28]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[29]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[4]	= (short)(t + temp1 + r6 + r7);
	block[5]	= (short)(x + temp1x + u6 + u7);
	block[12]	= (short)(t + temp2 + s6);
	block[13]	= (short)(x + temp2x + v6);
	block[20]	= (short)(t - temp2 + s5);
	block[21]	= (short)(x - temp2x + v5);
	block[28]	= (short)(t - temp1 + r4 + r5);
	block[29]	= (short)(x - temp1x + u4 + u5);
	block[36]	= (short)(t - temp1 - r4 - r5);
	block[37]	= (short)(x - temp1x - u4 - u5);
	block[44]	= (short)(t - temp2 - s5);
	block[45]	= (short)(x - temp2x - v5);
	block[52]	= (short)(t + temp2 - s6);
	block[53]	= (short)(x + temp2x - v6);
	block[60] = (short)(t + temp1 - r6 - r7);
	block[61] = (short)(x + temp1x - u6 - u7);

	/// Loop 6 & 7.

	t				= (int)block[6];
	x				= (int)block[7];
	r4			= (((int)block[14] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[15] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[14] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[15] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[22] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[23] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[22] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[23] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[30] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[31] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[30]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[31]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[6]	= (short)(t + temp1 + r6 + r7);
	block[7]	= (short)(x + temp1x + u6 + u7);
	block[14]	= (short)(t + temp2 + s6);
	block[15]	= (short)(x + temp2x + v6);
	block[22]	= (short)(t - temp2 + s5);
	block[23]	= (short)(x - temp2x + v5);
	block[30]	= (short)(t - temp1 + r4 + r5);
	block[31]	= (short)(x - temp1x + u4 + u5);
	block[38]	= (short)(t - temp1 - r4 - r5);
	block[39]	= (short)(x - temp1x - u4 - u5);
	block[46]	= (short)(t - temp2 - s5);
	block[47]	= (short)(x - temp2x - v5);
	block[54]	= (short)(t + temp2 - s6);
	block[55]	= (short)(x + temp2x - v6);
	block[62] = (short)(t + temp1 - r6 - r7);
	block[63] = (short)(x + temp1x - u6 - u7);

	/// 1-D inverse dct in horiz direction.

	/// Loop 0 & 8.

	t0 = (int)(block[0] + block[4]);
	x0 = (int)(block[8] + block[12]);
	t1 = (int)(block[0] - block[4]);
	x1 = (int)(block[8] - block[12]);
	r4 = (((int)(block[1] - block[7]) * FIDIZD2_W1) - ((int)block[1] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[9] - block[15]) * FIDIZD2_W1) - ((int)block[9] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[1] - block[7]) * FIDIZD2_W1) + ((int)block[7] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[9] - block[15]) * FIDIZD2_W1) + ((int)block[15] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[2] + block[6]) * FIDIZD2_W6) - ((int)block[6] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[10] + block[14]) * FIDIZD2_W6) - ((int)block[14] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[2] + block[6]) * FIDIZD2_W6) + ((int)block[2] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[10] + block[14]) * FIDIZD2_W6) + ((int)block[10] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[3] + block[5]) * FIDIZD2_W3) - ((int)block[3] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[11] + block[13]) * FIDIZD2_W3) - ((int)block[11] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[3] + block[5]) * FIDIZD2_W3) - ((int)block[5] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[11] + block[13]) * FIDIZD2_W3) - ((int)block[13] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[0]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[8]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[1]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[9]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[2]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[10]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[3]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[11]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[4]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[12]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[5]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[13]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[6]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[14]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[7]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[15]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);

	/// Loop 16 & 24.

	t0 = (int)(block[16] + block[20]);
	x0 = (int)(block[24] + block[28]);
	t1 = (int)(block[16] - block[20]);
	x1 = (int)(block[24] - block[28]);
	r4 = (((int)(block[17] - block[23]) * FIDIZD2_W1) - ((int)block[17] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[25] - block[31]) * FIDIZD2_W1) - ((int)block[25] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[17] - block[23]) * FIDIZD2_W1) + ((int)block[23] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[25] - block[31]) * FIDIZD2_W1) + ((int)block[31] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[18] + block[22]) * FIDIZD2_W6) - ((int)block[22] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[26] + block[30]) * FIDIZD2_W6) - ((int)block[30] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[18] + block[22]) * FIDIZD2_W6) + ((int)block[18] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[26] + block[30]) * FIDIZD2_W6) + ((int)block[26] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[19] + block[21]) * FIDIZD2_W3) - ((int)block[19] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[27] + block[29]) * FIDIZD2_W3) - ((int)block[27] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[19] + block[21]) * FIDIZD2_W3) - ((int)block[21] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[27] + block[29]) * FIDIZD2_W3) - ((int)block[29] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[16]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[24]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[17]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[25]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[18]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[26]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[19]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[27]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[20]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[28]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[21]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[29]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[22]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[30]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[23]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[31]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);

	/// Loop 32 & 40.

	t0 = (int)(block[32] + block[36]);
	x0 = (int)(block[40] + block[44]);
	t1 = (int)(block[32] - block[36]);
	x1 = (int)(block[40] - block[44]);
	r4 = (((int)(block[33] - block[39]) * FIDIZD2_W1) - ((int)block[33] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[41] - block[47]) * FIDIZD2_W1) - ((int)block[41] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[33] - block[39]) * FIDIZD2_W1) + ((int)block[39] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[41] - block[47]) * FIDIZD2_W1) + ((int)block[47] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[34] + block[38]) * FIDIZD2_W6) - ((int)block[38] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[42] + block[46]) * FIDIZD2_W6) - ((int)block[46] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[34] + block[38]) * FIDIZD2_W6) + ((int)block[34] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[42] + block[46]) * FIDIZD2_W6) + ((int)block[42] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[35] + block[37]) * FIDIZD2_W3) - ((int)block[35] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[43] + block[45]) * FIDIZD2_W3) - ((int)block[43] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[35] + block[37]) * FIDIZD2_W3) - ((int)block[37] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[43] + block[45]) * FIDIZD2_W3) - ((int)block[45] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[32]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[40]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[33]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[41]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[34]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[42]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[35]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[43]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[36]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[44]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[37]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[45]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[38]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[46]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[39]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[47]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);

	/// Loop 48 & 56.

	t0 = (int)(block[48] + block[52]);
	x0 = (int)(block[56] + block[60]);
	t1 = (int)(block[48] - block[52]);
	x1 = (int)(block[56] - block[60]);
	r4 = (((int)(block[49] - block[55]) * FIDIZD2_W1) - ((int)block[49] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[57] - block[63]) * FIDIZD2_W1) - ((int)block[57] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[49] - block[55]) * FIDIZD2_W1) + ((int)block[55] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[57] - block[63]) * FIDIZD2_W1) + ((int)block[63] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[50] + block[54]) * FIDIZD2_W6) - ((int)block[54] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[58] + block[62]) * FIDIZD2_W6) - ((int)block[62] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[50] + block[54]) * FIDIZD2_W6) + ((int)block[50] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[58] + block[62]) * FIDIZD2_W6) + ((int)block[58] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[51] + block[53]) * FIDIZD2_W3) - ((int)block[51] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[59] + block[61]) * FIDIZD2_W3) - ((int)block[59] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[51] + block[53]) * FIDIZD2_W3) - ((int)block[53] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[59] + block[61]) * FIDIZD2_W3) - ((int)block[61] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[48]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[56]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[49]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[57]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[50]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[58]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[51]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[59]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[52]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[60]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[53]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[61]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[54]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[62]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[55]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[63]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);

}//end Quad23Pattern.

/** Do the IDct assuming quad 3 has zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad3Pattern(void* ptr)
{
	short* block = (short *)ptr;
	register int t, t0, t1, t2, t3, temp1, temp2, s5, s6, r4, r5, r6, r7;
	register int x, x0, x1, x2, x3, temp1x, temp2x, v5, v6, u4, u5, u6, u7;
	
	/// 1-D inverse dct in vert direction.

	/// Loop 0 & 1.

	/// 1st stage transform with scaling. The sqrt(2) scaling 
	/// factor is built in to the constants. Scaling = sqrt(2).
	t0 = (int)(block[0]		 + block[32]);
	x0 = (int)(block[1]		 + block[33]);
	t1 = (int)(block[0]		 - block[32]);
	x1 = (int)(block[1]		 - block[33]);
	r4 = (((int)(block[8]  - block[56]) * FIDIZD2_W1) - ((int)block[8] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[9]  - block[57]) * FIDIZD2_W1) - ((int)block[9] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[8]  - block[56]) * FIDIZD2_W1) + ((int)block[56] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[9]  - block[57]) * FIDIZD2_W1) + ((int)block[57] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[16] + block[48]) * FIDIZD2_W6) - ((int)block[48] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[17] + block[49]) * FIDIZD2_W6) - ((int)block[49] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[16] + block[48]) * FIDIZD2_W6) + ((int)block[16] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[17] + block[49]) * FIDIZD2_W6) + ((int)block[17] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[24] + block[40]) * FIDIZD2_W3) - ((int)block[24] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[25] + block[41]) * FIDIZD2_W3) - ((int)block[25] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[24] + block[40]) * FIDIZD2_W3) - ((int)block[40] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[25] + block[41]) * FIDIZD2_W3) - ((int)block[41] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage.
	block[0]	= (short)(t0 + t3 + r6 + r7);
	block[1]	= (short)(x0 + x3 + u6 + u7);
	block[8]	= (short)(t1 + t2 + s6);
	block[9]	= (short)(x1 + x2 + v6);
	block[16]	= (short)(t1 - t2 + s5);
	block[17]	= (short)(x1 - x2 + v5);
	block[24]	= (short)(t0 - t3 + r4 + r5);
	block[25]	= (short)(x0 - x3 + u4 + u5);
	block[32]	= (short)(t0 - t3 - r4 - r5);
	block[33]	= (short)(x0 - x3 - u4 - u5);
	block[40]	= (short)(t1 - t2 - s5);
	block[41]	= (short)(x1 - x2 - v5);
	block[48]	= (short)(t1 + t2 - s6);
	block[49]	= (short)(x1 + x2 - v6);
	block[56] = (short)(t0 + t3 - r6 - r7);
	block[57] = (short)(x0 + x3 - u6 - u7);

	/// Loop 2 & 3.

	/// 1st stage transform with scaling. The sqrt(2) scaling 
	/// factor is built in to the constants. Scaling = sqrt(2).
	t0 = (int)(block[2]		 + block[34]);
	x0 = (int)(block[3]		 + block[35]);
	t1 = (int)(block[2]		 - block[34]);
	x1 = (int)(block[3]		 - block[35]);
	r4 = (((int)(block[10] - block[58]) * FIDIZD2_W1) - ((int)block[10] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[11] - block[59]) * FIDIZD2_W1) - ((int)block[11] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[10] - block[58]) * FIDIZD2_W1) + ((int)block[58] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[11] - block[59]) * FIDIZD2_W1) + ((int)block[59] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[18] + block[50]) * FIDIZD2_W6) - ((int)block[50] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[19] + block[51]) * FIDIZD2_W6) - ((int)block[51] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[18] + block[50]) * FIDIZD2_W6) + ((int)block[18] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[19] + block[51]) * FIDIZD2_W6) + ((int)block[19] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[26] + block[42]) * FIDIZD2_W3) - ((int)block[26] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[27] + block[43]) * FIDIZD2_W3) - ((int)block[27] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[26] + block[42]) * FIDIZD2_W3) - ((int)block[42] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[27] + block[43]) * FIDIZD2_W3) - ((int)block[43] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[2]	= (short)(t0 + t3 + r6 + r7);
	block[3]	= (short)(x0 + x3 + u6 + u7);
	block[10]	= (short)(t1 + t2 + s6);
	block[11]	= (short)(x1 + x2 + v6);
	block[18]	= (short)(t1 - t2 + s5);
	block[19]	= (short)(x1 - x2 + v5);
	block[26]	= (short)(t0 - t3 + r4 + r5);
	block[27]	= (short)(x0 - x3 + u4 + u5);
	block[34]	= (short)(t0 - t3 - r4 - r5);
	block[35]	= (short)(x0 - x3 - u4 - u5);
	block[42]	= (short)(t1 - t2 - s5);
	block[43]	= (short)(x1 - x2 - v5);
	block[50]	= (short)(t1 + t2 - s6);
	block[51]	= (short)(x1 + x2 - v6);
	block[58] = (short)(t0 + t3 - r6 - r7);
	block[59] = (short)(x0 + x3 - u6 - u7);

	/// Loop 4 & 5.

	/// The sqrt(2) scaling factor is built in to the constants. 
	/// Scaling = sqrt(2).
	t				= (int)block[4];
	x				= (int)block[5];
	r4			= (((int)block[12] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[13] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[12] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[13] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[20] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[21] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[20] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[21] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[28] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[29] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[28]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[29]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage.
	block[4]	= (short)(t + temp1 + r6 + r7);
	block[5]	= (short)(x + temp1x + u6 + u7);
	block[12]	= (short)(t + temp2 + s6);
	block[13]	= (short)(x + temp2x + v6);
	block[20]	= (short)(t - temp2 + s5);
	block[21]	= (short)(x - temp2x + v5);
	block[28]	= (short)(t - temp1 + r4 + r5);
	block[29]	= (short)(x - temp1x + u4 + u5);
	block[36]	= (short)(t - temp1 - r4 - r5);
	block[37]	= (short)(x - temp1x - u4 - u5);
	block[44]	= (short)(t - temp2 - s5);
	block[45]	= (short)(x - temp2x - v5);
	block[52]	= (short)(t + temp2 - s6);
	block[53]	= (short)(x + temp2x - v6);
	block[60] = (short)(t + temp1 - r6 - r7);
	block[61] = (short)(x + temp1x - u6 - u7);

	/// Loop 6 & 7.

	/// The sqrt(2) scaling factor is built in to the constants. 
	/// Scaling = sqrt(2).
	t				= (int)block[6];
	x				= (int)block[7];
	r4			= (((int)block[14] * FIDIZD2_W7) + 1024) >> 11;
	u4			= (((int)block[15] * FIDIZD2_W7) + 1024) >> 11;
	r7			= (((int)block[14] * FIDIZD2_W1) + 1024) >> 11;
	u7			= (((int)block[15] * FIDIZD2_W1) + 1024) >> 11;
	temp1		= (((int)block[22] * FIDIZD2_W2) + 1024) >> 11;
	temp1x	= (((int)block[23] * FIDIZD2_W2) + 1024) >> 11;
	temp2		= (((int)block[22] * FIDIZD2_W6) + 1024) >> 11;
	temp2x	= (((int)block[23] * FIDIZD2_W6) + 1024) >> 11;
	r5			= (1024 - ((int)block[30] * FIDIZD2_W5)) >> 11;
	u5			= (1024 - ((int)block[31] * FIDIZD2_W5)) >> 11;
	r6			= (((int)(block[30]) * FIDIZD2_W3) + 1024) >> 11;
	u6			= (((int)(block[31]) * FIDIZD2_W3) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage.
	block[6]	= (short)(t + temp1 + r6 + r7);
	block[7]	= (short)(x + temp1x + u6 + u7);
	block[14]	= (short)(t + temp2 + s6);
	block[15]	= (short)(x + temp2x + v6);
	block[22]	= (short)(t - temp2 + s5);
	block[23]	= (short)(x - temp2x + v5);
	block[30]	= (short)(t - temp1 + r4 + r5);
	block[31]	= (short)(x - temp1x + u4 + u5);
	block[38]	= (short)(t - temp1 - r4 - r5);
	block[39]	= (short)(x - temp1x - u4 - u5);
	block[46]	= (short)(t - temp2 - s5);
	block[47]	= (short)(x - temp2x - v5);
	block[54]	= (short)(t + temp2 - s6);
	block[55]	= (short)(x + temp2x - v6);
	block[62] = (short)(t + temp1 - r6 - r7);
	block[63] = (short)(x + temp1x - u6 - u7);

	/// 1-D inverse dct in horiz direction.

	/// Loop 0 & 8.

	t0 = (int)(block[0] + block[4]);
	x0 = (int)(block[8] + block[12]);
	t1 = (int)(block[0] - block[4]);
	x1 = (int)(block[8] - block[12]);
	r4 = (((int)(block[1] - block[7]) * FIDIZD2_W1) - ((int)block[1] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[9] - block[15]) * FIDIZD2_W1) - ((int)block[9] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[1] - block[7]) * FIDIZD2_W1) + ((int)block[7] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[9] - block[15]) * FIDIZD2_W1) + ((int)block[15] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[2] + block[6]) * FIDIZD2_W6) - ((int)block[6] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[10] + block[14]) * FIDIZD2_W6) - ((int)block[14] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[2] + block[6]) * FIDIZD2_W6) + ((int)block[2] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[10] + block[14]) * FIDIZD2_W6) + ((int)block[10] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[3] + block[5]) * FIDIZD2_W3) - ((int)block[3] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[11] + block[13]) * FIDIZD2_W3) - ((int)block[11] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[3] + block[5]) * FIDIZD2_W3) - ((int)block[5] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[11] + block[13]) * FIDIZD2_W3) - ((int)block[13] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[0]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[8]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[1]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[9]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[2]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[10]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[3]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[11]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[4]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[12]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[5]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[13]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[6]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[14]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[7]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[15]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);

	/// Loop 16 & 24.

	t0 = (int)(block[16] + block[20]);
	x0 = (int)(block[24] + block[28]);
	t1 = (int)(block[16] - block[20]);
	x1 = (int)(block[24] - block[28]);
	r4 = (((int)(block[17] - block[23]) * FIDIZD2_W1) - ((int)block[17] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[25] - block[31]) * FIDIZD2_W1) - ((int)block[25] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[17] - block[23]) * FIDIZD2_W1) + ((int)block[23] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[25] - block[31]) * FIDIZD2_W1) + ((int)block[31] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[18] + block[22]) * FIDIZD2_W6) - ((int)block[22] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[26] + block[30]) * FIDIZD2_W6) - ((int)block[30] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[18] + block[22]) * FIDIZD2_W6) + ((int)block[18] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[26] + block[30]) * FIDIZD2_W6) + ((int)block[26] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[19] + block[21]) * FIDIZD2_W3) - ((int)block[19] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[27] + block[29]) * FIDIZD2_W3) - ((int)block[27] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[19] + block[21]) * FIDIZD2_W3) - ((int)block[21] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[27] + block[29]) * FIDIZD2_W3) - ((int)block[29] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[16]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[24]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[17]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[25]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[18]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[26]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[19]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[27]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[20]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[28]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[21]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[29]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[22]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[30]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[23]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[31]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);

	/// Loop 32 & 40.

	t0 = (int)(block[32] + block[36]);
	x0 = (int)(block[40] + block[44]);
	t1 = (int)(block[32] - block[36]);
	x1 = (int)(block[40] - block[44]);
	r4 = (((int)(block[33] - block[39]) * FIDIZD2_W1) - ((int)block[33] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[41] - block[47]) * FIDIZD2_W1) - ((int)block[41] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[33] - block[39]) * FIDIZD2_W1) + ((int)block[39] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[41] - block[47]) * FIDIZD2_W1) + ((int)block[47] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[34] + block[38]) * FIDIZD2_W6) - ((int)block[38] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[42] + block[46]) * FIDIZD2_W6) - ((int)block[46] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[34] + block[38]) * FIDIZD2_W6) + ((int)block[34] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[42] + block[46]) * FIDIZD2_W6) + ((int)block[42] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[35] + block[37]) * FIDIZD2_W3) - ((int)block[35] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[43] + block[45]) * FIDIZD2_W3) - ((int)block[43] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[35] + block[37]) * FIDIZD2_W3) - ((int)block[37] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[43] + block[45]) * FIDIZD2_W3) - ((int)block[45] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	/// Final stage with scaling (dividing) by 8 and rounding.
	block[32]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[40]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[33]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[41]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[34]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[42]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[35]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[43]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[36]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[44]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[37]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[45]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[38]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[46]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[39]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[47]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);

	/// Loop 48 & 56.

	t0 = (int)(block[48] + block[52]);
	x0 = (int)(block[56] + block[60]);
	t1 = (int)(block[48] - block[52]);
	x1 = (int)(block[56] - block[60]);
	r4 = (((int)(block[49] - block[55]) * FIDIZD2_W1) - ((int)block[49] * FIDIZD2_W10) + 1024) >> 11;
	u4 = (((int)(block[57] - block[63]) * FIDIZD2_W1) - ((int)block[57] * FIDIZD2_W10) + 1024) >> 11;
	r7 = (((int)(block[49] - block[55]) * FIDIZD2_W1) + ((int)block[55] * FIDIZD2_W11) + 1024) >> 11;
	u7 = (((int)(block[57] - block[63]) * FIDIZD2_W1) + ((int)block[63] * FIDIZD2_W11) + 1024) >> 11;
	t2 = (((int)(block[50] + block[54]) * FIDIZD2_W6) - ((int)block[54] * FIDIZD2_W15) + 1024) >> 11;
	x2 = (((int)(block[58] + block[62]) * FIDIZD2_W6) - ((int)block[62] * FIDIZD2_W15) + 1024) >> 11;
	t3 = (((int)(block[50] + block[54]) * FIDIZD2_W6) + ((int)block[50] * FIDIZD2_W14) + 1024) >> 11;
	x3 = (((int)(block[58] + block[62]) * FIDIZD2_W6) + ((int)block[58] * FIDIZD2_W14) + 1024) >> 11;
	r5 = (((int)(block[51] + block[53]) * FIDIZD2_W3) - ((int)block[51] * FIDIZD2_W12) + 1024) >> 11;
	u5 = (((int)(block[59] + block[61]) * FIDIZD2_W3) - ((int)block[59] * FIDIZD2_W12) + 1024) >> 11;
	r6 = (((int)(block[51] + block[53]) * FIDIZD2_W3) - ((int)block[53] * FIDIZD2_W13) + 1024) >> 11;
	u6 = (((int)(block[59] + block[61]) * FIDIZD2_W3) - ((int)block[61] * FIDIZD2_W13) + 1024) >> 11;

	s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
	v5 = (((u7 - u6 - u4 + u5) * 181) + 128) >> 8;
	s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;
	v6 = (((u7 - u6 + u4 - u5) * 181) + 128) >> 8;

	block[48]	= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
	block[56]	= (short)((x0 + x3 + u6 + u7 + 4) >> 3);
	block[49]	= (short)((t1 + t2 + s6 + 4) >> 3);
	block[57]	= (short)((x1 + x2 + v6 + 4) >> 3);
	block[50]	= (short)((t1 - t2 + s5 + 4) >> 3);
	block[58]	= (short)((x1 - x2 + v5 + 4) >> 3);
	block[51]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
	block[59]	= (short)((x0 - x3 + u4 + u5 + 4) >> 3);
	block[52]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
	block[60]	= (short)((x0 - x3 - u4 - u5 + 4) >> 3);
	block[53]	= (short)((t1 - t2 - s5 + 4) >> 3);
	block[61]	= (short)((x1 - x2 - v5 + 4) >> 3);
	block[54]	= (short)((t1 + t2 - s6 + 4) >> 3);
	block[62]	= (short)((x1 + x2 - v6 + 4) >> 3);
	block[55]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	block[63]	= (short)((x0 + x3 - u6 - u7 + 4) >> 3);
}//end Quad3Pattern.

#else	///< !FIDIZD2_UNROLL_LOOPS_AND_INTERLEAVE

/** Do the IDct assuming no zero coeffs quadrants.
No operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad0123Pattern(void* ptr)
{
	short* block = (short *)ptr;
	int j;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 8; j++)
	{
		/// 1st stage transform with scaling. The sqrt(2) scaling 
		/// factor is built in to the constants. Scaling = sqrt(2).
		int t0 = (int)(block[0+j] + block[32+j]);
		int t1 = (int)(block[0+j] - block[32+j]);
		int r4 = (((int)(block[8+j] - block[56+j]) * FIDIZD2_W1) - ((int)block[8+j] * FIDIZD2_W10) + 1024) >> 11;
		int r7 = (((int)(block[8+j] - block[56+j]) * FIDIZD2_W1) + ((int)block[56+j] * FIDIZD2_W11) + 1024) >> 11;
		int t2 = (((int)(block[16+j] + block[48+j]) * FIDIZD2_W6) - ((int)block[48+j] * FIDIZD2_W15) + 1024) >> 11;
		int t3 = (((int)(block[16+j] + block[48+j]) * FIDIZD2_W6) + ((int)block[16+j] * FIDIZD2_W14) + 1024) >> 11;
		int r5 = (((int)(block[24+j] + block[40+j]) * FIDIZD2_W3) - ((int)block[24+j] * FIDIZD2_W12) + 1024) >> 11;
		int r6 = (((int)(block[24+j] + block[40+j]) * FIDIZD2_W3) - ((int)block[40+j] * FIDIZD2_W13) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		block[j]		= (short)(t0 + t3 + r6 + r7);
		block[j+8]	= (short)(t1 + t2 + s6);
		block[j+16]	= (short)(t1 - t2 + s5);
		block[j+24]	= (short)(t0 - t3 + r4 + r5);
		block[j+32]	= (short)(t0 - t3 - r4 - r5);
		block[j+40]	= (short)(t1 - t2 - s5);
		block[j+48]	= (short)(t1 + t2 - s6);
		block[j+56] = (short)(t0 + t3 - r6 - r7);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		int t0 = (int)(block[0+j] + block[4+j]);
		int t1 = (int)(block[0+j] - block[4+j]);
		int r4 = (((int)(block[1+j] - block[7+j]) * FIDIZD2_W1) - ((int)block[1+j] * FIDIZD2_W10) + 1024) >> 11;
		int r7 = (((int)(block[1+j] - block[7+j]) * FIDIZD2_W1) + ((int)block[7+j] * FIDIZD2_W11) + 1024) >> 11;
		int t2 = (((int)(block[2+j] + block[6+j]) * FIDIZD2_W6) - ((int)block[6+j] * FIDIZD2_W15) + 1024) >> 11;
		int t3 = (((int)(block[2+j] + block[6+j]) * FIDIZD2_W6) + ((int)block[2+j] * FIDIZD2_W14) + 1024) >> 11;
		int r5 = (((int)(block[3+j] + block[5+j]) * FIDIZD2_W3) - ((int)block[3+j] * FIDIZD2_W12) + 1024) >> 11;
		int r6 = (((int)(block[3+j] + block[5+j]) * FIDIZD2_W3) - ((int)block[5+j] * FIDIZD2_W13) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
		block[j+1]	= (short)((t1 + t2 + s6 + 4) >> 3);
		block[j+2]	= (short)((t1 - t2 + s5 + 4) >> 3);
		block[j+3]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
		block[j+4]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
		block[j+5]	= (short)((t1 - t2 - s5 + 4) >> 3);
		block[j+6]	= (short)((t1 + t2 - s6 + 4) >> 3);
		block[j+7]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	}//end for j...

}//end Quad0123Pattern.

/** Do the IDct assuming quad 1, 2 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad123Pattern(void* ptr)
{
	short* block = (short *)ptr;
	int j;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 4; j++)
	{
		/// The sqrt(2) scaling factor is built in to the constants. 
		/// Scaling = sqrt(2).
		int t  = (int)block[0+j];
		int r4 = (((int)block[8+j] * FIDIZD2_W7) + 1024) >> 11;
		int r7 = (((int)block[8+j] * FIDIZD2_W1) + 1024) >> 11;
		int temp1 = (((int)block[16+j] * FIDIZD2_W2) + 1024) >> 11;
		int temp2 = (((int)block[16+j] * FIDIZD2_W6) + 1024) >> 11;
		int r5 = (1024 - ((int)block[24+j] * FIDIZD2_W5)) >> 11;
		int r6 = (((int)(block[24+j]) * FIDIZD2_W3) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		block[j]		= (short)(t + temp1 + r6 + r7);
		block[j+8]	= (short)(t + temp2 + s6);
		block[j+16]	= (short)(t - temp2 + s5);
		block[j+24]	= (short)(t - temp1 + r4 + r5);
		block[j+32]	= (short)(t - temp1 - r4 - r5);
		block[j+40]	= (short)(t - temp2 - s5);
		block[j+48]	= (short)(t + temp2 - s6);
		block[j+56] = (short)(t + temp1 - r6 - r7);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		int t = (int)block[0+j];
		int r4 = (((int)block[1+j] * FIDIZD2_W7) + 1024) >> 11;
		int r7 = (((int)block[1+j] * FIDIZD2_W1) + 1024) >> 11;
		int temp1 = (((int)block[2+j] * FIDIZD2_W2) + 1024) >> 11;
		int temp2 = (((int)block[2+j] * FIDIZD2_W6) + 1024) >> 11;
		int r5 = (1024 - ((int)block[3+j] * FIDIZD2_W5)) >> 11;
		int r6 = (((int)(block[3+j]) * FIDIZD2_W3) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// Final stage with scaling (dividing) by 8 and rounding.
		block[j]		= (short)((t + temp1 + r6 + r7 + 4) >> 3);
		block[j+1]	= (short)((t + temp2 + s6 + 4) >> 3);
		block[j+2]	= (short)((t - temp2 + s5 + 4) >> 3);
		block[j+3]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
		block[j+4]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
		block[j+5]	= (short)((t - temp2 - s5 + 4) >> 3);
		block[j+6]	= (short)((t + temp2 - s6 + 4) >> 3);
		block[j+7]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	}//end for j...

}//end Quad123Pattern.

/** Do the IDct assuming quad 1 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad13Pattern(void* ptr)
{
	short* block = (short *)ptr;
	int j;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 4; j++)
	{
		/// 1st stage transform with scaling. The sqrt(2) scaling 
		/// factor is built in to the constants. Scaling = sqrt(2).
		int t0 = (int)(block[0+j] + block[32+j]);
		int t1 = (int)(block[0+j] - block[32+j]);
		int t2 = (((int)(block[16+j] + block[48+j]) * FIDIZD2_W6) - ((int)block[48+j] * FIDIZD2_W15) + 1024) >> 11;
		int t3 = (((int)(block[16+j] + block[48+j]) * FIDIZD2_W6) + ((int)block[16+j] * FIDIZD2_W14) + 1024) >> 11;
		int r4 = (((int)(block[8+j] - block[56+j]) * FIDIZD2_W1) - ((int)block[8+j] * FIDIZD2_W10) + 1024) >> 11;
		int r7 = (((int)(block[8+j] - block[56+j]) * FIDIZD2_W1) + ((int)block[56+j] * FIDIZD2_W11) + 1024) >> 11;
		int r5 = (((int)(block[24+j] + block[40+j]) * FIDIZD2_W3) - ((int)block[24+j] * FIDIZD2_W12) + 1024) >> 11;
		int r6 = (((int)(block[24+j] + block[40+j]) * FIDIZD2_W3) - ((int)block[40+j] * FIDIZD2_W13) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// Final stage.
		block[j]		= (short)(t0 + t3 + r6 + r7);
		block[j+8]	= (short)(t1 + t2 + s6);
		block[j+16]	= (short)(t1 - t2 + s5);
		block[j+24]	= (short)(t0 - t3 + r4 + r5);
		block[j+32]	= (short)(t0 - t3 - r4 - r5);
		block[j+40]	= (short)(t1 - t2 - s5);
		block[j+48]	= (short)(t1 + t2 - s6);
		block[j+56] = (short)(t0 + t3 - r6 - r7);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// 1st stage transform.
		int t			= (int)block[0+j];
		int r4		= (((int)block[1+j] * FIDIZD2_W7) + 1024) >> 11;
		int r7		= (((int)block[1+j] * FIDIZD2_W1) + 1024) >> 11;
		int temp1 = (((int)block[2+j] * FIDIZD2_W2) + 1024) >> 11;
		int temp2 = (((int)block[2+j] * FIDIZD2_W6) + 1024) >> 11;
		int r5		= (1024 - ((int)block[3+j] * FIDIZD2_W5)) >> 11;
		int r6		= (((int)(block[3+j]) * FIDIZD2_W3) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// Final stage with scaling (dividing) by 8 and rounding.
		block[j]		= (short)((t + temp1 + r6 + r7 + 4) >> 3);
		block[j+1]	= (short)((t + temp2 + s6 + 4) >> 3);
		block[j+2]	= (short)((t - temp2 + s5 + 4) >> 3);
		block[j+3]	= (short)((t - temp1 + r4 + r5 + 4) >> 3);
		block[j+4]	= (short)((t - temp1 - r4 - r5 + 4) >> 3);
		block[j+5]	= (short)((t - temp2 - s5 + 4) >> 3);
		block[j+6]	= (short)((t + temp2 - s6 + 4) >> 3);
		block[j+7]	= (short)((t + temp1 - r6 - r7 + 4) >> 3);
	}//end for j...

}//end Quad13Pattern.

/** Do the IDct assuming quad 2 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad23Pattern(void* ptr)
{
	short* block = (short *)ptr;
	int j;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 8; j++)
	{
		/// The sqrt(2) scaling factor is built in to the constants. 
		/// Scaling = sqrt(2).
		int t			= (int)block[0+j];
		int r4		= (((int)block[8+j] * FIDIZD2_W7) + 1024) >> 11;
		int r7		= (((int)block[8+j] * FIDIZD2_W1) + 1024) >> 11;
		int temp1 = (((int)block[16+j] * FIDIZD2_W2) + 1024) >> 11;
		int temp2 = (((int)block[16+j] * FIDIZD2_W6) + 1024) >> 11;
		int r5		= (1024 - ((int)block[24+j] * FIDIZD2_W5)) >> 11;
		int r6		= (((int)(block[24+j]) * FIDIZD2_W3) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// 4th stage.
		block[j]		= (short)(t + temp1 + r6 + r7);
		block[j+8]	= (short)(t + temp2 + s6);
		block[j+16]	= (short)(t - temp2 + s5);
		block[j+24]	= (short)(t - temp1 + r4 + r5);
		block[j+32]	= (short)(t - temp1 - r4 - r5);
		block[j+40]	= (short)(t - temp2 - s5);
		block[j+48]	= (short)(t + temp2 - s6);
		block[j+56] = (short)(t + temp1 - r6 - r7);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		int t0 = (int)(block[0+j] + block[4+j]);
		int t1 = (int)(block[0+j] - block[4+j]);
		int r4 = (((int)(block[1+j] - block[7+j]) * FIDIZD2_W1) - ((int)block[1+j] * FIDIZD2_W10) + 1024) >> 11;
		int r7 = (((int)(block[1+j] - block[7+j]) * FIDIZD2_W1) + ((int)block[7+j] * FIDIZD2_W11) + 1024) >> 11;
		int t2 = (((int)(block[2+j] + block[6+j]) * FIDIZD2_W6) - ((int)block[6+j] * FIDIZD2_W15) + 1024) >> 11;
		int t3 = (((int)(block[2+j] + block[6+j]) * FIDIZD2_W6) + ((int)block[2+j] * FIDIZD2_W14) + 1024) >> 11;
		int r5 = (((int)(block[3+j] + block[5+j]) * FIDIZD2_W3) - ((int)block[3+j] * FIDIZD2_W12) + 1024) >> 11;
		int r6 = (((int)(block[3+j] + block[5+j]) * FIDIZD2_W3) - ((int)block[5+j] * FIDIZD2_W13) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
		block[j+1]	= (short)((t1 + t2 + s6 + 4) >> 3);
		block[j+2]	= (short)((t1 - t2 + s5 + 4) >> 3);
		block[j+3]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
		block[j+4]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
		block[j+5]	= (short)((t1 - t2 - s5 + 4) >> 3);
		block[j+6]	= (short)((t1 + t2 - s6 + 4) >> 3);
		block[j+7]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	}//end for j...

}//end Quad23Pattern.

/** Do the IDct assuming quad 3 has zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet2::Quad3Pattern(void* ptr)
{
	short* block = (short *)ptr;
	int j;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 4; j++)
	{
		/// 1st stage transform with scaling. The sqrt(2) scaling 
		/// factor is built in to the constants. Scaling = sqrt(2).
		int t0 = (int)(block[0+j]		 + block[32+j]);
		int t1 = (int)(block[0+j]		 - block[32+j]);
		int r4 = (((int)(block[8+j]  - block[56+j]) * FIDIZD2_W1) - ((int)block[8+j] * FIDIZD2_W10) + 1024) >> 11;
		int r7 = (((int)(block[8+j]  - block[56+j]) * FIDIZD2_W1) + ((int)block[56+j] * FIDIZD2_W11) + 1024) >> 11;
		int t2 = (((int)(block[16+j] + block[48+j]) * FIDIZD2_W6) - ((int)block[48+j] * FIDIZD2_W15) + 1024) >> 11;
		int t3 = (((int)(block[16+j] + block[48+j]) * FIDIZD2_W6) + ((int)block[16+j] * FIDIZD2_W14) + 1024) >> 11;
		int r5 = (((int)(block[24+j] + block[40+j]) * FIDIZD2_W3) - ((int)block[24+j] * FIDIZD2_W12) + 1024) >> 11;
		int r6 = (((int)(block[24+j] + block[40+j]) * FIDIZD2_W3) - ((int)block[40+j] * FIDIZD2_W13) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// Final stage.
		block[j]		= (short)(t0 + t3 + r6 + r7);
		block[j+8]	= (short)(t1 + t2 + s6);
		block[j+16]	= (short)(t1 - t2 + s5);
		block[j+24]	= (short)(t0 - t3 + r4 + r5);
		block[j+32]	= (short)(t0 - t3 - r4 - r5);
		block[j+40]	= (short)(t1 - t2 - s5);
		block[j+48]	= (short)(t1 + t2 - s6);
		block[j+56] = (short)(t0 + t3 - r6 - r7);
	}//end for j...
	for(; j < 8; j++)
	{
		/// The sqrt(2) scaling factor is built in to the constants. 
		/// Scaling = sqrt(2).
		int t			= (int)block[0+j];
		int r4		= (((int)block[8+j] * FIDIZD2_W7) + 1024) >> 11;
		int r7		= (((int)block[8+j] * FIDIZD2_W1) + 1024) >> 11;
		int temp1 = (((int)block[16+j] * FIDIZD2_W2) + 1024) >> 11;
		int temp2 = (((int)block[16+j] * FIDIZD2_W6) + 1024) >> 11;
		int r5		= (1024 - ((int)block[24+j] * FIDIZD2_W5)) >> 11;
		int r6		= (((int)(block[24+j]) * FIDIZD2_W3) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// Final stage.
		block[j]		= (short)(t + temp1 + r6 + r7);
		block[j+8]	= (short)(t + temp2 + s6);
		block[j+16]	= (short)(t - temp2 + s5);
		block[j+24]	= (short)(t - temp1 + r4 + r5);
		block[j+32]	= (short)(t - temp1 - r4 - r5);
		block[j+40]	= (short)(t - temp2 - s5);
		block[j+48]	= (short)(t + temp2 - s6);
		block[j+56] = (short)(t + temp1 - r6 - r7);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// 1st stage transform.
		int t0 = (int)(block[0+j] + block[4+j]);
		int t1 = (int)(block[0+j] - block[4+j]);
		int r4 = (((int)(block[1+j] - block[7+j]) * FIDIZD2_W1) - ((int)block[1+j] * FIDIZD2_W10) + 1024) >> 11;
		int r7 = (((int)(block[1+j] - block[7+j]) * FIDIZD2_W1) + ((int)block[7+j] * FIDIZD2_W11) + 1024) >> 11;
		int t2 = (((int)(block[2+j] + block[6+j]) * FIDIZD2_W6) - ((int)block[6+j] * FIDIZD2_W15) + 1024) >> 11;
		int t3 = (((int)(block[2+j] + block[6+j]) * FIDIZD2_W6) + ((int)block[2+j] * FIDIZD2_W14) + 1024) >> 11;
		int r5 = (((int)(block[3+j] + block[5+j]) * FIDIZD2_W3) - ((int)block[3+j] * FIDIZD2_W12) + 1024) >> 11;
		int r6 = (((int)(block[3+j] + block[5+j]) * FIDIZD2_W3) - ((int)block[5+j] * FIDIZD2_W13) + 1024) >> 11;

		int s5 = (((r7 - r6 - r4 + r5) * 181) + 128) >> 8;
		int s6 = (((r7 - r6 + r4 - r5) * 181) + 128) >> 8;

		/// Final stage with scaling (dividing) by 8 and rounding.
		block[j]		= (short)((t0 + t3 + r6 + r7 + 4) >> 3);
		block[j+1]	= (short)((t1 + t2 + s6 + 4) >> 3);
		block[j+2]	= (short)((t1 - t2 + s5 + 4) >> 3);
		block[j+3]	= (short)((t0 - t3 + r4 + r5 + 4) >> 3);
		block[j+4]	= (short)((t0 - t3 - r4 - r5 + 4) >> 3);
		block[j+5]	= (short)((t1 - t2 - s5 + 4) >> 3);
		block[j+6]	= (short)((t1 + t2 - s6 + 4) >> 3);
		block[j+7]	= (short)((t0 + t3 - r6 - r7 + 4) >> 3);
	}//end for j...

}//end Quad3Pattern.

#endif	///< end FIDIZD2_UNROLL_LOOPS_AND_INTERLEAVE

/*
--------------------------------------------------------------------------------
Extra code not used.
--------------------------------------------------------------------------------
*/
