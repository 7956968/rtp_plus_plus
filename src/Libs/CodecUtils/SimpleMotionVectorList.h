/** @file

MODULE						: SimpleMotionVectorList

TAG								: MVL

FILE NAME					: SimpleMotionVectorList.h

DESCRIPTION				: A class to contain motion estimation results in a
										contiguous byte mem array. The structure of the
										motion vectors is determined by its type as either
										simple or complex.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _SIMPLEMOTIONVECTORLIST_H
#define _SIMPLEMOTIONVECTORLIST_H

#include "VectorList.h"

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class SimpleMotionVectorList : public VectorList
{
	public:
		SimpleMotionVectorList(void);
		virtual ~SimpleMotionVectorList(void);

};//end SimpleMotionVectorList.


#endif
