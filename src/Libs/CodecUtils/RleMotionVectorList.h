/** @file

MODULE						: RleMotionVectorList

TAG								: RMVL

FILE NAME					: RleMotionVectorList.h

DESCRIPTION				: A class to contain motion run-length results in a
										contiguous struct array. The structure of the
										list is defined in a struct.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _RLEMOTIONVECTORLIST_H
#define _RLEMOTIONVECTORLIST_H

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------
	Type definitions.
---------------------------------------------------------------------------
*/
typedef struct _RleMotionVectorType
{
  int		run;				// No. of preceding zero vectors [0,0].
  int		runBits;		// No. of bits in the vlc codeword for the run value.
  int		runCode;		// The run vlc codeword (limited to 32 bit length).
  int		x;					// The motion x coord.
  int		y;					// The motion y coord.
  int		xyBits;			// No. of bits in the total vlc codeword for both coords.
  int		xyCode;			// The motion vlc codeword (limited to 32 bit length).
} RleMotionVectorType;

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class RleMotionVectorList
{
	public:
		RleMotionVectorList(void);
		~RleMotionVectorList(void);

		int		SetMaxLength(int maxLength);
		int		GetMaxLength(void)							{ return(_maxLength); }
		void  SetLength(int length)						{ if(length <= _maxLength) _length = length; }
		int		GetLength(void)									{ return(_length); }
		RleMotionVectorType* GetListPtr(void)	{ return(_pList); }

	private:
		int		_maxLength;				// Length of the list.
		int		_length;					// Num of valid structs in the list.

		// The data is stored in an array of the list structs.
		RleMotionVectorType*	_pList;	
};//end RleMotionVectorList.

#endif
