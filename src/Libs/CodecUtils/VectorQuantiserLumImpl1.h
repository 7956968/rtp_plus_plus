/** @file

MODULE						: VectorQuantiserLumImpl1

TAG								: VQLI1

FILE NAME					: VectorQuantiserLumImpl1.h

DESCRIPTION				: A class to implement the IVectorQuantiser interface for video 
										4x4 (dim = 16) dimension values. Derived from the 
                    VectorQuantiserBaseImpl1() base class to implement the Lum 
                    colour component.

COPYRIGHT					: (c)CSIR 2013-2014  all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _VECTORQUANTISERLUMIMPL1_H
#define _VECTORQUANTISERLUMIMPL1_H

#include "VectorQuantiserBaseImpl1.h"

/*
===========================================================================
	Class definition.
===========================================================================
*/
class VectorQuantiserLumImpl1 : public VectorQuantiserBaseImpl1
{
public:
  VectorQuantiserLumImpl1(void);
  virtual ~VectorQuantiserLumImpl1(void);

	// IVectorQuantisation Interface.

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
  int		Create(void);
  void	Destroy(void);

};// end class VectorQuantiserLumImpl1.

#endif	// _VECTORQUANTISERLUMIMPL1_H

