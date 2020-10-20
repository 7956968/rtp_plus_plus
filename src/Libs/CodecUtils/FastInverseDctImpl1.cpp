/** @file

MODULE						: FastInverseDctImpl1

TAG								: FIDI1

FILE NAME					: FastInverseDctImpl1.cpp

DESCRIPTION				: A class to implement a fast inverse 8x8 2-D dct on the 
										input. It implements the IInverseDct interface. The 
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

#include "FastInverseDctImpl1.h"

typedef short dctType;

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define FIDI1_W1	2841				// sqrt(2).cos(pi/16) << 11			or sin(7.pi/16)
#define FIDI1_W2	2676				// sqrt(2).cos(2.pi/16) << 11		or sin(6.pi/16)
#define FIDI1_W3	2408				// sqrt(2).cos(3.pi/16) << 11		or sin(5.pi/16)
#define FIDI1_W5	1609				// sqrt(2).cos(5.pi/16) << 11		or sin(3.pi/16)
#define FIDI1_W6	1108				// sqrt(2).cos(6.pi/16) << 11		or sin(2.pi/16)
#define FIDI1_W7	 565				// sqrt(2).cos(7.pi/16) << 11		or sin(pi/16)
#define FIDI1_W10 2276				// W1 - W7
#define FIDI1_W11 3406				// W1 + W7
#define FIDI1_W12 4017				// W3 + W5
#define FIDI1_W13  799				// W3 - W5
#define FIDI1_W14 1568				// W2 - W6
#define FIDI1_W15 3784				// W2 + W6

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
void FastInverseDctImpl1::idct(void* ptr)
{
	dctType* block = (dctType *)ptr;
	int j, temp;
	
	// 1-D inverse dct in vert direction.
	for(j = 0; j < 8; j++)
	{
		// 1st stage transform with scaling. The sqrt(2) scaling 
		// factor is built in to the constants. Scaling = sqrt(2).
		int t0 = (int)(block[0+j] + block[32+j]);
		int t1 = (int)(block[0+j] - block[32+j]);
		temp = (int)(block[16+j] + block[48+j]) * FIDI1_W6;
		int t2 = (temp - ((int)block[48+j] * FIDI1_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[16+j] * FIDI1_W14) + 1024) >> 11;
		temp = (int)(block[8+j] - block[56+j]) * FIDI1_W1;
		int r4 = (temp - ((int)block[8+j] * FIDI1_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[56+j] * FIDI1_W11) + 1024) >> 11;
		temp = (int)(block[24+j] + block[40+j]) * FIDI1_W3;
		int r5 = (temp - ((int)block[24+j] * FIDI1_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[40+j] * FIDI1_W13) + 1024) >> 11;

		// 2nd stage transform.
		int t5 = r4 - r5;
		int t6 = r7 - r6;

		// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		// 4th stage.
		block[j]		= (dctType)(s0 + s7);
		block[j+56] = (dctType)(s0 - s7);
		block[j+8]	= (dctType)(s1 + s6);
		block[j+48]	= (dctType)(s1 - s6);
		block[j+16]	= (dctType)(s2 + s5);
		block[j+40]	= (dctType)(s2 - s5);
		block[j+24]	= (dctType)(s3 + s4);
		block[j+32]	= (dctType)(s3 - s4);
	}//end for j...

	// 1-D inverse dct in horiz direction.
	for(j = 0; j < 64; j += 8)
	{
		// 1st stage transform.
		int t0 = (int)(block[0+j] + block[4+j]);
		int t1 = (int)(block[0+j] - block[4+j]);
		temp = (int)(block[2+j] + block[6+j]) * FIDI1_W6;
		int t2 = (temp - ((int)block[6+j] * FIDI1_W15) + 1024) >> 11;
		int t3 = (temp + ((int)block[2+j] * FIDI1_W14) + 1024) >> 11;
		temp = (int)(block[1+j] - block[7+j]) * FIDI1_W1;
		int r4 = (temp - ((int)block[1+j] * FIDI1_W10) + 1024) >> 11;
		int r7 = (temp + ((int)block[7+j] * FIDI1_W11) + 1024) >> 11;
		temp = (int)(block[3+j] + block[5+j]) * FIDI1_W3;
		int r5 = (temp - ((int)block[3+j] * FIDI1_W12) + 1024) >> 11;
		int r6 = (temp - ((int)block[5+j] * FIDI1_W13) + 1024) >> 11;

		// 2nd stage transform.
		int t5 = r4 - r5;
		int t6 = r7 - r6;

		// 3rd stage.
		int s0 = t0 + t3;
		int s1 = t1 + t2;
		int s2 = t1 - t2;
		int s3 = t0 - t3;
		int s4 = r4 + r5;
		int s7 = r6 + r7;
		int s5 = (((t6 - t5) * 181) + 128) >> 8;
		int s6 = (((t6 + t5) * 181) + 128) >> 8;

		// 4th stage with scaling (dividing) by 8 and rounding.
		block[j]		= (dctType)((s0 + s7 + 4) >> 3);
		block[j+7]	= (dctType)((s0 - s7 + 4) >> 3);
		block[j+1]	= (dctType)((s1 + s6 + 4) >> 3);
		block[j+6]	= (dctType)((s1 - s6 + 4) >> 3);
		block[j+2]	= (dctType)((s2 + s5 + 4) >> 3);
		block[j+5]	= (dctType)((s2 - s5 + 4) >> 3);
		block[j+3]	= (dctType)((s3 + s4 + 4) >> 3);
		block[j+4]	= (dctType)((s3 - s4 + 4) >> 3);
	}//end for j...

}//end idct.


