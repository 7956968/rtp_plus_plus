/** @file

MODULE						: BlockH263

TAG								: BH263

FILE NAME					: BlockH263.h

DESCRIPTION				: A class to hold H.263 block data and define all block
										operations on the 8x8 unit.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)CSIR, Meraka Institute 2007 all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifndef _BLOCKH263_H
#define _BLOCKH263_H

#pragma once

#include "OverlayMem2Dv2.h"
#include "IForwardDct.h"
#include "IInverseDct.h"
#include "IScalarQuantiser.h"
#include "IRunLengthCodec.h"
#include "LastRunLevelH263List.h"

typedef short bType;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class BlockH263
{
public:
	BlockH263(void);
	virtual ~BlockH263(void);
	int Ready(void) { return(_ready); }

// Block operations.
public:
	void Dct(IForwardDct* pDct) { pDct->dct(_pBlk); }
	void IDct(IInverseDct* pIDct) { pIDct->idct(_pBlk); }
	void Quantise(IScalarQuantiser* pQ, int q) { pQ->quantise(_pBlk, q); }
	void InverseQuantise(IScalarQuantiser* pQ, int q) { pQ->inverseQuantise(_pBlk, q); }
	void RleEncode(IRunLengthCodec* rlc) { rlc->Encode((void *)_pBlk, (void *)_pLastRunLevelList); } 
	void RleDecode(IRunLengthCodec* rlc) { rlc->Decode((void *)_pLastRunLevelList, (void *)_pBlk); } 

// Member access.
public:
	OverlayMem2Dv2*				GetBlkOverlay(void) { return(_blk); }
	bType*								GetBlk(void) { return(_pBlk); }
	LastRunLevelH263List* GetRleList(void) { return(_pLastRunLevelList); }
	int										IsCoded(void) { return(_cod); }
	void									SetCoded(int cod) { _cod = cod; }
	int										GetIntraDC(void) { return(_intraDC); }
	void									SetIntraDC(int intraDC) { _intraDC = (bType)intraDC; }
	void									Copy(void* buff);
	void									CopyRow(int row, void* buff);
	void									CopyCol(int col, void* buff);
	void									Zero(void);

// Data block members.
protected:
	// Code/no code flag. Same as Macroblock level flag, but for this block 
	// only. Is used to help determine the MB type.
	int						_cod;
	// The 8x8 block coeffs.
	bType*				_pBlk;
	bType					_intraDC;	// Temp holder if needed.
	// The block overlay.
	OverlayMem2Dv2* _blk;
	// The last-run-level encoding.
	LastRunLevelH263List* _pLastRunLevelList;
  
	// Mem status.
	int	_ready;

};// end class BlockH263.

#endif	//_BLOCKH263_H
