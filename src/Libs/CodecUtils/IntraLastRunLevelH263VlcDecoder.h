/** @file

MODULE						: IntraLastRunLevelH263VlcDecoder

TAG								: ILRLH263VD

FILE NAME					: IntraLastRunLevelH263VlcDecoder.h

DESCRIPTION				: An Intra last-run-level Vlc decoder implementation as defined in
										H.263 Recommendation (02/98) Annex D Table I.2 page 75. This is
										defined as the Advanced Intra mode and is implemented with an
										IVlcDecoder Interface. The table has the same codes as those used
										for the AC coeff but with a different interpretation for the run,
										level values.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRALASTRUNLEVELH263VLCDECODER_H
#define _INTRALASTRUNLEVELH263VLCDECODER_H

#include "IVlcDecoder.h"
#include "LastRunLevelTypeStruct.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define ILRLH263VD_TABLE_LENGTH 103

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraLastRunLevelH263VlcDecoder : public IVlcDecoder
{
public:
	IntraLastRunLevelH263VlcDecoder();
	virtual ~IntraLastRunLevelH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }	// No markers for this decoder.
	// Single symbols are not relevant for a last-run-level decoder.
	virtual int Decode(IBitStreamReader* bsr) { _numCodeBits = 0; return(0); } 

	// Optional interface implementation.
	virtual int	Decode3(IBitStreamReader* bsr, int* symbol1, int* symbol2, int* symbol3);

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

	static const LastRunLevelType VLC_TABLE[ILRLH263VD_TABLE_LENGTH];

};// end class IntraLastRunLevelH263VlcDecoder.

#endif	// _INTRALASTRUNLEVELH263VLCDECODER_H
