/** @file

MODULE						: FastInterLastRunLevelH263VlcEncoder

TAG								: FLRLH263VE

FILE NAME					: FastInterLastRunLevelH263VlcEncoder.h

DESCRIPTION				: A fast Inter last-run-level Vlc encoder implementation as defined in
										H.263 Recommendation (02/98). This is defined in the Inter and the std
										Intra mode and is implemented with an	IVlcEncoder Interface. It extends 
										from an LastRunLevelH263VlcEncoder() class to make a look up table of 
										some of the symbol combinations.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _FASTINTERLASTRUNLEVELH263VLCENCODER_H
#define _FASTINTERLASTRUNLEVELH263VLCENCODER_H

#include "LastRunLevelH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
/// 13 bits = 1 bit (last) + 6 bits (run) + 6 bits (level). [level range = -32..+31]
#define FLRLH263VE_TABLE_LENGTH 8192

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastInterLastRunLevelH263VlcEncoder : public LastRunLevelH263VlcEncoder
{
public:
	FastInterLastRunLevelH263VlcEncoder(void);
	virtual ~FastInterLastRunLevelH263VlcEncoder(void);

public:
	/// Optional interface implementation.
	/// The 3 symbols represent last, run and level, respectively.
	virtual int	Encode3(int symbol1, int symbol2, int symbol3);

protected:
	/// Constants set on construction.
	int VLC_LENGTH[FLRLH263VE_TABLE_LENGTH];
	int VLC_CODEWORD[FLRLH263VE_TABLE_LENGTH];

};// end class FastInterLastRunLevelH263VlcEncoder.

#endif	// end _FASTINTERLASTRUNLEVELH263VLCENCODER_H.
