/** @file

MODULE						: InterPMCBPCH263VlcDecoder

TAG								: IPMCBPCH263VD

FILE NAME					: InterPMCBPCH263VlcDecoder.cpp

DESCRIPTION				: An MCBPC Vlc decoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										Table 8 page 35 with an	IVlcDecoder Interface.

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

#include "InterPMCBPCH263VlcDecoder.h"

InterPMCBPCH263VlcDecoder::InterPMCBPCH263VlcDecoder()
{
	_numCodeBits	= 0;
	_marker				= 0;
}//end constructor.

InterPMCBPCH263VlcDecoder::~InterPMCBPCH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode a MB type and CBPC symbol from the bit stream.
Extract the symbols from the input stream. Decode as a binary tree 
to determine the symbol pair.
@param bsr			: Bit stream to get from.
@param symbol1	: Returned MB type value.
@param symbol2	: Returned CBPC value.
@return					: Num bits extracted.
*/
int	InterPMCBPCH263VlcDecoder::Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2)
{
	int mbt		= 0;
	int cbpc	= 0;
	int bits	= 0;
	_marker		= 0;

	// Construct an inline binary tree with a "need for speed".
	if(bsr->Read())	// 1
	{
		bits	= 1;
		mbt		= 0;
		cbpc	= 0;
	}//end if 1...
	else						// 0
	{
		if(bsr->Read())	// 01
		{
			if(bsr->Read())	// 011
			{
				bits	= 3;
				mbt		= 1;
				cbpc	= 0;
			}//end if 011...
			else						// 010
			{
				bits	= 3;
				mbt		= 2;
				cbpc	= 0;
			}//end else 010...
		}//end if 01...
		else						// 00
		{
			if(bsr->Read())	// 001
			{
				if(bsr->Read())	// 0011
				{
					bits	= 4;
					mbt		= 0;
					cbpc	= 1;
				}//end if 0011...
				else						// 0010
				{
					bits	= 4;
					mbt		= 0;
					cbpc	= 2;
				}//end else 0010... 
			}//end if 001...
			else						// 000
			{
				if(bsr->Read())	// 0001
				{
					if(bsr->Read())	// 0001 1
					{
						bits	= 5;
						mbt		= 3;
						cbpc	= 0;
					}//end if 0001 1...
					else						// 0001 0
					{
						if(bsr->Read())	// 0001 01
						{
							bits	= 6;
							mbt		= 0;
							cbpc	= 3;
						}//end if 0001 01...
						else						// 0001 00
						{
							bits	= 6;
							mbt		= 4;
							cbpc	= 0;
						}//end else 0001 00...
					}//end else 0001 0...
				}//end if 0001...
				else						// 0000
				{
					if(bsr->Read())	// 0000 1
					{
						if(bsr->Read())	// 0000 11
						{
							if(bsr->Read())	// 0000 111
							{
								bits	= 7;
								mbt		= 1;
								cbpc	= 1;
							}//end if 0000 111...
							else						// 0000 110
							{
								bits	= 7;
								mbt		= 1;
								cbpc	= 2;
							}//end else 0000 110...
						}//end if 0000 11...
						else						// 0000 10
						{
							if(bsr->Read())	// 0000 101
							{
								bits	= 7;
								mbt		= 2;
								cbpc	= 1;
							}//end if 0000 101...
							else						// 0000 100
							{
								bits	= 7;
								mbt		= 2;
								cbpc	= 2;
							}//end else 0000 100...
						}//end else 0000 10...
					}//end if 0000 1...
					else						// 0000 0
					{
						if(bsr->Read())	// 0000 01
						{
							if(bsr->Read())	// 0000 011
							{
								bits	= 7;
								mbt		= 3;
								cbpc	= 3;
							}//end if 0000 011...
							else						// 0000 010
							{
								if(bsr->Read())	// 0000 0101
								{
									bits	= 8;
									mbt		= 2;
									cbpc	= 3;
								}//end if 0000 0101...
								else						// 0000 0100
								{
									bits	= 8;
									mbt		= 3;
									cbpc	= 1;
								}//end else 0000 0100...
							}//end else 0000 010...
						}//end if 0000 01...
						else						// 0000 00
						{
							if(bsr->Read())	// 0000 001
							{
								if(bsr->Read())	// 0000 0011
								{
									bits	= 8;
									mbt		= 3;
									cbpc	= 2;
								}//end if 0000 0011...
								else						// 0000 0010
								{
									if(bsr->Read())	// 0000 0010 1
									{
										bits	= 9;
										mbt		= 1;
										cbpc	= 3;
									}//end if 0000 0010 1...
									else						// 0000 0010 0
									{
										bits	= 9;
										mbt		= 4;
										cbpc	= 1;
									}//end else 0000 0010 0...
								}//end else 0000 0010...
							}//end if 0000 001...
							else						// 0000 000
							{
								if(bsr->Read())	// 0000 0001
								{
									if(bsr->Read())	// 0000 0001 1
									{
										bits	= 9;
										mbt		= 4;
										cbpc	= 2;
									}//end if 0000 0001 1...
									else						// 0000 0001 0
									{
										bits	= 9;
										mbt		= 4;
										cbpc	= 3;
									}//end else 0000 0001 0...
								}//end if 0000 0001...
								else						// 0000 0000
								{
									if(bsr->Read())	// 0000 0000 1
									{
										bits	= 9;
										mbt		= 6;	// Meaningless for marker.
										cbpc	= 3;
										_marker = 1;
									}//end if stuffing 0000 0000 1...
									else						// 0000 0000 0
									{
										if(bsr->Read())	// 0000 0000 01
										{
											if(bsr->Read())	// 0000 0000 011
											{
												if(bsr->Read())	// 0000 0000 0111
												{
													if(bsr->Read())	// 0000 0000 0111 1
													{
														bits	= 13;
														mbt		= 5;
														cbpc	= 3;
													}//end if 0000 0000 0111 1...
													else						// 0000 0000 0111 0
													{
														bits	= 13;
														mbt		= 5;
														cbpc	= 2;
													}//end else 0000 0000 0111 0...
												}//end if 0000 0000 0111...
												else						// 0000 0000 0110
												{
													if(!bsr->Read())	// 0000 0000 0110 0
													{
														bits	= 13;
														mbt		= 5;
														cbpc	= 1;
													}//end if 0000 0000 0110 0... 
												}//end else 0000 0000 0110...
											}//end if 0000 0000 011...
											else						// 0000 0000 010
											{
												bits	= 11;
												mbt		= 5;
												cbpc	= 0;
											}//end else 0000 0000 010...
										}//end if 0000 0000 01...
									}//end else 0000 0000 0...
								}//end else 0000 0000...
							}//end else 0000 000...
						}//end else 0000 00...
					}//end else 0000 0...
				}//end else 0000...
			}//end else 000...
		}//end else 00...
	}//end else 0...

	// Set symbols.
	*symbol1			= mbt;
	*symbol2			= cbpc;
	_numCodeBits	= bits;
  return(bits);
}//end Decode2.

