/** @file

MODULE						: MacroBlockH263

TAG								: MBH263

FILE NAME					: MacroBlockH263.h

DESCRIPTION				: A class to hold H.263 macroblock data.

REVISION HISTORY	:

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2008 all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifndef _MACROBLOCKH263_H
#define _MACROBLOCKH263_H

#pragma once

#include "BlockH263.h"

typedef short mbType;

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define MBH263_Y1				0
#define MBH263_Y2				1
#define MBH263_Y3				2
#define MBH263_Y4				3
#define MBH263_Cb				4
#define MBH263_Cr				5
#define MBH263_BLKS			6
#define MBH263_LUM_BLKS	4

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MacroBlockH263
{
public:
	MacroBlockH263(void);
	virtual ~MacroBlockH263(void);

// Interface implementation.
public:
  // Calc the sqr error distortion between two images inside the mb space.
  int Distortion(OverlayMem2Dv2* p1Y, OverlayMem2Dv2* p1Cb, OverlayMem2Dv2* p1Cr, OverlayMem2Dv2* p2Y, OverlayMem2Dv2* p2Cb, OverlayMem2Dv2* p2Cr);

// Aux data members.
public:
	// Relative coords of each of 6 blocks in this macroblock in a contiguous image mem.
	int					_offX[6];
	int					_offY[6];
	int					_mbIndex;	// The index of this macroblock in the image.

	// Extra motion vector diff holders. During vlc decoding there are 2 possible vectors
	// but only one will yield a vector inside the [-16.0...15.5] allowed motion range. 
	// These are used for temp storage until the final decision is made.
	int					_xmvdX[4];
	int					_xmvdY[4];

	// The Advanced Intra Coding mode requires holding some partially processed data
	// to be used for prediction of later decoded macroblocks and blocks.
	BlockH263*	_qCoeffBlk[6];	// A copy of the quantised DCT coeffs.
	int					_mbQuant;				// Macroblock quantisation value.

	// Distortion-Rate optimisations require storing intermediate values. The arrays
	// are directly addressable by _mbQuant. Use an include flag to help with 
	// heuristic elimination.
	int					_distortion[32];
	int					_rate[32];
	int					_include;

// Coded data members.
public:
	// COD (1 bit): Coded macroblock indication. 0 = coded, 1 = not coded. This is
	// not present in INTRA coded pictures (as it is always assumed to be coded).
	int _cod;

	// MCBPC (vlc): Macroblock type and Coded Block Pattern for Chrominance. Table 7/8
	// in Recommendation H.263 (02/98) page 34 for I/P-pictures.
	int _mbt;
	int _cbpc;

	// INTRA_MODE (vlc): Only inserted for Advanced INTRA Coding mode to describe the
	// prediction used. Table I.1 in Recommendation H.263 (02/98) page 74.
	int _intramode;

	// MODB (vlc): Macroblock mode for B-blocks. Only present in picture types of
	// PB-frame and indicates the existance of CBPB (_cbpb) and MVDB (_mvdbX, _mvdbY)
	// in the coded stream. Table 11 in Recommendation H.263 (02/98) page 37.
	int _modb;

	// CBPB (6 bits): Coded Block Pattern for B-blocks. Only exists if MODB (_modb)
	// indicates. One bit (1 = present, 0 = not present) for each block present of 
	// 4xlum and 2xchr blocks in _blkB[0..5].
	int _cbpb;

	// CBPY (vlc): Coded Block Pattern for luminance. Indicates presence of non-INTRADC
	// coeffs in pattern. Table 13 in Recommendation H.263 (02/98) page 38.
	int _cbpy;

	// DQUANT (2 bits): Differential Quantiser info. Change in the current QUANT value
	// truncated to the range [1..31]. Table 12 in Recommendation H.263 (02/98) page 37.
	int _dquant;

	// MVD (vlc): Motion Vector Difference. 1 or 4 vectors depending on MCBPC (_mcbpc)
	// and for advanced prediction mode.
	int _mvdX[4];
	int _mvdY[4];

	// MVDB (vlc): Motion Vector Data for B-macroblock.
	int _mvdbX;
	int _mvdbY;

	// Pointers to 8x8 Blocks _blk[0..5] for I- and P-blocks and _blkB[0..5] for B-blocks.
	BlockH263*	_blk[6];
	BlockH263*	_blkB[6];

};// end class MacroBlockH263.

#endif	//_MACROBLOCKH263_H
