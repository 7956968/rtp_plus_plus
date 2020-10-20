/** @file

MODULE						: VectorQuantiserCbImpl1

TAG								: VQCBI1

FILE NAME					: VectorQuantiserCbImpl1.h

DESCRIPTION				: A class to implement the IVectorQuantiser interface for video 
										4x4 (dim = 16) dimension values. Derived from the 
                    VectorQuantiserBaseImpl1() base class to implement the Cb 
                    colour component.

COPYRIGHT					: (c)CSIR 2013-2014  all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _VECTORQUANTISERCBIMPL1_H
#define _VECTORQUANTISERCBIMPL1_H

#include "VectorQuantiserBaseImpl1.h"

/*
===========================================================================
	Class definition.
===========================================================================
*/
class VectorQuantiserCbImpl1 : public VectorQuantiserBaseImpl1
{
public:
  VectorQuantiserCbImpl1(void);
  virtual ~VectorQuantiserCbImpl1(void);

	// IVectorQuantisation Interface.

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
  int		Create(void);
  void	Destroy(void);

};// end class VectorQuantiserCbImpl1.

#endif	// _VECTORQUANTISERCBIMPL1_H

