/** @file

MODULE						: FastIntraLastRunLevelH263VlcEncoder

TAG								: FILRLH263VE

FILE NAME					: FastIntraLastRunLevelH263VlcEncoder.h

DESCRIPTION				: A fast Intra last-run-level Vlc encoder implementation as defined in
										H.263 Recommendation (02/98) Annex D Table I.2 page 75. This is
										defined in the Advanced Intra mode and is implemented with an
										IVlcEncoder Interface. It extends from an IntraLastRunLevelH263VlcEncoder()
										class to make a look up table of some of the symbol combinations.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _FASTINTRALASTRUNLEVELH263VLCENCODER_H
#define _FASTINTRALASTRUNLEVELH263VLCENCODER_H

#include "IntraLastRunLevelH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
/// 12 bits = 1 bit (last) + 5 bits (run) + 6 bits (level). [level range = -32..+31]
#define FILRLH263VE_TABLE_LENGTH 4096

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastIntraLastRunLevelH263VlcEncoder : public IntraLastRunLevelH263VlcEncoder
{
public:
	FastIntraLastRunLevelH263VlcEncoder(void);
	virtual ~FastIntraLastRunLevelH263VlcEncoder(void);

public:
	/// Optional interface implementation.
	/// The 3 symbols represent last, run and level, respectively.
	virtual int	Encode3(int symbol1, int symbol2, int symbol3);

protected:
	/// Constants set on construction.
	int VLC_LENGTH[FILRLH263VE_TABLE_LENGTH];
	int VLC_CODEWORD[FILRLH263VE_TABLE_LENGTH];

};// end class FastLastRunLevelH263VlcEncoder.

#endif	// end _FASTINTRALASTRUNLEVELH263VLCENCODER_H.
