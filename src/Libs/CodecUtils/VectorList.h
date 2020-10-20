/** @file

MODULE						: VectorList

TAG								: VL

FILE NAME					: VectorList.h

DESCRIPTION				: A generic class to contain vectors in a contiguous byte 
										mem array. Used when fast access to mem is required 
										otherwise use STL::vector type. The structure of the	
										vectors is determined by its type as either	simple or 
										complex.

REVISION HISTORY	:	29/09/2006 (KF): Added a GetPatternLoc() method to
										give random access to the head of any pattern in 
										the list. 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _VECTORLIST_H
#define _VECTORLIST_H

/*
---------------------------------------------------------------------------
	Vector type description.
---------------------------------------------------------------------------
*/

/* 
Simple structure. SIMPLE:
Order of the elements are a simple continuous stream of bytes representing
a vector element order of: [x0,y0,x1,y1,....] for a 2D vector.
 
Bytes per pattern = dimension * bytes per element.

Complex structure	COMPLEX:
Order of the elements have a structure to them where a group of vectors are
represented as a pattern and an unknown number of vectors in the following 
way: [pattern0, no. of vecs0, x00, y00, x01, y01,..., pattern1,
no. of vecs1, x10, y10, x11, y11, ...].
 
Bytes per pattern = (2 + (max num of vecs per pattern * dimension)) * bytes per element.
*/  

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class VectorList
{
	public:
		VectorList(int type, int dim, int bytesPerElement, int maxVecsPerPattern = 1);
		virtual ~VectorList(void);

		// Set mem size dependent on type, bytes per pattern and length of list.
		virtual int	SetLength(int length);

		int		GetType(void)								{ return(_type); }
		int		GetDimension(void)					{ return(_dim); }
		int		GetPatternSize(void)				{ return(_bytesPerPattern); }
		int		GetMaxVecsPerPattern(void)	{ return(_maxVecsPerPattern); }
		int		GetElementSize(void)				{ return(_bytesPerElement); }
		int		GetLength(void)							{ return(_length); }
		int		GetSize(void)								{ return(_size); }
		void* GetDataPtr(void)						{ return(_pData); }
		void* GetPatternLoc(int pos);

	public:
		static const int	SIMPLE;
		static const int	COMPLEX;

	protected:
		int		_type;							// SIMPLE/COMPLEX.
		int		_dim;								// Vector dimension.
		int		_bytesPerElement;		// Set using sizeof() operator.
		int		_bytesPerPattern;		// Max bytes per vector pattern (COMPLEX). Used as a stride.
		int		_maxVecsPerPattern;	// Max vectors per pattern. (Default = 1 for SIMPLE.)
		int		_length;						// Num of patterns of type _type.	( Is num of vecs for SIMPLE).

		// The data is stored in a byte array whose structure 
		// is dependent on the type and size by bytesPerElement.
		int		_size;						// Size of data array in bytes.
		void*	_pData;	
};//end VectorList.


#endif // _VECTORLIST_H

