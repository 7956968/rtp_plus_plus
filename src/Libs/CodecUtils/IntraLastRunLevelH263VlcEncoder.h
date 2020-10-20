/** @file

MODULE						: IntraLastRunLevelH263VlcEncoder

TAG								: ILRLH263VE

FILE NAME					: IntraLastRunLevelH263VlcEncoder.h

DESCRIPTION				: An Intra last-run-level Vlc encoder implementation as defined in
										H.263 Recommendation (02/98) Annex D Table I.2 page 75. This is
										defined as the Advanced Intra mode and is implemented with an
										IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRALASTRUNLEVELH263VLCENCODER_H
#define _INTRALASTRUNLEVELH263VLCENCODER_H

#include "IVlcEncoder.h"
#include "LastRunLevelTypeStruct.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define ILRLH263VE_TABLE_LENGTH 102

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraLastRunLevelH263VlcEncoder : public IVlcEncoder
{
public:
	IntraLastRunLevelH263VlcEncoder(void);
	virtual ~IntraLastRunLevelH263VlcEncoder(void);

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }
	// A single symbol has no meaning for last-run-level elements.
	virtual int Encode(int symbol) { return(0); }

	// Optional interface implementation.
	// The 3 symbols represent last, run and level, respectively.
	virtual int	Encode3(int symbol1, int symbol2, int symbol3);

protected:
	void EncodeESC(int last, int run, int level);

protected:
	int 	_numCodeBits;	// Number of coded bits for the encoding.
	int 	_bitCode;			// The code of length numCodeBits.

	// Constants.
	static const LastRunLevelType VLC_TABLE[ILRLH263VE_TABLE_LENGTH];

};// end class LastRunLevelH263VlcEncoder.

#endif	// end _INTRALASTRUNLEVELH263VLCENCODER_H.
