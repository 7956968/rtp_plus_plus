/** @file

MODULE						: FastInverseDctImplZDet

TAG								: FIDIZD

FILE NAME					: FastInverseDctImplZDet.cpp

DESCRIPTION				: A class to implement a fast inverse 8x8 2-D dct on the 
										input. Zero coeff detection is performed before the 
										inverse transform to reduce the number of operations. It 
										implements the IInverseDct interface. The scaling is 
										designed for use in H.263 codecs.

REVISION HISTORY	:

COPYRIGHT					: (c)CSIR, Meraka Institute 2010 all rights resevered

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

#include "FastInverseDctImplZDet.h"

typedef short dctType;

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define FIDIZD_W1	2841				// sqrt(2).cos(pi/16) << 11			or sin(7.pi/16)
#define FIDIZD_W2	2676				// sqrt(2).cos(2.pi/16) << 11		or sin(6.pi/16)
#define FIDIZD_W3	2408				// sqrt(2).cos(3.pi/16) << 11		or sin(5.pi/16)
#define FIDIZD_W5	1609				// sqrt(2).cos(5.pi/16) << 11		or sin(3.pi/16)
#define FIDIZD_W6	1108				// sqrt(2).cos(6.pi/16) << 11		or sin(2.pi/16)
#define FIDIZD_W7	 565				// sqrt(2).cos(7.pi/16) << 11		or sin(pi/16)
#define FIDIZD_W10 2276				// W1 - W7
#define FIDIZD_W11 3406				// W1 + W7
#define FIDIZD_W12 4017				// W3 + W5
#define FIDIZD_W13  799				// W3 - W5
#define FIDIZD_W14 1568				// W2 - W6
#define FIDIZD_W15 3784				// W2 + W6

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
void FastInverseDctImplZDet::idct(void* ptr)
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
int FastInverseDctImplZDet::GetPattern(void* ptr)
{
	dctType* block = (dctType *)ptr;
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
				goto FIDIZD_EXIT1;
		}//end for i & j...
	FIDIZD_EXIT1:

	for(i = 32; i < 64; i += 8)
		for(j = 0; j < 4; j++)
		{
			if( block[i + j] == 0)
				quad2zeros++;
			else
				goto FIDIZD_EXIT2;
		}//end for i & j...
	FIDIZD_EXIT2:

	for(i = 32; i < 64; i += 8)
		for(j = 4; j < 8; j++)
		{
			if( block[i + j] == 0)
				quad3zeros++;
			else
				goto FIDIZD_EXIT3;
		}//end for i & j...
	FIDIZD_EXIT3:

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

/** Do the IDct assuming no zero coeffs quadrants.
No operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet::Quad0123Pattern(void* ptr)
{
	dctType* block = (dctType *)ptr;
	int j, temp;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 8; j++)
	{
		/// 1st stage transform with scaling. The sqrt(2) scaling 
		/// factor is built in to the constants. Scaling = sqrt(2).
		int t0 = (int)(block[0+j] + block[32+j]);
		int t1 = (int)(block[0+j] - block[32+j]);
		temp = (int)(block[16+j] + block[48+j]) * FIDIZD_W6;
		int t2 = (temp - ((int)block[48+j] * FIDIZD_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[16+j] * FIDIZD_W14) + 1024) >> 11;
		temp = (int)(block[8+j] - block[56+j]) * FIDIZD_W1;
		int r4 = (temp - ((int)block[8+j] * FIDIZD_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[56+j] * FIDIZD_W11) + 1024) >> 11;
		temp = (int)(block[24+j] + block[40+j]) * FIDIZD_W3;
		int r5 = (temp - ((int)block[24+j] * FIDIZD_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[40+j] * FIDIZD_W13) + 1024) >> 11;

		/// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int t5 = r4 - r5;	///< 2nd stage.
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage.
		block[j]		= (dctType)(s0 + s7);
		block[j+56] = (dctType)(s0 - s7);
		block[j+8]	= (dctType)(s1 + s6);
		block[j+48]	= (dctType)(s1 - s6);
		block[j+16]	= (dctType)(s2 + s5);
		block[j+40]	= (dctType)(s2 - s5);
		block[j+24]	= (dctType)(s3 + s4);
		block[j+32]	= (dctType)(s3 - s4);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// 1st stage transform.
		int t0 = (int)(block[0+j] + block[4+j]);
		int t1 = (int)(block[0+j] - block[4+j]);
		temp = (int)(block[2+j] + block[6+j]) * FIDIZD_W6;
		int t2 = (temp - ((int)block[6+j] * FIDIZD_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[2+j] * FIDIZD_W14) + 1024) >> 11;
		temp = (int)(block[1+j] - block[7+j]) * FIDIZD_W1;
		int r4 = (temp - ((int)block[1+j] * FIDIZD_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[7+j] * FIDIZD_W11) + 1024) >> 11;
		temp = (int)(block[3+j] + block[5+j]) * FIDIZD_W3;
		int r5 = (temp - ((int)block[3+j] * FIDIZD_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[5+j] * FIDIZD_W13) + 1024) >> 11;

		/// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int t5 = r4 - r5;	///< 2nd stage.
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (dctType)((s0 + s7 + 4) >> 3);
		block[j+7]	= (dctType)((s0 - s7 + 4) >> 3);
		block[j+1]	= (dctType)((s1 + s6 + 4) >> 3);
		block[j+6]	= (dctType)((s1 - s6 + 4) >> 3);
		block[j+2]	= (dctType)((s2 + s5 + 4) >> 3);
		block[j+5]	= (dctType)((s2 - s5 + 4) >> 3);
		block[j+3]	= (dctType)((s3 + s4 + 4) >> 3);
		block[j+4]	= (dctType)((s3 - s4 + 4) >> 3);
	}//end for j...

}//end Quad0123Pattern.

/** Do the IDct assuming quad 1, 2 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet::Quad123Pattern(void* ptr)
{
	dctType* block = (dctType *)ptr;
	int j;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 4; j++)
	{
		/// The sqrt(2) scaling factor is built in to the constants. 
		/// Scaling = sqrt(2).
		int r4 = (((int)block[8+j] * FIDIZD_W7) + 1024) >> 11;
		int r7 = (((int)block[8+j] * FIDIZD_W1) + 1024) >> 11;

		int r5 = (1024 - ((int)block[24+j] * FIDIZD_W5)) >> 11;
		int r6 = (((int)(block[24+j]) * FIDIZD_W3) + 1024) >> 11;

		/// 3rd stage.
		int temp1 = (((int)block[16+j] * FIDIZD_W2) + 1024) >> 11;
		int temp2 = (((int)block[16+j] * FIDIZD_W6) + 1024) >> 11;
		int s0 = (int)block[0+j] + temp1; ///< t0 + t3;
		int s3 = (int)block[0+j] - temp1; ///< t0 - t3;
		int s1 = (int)block[0+j] + temp2; ///< t1 + t2;
		int s2 = (int)block[0+j] - temp2; ///< t1 - t2;
		int s4 = r4 + r5;
		int t5 = r4 - r5;
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage.
		block[j]		= (dctType)(s0 + s7);
		block[j+56] = (dctType)(s0 - s7);
		block[j+8]	= (dctType)(s1 + s6);
		block[j+48]	= (dctType)(s1 - s6);
		block[j+16]	= (dctType)(s2 + s5);
		block[j+40]	= (dctType)(s2 - s5);
		block[j+24]	= (dctType)(s3 + s4);
		block[j+32]	= (dctType)(s3 - s4);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// 1st stage transform.
		int r4 = (((int)block[1+j] * FIDIZD_W7) + 1024) >> 11;
		int r7 = (((int)block[1+j] * FIDIZD_W1) + 1024) >> 11;

		int r5 = (1024 - ((int)block[3+j] * FIDIZD_W5)) >> 11;
		int r6 = (((int)(block[3+j]) * FIDIZD_W3) + 1024) >> 11;

		/// 3rd stage.
		int temp1 = (((int)block[2+j] * FIDIZD_W2) + 1024) >> 11;
		int temp2 = (((int)block[2+j] * FIDIZD_W6) + 1024) >> 11;
		int s0 = (int)block[0+j] + temp1; ///< t0 + t3;
		int s3 = (int)block[0+j] - temp1; ///< t0 - t3;
		int s1 = (int)block[0+j] + temp2; ///< t1 + t2;
		int s2 = (int)block[0+j] - temp2; ///< t1 - t2;
		int s4 = r4 + r5;
		int t5 = r4 - r5;
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (dctType)((s0 + s7 + 4) >> 3);
		block[j+7]	= (dctType)((s0 - s7 + 4) >> 3);
		block[j+1]	= (dctType)((s1 + s6 + 4) >> 3);
		block[j+6]	= (dctType)((s1 - s6 + 4) >> 3);
		block[j+2]	= (dctType)((s2 + s5 + 4) >> 3);
		block[j+5]	= (dctType)((s2 - s5 + 4) >> 3);
		block[j+3]	= (dctType)((s3 + s4 + 4) >> 3);
		block[j+4]	= (dctType)((s3 - s4 + 4) >> 3);
	}//end for j...

}//end Quad123Pattern.

/** Do the IDct assuming quad 1 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet::Quad13Pattern(void* ptr)
{
	dctType* block = (dctType *)ptr;
	int j, temp;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 4; j++)
	{
		/// 1st stage transform with scaling. The sqrt(2) scaling 
		/// factor is built in to the constants. Scaling = sqrt(2).
		int t0 = (int)(block[0+j] + block[32+j]);
		int t1 = (int)(block[0+j] - block[32+j]);
		temp = (int)(block[16+j] + block[48+j]) * FIDIZD_W6;
		int t2 = (temp - ((int)block[48+j] * FIDIZD_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[16+j] * FIDIZD_W14) + 1024) >> 11;
		temp = (int)(block[8+j] - block[56+j]) * FIDIZD_W1;
		int r4 = (temp - ((int)block[8+j] * FIDIZD_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[56+j] * FIDIZD_W11) + 1024) >> 11;
		temp = (int)(block[24+j] + block[40+j]) * FIDIZD_W3;
		int r5 = (temp - ((int)block[24+j] * FIDIZD_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[40+j] * FIDIZD_W13) + 1024) >> 11;

		/// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int t5 = r4 - r5;	///< 2nd stage.
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage.
		block[j]		= (dctType)(s0 + s7);
		block[j+56] = (dctType)(s0 - s7);
		block[j+8]	= (dctType)(s1 + s6);
		block[j+48]	= (dctType)(s1 - s6);
		block[j+16]	= (dctType)(s2 + s5);
		block[j+40]	= (dctType)(s2 - s5);
		block[j+24]	= (dctType)(s3 + s4);
		block[j+32]	= (dctType)(s3 - s4);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// 1st stage transform.
		int r4 = (((int)block[1+j] * FIDIZD_W7) + 1024) >> 11;
		int r7 = (((int)block[1+j] * FIDIZD_W1) + 1024) >> 11;

		int r5 = (1024 - ((int)block[3+j] * FIDIZD_W5)) >> 11;
		int r6 = (((int)(block[3+j]) * FIDIZD_W3) + 1024) >> 11;

		/// 3rd stage.
		int temp1 = (((int)block[2+j] * FIDIZD_W2) + 1024) >> 11;
		int temp2 = (((int)block[2+j] * FIDIZD_W6) + 1024) >> 11;
		int s0 = (int)block[0+j] + temp1; ///< t0 + t3;
		int s3 = (int)block[0+j] - temp1; ///< t0 - t3;
		int s1 = (int)block[0+j] + temp2; ///< t1 + t2;
		int s2 = (int)block[0+j] - temp2; ///< t1 - t2;
		int s4 = r4 + r5;
		int t5 = r4 - r5;
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (dctType)((s0 + s7 + 4) >> 3);
		block[j+7]	= (dctType)((s0 - s7 + 4) >> 3);
		block[j+1]	= (dctType)((s1 + s6 + 4) >> 3);
		block[j+6]	= (dctType)((s1 - s6 + 4) >> 3);
		block[j+2]	= (dctType)((s2 + s5 + 4) >> 3);
		block[j+5]	= (dctType)((s2 - s5 + 4) >> 3);
		block[j+3]	= (dctType)((s3 + s4 + 4) >> 3);
		block[j+4]	= (dctType)((s3 - s4 + 4) >> 3);
	}//end for j...

}//end Quad13Pattern.

/** Do the IDct assuming quad 2 and 3 are zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet::Quad23Pattern(void* ptr)
{
	dctType* block = (dctType *)ptr;
	int j, temp;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 8; j++)
	{
		/// The sqrt(2) scaling factor is built in to the constants. 
		/// Scaling = sqrt(2).
		int r4 = (((int)block[8+j] * FIDIZD_W7) + 1024) >> 11;
		int r7 = (((int)block[8+j] * FIDIZD_W1) + 1024) >> 11;

		int r5 = (1024 - ((int)block[24+j] * FIDIZD_W5)) >> 11;
		int r6 = (((int)(block[24+j]) * FIDIZD_W3) + 1024) >> 11;

		/// 3rd stage.
		int temp1 = (((int)block[16+j] * FIDIZD_W2) + 1024) >> 11;
		int temp2 = (((int)block[16+j] * FIDIZD_W6) + 1024) >> 11;
		int s0 = (int)block[0+j] + temp1; ///< t0 + t3;
		int s3 = (int)block[0+j] - temp1; ///< t0 - t3;
		int s1 = (int)block[0+j] + temp2; ///< t1 + t2;
		int s2 = (int)block[0+j] - temp2; ///< t1 - t2;
		int s4 = r4 + r5;
		int t5 = r4 - r5;
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage.
		block[j]		= (dctType)(s0 + s7);
		block[j+56] = (dctType)(s0 - s7);
		block[j+8]	= (dctType)(s1 + s6);
		block[j+48]	= (dctType)(s1 - s6);
		block[j+16]	= (dctType)(s2 + s5);
		block[j+40]	= (dctType)(s2 - s5);
		block[j+24]	= (dctType)(s3 + s4);
		block[j+32]	= (dctType)(s3 - s4);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// 1st stage transform.
		int t0 = (int)(block[0+j] + block[4+j]);
		int t1 = (int)(block[0+j] - block[4+j]);
		temp = (int)(block[2+j] + block[6+j]) * FIDIZD_W6;
		int t2 = (temp - ((int)block[6+j] * FIDIZD_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[2+j] * FIDIZD_W14) + 1024) >> 11;
		temp = (int)(block[1+j] - block[7+j]) * FIDIZD_W1;
		int r4 = (temp - ((int)block[1+j] * FIDIZD_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[7+j] * FIDIZD_W11) + 1024) >> 11;
		temp = (int)(block[3+j] + block[5+j]) * FIDIZD_W3;
		int r5 = (temp - ((int)block[3+j] * FIDIZD_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[5+j] * FIDIZD_W13) + 1024) >> 11;

		/// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int t5 = r4 - r5;	///< 2nd stage.
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (dctType)((s0 + s7 + 4) >> 3);
		block[j+7]	= (dctType)((s0 - s7 + 4) >> 3);
		block[j+1]	= (dctType)((s1 + s6 + 4) >> 3);
		block[j+6]	= (dctType)((s1 - s6 + 4) >> 3);
		block[j+2]	= (dctType)((s2 + s5 + 4) >> 3);
		block[j+5]	= (dctType)((s2 - s5 + 4) >> 3);
		block[j+3]	= (dctType)((s3 + s4 + 4) >> 3);
		block[j+4]	= (dctType)((s3 - s4 + 4) >> 3);
	}//end for j...

}//end Quad23Pattern.

/** Do the IDct assuming quad 3 has zero coeffs.
Operations are elliminated from the transform assuming this pattern. 
@param ptr	: Coeffs.
@return			: none.
*/
void FastInverseDctImplZDet::Quad3Pattern(void* ptr)
{
	dctType* block = (dctType *)ptr;
	int j, temp;
	
	/// 1-D inverse dct in vert direction.
	for(j = 0; j < 4; j++)
	{
		/// 1st stage transform with scaling. The sqrt(2) scaling 
		/// factor is built in to the constants. Scaling = sqrt(2).
		int t0 = (int)(block[0+j] + block[32+j]);
		int t1 = (int)(block[0+j] - block[32+j]);
		temp = (int)(block[16+j] + block[48+j]) * FIDIZD_W6;
		int t2 = (temp - ((int)block[48+j] * FIDIZD_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[16+j] * FIDIZD_W14) + 1024) >> 11;
		temp = (int)(block[8+j] - block[56+j]) * FIDIZD_W1;
		int r4 = (temp - ((int)block[8+j] * FIDIZD_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[56+j] * FIDIZD_W11) + 1024) >> 11;
		temp = (int)(block[24+j] + block[40+j]) * FIDIZD_W3;
		int r5 = (temp - ((int)block[24+j] * FIDIZD_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[40+j] * FIDIZD_W13) + 1024) >> 11;

		/// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int t5 = r4 - r5;	///< 2nd stage.
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage.
		block[j]		= (dctType)(s0 + s7);
		block[j+56] = (dctType)(s0 - s7);
		block[j+8]	= (dctType)(s1 + s6);
		block[j+48]	= (dctType)(s1 - s6);
		block[j+16]	= (dctType)(s2 + s5);
		block[j+40]	= (dctType)(s2 - s5);
		block[j+24]	= (dctType)(s3 + s4);
		block[j+32]	= (dctType)(s3 - s4);
	}//end for j...
	for(; j < 8; j++)
	{
		/// The sqrt(2) scaling factor is built in to the constants. 
		/// Scaling = sqrt(2).
		int r4 = (((int)block[8+j] * FIDIZD_W7) + 1024) >> 11;
		int r7 = (((int)block[8+j] * FIDIZD_W1) + 1024) >> 11;

		int r5 = (1024 - ((int)block[24+j] * FIDIZD_W5)) >> 11;
		int r6 = (((int)(block[24+j]) * FIDIZD_W3) + 1024) >> 11;

		/// 3rd stage.
		int temp1 = (((int)block[16+j] * FIDIZD_W2) + 1024) >> 11;
		int temp2 = (((int)block[16+j] * FIDIZD_W6) + 1024) >> 11;
		int s0 = (int)block[0+j] + temp1; ///< t0 + t3;
		int s3 = (int)block[0+j] - temp1; ///< t0 - t3;
		int s1 = (int)block[0+j] + temp2; ///< t1 + t2;
		int s2 = (int)block[0+j] - temp2; ///< t1 - t2;
		int s4 = r4 + r5;
		int t5 = r4 - r5;
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage.
		block[j]		= (dctType)(s0 + s7);
		block[j+56] = (dctType)(s0 - s7);
		block[j+8]	= (dctType)(s1 + s6);
		block[j+48]	= (dctType)(s1 - s6);
		block[j+16]	= (dctType)(s2 + s5);
		block[j+40]	= (dctType)(s2 - s5);
		block[j+24]	= (dctType)(s3 + s4);
		block[j+32]	= (dctType)(s3 - s4);
	}//end for j...

	/// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		/// 1st stage transform.
		int t0 = (int)(block[0+j] + block[4+j]);
		int t1 = (int)(block[0+j] - block[4+j]);
		temp = (int)(block[2+j] + block[6+j]) * FIDIZD_W6;
		int t2 = (temp - ((int)block[6+j] * FIDIZD_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[2+j] * FIDIZD_W14) + 1024) >> 11;
		temp = (int)(block[1+j] - block[7+j]) * FIDIZD_W1;
		int r4 = (temp - ((int)block[1+j] * FIDIZD_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[7+j] * FIDIZD_W11) + 1024) >> 11;
		temp = (int)(block[3+j] + block[5+j]) * FIDIZD_W3;
		int r5 = (temp - ((int)block[3+j] * FIDIZD_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[5+j] * FIDIZD_W13) + 1024) >> 11;

		/// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int t5 = r4 - r5;	///< 2nd stage.
		int t6 = r7 - r6;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		/// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (dctType)((s0 + s7 + 4) >> 3);
		block[j+7]	= (dctType)((s0 - s7 + 4) >> 3);
		block[j+1]	= (dctType)((s1 + s6 + 4) >> 3);
		block[j+6]	= (dctType)((s1 - s6 + 4) >> 3);
		block[j+2]	= (dctType)((s2 + s5 + 4) >> 3);
		block[j+5]	= (dctType)((s2 - s5 + 4) >> 3);
		block[j+3]	= (dctType)((s3 + s4 + 4) >> 3);
		block[j+4]	= (dctType)((s3 - s4 + 4) >> 3);
	}//end for j...

}//end Quad3Pattern.


