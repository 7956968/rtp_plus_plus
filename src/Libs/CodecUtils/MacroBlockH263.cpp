/** @file

MODULE						: MacroBlockH263

TAG								: MBH263

FILE NAME					: MacroBlockH263.cpp

DESCRIPTION				: A class to hold H.263 macroblock data.

REVISION HISTORY	:
									: 

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

#include "MacroBlockH263.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
MacroBlockH263::MacroBlockH263(void)
{
	int i;
	// Aux data.
	for(i = 0; i < MBH263_BLKS; i++)
	{
		_offX[i] = 0;
		_offY[i] = 0;
	}//end for i...
	_mbIndex = 0;

	_mbQuant = 1;	// Quantisation for this macroblock.

	// COD (1 bit): Coded macroblock indication. 0 = coded, 1 = not coded. This is
	// not present in INTRA coded pictures (always assumed to be coded).
	_cod = 1;

	// MCBPC (vlc): Macroblock type and Coded Block Pattern for Chrominance. Table 7/8
	// in Recommendation H.263 (02/98) page 34 for I/P-pictures.
	_mbt	= 0;
	_cbpc = 0;

	// INTRA_MODE (vlc): Only inserted for Advanced INTRA Coding mode to describe the
	// prediction used. Table I.1 in Recommendation H.263 (02/98) page 74.
	_intramode = 0;

	// MODB (vlc): Macroblock mode for B-blocks. Only present in picture types of
	// PB-frame and indicates the existance of CBPB (_cbpb) and MVDB (_mvdbX, _mvdbY)
	// in the coded stream. Table 11 in Recommendation H.263 (02/98) page 37.
	_modb = 0;

	// CBPB (6 bits): Coded Block Pattern for B-blocks. Only exists if MODB (_modb)
	// indicates. One bit (1 = present, 0 = not present) for each block present of 
	// 4xlum and 2xchr blocks in _blkB[0..5].
	_cbpb = 0;

	// CBPY (vlc): Coded Block Pattern for luminance. Indicates presence of non-INTRADC
	// coeffs in pattern. Table 13 in Recommendation H.263 (02/98) page 38.
	_cbpy = 0;

	// DQUANT (2 bits): Differential Quantiser info. Change in the current QUANT value
	// truncated to the range [1..31]. Table 12 in Recommendation H.263 (02/98) page 37.
	_dquant = 0;

	// MVD (vlc): Motion Vector Difference. 1 or 4 vectors depending on MCBPC (_mcbpc)
	// and for advanced prediction mode.
	for(i = 0; i < 4; i++)
	{
		_mvdX[i]  = 0;
		_mvdY[i]  = 0;
		_xmvdX[i] = 0;
		_xmvdY[i] = 0;
	}//end for i...

	// MVDB (vlc): Motion Vector Data for B-macroblock.
	_mvdbX = 0;
	_mvdbY = 0;

	// Pointers to 8x8 Blocks _blkP[0..5] for P-blocks and _blkB[0..5] for B-blocks. The
	// Advanced Intra Coding Mode requires extra quant coeff blocks.
	for(i = 0; i < 6; i++)
	{
		_blk[i]				= NULL;
		_blkB[i]			= NULL;
		_qCoeffBlk[i] = NULL;
		_blk[i]				= new BlockH263();
		_blkB[i]			= new BlockH263();
		_qCoeffBlk[i]	= new BlockH263();
	}//end for i...

}//end constructor.

MacroBlockH263::~MacroBlockH263(void)
{
	for(int i = 0; i < 6; i++)
	{
		if(_blk[i] != NULL)
                        delete _blk[i];
		_blk[i] = NULL;

		if(_blkB[i] != NULL)
                        delete _blkB[i];
		_blkB[i] = NULL;

		if(_qCoeffBlk[i] != NULL)
                        delete _qCoeffBlk[i];
		_qCoeffBlk[i] = NULL;

	}//end for i...
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/

int MacroBlockH263::Distortion(OverlayMem2Dv2* p1Y, OverlayMem2Dv2* p1Cb, OverlayMem2Dv2* p1Cr, OverlayMem2Dv2* p2Y, OverlayMem2Dv2* p2Cb, OverlayMem2Dv2* p2Cr)
{
  int distortion = 0;
  int tmp1Width, tmp1Height, tmp1OrgX, tmp1OrgY;
  int tmp2Width, tmp2Height, tmp2OrgX, tmp2OrgY;

  /// Lum
  tmp1Width   = p1Y->GetWidth();
  tmp1Height  = p1Y->GetHeight();
  tmp1OrgX    = p1Y->GetOriginX();
  tmp1OrgY    = p1Y->GetOriginY();
  p1Y->SetOverlayDim(16,16);
  p1Y->SetOrigin(_offX[MBH263_Y1], _offY[MBH263_Y1]);
  tmp2Width   = p2Y->GetWidth();
  tmp2Height  = p2Y->GetHeight();
  tmp2OrgX    = p2Y->GetOriginX();
  tmp2OrgY    = p2Y->GetOriginY();
  p2Y->SetOverlayDim(16,16);
  p2Y->SetOrigin(_offX[MBH263_Y1], _offY[MBH263_Y1]);

  distortion = p1Y->Tsd16x16( *p2Y );

  p1Y->SetOverlayDim(tmp1Width,tmp1Height);
  p1Y->SetOrigin(tmp1OrgX, tmp1OrgY);
  p2Y->SetOverlayDim(tmp2Width,tmp2Height);
  p2Y->SetOrigin(tmp2OrgX, tmp2OrgY);

  /// Cb
  tmp1Width   = p1Cb->GetWidth();
  tmp1Height  = p1Cb->GetHeight();
  tmp1OrgX    = p1Cb->GetOriginX();
  tmp1OrgY    = p1Cb->GetOriginY();
  p1Cb->SetOverlayDim(8,8);
  p1Cb->SetOrigin(_offX[MBH263_Cb], _offY[MBH263_Cb]);
  tmp2Width   = p2Cb->GetWidth();
  tmp2Height  = p2Cb->GetHeight();
  tmp2OrgX    = p2Cb->GetOriginX();
  tmp2OrgY    = p2Cb->GetOriginY();
  p2Cb->SetOverlayDim(8,8);
  p2Cb->SetOrigin(_offX[MBH263_Cb], _offY[MBH263_Cb]);

  distortion += p1Cb->Tsd8x8( *p2Cb );

  p1Cb->SetOverlayDim(tmp1Width,tmp1Height);
  p1Cb->SetOrigin(tmp1OrgX, tmp1OrgY);
  p2Cb->SetOverlayDim(tmp2Width,tmp2Height);
  p2Cb->SetOrigin(tmp2OrgX, tmp2OrgY);

  /// Cr
  tmp1Width   = p1Cr->GetWidth();
  tmp1Height  = p1Cr->GetHeight();
  tmp1OrgX    = p1Cr->GetOriginX();
  tmp1OrgY    = p1Cr->GetOriginY();
  p1Cr->SetOverlayDim(8,8);
  p1Cr->SetOrigin(_offX[MBH263_Cr], _offY[MBH263_Cr]);
  tmp2Width   = p2Cr->GetWidth();
  tmp2Height  = p2Cr->GetHeight();
  tmp2OrgX    = p2Cr->GetOriginX();
  tmp2OrgY    = p2Cr->GetOriginY();
  p2Cr->SetOverlayDim(8,8);
  p2Cr->SetOrigin(_offX[MBH263_Cr], _offY[MBH263_Cr]);

  distortion += p1Cr->Tsd8x8( *p2Cr );

  p1Cr->SetOverlayDim(tmp1Width,tmp1Height);
  p1Cr->SetOrigin(tmp1OrgX, tmp1OrgY);
  p2Cr->SetOverlayDim(tmp2Width,tmp2Height);
  p2Cr->SetOrigin(tmp2OrgX, tmp2OrgY);

  return(distortion);
}//end Distortion.

