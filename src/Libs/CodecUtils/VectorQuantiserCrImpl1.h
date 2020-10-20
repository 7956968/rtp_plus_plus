/** @file

MODULE						: VectorQuantiserCrImpl1

TAG								: VQCRI1

FILE NAME					: VectorQuantiserCrImpl1.h

DESCRIPTION				: A class to implement the IVectorQuantiser interface for video 
										4x4 (dim = 16) dimension values. Derived from the 
                    VectorQuantiserBaseImpl1() base class to implement the Cr 
                    colour component.

COPYRIGHT					: (c)CSIR 2013-2014  all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _VECTORQUANTISERCRIMPL1_H
#define _VECTORQUANTISERCRIMPL1_H

#include "VectorQuantiserBaseImpl1.h"

/*
===========================================================================
	Class definition.
===========================================================================
*/
class VectorQuantiserCrImpl1 : public VectorQuantiserBaseImpl1
{
public:
  VectorQuantiserCrImpl1(void);
  virtual ~VectorQuantiserCrImpl1(void);

	// IVectorQuantisation Interface.

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
  int		Create(void);
  void	Destroy(void);

};// end class VectorQuantiserCrImpl1.

#endif	// _VECTORQUANTISERCRIMPL1_H

