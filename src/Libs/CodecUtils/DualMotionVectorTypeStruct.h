/** @file

MODULE						: DualMotionVectorTypeStruct

TAG								: 

FILE NAME					: DualMotionVectorTypeStruct.h

DESCRIPTION				: A dual motion vector structure definition.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _DUALMOTIONVECTORTYPESTRUCT_H
#define _DUALMOTIONVECTORTYPESTRUCT_H

typedef struct _DualMotionVectorType
{
	int possibleCoord1;
	int possibleCoord2;
	int numBits;
	int codeWord;
} DualMotionVectorType;

#endif	// end _DUALMOTIONVECTORTYPESTRUCT_H.
