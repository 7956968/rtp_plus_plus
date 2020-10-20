/** @file

MODULE						: LastRunLevelH263VlcDecoder

TAG								: LRLH263VD

FILE NAME					: LastRunLevelH263VlcDecoder.h

DESCRIPTION				: A last-run-level Vlc decoder implementation as defined 
										in the H.263 Recommendation (02/98) with an	IVlcDecoder 
										Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _LASTRUNLEVELH263VLCDECODER_H
#define _LASTRUNLEVELH263VLCDECODER_H

#include "IVlcDecoder.h"
#include "LastRunLevelTypeStruct.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define LRLH263VD_TABLE_LENGTH 103

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class LastRunLevelH263VlcDecoder : public IVlcDecoder
{
public:
	LastRunLevelH263VlcDecoder();
	virtual ~LastRunLevelH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }	// No markers for this decoder.
	// Single symbols are not relevant for a last-run-level decoder.
	virtual int Decode(IBitStreamReader* bsr) { _numCodeBits = 0; return(0); } 

	// Optional interface implementation.
	virtual int	Decode3(IBitStreamReader* bsr, int* symbol1, int* symbol2, int* symbol3);
	virtual int	Decode3(int numBits, int codeword, int* symbol1, int* symbol2, int* symbol3);

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

	static const LastRunLevelType VLC_TABLE[LRLH263VD_TABLE_LENGTH];

};// end class LastRunLevelH263VlcDecoder.

#endif	// _LASTRUNLEVELH263VLCDECODER_H
