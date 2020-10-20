/** @file

MODULE						: FastMotionVectorVlcDecoderImpl2

TAG								: FMVVDI2

FILE NAME					: FastMotionVectorVlcDecoderImpl2.cpp

DESCRIPTION				: A fast motion vector Vlc decoder implementation with an
										IVlcDecoder Interface.

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

#include "FastMotionVectorVlcDecoderImpl2.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
const int FastMotionVectorVlcDecoderImpl2::NUM_ESC_BITS	= 5;
const int FastMotionVectorVlcDecoderImpl2::ESC_LENGTH		= 8;

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
int	FastMotionVectorVlcDecoderImpl2::Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2)
{
	*symbol1	= Decode(bsr);
	int xBits = _numDecodedBits;
	if(!xBits)
	{
		_numDecodedBits = 0;
		return(0);
	}//end if !xBits...

	*symbol2	= Decode(bsr);
	int yBits = _numDecodedBits;
	if(!yBits)
	{
		_numDecodedBits = 0;
		return(0);
	}//end if !yBits...

	return(xBits + yBits);
}//end Decode2.

int FastMotionVectorVlcDecoderImpl2::Decode(IBitStreamReader* bsr)
{
	// Descend a tree that combines a binary tree with lookup.
	switch(bsr->Read(2))
	{
		case 0:		// 00
			if(bsr->Read())		// 100
			{
				_numDecodedBits = 3;
				return(-1);
			}
			else								// 000
			{
				if(bsr->Read())	// 1000
				{
					if(bsr->Read())		// 1 1000
					{
						_numDecodedBits = 5;
						return(3);
					}//end if...
					else								// 0 1000
					{
						if(bsr->Read())		// 10 1000
						{
							if(bsr->Read())			// 110 1000
							{
								_numDecodedBits = 7;
								return(-9);
							}
							else									// 010 1000
							{
								_numDecodedBits = 8;
								if(bsr->Read())		// 1010 1000
									return(12);
								else								// 0010 1000
									return(-12);
							}//end else...
						}//end if...
						else								// 00 1000
						{
							_numDecodedBits = 6;
							return(6);
						}//end else...
					}//end else...
				}//end if...
				else							// 0000
				{
					_numDecodedBits = 4;
					return(2);
				}//end else...
			}//ens else...
		case 1:		// 01
			_numDecodedBits = 2;
			return(0);
		case 2:		// 10
			if(bsr->Read())	// 110
			{
				_numDecodedBits = 3;
				return(1);
			}//end if ...
			else							// 010
			{
				switch(bsr->Read(2))
				{
					case 0:				// 0 0010
						if(bsr->Read())		// 10 0010
						{
							if(bsr->Read())		// 110 0010
							{
								_numDecodedBits = 7;
								return(9);
							}//end if ...
							else								// 010 0010
							{
								if(bsr->Read())	// 1010 0010
								{
									_numDecodedBits = 9;
									if(bsr->Read())	// 1 1010 0010
										return(15);
									else							// 0 1010 0010
										return(-15);
								}//end if...
								else							// 0010 0010
								{
									_numDecodedBits = 8;
									return(-11);
								}//end else...
							}//end else...
						}//end if...
						else								// 00 0010
						{
							_numDecodedBits = 6;
							return(-5);
						}//end else...
					case 1:				// 0 1010	ESC
						_numDecodedBits = NUM_ESC_BITS + ESC_LENGTH;
						return(bsr->Read(ESC_LENGTH) - 128);
					case 2:				// 1 0010
						if(bsr->Read())		// 11 0010
						{
							if(bsr->Read())	// 111 0010
							{
								_numDecodedBits = 8;
								if(bsr->Read())	// 1111 0010
									return(-10);
								else							// 0111 0010
									return(11);
							}//end if...
							else							// 011 0010
							{
								_numDecodedBits = 7;
								return(-8);
							}//end else...
						}//end if...
						else								// 01 0010
						{
							_numDecodedBits = 6;
							return(5);
						}//end else...
					case 3:				// 1 1010
						_numDecodedBits = 6;
						if(bsr->Read())	// 11 1010
							return(4);
						else							// 01 1010
							return(-4);
				}//end switch...
			}//end else...
		case 3:		// 11
			switch(bsr->Read(2))
			{
				case 0:		// 0011
					if(bsr->Read())	// 1 0011
					{
						_numDecodedBits = 5;
						return(-3);
					}//end if...
					else							// 0 0011
					{
						_numDecodedBits = 6;
						if(bsr->Read())	// 10 0011
							return(-6);
						else							// 00 0011
							return(7);
					}//end else...
				case 1:		// 0111
					switch(bsr->Read(2))
					{
						case 0:		// 00 0111
							if(bsr->Read())	// 100 0111
							{
								_numDecodedBits = 7;
								return(10);
							}//end if...
							else							// 000 0111
							{
								_numDecodedBits = 8;
								if(bsr->Read())	// 1000 0111
									return(-13);
								else							// 0000 0111
									return(-14);
							}//end else...
						case 1:		// 01 0111
							switch(bsr->Read(2))
							{
								case 0:		// 0001 0111
									_numDecodedBits = 8;
									return(14);
								case 1:		// 0101 0111
									_numDecodedBits = 9;
									if(bsr->Read())	// 1 0101 0111
										return(17);
									else							// 0 0101 0111
										return(-16);
								case 2:		// 1001 0111
									_numDecodedBits = 8;
									return(13);
								case 3:		// 1101 0111
									_numDecodedBits = 9;
									if(bsr->Read())	// 1 1101 0111
										return(18);
									else							// 0 1101 0111
										return(-17);
							}//end switch...
						case 2:		// 10 0111
							_numDecodedBits = 6;
							return(8);
						case 3:		// 11 0111
							_numDecodedBits = 9;
							switch(bsr->Read(3))
							{
								case 0:		// 0 0011 0111
									return(-18);
								case 1:		// 0 0111 0111
									return(-20);
								case 2:		// 0 1011 0111
									return(-19);
								case 3:		// 0 1111 0111
									return(-21);
								case 4:		// 1 0011 0111
									return(19);
								case 5:		// 1 0111 0111
									return(21);
								case 6:		// 1 1011 0111
									return(20);
								case 7:		// 1 1111 0111
									return(22);
							}//end switch...
					}//end switch...
				case 2:		// 1011
					_numDecodedBits = 4;
					return(-2);
				case 3:		// 1111
					switch(bsr->Read(2))
					{
						case 0:		// 00 1111
							_numDecodedBits = 9;
							switch(bsr->Read(3))
							{
								case 0:	// 0 0000 1111
									return(-22);
								case 1:	// 0 0100 1111
									return(-24);
								case 2:	// 0 1000 1111
									return(-23);
								case 3:	// 0 1100 1111
									return(-25);
								case 4:	// 1 0000 1111
									return(23);
								case 5:	// 1 0100 1111
									return(25);
								case 6:	// 1 1000 1111
									return(24);
								case 7:	// 1 1100 1111
									return(26);
							}//end switch...
						case 1:		// 01 1111
							_numDecodedBits = 6;
							return(-7);
						case 2:		// 10 1111
							_numDecodedBits = 9;
							switch(bsr->Read(3))
							{
								case 0:	// 0 0010 1111
									return(-26);
								case 1:	// 0 0110 1111
									return(-28);
								case 2:	// 0 1010 1111
									return(-27);
								case 3:	// 0 1110 1111
									return(-29);
								case 4:	// 1 0010 1111
									return(27);
								case 5:	// 1 0110 1111
									return(29);
								case 6:	// 1 1010 1111
									return(28);
								case 7:	// 1 1110 1111
									return(30);
							}//end switch...
						case 3:		// 11 1111
							_numDecodedBits = 9;
							switch(bsr->Read(3))
							{
								case 0:	// 0 0011 1111
									return(-30);
								case 1:	// 0 0111 1111
									return(-32);
								case 2:	// 0 1011 1111
									return(-31);
								case 3:	// 0 1111 1111
									return(16);
								case 4:	// 1 0011 1111
									return(31);
								case 6:	// 1 1011 1111
									return(32);
								case 5:	// 1 0111 1111
								case 7:	// 1 1111 1111
								 	_numDecodedBits = 0; // Implies an error.
									return(0);
							}//end switch...
					}//end switch...
			}//end switch...
	}//end switch...

	// Safety net that should never be reached.
	_numDecodedBits = 0; // Implies an error.
	return(0);
}//end Decode.

