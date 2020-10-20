/** @file

MODULE						: RleMotionVectorCodec

TAG								: RMVC

FILE NAME					: RLEMotionVectorCodec.cpp

DESCRIPTION				: A class to run-length encode (decode) motion vectors in a
										MotionVectorList to (from) a RleMotionVectorCodec. The class
										does not own the memory of these objects.

REVISION HISTORY	:	

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

#include <memory.h>
#include <string.h>
#include <stdlib.h>

#include "RleMotionVectorCodec.h"
#include "RunLengthVlcEncoder.h"
#include "MotionVectorVlcEncoder.h"

/*
---------------------------------------------------------------------
	Class Constants.
---------------------------------------------------------------------
*/
const int RleMotionVectorCodec::NORMAL				= 1;
const int RleMotionVectorCodec::DIFFERENTIAL	= 2;

/*
---------------------------------------------------------------------
	Public Methods.
---------------------------------------------------------------------
*/

RleMotionVectorCodec::RleMotionVectorCodec(void)
{
	_mode						= NORMAL;	// Default = normal.
	_maxRunLength		= 256;
	_escNumBits			= 8;
	_escBitMask			= 0x000000FF;

	_pRleVlcEncoder = NULL;
	_pRleVlcEncoder = new RunLengthVlcEncoder();

	_pMvVlcEncoder	= NULL;
	_pMvVlcEncoder	= new MotionVectorVlcEncoder();
}//end constructor.

RleMotionVectorCodec::~RleMotionVectorCodec(void)
{
	if(_pRleVlcEncoder != NULL)
		delete _pRleVlcEncoder;
	_pRleVlcEncoder = NULL;

	if(_pMvVlcEncoder != NULL)
		delete _pMvVlcEncoder;
	_pMvVlcEncoder = NULL;

}//end destructor.

/** Set the mode of the codec.
The mode determines how to treat the encoding process as 
either normal or differential.
@param	mode	: Default=0, normal=1, diff=2;
@return				: 1 = success, 0 = failure.
*/
int RleMotionVectorCodec::SetMode(int mode)
{
	if((mode < NORMAL)||(mode > DIFFERENTIAL))
		return(0);

	// The default is normal mode.
	_mode = NORMAL;
	if(mode == DIFFERENTIAL)
		_mode = DIFFERENTIAL;

	return(1);
}//end SetMode.

/** Set the max run-length value.
This defines the number of escape bits and the mask to use during vlc encoding.
It is determined as the max number of motion vectors in the frame being coded.
@param	maxRunLength	: Max number of vectors.
@return								:	none.
*/
void RleMotionVectorCodec::SetMaxRunLength(int maxRunLength)
{
	if(maxRunLength < 1)	// Out of range.
	{
		// Set defaults.
		_maxRunLength	= 256;
		_escNumBits		= 8;
		_escBitMask		= 0x000000FF;
		return;
	}//end if maxRunLength...

	int lclLength = maxRunLength - 1;

	int searchMask	= 0xFFFFFFFF;
	int notFound		= lclLength;
	int bitCount		= 0;

	while(notFound)
	{
		searchMask	= searchMask << 1;
		bitCount++;
		notFound		= lclLength & searchMask;
	}//end while notFound...

	_escNumBits		= bitCount;
	_escBitMask		= ~searchMask;
	_maxRunLength = maxRunLength;

	if(_pRleVlcEncoder != NULL)
		_pRleVlcEncoder->SetEsc(_escNumBits, _escBitMask);

}//end SetMaxRunLength.

