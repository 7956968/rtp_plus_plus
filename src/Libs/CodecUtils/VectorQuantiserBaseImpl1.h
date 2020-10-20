/** @file

MODULE						: VectorQuantiserBaseImpl1

TAG								: VQBI1

FILE NAME					: VectorQuantiserBaseImpl1.h

DESCRIPTION				: A base class to implement the IVectorQuantiser interface for video 
										4x4 (dim = 16) dimension values on colour components. The quantisation
                    and inverse quantisation processes are typically common across all
                    codebooks and are therefore grouped into this class. 

COPYRIGHT					: (c)CSIR 2013-2014  all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _VECTORQUANTISERBASEIMPL1_H
#define _VECTORQUANTISERBASEIMPL1_H

#include "IVectorQuantiser.h"
#include "IVQCodebookDim16.h"

/// Structure for fast algorithms to hold intermediate data per vector.
typedef struct _VQBI1_VECTOR_PARAMS_STRUCT
{
  int     index;
	int     sum;
  double  var;
} VQBI1_VECTOR_PARAMS_STRUCT;

/*
===========================================================================
	Class definition.
===========================================================================
*/
class VectorQuantiserBaseImpl1 : public IVectorQuantiser
{
public:
  VectorQuantiserBaseImpl1(void);
  virtual ~VectorQuantiserBaseImpl1(void) {}

	// IVectorQuantisation Interface.

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
  virtual int		Create(void)  { return(1); }
  virtual void	Destroy(void) {}

	/** Get the implementation vector dimension.
	This is defined as a constant of the code book.
	@return	: Vector dimension.
	*/
  virtual int	GetDimension(void) { return(16); }

	/** Get the number of vectors in the code book.
	This is defined as a constant of the code book.
	@return	: Code book length.
	*/
  virtual int GetCodeBookLength(void) 
  {
    if(_pVQCodebook != NULL)
      if(_pVQCodebook[_qp] != NULL)
        return(_pVQCodebook[_qp]->GetCodeBookLength());
    return(0);
  }

	/** Quantise the input vector.
	Match the input vector with the implementation code book element
	that produces the smallest distortion.
	@param vector			:	Input vector to match in the code book.
	@param distortion	:	Distortion of the winning vector.
  @param codebook   : Use this code book.
	@return						: Winning vector index.
	*/
	virtual int Quantise(const void* vector, int* distortion);

	/** Inverse quantise the index.
	Return a pointer to the code book vector referenced by the input
	index.
	@param index	  : Code book vector index.
  @param codebook : Use this code book.
	@return				  : Reference to the code book vector.
	*/
	virtual const void* InverseQuantise(int index);

  /** Select the codebook.
  Allow the vector quantiser object to encapsulate multiple codebooks. This
  implementation has a seperate codebook for each QP value.
  @param codebook : Activate this codebook.
  @return         : none
  */
  virtual void SetCodebook(int codebook) { _qp = codebook; }

protected:
  /// QP references the codebook to use.
  int   _qp;
  /// List of VQ codebook references, one per QP.
  IVQCodebookDim16**  _pVQCodebook;
  /// List of VQ codebook parameters per QP.
  VQBI1_VECTOR_PARAMS_STRUCT** _pVQParam;

};// end class VectorQuantiserBaseImpl1.

#endif	// _VECTORQUANTISERBASEIMPL1_H

