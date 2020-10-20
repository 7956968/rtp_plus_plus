/** @file

MODULE						: FastRunLengthVlcDecoderImpl2

TAG								: FRLVDI2

FILE NAME					: FastRunLengthVlcDecoderImpl2.cpp

DESCRIPTION				: A class to implement a fast run length vlc decoder derived
										from an IVlcDecoder interface. The table is inline with
										the decode method.

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

#include "FastRunLengthVlcDecoderImpl2.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
const int FastRunLengthVlcDecoderImpl2::NUM_ESC_CODE_BITS	= 5;
const int FastRunLengthVlcDecoderImpl2::EOP_MARKER					= 32;
const int FastRunLengthVlcDecoderImpl2::EOI_MARKER					= 33;

FastRunLengthVlcDecoderImpl2::FastRunLengthVlcDecoderImpl2()
{
	_numDecodedBits	= 0;
	_marker					= 0;
	_numEscBits			= 8;
	_escMask				= 255;
}//end constructor.

FastRunLengthVlcDecoderImpl2::~FastRunLengthVlcDecoderImpl2()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
int FastRunLengthVlcDecoderImpl2::Decode(IBitStreamReader* bsr)
{
  _marker	= 0; //Default setting.

	// Descend a tree that combines a binary tree with lookup.
	if(bsr->Read())	// 1
	{
		_numDecodedBits = 1;
		return(0);
	}
	else							// 0
	{
		switch(bsr->Read(2))
		{
			case 0:				// 000
				switch(bsr->Read(2))
				{
					case 0:		// 0 0000
						switch(bsr->Read(3))
						{
							case 0:		// 0000 0000
							case 4:		// 1000 0000
								_numDecodedBits	= 0; // Implies an error.
								return(0);
							case 1:		// 0010 0000
								switch(bsr->Read(2))
								{
									case 0:		// 00 0010 0000
										_numDecodedBits = 11;
										if(bsr->Read())		// 100 0010 0000
											return(23);
										else								// 000 0010 0000
											return(22);
									case 1:		// 01 0010 0000
										_numDecodedBits = 10;
										return(18);
									case 2:		// 10 0010 0000
										_numDecodedBits = 11;
										if(bsr->Read())		// 110 0010 0000
											return(21);
										else								// 010 0010 0000
											return(20);
									case 3:		// 11 0010 0000
										_numDecodedBits = 10;
										return(19);
								}//end switch...
							case 2:		// 0100 0000
								switch(bsr->Read(3))
								{
									case 0:		// 000 0100 0000
									case 1:		// 001 0100 0000
									case 2:		// 010 0100 0000
									case 4:		// 100 0100 0000
									case 5:		// 101 0100 0000
									case 6:		// 110 0100 0000
										_numDecodedBits	= 0; // Implies an error.
										return(0);
									case 3:		// 011 0100 0000	EOP marker.
										_marker		 = 1;
										_numDecodedBits = 11;
										return(EOP_MARKER);
									case 7:	  // 111 0100 0000	EOI marker.
										_marker		 = 1;
										_numDecodedBits = 11;
										return(EOI_MARKER);
								}//end switch...
							case 3:		// 0110 0000
								_numDecodedBits = 8;
								return(12);
							case 5:		// 1010 0000
								_numDecodedBits = 10;
								switch(bsr->Read(2))
								{
									case 0:		// 00 1010 0000
										return(16);
									case 1:		// 01 1010 0000
										return(14);
									case 2:		// 10 1010 0000
										return(17);
									case 3:		// 11 1010 0000
										return(15);
								}//end switch...
							case 6:		// 1100 0000
								_numDecodedBits = 11;
								switch(bsr->Read(3))
								{
									case 0:		// 000 1100 0000
										return(30);
									case 1:		// 001 1100 0000
										return(26);
									case 2:		// 010 1100 0000
										return(28);
									case 3:		// 011 1100 0000
										return(24);
									case 4:		// 100 1100 0000
										return(31);
									case 5:		// 101 1100 0000
										return(27);
									case 6:		// 110 1100 0000
										return(29);
									case 7:		// 111 1100 0000
										return(25);
								}//end switch...
							case 7:		// 1110 0000
								_numDecodedBits = 8;
								return(13);
						}//end switch...
					case 1:		// 0 1000
						_numDecodedBits = 5;
						return(5);
					case 2:		// 1 0000
						switch(bsr->Read(2))
						{
							case 0:		// 001 0000
								_numDecodedBits = 8;
								if(bsr->Read())	// 1001 0000
									return(11);
								else							// 0001 0000
									return(10);
							case 1:		// 011 0000
								_numDecodedBits = 7;
								return(6);
							case 2:		// 101 0000
								_numDecodedBits = 8;
								if(bsr->Read())	// 1101 0000
									return(9);
								else							// 0101 0000
									return(8);
							case 3:		// 111 0000
								_numDecodedBits = 7;
								return(7);
						}//end switch...
					case 3:		// 1 1000	ESC
						_numDecodedBits = NUM_ESC_CODE_BITS + _numEscBits;
						return(bsr->Read(_numEscBits) & _escMask);
				}//end switch...
			case 1:				// 010
				_numDecodedBits = 3;
				return(1);
			case 2:				// 100
				_numDecodedBits = 4;
				if(bsr->Read())	// 1100
					return(4);
				else							// 0100
					return(3);
			case 3:				// 110
				_numDecodedBits = 3;
				return(2);
		}//end switch...
	}//end else...

	// Return path safty net.
	_numDecodedBits	= 0; // Implies an error.
	return(0);
}//end Decode.