/** Encode the src list to zero run-length list.
The encode process adds the vlc codewords to the structs of the
dst list.
@param	pSrc	: Contiguous list of input 2D vectors.
@param	pDst	: Encoded list of RleMotionVectorType structs.
@return				: Total sum of all vlc bit lengths, 0 = failed.
*/
int	RleMotionVectorCodec::Encode(VectorList* pSrc, RleMotionVectorList* pDst)
{
	// Needed to generate the codewords. They could have failed to be
	// instantiated in the constructor.
	if( (_pRleVlcEncoder == NULL)||(_pMvVlcEncoder == NULL) )
		return(0);

	// Set the motion vector byte storage structure (assume SIMPLE format).
	char* pMv			= (char *)(pSrc->GetDataPtr());
	int		vectors	= pSrc->GetLength();
	int   step		= pSrc->GetPatternSize();
	// ...and make sure we are sized correctly.
	if(vectors != _maxRunLength)
		SetMaxRunLength(vectors);

	// Set the rle encoded struct list.
	RleMotionVectorType* pRle = pDst->GetListPtr();
	int maxListLen						= pDst->GetMaxLength();
	pDst->SetLength(0);

  // Encode in the sequence order.
	int listLen				= 0; // Reset no. of valid structures in pRle[].
  int zeroRun				= 0;
	int totalVlcBits	= 0;
	int length;

	// Seperate out loop for speed.
	if(_mode == NORMAL)
	{
		for(length = 0; (length < vectors)&&(listLen < maxListLen); length++)
		{
			// Byte array in SIMPLE structure is orderd as [x0,y0], [x1,y1], ...
			int mvx = (int)(*pMv);
			int mvy = (int)(*(pMv+1));
			pMv	+= step;

			// Determine the normal run-length motion.
			if(mvx || mvy)	// ...not a zero vector.
			{
				pRle[listLen].run			= zeroRun;
				pRle[listLen].runBits	= _pRleVlcEncoder->Encode(zeroRun);
				pRle[listLen].runCode = _pRleVlcEncoder->GetCode();
				pRle[listLen].x				= mvx;
				pRle[listLen].y				= mvy;
				pRle[listLen].xyBits	= _pMvVlcEncoder->Encode2(mvx,mvy);
				pRle[listLen].xyCode	= _pMvVlcEncoder->GetCode();
				// Accumulate the bits used so far.
				totalVlcBits += (pRle[listLen].runBits + pRle[listLen].xyBits);

				listLen++;

				zeroRun = 0;
			}//end if mvx...
			else
			{
				zeroRun++;
				mvx = 0;
				mvy = 0;
			}//end else...
	  }//end for length...

	}//end if NORMAL...
	else // _mode == DIFFERENTIAL
	{
		int predx	= 0; // Start the prediction with [0,0].
		int predy	= 0;

		for(length = 0; (length < vectors)&&(listLen < maxListLen); length++)
		{
			// Byte array in SIMPLE structure is orderd as [x0,y0], [x1,y1], ...
			int mvx = (int)(*pMv);
			int mvy = (int)(*(pMv+1));
			pMv	+= step;

			// Implement the prediction loop.
			int dmvx	= mvx - predx;
			int dmvy	= mvy - predy;

			// Determine the differential run-length motion.
			if(dmvx || dmvy)
			{
				pRle[listLen].run			= zeroRun;
				pRle[listLen].runBits	= _pRleVlcEncoder->Encode(zeroRun);
				pRle[listLen].runCode = _pRleVlcEncoder->GetCode();
				pRle[listLen].x				= dmvx;
				pRle[listLen].y				= dmvy;
				pRle[listLen].xyBits	= _pMvVlcEncoder->Encode2(dmvx,dmvy);
				pRle[listLen].xyCode	= _pMvVlcEncoder->GetCode();

				totalVlcBits += (pRle[listLen].runBits +	pRle[listLen].xyBits);

				listLen++;

				zeroRun = 0;
			}//end if dmvx...
			else
			{
				zeroRun++;
				dmvx = 0;
				dmvy = 0;
			}//end else...

			// Implement the feedback loop.
			predx	= predx + dmvx;
			predy	= predy + dmvy;
		}//end for length...

	}//end else DIFFERENTIAL...

	// Set length of encoded list.
	pDst->SetLength(listLen);

	return(totalVlcBits);
}//end Encode.

/** Decode the src zero run-length list to the dst list.
The decode process does not interpret (or decode) the vlc codewords and
expects the vector values and the zero run value to be valid in the src list. 
@param	pSrc	: Encoded list of RleMotionVectorType structs.
@param	pDst	: Contiguous list of input 2D vectors.
@return				: 1 = success, 0 = no vectors to decode (all zeros).
*/
int	RleMotionVectorCodec::Decode(RleMotionVectorList* pSrc, VectorList* pDst)
{
	// Get the rle list to work with.
	RleMotionVectorType* pRleList = pSrc->GetListPtr();
	int listLen = pSrc->GetLength();

	if(listLen == 0)	// Empty list.
		return(0);

	// Set the motion vector byte storage structure (assume SIMPLE format).
	char* pMv			= (char *)(pDst->GetDataPtr());
	int		vectors	= pDst->GetLength();
	int   step		= pDst->GetPatternSize();

	// Fill the dest vectors.
	int v;
	int currPos = 0;
	int zeroRun = pRleList[0].run;	// There is at least 1 in the list.
	// Seperate out loop for speed.
	if(_mode == NORMAL)
	{
		char mvx, mvy;

		for(v = 0; v < vectors; v++)
		{
			if(zeroRun)
			{
				mvx = 0;
				mvy = 0;
				zeroRun--;
			}//end if zeroRun...
			else
			{
				mvx = pRleList[currPos].x;
				mvy = pRleList[currPos].y;

				currPos++; // Next rle set.

				// Is there still more?
				if(currPos < listLen)
					zeroRun = pRleList[currPos].run;
				else
					zeroRun = -1; // Infinity (well, at least to the end).
			}//end else...

			// Store the byte vector.
			*pMv			= mvx;
			*(pMv+1)	= mvy;
			pMv += step;
		}//end for v...

	}//end if NORMAL...
	else // _mode == DIFFERENTIAL
	{
		char mvx = 0;	 // Initial diff encoding set to zero.
		char mvy = 0;

		for(v = 0; v < vectors; v++)
		{
			if(zeroRun)
			{
				zeroRun--;
			}//end if zeroRun...
			else
			{
				mvx = mvx + pRleList[currPos].x;
				mvy = mvy + pRleList[currPos].y;

				currPos++; // Next rle set.

				// Is there still more?
				if(currPos < listLen)
					zeroRun = pRleList[currPos].run;
				else
					zeroRun = -1; // Infinity (well, at least to the end).
			}//end else...

			// Store the byte vector.
			*pMv			= mvx;
			*(pMv+1)	= mvy;
			pMv += step;
		}//end for v...

	}//end else DIFFERENTIAL...

	return(1);
}//end Decode.






