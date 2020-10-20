/** @file

MODULE						: BlockH263

TAG								: BH263

FILE NAME					: BlockH263.cpp

DESCRIPTION				: A class to hold H.263 block data and define all block
										operations on the 8x8 unit.

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
#include <string.h>
#endif

#include <memory.h>
#include "BlockH263.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
BlockH263::BlockH263(void)
{
	// Dangerous to alloc mem in the constructor. Should be 2 phase 
	// construction. Use ready flag to check against.
	_ready	= 0;
	_cod		= 0;

	// The 8x8 block mem.
	_pBlk = NULL;
	_pBlk = new bType[64];

	// The block overlay.
	_blk = NULL;
	_blk = new OverlayMem2Dv2(_pBlk, 8, 8, 8, 8);

	// The run length encoding list.
	_pLastRunLevelList = NULL;
	_pLastRunLevelList = new LastRunLevelH263List();

	if( (_pBlk != NULL)&&(_blk != NULL)&&(_pLastRunLevelList != NULL) )
		_ready = 1;

	if(!_pLastRunLevelList->Ready())
		_ready = 0;

}//end constructor.

BlockH263::~BlockH263(void)
{
	if(_blk != NULL)
		delete _blk;
	_blk = NULL;

	if(_pBlk != NULL)
		delete[] _pBlk;
	_pBlk = NULL;

	if(_pLastRunLevelList != NULL)
		delete _pLastRunLevelList;
	_pLastRunLevelList = NULL;

	_ready = 0;	// Not really necessary.
}//end destructor.

/*
---------------------------------------------------------------------------
	Access Methods.
---------------------------------------------------------------------------
*/
/** Copy from the blk into the buffer.
No checking is done for overflow and is assumed to 
be 8 x bType in size.
@param buff	: Buffer to copy into.
@return			: none.
*/
void BlockH263::Copy(void* buff)
{
	if(buff == NULL)
		return;
	memcpy(buff, (const void *)_pBlk, 64 * sizeof(bType));
}//end Copy.

/** Copy a row from the blk into the buffer.
No checking is done for overflow and is assumed to 
be 8 x bType in size.
@param row	: Block row to copy from.
@param buff	: Buffer to copy into.
@return			: none.
*/
void BlockH263::CopyRow(int row, void* buff)
{
	if(buff == NULL)
		return;
	bType* pSrc = &(_pBlk[8*row]);
	memcpy(buff, (const void *)pSrc, 8 * sizeof(bType));
}//end CopyRow.

/** Copy a col from the blk into the buffer.
No checking is done for overflow and is assumed to 
be 8 x bType in size.
@param col	: Block col to copy from.
@param buff	: Buffer to copy into.
@return			: none.
*/
void BlockH263::CopyCol(int col, void* buff)
{
	if(buff == NULL)
		return;
	bType*	pDst	= (bType *)buff;
	int			i			= 0;
	for(int j = 0; j < 8; j++, i += 8)
		pDst[j] = _pBlk[i];
}//end CopyCol.

/** Zero all elements in the block.
No checking is done for overflow and is assumed to 
be 8 x bType in size.
@return			: none.
*/
void BlockH263::Zero(void)
{
	bType* pSrc = &(_pBlk[0]);
	memset((void *)pSrc, 0, 64 * sizeof(bType));
}//end Zero.

