/** @file

MODULE						: LastRunLevelH263List

TAG								: LRLH263L

FILE NAME					: LastRunLevelH263List.h

DESCRIPTION				: A class to hold H.263 last-run-level 8x8 block data.
										Operations on the data are defined elsewhere.

REVISION HISTORY	:
									: 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _LASTRUNLEVELH263LIST_H
#define _LASTRUNLEVELH263LIST_H

#pragma once

/*
---------------------------------------------------------------------------
	Type definition.
---------------------------------------------------------------------------
*/
// Mem space is 1 x 32 bit word.
typedef struct _LastRunLevelH263
{
	unsigned char last;		// [0..1]
	unsigned char run;		// [0..63]
	short					level;	// Dct coeff range.
} LastRunLevelH263;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class LastRunLevelH263List
{
public:
	LastRunLevelH263List(void);
	virtual ~LastRunLevelH263List(void);
	int Ready(void) { return(_ready); }

// Member access.
public:
	LastRunLevelH263* GetList(void) { return(_pList); }
	int GetNumOfElements(void) { return(_numElements); }
	void SetNumOfElements(int numElements) { _numElements = numElements; }

// Data struct members.
protected:
	// The struct list.
	LastRunLevelH263*	_pList;
	int								_numElements;	// Valid elements in list.

	// Mem status.
	int	_ready;

};// end class LastRunLevelH263List.

#endif	//_LASTRUNLEVELH263LIST_H
