/** @file

MODULE						: RleMotionVectorCodec

TAG								: RMVC

FILE NAME					: RleMotionVectorCodec.h

DESCRIPTION				: A class to run-length encode (decode) motion vectors in a
										MotionVectorList to (from) a RleMotionVectorCodec. The class
										does not own the memory of these objects.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _RLEMOTIONVECTORCODEC_H
#define _RLEMOTIONVECTORCODEC_H

#include "VectorList.h"
#include "RleMotionVectorList.h"
#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------
	Type definitions.
---------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class RleMotionVectorCodec
{
	public:
		RleMotionVectorCodec(void);
		virtual ~RleMotionVectorCodec(void);

		// Encoding methods.
		int		Encode(VectorList* pSrc, RleMotionVectorList* pDst);
		int		Decode(RleMotionVectorList* pSrc, VectorList* pDst);

		// Mode setting.
		int		SetMode(int mode);
		int		GetMode(void)					{ return(_mode); }

		void	SetMaxRunLength(int maxRunLength);
		int		GetMaxRunLength(void)	{ return(_maxRunLength); }

	public:
		// Mode constants.
		static const int NORMAL;
		static const int DIFFERENTIAL;

	private:
		int		_mode;					// The lists are treated as default=0, normal=1. differential=2.
		int		_maxRunLength;	// Defines the escape bit length and mask.
		int   _escNumBits;
		int		_escBitMask;

		IVlcEncoder*	_pRleVlcEncoder;
		IVlcEncoder*	_pMvVlcEncoder;

};//end RleMotionVectorCodec.

#endif
