/** @file

MODULE						: FastForwardDctImpl1

TAG								: FFDI1

FILE NAME					: FastForwardDctImpl1.cpp

DESCRIPTION				: A class to implement a fast forward 8x8 2-D dct on the 
										input. It implements the IForwardDct interface. The 
										scaling is designed for use in H.263 codecs.

REVISION HISTORY	:
									: 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <string.h>
#include "FastForwardDctImpl1.h"

typedef short dctType;

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define FFDI1_W1	2841				// sqrt(2).cos(pi/16) << 11
#define FFDI1_W2	2676				// sqrt(2).cos(2.pi/16) << 11
#define FFDI1_W3	2408				// sqrt(2).cos(3.pi/16) << 11
#define FFDI1_W5	1609				// sqrt(2).cos(5.pi/16) << 11
#define FFDI1_W6	1108				// sqrt(2).cos(6.pi/16) << 11
#define FFDI1_W7	 565				// sqrt(2).cos(7.pi/16) << 11
#define FFDI1_W10 2276				// W1 - W7
#define FFDI1_W11 3406				// W1 + W7
#define FFDI1_W12 4017				// W3 + W5
#define FFDI1_W13  799				// W3 - W5
#define FFDI1_W14 1568				// W2 - W6
#define FFDI1_W15 3784				// W2 + W6

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
void FastForwardDctImpl1::dct(void* ptr)
{
	dctType* block = (dctType *)ptr;
	int j, temp;
	
	// 1-D forward horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		// 1st stage transform.
		int s0 = (int)(block[j]	  + block[j+7]);
		int s1 = (int)(block[j+1] + block[j+6]);
		int s2 = (int)(block[j+2] + block[j+5]);
		int s3 = (int)(block[j+3] + block[j+4]);
		int s4 = (int)(block[j+3] - block[j+4]);
		int s5 = (int)(block[j+2] - block[j+5]);
		int s6 = (int)(block[j+1] - block[j+6]);
		int s7 = (int)(block[j]		- block[j+7]);

		// 2nd stage transform.
		int t0 = s0 + s3;
		int t1 = s1 + s2;
		int t2 = s1 - s2;
		int t3 = s0 - s3;
		int t5 = (((s6 - s5) * 181) + 128) >> 8;
		int t6 = (((s6 + s5) * 181) + 128) >> 8;

		// 3rd stage transform.
		int r4 = s4 + t5;
		int r5 = s4 - t5;
		int r6 = s7 - t6;
		int r7 = s7 + t6;

		// 4th stage transform.
		block[0+j] = (dctType)(t0 + t1);
		block[4+j] = (dctType)(t0 - t1);
		temp = (r4 + r7) * FFDI1_W1;
		block[1+j] = (dctType)((temp - (r4 * FFDI1_W10) + 1024) >> 11);
		block[7+j] = (dctType)(((r7 * FFDI1_W11) - temp + 1024) >> 11);
		temp = (r5 + r6) * FFDI1_W3;
		block[3+j] = (dctType)((temp - (r5 * FFDI1_W12) + 1024) >> 11);
		block[5+j] = (dctType)((temp - (r6 * FFDI1_W13) + 1024) >> 11);
		temp = (t2 + t3) * FFDI1_W6;
		block[2+j] = (dctType)((temp + (t3 * FFDI1_W14) + 1024) >> 11);
		block[6+j] = (dctType)((temp - (t2 * FFDI1_W15) + 1024) >> 11);

	}//end for j...

	// 1-D forward vert direction.
	for(j = 0; j < 8; j++)
	{
		// 1st stage.
		int s0 = (int)(block[j]			+ block[j+56]);
		int s1 = (int)(block[j+8]		+ block[j+48]);
		int s2 = (int)(block[j+16]	+ block[j+40]);
		int s3 = (int)(block[j+24]	+ block[j+32]);
		int s4 = (int)(block[j+24]	- block[j+32]);
		int s5 = (int)(block[j+16]	- block[j+40]);
		int s6 = (int)(block[j+8]		- block[j+48]);
		int s7 = (int)(block[j]			- block[j+56]);

		// 2nd stage.
		int t0 = s0 + s3;
		int t1 = s1 + s2;
		int t2 = s1 - s2;
		int t3 = s0 - s3;
		int t5 = (((s6 - s5) * 181) + 128) >> 8;
		int t6 = (((s6 + s5) * 181) + 128) >> 8;

		// 3rd stage transform.
		int r4 = s4 + t5;
		int r5 = s4 - t5;
		int r6 = s7 - t6;
		int r7 = s7 + t6;

		// 4th stage transform with scaling (dividing) by 8 and rounding. The
		// sqrt(2) scaling factor is built in to the constants.
		// scaling = sqrt(2)/8.
		block[0+j]	= (dctType)((t0 + t1 + 4) >> 3);
		block[32+j] = (dctType)((t0 - t1 + 4) >> 3);
		temp = (r4 + r7) * FFDI1_W1;
		block[8+j]	= (dctType)((temp - (r4 * FFDI1_W10) + 8192) >> 14);
		block[56+j] = (dctType)(((r7 * FFDI1_W11) - temp + 8192) >> 14);
		temp = (r5 + r6) * FFDI1_W3;
		block[24+j] = (dctType)((temp - (r5 * FFDI1_W12) + 8192) >> 14);
		block[40+j] = (dctType)((temp - (r6 * FFDI1_W13) + 8192) >> 14);
		temp = (t2 + t3) * FFDI1_W6;
		block[16+j] = (dctType)((temp + (t3 * FFDI1_W14) + 8192) >> 14);
		block[48+j] = (dctType)((temp - (t2 * FFDI1_W15) + 8192) >> 14);

	}//end for j...

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
void FastForwardDctImpl1::dct(void* pIn, void* pCoeff)
{
	/// Copy to output and then do in-place inverse transform.
	memcpy(pCoeff, pIn, sizeof(short) * 64);
	dct(pCoeff);
}//end dct.


