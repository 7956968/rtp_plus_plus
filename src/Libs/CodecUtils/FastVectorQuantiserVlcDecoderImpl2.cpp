/** @file

MODULE						: FastVectorQuantiserVlcDecoderImpl2

TAG								: FVQVDI2

FILE NAME					: FastVectorQuantiserVlcDecoderImpl2.cpp

DESCRIPTION				: A class to implement a fast vector quantiser decoder where 
										the	binary table fully contains all indeces. The table is
										inline with the decode function. There is no Esc coding 
										requirements. It implements the IVlcDecoder interface.

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

#include "FastVectorQuantiserVlcDecoderImpl2.h"

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
int FastVectorQuantiserVlcDecoderImpl2::Decode(IBitStreamReader* bsr)
{
	// Descend a tree that combines a binary tree with lookup.
	switch(bsr->Read(4))
	{
		case 0:		// 0000
			switch(bsr->Read(2))
			{
				case 0:	// 00 0000
					if(bsr->Read())	// 100 0000
					{
						_numDecodedBits = 7;
						return(28);
					}//end if...
					else							// 000 0000
					{
						if(bsr->Read())	// 1000 0000
						{
							_numDecodedBits = 8;
							return(63);
						}//end if...
						else							// 0000 0000
						{
							if(!bsr->Read())	// 0 0000 0000
							{
								_numDecodedBits = 9;
								return(111);
							}//end if...
							else							// 1 0000 0000
							{
								if(bsr->Read())	// 11 0000 0000
								{
									_numDecodedBits = 10;
									return(169);
								}//end if...
								else							// 01 0000 0000
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 101 0000 0000
										return(210);
									else							// 001 0000 0000
										return(211);
								}//end else...
							}//end else...
						}//end else...
					}//end else...
				case 1:	// 01 0000
					_numDecodedBits = 6;
					return(14);
				case 2:	// 10 0000
					switch(bsr->Read(2))
					{
						case 0:	// 0010 0000
							if(bsr->Read())	// 1 0010 0000
							{
								_numDecodedBits = 9;
								return(110);
							}//end if...
							else							// 0 0010 0000
							{
								_numDecodedBits = 10;
								if(bsr->Read())	// 10 0010 0000
									return(170);
								else							// 00 0010 0000
									return(171);
							}//end else...
						case 1:	// 0110 0000
							_numDecodedBits = 9;
							if(bsr->Read())	// 1 0110 0000
								return(107);
							else							// 0 0110 0000
								return(109);
						case 2:	// 1010 0000
							_numDecodedBits = 8;
							return(62);
						case 3:	// 1110 0000
							if(bsr->Read())	// 1 1110 0000
							{
								_numDecodedBits = 9;
								return(108);
							}//end if...
							else							// 0 1110 0000
							{
								_numDecodedBits = 10;
								if(bsr->Read())	// 10 1110 0000
									return(167);
								else							// 00 1110 0000
									return(168);
							}//end else...
					}//end switch...
				case 3:	// 11 0000
					switch(bsr->Read(2))
					{
						case 0:	// 0011 0000
							_numDecodedBits = 8;
							return(61);
						case 1:	// 0111 0000
							_numDecodedBits = 8;
							return(60);
						case 2:	// 1011 0000
							if(!bsr->Read())	// 0 1011 0000
							{
								_numDecodedBits = 9;
								return(105);
							}//end if...
							else							// 1 1011 0000
							{
								if(!bsr->Read())	// 01 1011 0000
								{
									_numDecodedBits = 10;
									return(166);
								}//end if...
								else							// 11 1011 0000
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 111 1011 0000
										return(208);
									else							// 011 1011 0000
										return(209);
								}//end else...
							}//end else...
						case 3:	// 1111 0000
							_numDecodedBits = 8;
							return(58);
					}//end switch...
			}//end switch...
		case 1:		// 0001
			_numDecodedBits = 4;
			return(4);
		case 2:		// 0010
			switch(bsr->Read(2))
			{
				case 0:	// 00 0010
					_numDecodedBits = 6;
					return(11);
				case 1:	// 01 0010
					switch(bsr->Read(2))
					{
						case 0:	// 0001 0010
							_numDecodedBits = 8;
							return(45);
						case 1:	// 0101 0010
							if(!bsr->Read())	// 0 0101 0010
							{
								_numDecodedBits = 9;
								return(96);
							}//end if...
							else							// 1 0101 0010
							{
								_numDecodedBits = 10;
								if(bsr->Read())	// 11 0101 0010
									return(145);
								else							// 01 0101 0010
									return(147);
							}//end else...
						case 2:	// 1001 0010
							_numDecodedBits = 9;
							if(bsr->Read())	// 1 1001 0010
								return(97);
							else							// 0 1001 0010
								return(98);
						case 3:	// 1101 0010
							_numDecodedBits = 8;
							return(44);
					}//end switch...
				case 2:	// 10 0010
					switch(bsr->Read(2))
					{
						case 0:	// 0010 0010
							_numDecodedBits = 8;
							return(47);
						case 1:	// 0110 0010
							_numDecodedBits = 8;
							return(46);
						case 2:	// 1010 0010
							if(bsr->Read())	// 1 1010 0010
							{
								_numDecodedBits = 9;
								return(100);
							}//end if...
							else							// 0 1010 0010
							{
								_numDecodedBits = 10;
								if(bsr->Read())	// 10 1010 0010
									return(148);
								else							// 00 1010 0010
									return(151);
							}//end else...
						case 3:	// 1110 0010
							if(!bsr->Read())	// 0 1110 0010
							{
								_numDecodedBits = 9;
								return(99);
							}//end if...
							else							// 1 1110 0010
							{
								if(!bsr->Read())	// 01 1110 0010
								{
									_numDecodedBits = 10;
									return(149);
								}//end if...
								else							// 11 1110 0010
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 111 1110 0010
										return(202);
									else							// 011 1110 0010
										return(203);
								}//end else...
							}//end else...
					}//end switch...
				case 3:	// 11 0010
					_numDecodedBits = 6;
					return(10);
			}//end switch...
		case 3:		// 0011
			_numDecodedBits = 4;
			return(3);
		case 4:		// 0100
			switch(bsr->Read(2))
			{
				case 0:	// 00 0100
					switch(bsr->Read(2))
					{
						case 0:	// 0000 0100
							_numDecodedBits = 8;
							return(51);
						case 1:	// 0100 0100
							_numDecodedBits = 9;
							if(bsr->Read())	// 1 0100 0100
								return(102);
							else							// 0 0100 0100
								return(101);
						case 2:	// 1000 0100
							_numDecodedBits = 8;
							return(52);
						case 3:	// 1100 0100
							_numDecodedBits = 8;
							return(50);
					}//end switch...
				case 1:	// 01 0100
					switch(bsr->Read(2))
					{
						case 0:	// 0001 0100
							_numDecodedBits = 8;
							return(49);
						case 1:	// 0101 0100
							_numDecodedBits = 8;
							return(48);
						case 2:	// 1001 0100
							_numDecodedBits = 10;
							switch(bsr->Read(2))
							{
								case 0:	// 00 1001 0100
									return(157);
								case 1:	// 01 1001 0100
									return(155);
								case 2:	// 10 1001 0100
									return(153);
								case 3:	// 11 1001 0100
									return(156);
							}//end switch...
						case 3:	// 1101 0100
							switch(bsr->Read(2))
							{
								case 0:	// 00 1101 0100
									_numDecodedBits = 10;
									return(152);
								case 1:	// 01 1101 0100
									_numDecodedBits = 10;
									return(154);
								case 2:	// 10 1101 0100
									_numDecodedBits = 11;
									if(bsr->Read())	// 110 1101 0100
										return(206);
									else							// 010 1101 0100
										return(204);
								case 3:	// 11 1101 0100
									_numDecodedBits = 10;
									return(150);
							}//end switch...
					}//end switch...
				case 2:	// 10 0100
					_numDecodedBits = 6;
					return(12);
				case 3:	// 11 0100
					_numDecodedBits = 7;
					if(bsr->Read())	// 111 0100
						return(25);
					else							// 011 0100
						return(26);
			}//end switch...
		case 5:		// 0101
			switch(bsr->Read(2))
			{
				case 0:	// 00 0101
					if(bsr->Read())	// 100 0101
					{
						_numDecodedBits = 7;
						return(20);
					}//end if...
					else							// 000 0101
					{
						if(bsr->Read())	// 1000 0101
						{
							_numDecodedBits = 8;
							return(31);
						}//end if...
						else							// 0000 0101
						{
							if(bsr->Read())	// 1 0000 0101
							{
								_numDecodedBits = 9;
								return(77);
							}//end if...
							else							// 0 0000 0101
							{
								if(!bsr->Read())	// 00 0000 0101
								{
									_numDecodedBits = 10;
									return(126);
								}//end if...
								else							// 10 0000 0101
								{
									_numDecodedBits = 12;
									switch(bsr->Read(2))
									{
										case 0:		// 0010 0000 0101
											return(218);
										case 1:		// 0110 0000 0101
											return(216);
										case 2:		// 1010 0000 0101
											return(215);
										case 3:		// 1110 0000 0101
											return(217);
									}//end switch...
								}//end else...
							}//end else...
						}//else...
					}//end else...
				case 1:	// 01 0101
					if(bsr->Read())	// 101 0101
					{
						_numDecodedBits = 7;
						return(18);
					}//end if...
					else							// 001 0101
					{
						switch(bsr->Read(2))
						{
							case 0:				// 0 0001 0101
								if(bsr->Read())	// 10 0001 0101
								{
									_numDecodedBits = 10;
									return(122);
								}//end if...
								else							// 00 0001 0101
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 100 0001 0101
										return(182);
									else							// 000 0001 0101
										return(184);
								}//end else...
							case 1:				// 0 1001 0101
								_numDecodedBits = 9;
								return(72);
							case 2:				// 1 0001 0101
								_numDecodedBits = 10;
								if(bsr->Read())	// 11 0001 0101
									return(123);
								else							// 01 0001 0101
									return(124);
							case 3:				// 1 1001 0101
								_numDecodedBits = 9;
								return(73);
						}//end switch...
					}//end else...
				case 2:	// 10 0101
					if(bsr->Read())	// 110 0101
					{
						_numDecodedBits = 7;
						return(19);
					}//end if...
					else							// 010 0101
					{
						switch(bsr->Read(2))
						{
							case 0:			// 0 0010 0101
								if(bsr->Read())	// 10 0010 0101
								{
									_numDecodedBits = 10;
									return(125);
								}//end if...
								else							// 00 0010 0101
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 100 0010 0101
										return(183);
									else							// 000 0010 0101
										return(186);
								}//end else...
							case 1:			// 0 1010 0101
								_numDecodedBits = 9;
								return(75);
							case 2:			// 1 0010 0101
								_numDecodedBits = 9;
								return(76);
							case 3:			// 1 1010 0101
								_numDecodedBits = 9;
								return(74);
						}//end switch...
					}//end else...
				case 3:	// 11 0101
					_numDecodedBits = 6;
					return(8);
			}//end switch...
		case 6:		// 0110
			switch(bsr->Read(3))
			{
				case 0:	// 000 0110
					_numDecodedBits = 9;
					switch(bsr->Read(2))
					{
						case 0:	// 0 0000 0110
							return(94);
						case 1:	// 0 1000 0110
							return(95);
						case 2:	// 1 0000 0110
							return(93);
						case 3:	// 1 1000 0110
							return(92);
					}//end switch...
				case 1:	// 001 0110
					_numDecodedBits = 7;
					return(23);
				case 2:	// 010 0110
					switch(bsr->Read(2))
					{
						case 0:	// 0 0010 0110
							_numDecodedBits = 9;
							return(91);
						case 1:	// 0 1010 0110
							_numDecodedBits = 9;
							return(90);
						case 2:	// 1 0010 0110
							if(!bsr->Read())	// 01 0010 0110
							{
								_numDecodedBits = 10;
								return(146);
							}//end if...
							else							// 11 0010 0110
							{
								_numDecodedBits = 11;
								if(bsr->Read())	// 111 0010 0110
									return(201);
								else							// 011 0010 0110
									return(200);
							}//end else...
						case 3:	// 1 1010 0110
							_numDecodedBits = 10;
							if(bsr->Read())	// 11 1010 0110
								return(140);
							else							// 01 1010 0110
								return(144);
					}//end switch...
				case 3:	// 011 0110
					if(!bsr->Read())	// 0011 0110
					{
						_numDecodedBits = 8;
						return(41);
					}//end if...
					else							// 1011 0110
					{
						if(bsr->Read())	// 1 1011 0110
						{
							_numDecodedBits = 9;
							return(87);
						}//end if...
						else							// 0 1011 0110
						{
							switch(bsr->Read(2))
							{
								case 0:	// 000 1011 0110
									_numDecodedBits = 11;
									return(197);
								case 1:	// 010 1011 0110
									switch(bsr->Read(2))
									{
										case 0:	// 0 0010 1011 0110
											_numDecodedBits = 13;
											return(226);
										case 1:	// 0 1010 1011 0110
											_numDecodedBits = 13;
											return(224);
										case 2:	// 1 0010 1011 0110
											_numDecodedBits = 14;
											if(bsr->Read())	// 11 0010 1011 0110
												return(227);
											else							// 01 0010 1011 0110
												return(234);
										case 3:	// 1 1010 1011 0110
											_numDecodedBits = 13;
											return(223);
									}//end switch...
								case 2:	// 100 1011 0110
									_numDecodedBits = 11;
									return(199);
								case 3:	// 110 1011 0110
									_numDecodedBits = 11;
									return(198);
							}//end switch...
						}//end else...
					}//end else...
				case 4:	// 100 0110
					_numDecodedBits = 7;
					return(24);
				case 5:	// 101 0110
					if(bsr->Read())	// 1101 0110
					{
						_numDecodedBits = 8;
						return(42);
					}//end if...
					else							// 0101 0110
					{
						_numDecodedBits = 9;
						if(bsr->Read())	// 1 0101 0110
							return(89);
						else							// 0 0101 0110
							return(88);
					}//end else...
				case 6:	// 110 0110
					if(bsr->Read())	// 1110 0110
					{
						_numDecodedBits = 8;
						return(43);
					}//end if...
					else							// 0110 0110
					{
						_numDecodedBits = 10;
						switch(bsr->Read(2))
						{
							case 0:	// 00 0110 0110
								return(143);
							case 1:	// 01 0110 0110
								return(142);
							case 2:	// 10 0110 0110
								return(141);
							case 3:	// 11 0110 0110
								return(139);
						}//end switch...
					}//end else...
				case 7:	// 111 0110
					if(!bsr->Read())	// 0111 0110
					{
						_numDecodedBits = 8;
						return(40);
					}//end if...
					else							// 1111 0110
					{
						_numDecodedBits = 9;
						if(bsr->Read())	// 1 1111 0110
							return(85);
						else							// 0 1111 0110
							return(86);
					}//end else...
			}//end switch...
		case 7:		// 0111
			_numDecodedBits = 4;
			return(1);
		case 8:		// 1000
			switch(bsr->Read(2))
			{
				case 0:	// 00 1000
					switch(bsr->Read(2))
					{
						case 0:	// 0000 1000
							_numDecodedBits = 8;
							return(59);
						case 1:	// 0100 1000
							_numDecodedBits = 8;
							return(57);
						case 2:	// 1000 1000
							if(!bsr->Read())	// 0 1000 1000
							{
								_numDecodedBits = 9;
								return(106);
							}//end if...
							else							// 1 1000 1000
							{
								_numDecodedBits = 10;
								if(bsr->Read())	// 11 1000 1000
									return(163);
								else							// 01 1000 1000
									return(165);
							}//end else...
						case 3:	// 1100 1000
							_numDecodedBits = 8;
							return(56);
					}//end if switch...
				case 1:	// 01 1000
					_numDecodedBits = 6;
					return(13);
				case 2:	// 10 1000
					if(!bsr->Read())	// 010 1000
					{
						_numDecodedBits = 7;
						return(27);
					}//end if...
					else							// 110 1000
					{
						switch(bsr->Read(2))
						{
							case 0:	// 0 0110 1000
								_numDecodedBits = 10;
								if(bsr->Read())	// 10 0110 1000
									return(162);
								else							// 00 0110 1000
									return(164);
							case 1:	// 0 1110 1000
								_numDecodedBits = 9;
								return(103);
							case 2:	// 1 0110 1000
								_numDecodedBits = 9;
								return(104);
							case 3:	// 1 1110 1000
								if(bsr->Read())	// 11 1110 1000
								{
									_numDecodedBits = 10;
									return(160);
								}//end if...
								else							// 01 1110 1000
								{
									if(!bsr->Read())	// 001 1110 1000
									{
										_numDecodedBits = 11;
										return(207);
									}//end if...
									else							// 101 1110 1000
									{
										if(!bsr->Read())	// 0101 1110 1000
										{
											_numDecodedBits = 12;
											return(220);
										}//end if...
										else							// 1101 1110 1000
										{
											switch(bsr->Read(2))
											{
												case 0:	// 00 1101 1110 1000
													_numDecodedBits = 14;
													return(229);
												case 1:	// 01 1101 1110 1000
													_numDecodedBits = 15;
													if(bsr->Read())	// 101 1101 1110 1000
														return(237);
													else							// 001 1101 1110 1000
														return(244);
												case 2:	// 10 1101 1110 1000
													_numDecodedBits = 14;
													return(230);
												case 3:	// 11 1101 1110 1000
													_numDecodedBits = 14;
													return(231);
											}//end switch...
										}//end else...
									}//end else...
								}//end else...
						}//end switch...
					}//end else...
				case 3:	// 11 1000
					switch(bsr->Read(2))
					{
						case 0:	// 0011 1000
							_numDecodedBits = 8;
							return(55);
						case 1:	// 0111 1000
							_numDecodedBits = 8;
							return(53);
						case 2:	// 1011 1000
							_numDecodedBits = 8;
							return(54);
						case 3:	// 1111 1000
							switch(bsr->Read(2))
							{
								case 0:	// 00 1111 1000
									_numDecodedBits = 10;
									return(161);
								case 1:	// 01 1111 1000
									if(bsr->Read())	// 101 1111 1000
									{
										_numDecodedBits = 11;
										return(205);
									}//end if...
									else							// 001 1111 1000
									{
										if(!bsr->Read())	// 0001 1111 1000
										{
											_numDecodedBits = 12;
											return(219);
										}//end if...
										else							// 1001 1111 1000
										{
											switch(bsr->Read(2))
											{
												case 0:	// 00 1001 1111 1000
													_numDecodedBits = 15;
													if(bsr->Read())	// 100 1001 1111 1000
														return(238);
													else							// 000 1001 1111 1000
														return(242);
												case 1:	// 01 1001 1111 1000
													_numDecodedBits = 14;
													return(232);
												case 2:	// 10 1001 1111 1000
													_numDecodedBits = 15;
													if(bsr->Read())	// 110 1001 1111 1000
														return(239);
													else							// 010 1001 1111 1000
														return(245);
												case 3:	// 11 1001 1111 1000
													_numDecodedBits = 14;
													return(233);
											}//end switch...
										}//end else...
									}//end else...
								case 2:	// 10 1111 1000
									_numDecodedBits = 10;
									return(159);
								case 3:	// 11 1111 1000
									_numDecodedBits = 10;
									return(158);
							}//end switch...
					}//end switch...
			}//end switch...
		case 9:		// 1001
			if(bsr->Read())	// 1 1001
			{
				_numDecodedBits = 5;
				return(7);
			}//end if...
			else							// 0 1001
			{
				switch(bsr->Read(2))
				{
					case 0:				// 000 1001
						if(bsr->Read())	// 1000 1001
						{
							_numDecodedBits = 8;
							return(35);
						}//end if...
						else							// 0000 1001
						{
							if(!bsr->Read())	// 0 0000 1001
							{
								_numDecodedBits = 9;
								return(79);
							}//end if...
							else							// 1 0000 1001
							{
								if(!bsr->Read())	// 01 0000 1001
								{
									_numDecodedBits = 10;
									return(130);
								}//end if...
								else							// 11 0000 1001
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 111 0000 1001
										return(189);
									else							// 011 0000 1001
										return(187);
								}//end else...
							}//end else...
						}//end else...
					case 1:				// 010 1001
						if(!bsr->Read())	// 0010 1001
						{
							_numDecodedBits = 8;
							return(34);
						}//end if...
						else							// 1010 1001
						{
							if(!bsr->Read())	// 0 1010 1001
							{
								_numDecodedBits = 9;
								return(78);
							}//end if...
							else							// 1 1010 1001
							{
								if(!bsr->Read())	// 01 1010 1001
								{
									_numDecodedBits = 10;
									return(127);
								}//end if...
								else							// 11 1010 1001
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 111 1010 1001
										return(185);
									else							// 011 1010 1001
										return(188);
								}//end else...
							}//end else...
						}//end else...
					case 2:				// 100 1001
						_numDecodedBits = 7;
						return(21);
					case 3:				// 110 1001
						_numDecodedBits = 8;
						if(bsr->Read())	// 1110 1001
							return(32);
						else							// 0110 1001
							return(33);
				}//end switch...
			}//end else...
		case 10:	// 1010
			_numDecodedBits = 4;
			return(5);
		case 11:	// 1011
			_numDecodedBits = 4;
			return(2);
		case 12:	// 1100
			_numDecodedBits = 4;
			return(6);
		case 13:	// 1101
			switch(bsr->Read(3))
			{
				case 0:	// 000 1101
					_numDecodedBits = 7;
					return(17);
				case 1:	// 001 1101
					switch(bsr->Read(2))
					{
						case 0:	// 0 0001 1101
							if(!bsr->Read())	// 00 0001 1101
							{
								_numDecodedBits = 10;
								return(119);
							}//end if...
							else							// 10 0001 1101
							{
								_numDecodedBits = 11;
								if(bsr->Read())	// 110 0001 1101
									return(174);
								else							// 010 0001 1101
									return(177);
							}//else...
						case 1:	// 0 1001 1101
							_numDecodedBits = 9;
							return(66);
						case 2:	// 1 0001 1101
							_numDecodedBits = 10;
							if(bsr->Read())	// 11 0001 1101
								return(117);
							else							// 01 0001 1101
								return(118);
						case 3:	// 1 1001 1101
							if(bsr->Read())	// 11 1001 1101	
							{
								_numDecodedBits = 10;
								return(115);
							}//end if...
							else							// 01 1001 1101
							{
								_numDecodedBits = 11;
								if(bsr->Read())	// 101 1001 1101
									return(172);
								else							// 001 1001 1101
									return(173);
							}//end else...
					}//end switch...
				case 2:	// 010 1101
					if(!bsr->Read())	// 0010 1101
					{
						_numDecodedBits = 8;
						return(30);
					}//end if...
					else							// 1010 1101
					{
						_numDecodedBits = 9;
						if(bsr->Read())	// 1 1010 1101
							return(67);
						else							// 0 1010 1101
							return(68);
					}//end else...
				case 3:	// 011 1101
					_numDecodedBits = 7;
					return(15);
				case 4:	// 100 1101
					switch(bsr->Read(2))
					{
						case 0:	// 0 0100 1101
							switch(bsr->Read(2))
							{
								case 0:	// 000 0100 1101
									_numDecodedBits = 11;
									return(180);
								case 1:	// 010 0100 1101
									_numDecodedBits = 11;
									return(181);
								case 2:	// 100 0100 1101
									_numDecodedBits = 12;
									if(bsr->Read())	// 1100 0100 1101
										return(212);
									else							// 0100 0100 1101
										return(214);
								case 3:	// 110 0100 1101
									_numDecodedBits = 11;
									return(179);
							}//end switch...
						case 1:	// 0 1100 1101
							_numDecodedBits = 9;
							return(70);
						case 2:	// 1 0100 1101
							_numDecodedBits = 9;
							return(71);
						case 3:	// 1 1100 1101
							_numDecodedBits = 9;
							return(69);
					}//end switch...
				case 5:	// 101 1101
					_numDecodedBits = 7;
					return(16);
				case 6:	// 110 1101
					if(!bsr->Read())	// 0110 1101
					{
						_numDecodedBits = 8;
						return(29);
					}//end if...
					else							// 1110 1101
					{
						switch(bsr->Read(2))
						{
							case 0:	// 00 1110 1101
								if(bsr->Read())	// 100 1110 1101
								{
									_numDecodedBits = 11;
									return(178);
								}//end if...
								else							// 000 1110 1101
								{
									if(bsr->Read())	// 1000 1110 1101
									{
										_numDecodedBits = 12;
										return(213);
									}//end if...
									else							// 0000 1110 1101
									{
										if(bsr->Read())	// 1 0000 1110 1101
										{
											_numDecodedBits = 13;
											return(221);
										}//end if...
										else							// 0 0000 1110 1101
										{
											if(!bsr->Read())	// 00 0000 1110 1101
											{
												_numDecodedBits = 14;
												return(228);
											}//end if...
											else							// 10 0000 1110 1101
											{
												if(!bsr->Read())	// 010 0000 1110 1101
												{
													_numDecodedBits = 15;
													return(235);
												}//end if...
												else							// 110 0000 1110 1101
												{
													_numDecodedBits = 16;
													if(bsr->Read())	// 1110 0000 1110 1101
														return(246);
													else							// 0110 0000 1110 1101
														return(255);
												}//end else...
											}//end else...
										}//end else...
									}//end else...
								}//end else...
							case 1:	// 01 1110 1101
								_numDecodedBits = 10;
								return(121);
							case 2:	// 10 1110 1101
								_numDecodedBits = 10;
								return(120);
							case 3:	// 11 1110 1101
								_numDecodedBits = 11;
								if(bsr->Read())	// 111 1110 1101
									return(176);
								else							// 011 1110 1101
									return(175);
						}//end switch...
					}//end else...
				case 7:	// 111 1101
					switch(bsr->Read(2))
					{
						case 0:	// 0 0111 1101
							_numDecodedBits = 10;
							if(bsr->Read())	// 10 0111 1101
								return(114);
							else							// 00 0111 1101
								return(116);
						case 1:	// 0 1111 1101
							_numDecodedBits = 10;
							if(bsr->Read())	// 10 1111 1101
								return(113);
							else							// 00 1111 1101
								return(112);
						case 2:	// 1 0111 1101
							_numDecodedBits = 9;
							return(65);
						case 3:	// 1 1111 1101
							_numDecodedBits = 9;
							return(64);
					}//end switch...
			}//end switch...
		case 14:	// 1110
			switch(bsr->Read(2))
			{
				case 0:	// 00 1110
					_numDecodedBits = 6;
					return(9);
				case 1:	// 01 1110
					switch(bsr->Read(2))
					{
						case 0:	// 0001 1110
							_numDecodedBits = 8;
							return(38);
						case 1:	// 0101 1110
							if(!bsr->Read())	// 0 0101 1110
							{
								_numDecodedBits = 9;
								return(80);
							}//end if...
							else							// 1 0101 1110
							{
								if(bsr->Read())	// 11 0101 1110
								{
									_numDecodedBits = 10;
									return(132);
								}//end if...
								else							// 01 0101 1110
								{
									_numDecodedBits = 11;
									if(bsr->Read())	// 101 0101 1110
										return(190);
									else							// 001 0101 1110
										return(193);
								}//end else...
							}//end else...
						case 2:	// 1001 1110
							if(!bsr->Read())	// 0 1001 1110
							{
								_numDecodedBits = 9;
								return(81);
							}//end if...
							else							// 1 1001 1110
							{
								if(bsr->Read())	// 11 1001 1110
								{
									_numDecodedBits = 10;
									return(133);
								}//end if...
								else							// 01 1001 1110
								{
									if(!bsr->Read())	// 001 1001 1110
									{
										_numDecodedBits = 11;
										return(194);
									}//end if...
									else							// 101 1001 1110
									{
										switch(bsr->Read(2))
										{
											case 0:	// 0 0101 1001 1110
												switch(bsr->Read(2))
												{
													case 0:	// 000 0101 1001 1110
														_numDecodedBits = 16;
														if(bsr->Read())	// 1000 0101 1001 1110
															return(253);
														else							// 0000 0101 1001 1110
															return(252);
													case 1:	// 010 0101 1001 1110
														_numDecodedBits = 15;
														return(240);
													case 2:	// 100 0101 1001 1110
														_numDecodedBits = 15;
														return(236);
													case 3:	// 110 0101 1001 1110
														_numDecodedBits = 16;
														if(bsr->Read())	// 1110 0101 1001 1110
															return(254);
														else							// 0110 0101 1001 1110
															return(249);
												}//end switch...
											case 1:	// 0 1101 1001 1110
												switch(bsr->Read(2))
												{
													case 0:	// 000 1101 1001 1110
														_numDecodedBits = 15;
														return(241);
													case 1:	// 010 1101 1001 1110
														_numDecodedBits = 16;
														if(bsr->Read())	// 1010 1101 1001 1110
															return(248);
														else							// 0010 1101 1001 1110
															return(247);
													case 2:	// 100 1101 1001 1110
														_numDecodedBits = 15;
														return(243);
													case 3:	// 110 1101 1001 1110
														_numDecodedBits = 16;
														if(bsr->Read())	// 1110 1101 1001 1110
															return(251);
														else							// 0110 1101 1001 1110
															return(250);
												}//end switch...
											case 2:	// 1 0101 1001 1110
												_numDecodedBits = 13;
												return(225);
											case 3:	// 1 1101 1001 1110
												_numDecodedBits = 13;
												return(222);
										}//end switch...
									}//end else...
								}//end else...
							}//end else...
						case 3:	// 1101 1110
							_numDecodedBits = 8;
							return(37);
					}//end switch...
				case 2:	// 10 1110
					switch(bsr->Read(2))
					{
						case 0:	// 0010 1110
							if(bsr->Read())	// 1 0010 1110
							{
								_numDecodedBits = 9;
								return(84);
							}//end if...
							else							// 0 0010 1110
							{
								_numDecodedBits = 10;
								if(bsr->Read())	// 10 0010 1110
									return(137);
								else							// 00 0010 1110
									return(138);
							}//end else...
						case 1:	// 0110 1110
							_numDecodedBits = 8;
							return(39);
						case 2:	// 1010 1110
							_numDecodedBits = 9;
							if(bsr->Read())	// 1 1010 1110
								return(83);
							else							// 0 1010 1110
								return(82);
						case 3:	// 1110 1110
							switch(bsr->Read(2))
							{
								case 0:	// 00 1110 1110
									_numDecodedBits = 11;
									if(bsr->Read())	// 100 1110 1110
										return(196);
									else							// 000 1110 1110
										return(195);
								case 1:	// 01 1110 1110
									_numDecodedBits = 10;
									return(136);
								case 2:	// 10 1110 1110
									_numDecodedBits = 10;
									return(135);
								case 3:	// 11 1110 1110
									_numDecodedBits = 10;
									return(134);
							}//end switch...
					}//end switch...
				case 3:	// 11 1110
					if(!bsr->Read())	// 011 1110
					{
						_numDecodedBits = 7;
						return(22);
					}//end if...
					else							// 111 1110
					{
						if(bsr->Read())	// 1111 1110
						{
							_numDecodedBits = 8;
							return(36);
						}//end if...
						else							// 0111 1110
						{
							switch(bsr->Read(2))
							{
								case 0:	// 00 0111 1110
									_numDecodedBits = 10;
									return(129);
								case 1:	// 01 0111 1110
									_numDecodedBits = 11;
									if(bsr->Read())	// 101 0111 1110
										return(192);
									else							// 001 0111 1110
										return(191);
								case 2:	// 10 0111 1110
									_numDecodedBits = 10;
									return(131);
								case 3:	// 11 0111 1110
									_numDecodedBits = 10;
									return(128);
							}//end switch...
						}//end else...
					}//end else...
			}//end switch...
		case 15:	// 1111
			_numDecodedBits = 4;
			return(0);
	}//end switch...

	// Safety net that should never be reached.
	_numDecodedBits = 0; // Implies an error.
	return(0);
}//end Decode.


