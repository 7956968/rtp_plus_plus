/** @file

MODULE						: ImagePlaneEncoderFastImpl1

TAG								: IPEFI1

FILE NAME					: ImagePlaneEncoderFastImpl1.cpp

DESCRIPTION				: A faster implementation of the base class
										ImagePlaneEncoder by bin'ing the distortion
										selection.

REVISION HISTORY	:	

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

#include <memory.h>
#include <string.h>
#include <stdlib.h>

#include	"ImagePlaneEncoderFastImpl1.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneEncoderFastImpl1::ImagePlaneEncoderFastImpl1(void)
{
	// Fast encoding histograms.
	_threshold		= NULL;
	_fThreshold		= NULL;
	_energyHist		= NULL;
	_energy				= NULL;
}//end constructor.

ImagePlaneEncoderFastImpl1::~ImagePlaneEncoderFastImpl1(void)
{
	Destroy();
}//end destructor.

int ImagePlaneEncoderFastImpl1::Create(	cpeType* srcLum,			cpeType* refLum, 
																				int			 lumWidth,		int			 lumHeight,
																				cpeType* srcChrU,			cpeType* refChrU,
																				cpeType* srcChrV,			cpeType* refChrV,
																				int			 chrWidth,		int			 chrHeight,
																				int			 vecLumWidth,	int			 vecLumHeight,
																				int			 vecChrWidth,	int			 vecChrHeight)
{
	// Call base class.
	int ret = ImagePlaneEncoder::Create(srcLum,	refLum, lumWidth,	lumHeight,
																			srcChrU, refChrU, srcChrV, refChrV,
																			chrWidth,	chrHeight, 
																			vecLumWidth, vecLumHeight, vecChrWidth, vecChrHeight);
	if(!ret)
		return(0);

  // Prepare some mem for Energy measurement arrays needed in FindThresholdEnergies().
	int length	= (lumWidth/vecLumWidth) * (lumHeight/vecLumHeight); // Total num vectors.
  _energy			= new int[length];
	_threshold	= new int[IPEFI1_THRESHOLDS];
	_fThreshold = new double[IPEFI1_THRESHOLDS];
	_energyHist = new int[length/5];
	if( (_energy == NULL)||(_threshold == NULL)||(_fThreshold == NULL)||(_energyHist == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_energy...

	return(1);
}//end Create.

void ImagePlaneEncoderFastImpl1::Destroy(void)
{
  if(_energy != NULL)
    delete[] _energy;
  _energy = NULL;

	if (_threshold != NULL)
		delete[] _threshold;
	_threshold = NULL;

	if (_fThreshold != NULL)
		delete[] _fThreshold;
	_fThreshold = NULL;

	if (_energyHist != NULL)
		delete[] _energyHist;
	_energyHist = NULL;

	// Call base class.
	ImagePlaneEncoder::Destroy();
}//end Destroy.

/*
--------------------------------------------------------------------------
  Implementation. 
--------------------------------------------------------------------------
*/

/** Encode the image.
To be implemented by each implementation. No base implementation provided.
Add up to allowedBits of quantised and vlc'ed samples to the bit stream
in sequential colour planes with an endOfPlaneMarkerCode delimiter.
@param	allowedBits	: Stop before exceeding this bit limit.
@param	bitsUsed		: Return the exact num of bits used.
@return							: 0 = all coded, 1 = bit limit reached, 2 = coding failure.
*/
int ImagePlaneEncoderFastImpl1::Encode(int allowedBits, int* bitsUsed)
{
	int								colour;
	int								i,pos,length;
	CPE_CodedVector*	vecList;

	// First, code each vector and load the coded vector structures
	// for each colour partition.
	for(colour = IPE_LUM; colour < IPE_COLOUR_PLANES; colour++)
	{
		length	= _pColourPlane[colour]->GetVecListLength();
		vecList = _pColourPlane[colour]->GetEncodingList();

		int maxEnergy = 0;

		// Scan each vector for the given colour.
		for(pos = 0; pos < length; pos++)
		{
			// Set the encodings for this vector with vq and vlc.
			_pColourPlane[colour]->SetEncoding(pos, _pVectorQuantiser, _pVqVlcEncoder);

			// Collect thresholding information.
			_energy[pos] = vecList[pos].weightedDistortion;
			if(_energy[pos] > maxEnergy)
				maxEnergy = _energy[pos];
  
		}//end for pos...

		// Calculate the threshold values for this colour partition.
		FindThresholdEnergies(_energy, maxEnergy, pos);

		// Load thresholding array.
		int* threshList = _pColourPlane[colour]->GetThesholdList();
		for(i = 0; i < IPEFI1_THRESHOLDS; i++)
			threshList[i] = _threshold[i];	//_threshold is global loaded in FindThresholdEnergies().

	}//end for colour...

	// Continually insert the next largest until available bits are reached.
	int availableBits = allowedBits - ((IPE_COLOUR_PLANES - 1) * _endOfPlaneMarkerNumBits);

  int	runbits, bin;
	int	distortionthreshold;
	int	bitcount = 0;
	for(bin = 30; (bin >= 0)&&(bitcount < availableBits); bin--)
	{
		// Pick from the list any vector with greater distortion than that defined by the bin.
		for(colour = IPE_LUM; (colour < IPE_COLOUR_PLANES)&&(bitcount < availableBits); colour++)
		{
			// Which vector struct we are dealing with.
			length				= _pColourPlane[colour]->GetVecListLength();
			vecList				= _pColourPlane[colour]->GetEncodingList();
			_pRunVlcEncoder->SetEsc(_pColourPlane[colour]->GetEscRunBits(),
															_pColourPlane[colour]->GetEscRunMask());
			distortionthreshold	= (_pColourPlane[colour]->GetThesholdList())[bin];

			for(pos = 0; (pos < length)&&(bitcount < availableBits); pos++)
			{
				// Only check those that are to be included.
				if(vecList[pos].includeFlag)
				{
					if(vecList[pos].weightedDistortion > distortionthreshold)
					{
						// Add the vector to the list and recalculate the bit count.
						// Corresponding vector struct that we are dealing with.

						// Still not excluded?
						if(vecList[pos].includeFlag)
						{
							// Run count before and after index to be inserted.
							int beforerun = 0;
							int afterrun  = 0;
							i = pos - 1;	// Before.
							while((i >= 0)&&(!vecList[i].codedFlag))
							{
								beforerun++;
								i--;
							}//end while i...

							runbits = _pRunVlcEncoder->Encode(beforerun);
  					
							i = pos + 1;	// After.
							while((i < length)&&(!vecList[i].codedFlag))
							{
								afterrun++;
								i++;
							}//end while i...
							// Only interested in the after run if it did not reach end of sequence.
							if(i < length)
								runbits += _pRunVlcEncoder->Encode(afterrun);

							// Add the new run bits and vq bits.
							bitcount += (runbits + vecList[pos].numCodeBits);
							vecList[pos].codedFlag	 = 1; // Mark as coded.
							vecList[pos].includeFlag = 0; // And exclude from further decisions.
  					
							// Remove old run bits if not at end of sequence.
							if(i < length)
								bitcount -= _pRunVlcEncoder->Encode(beforerun + afterrun + 1);
						
							if(bitcount < 0) // Bounds check.
								bitcount = 0;
						}//end if IncludeFlag...

					}//end if weightedDistortion...
				}//end if includeFlag...
			}//end for pos...

		}//end for colour...

	}//end for bin...

	return(WriteToStreamWithDecode(allowedBits, bitsUsed));

}//end Encode.

/*
--------------------------------------------------------------------------
  Private utilities. 
--------------------------------------------------------------------------
*/
void ImagePlaneEncoderFastImpl1::FindThresholdEnergies(	int*	energy, 
																												int		maxEnergy,
																												int		seqLength	)
{
	int i,j,seq;

	// Allocate histogram
	// Histogram length is proportional to the sequency length
	// (i.e. number of energy samples). The energies are 
	// effectively binned into the histogram bands
	int histLength = seqLength / 5;
		
	for (i = 0; i < histLength; ++i)
		_energyHist[i] = 0;

	// Construct the histogram
	// histogram index is obtained from the current and 
	// maximum energy value identified before
	double fmax_energy = (double) maxEnergy;
	double fHistLength = (double) histLength;
	double step = fmax_energy / fHistLength;
	int		 index;
	for(seq = 0; seq < seqLength; seq++)
	{
		if (energy[seq] > 0)
		{
			index = (int) ((double) energy[seq] / fmax_energy * fHistLength);
			if (index >= histLength)
				index = histLength - 1;

			++_energyHist[index];
		}//end if energy[seq]...
	}//end for seq...
		
	// Multiply each histogram value by its index as for example
	// bin 2 carries twice as much energy as bin 1
	for(i = 0; i < histLength; ++i)
		_energyHist[i] = _energyHist[i] * (i+1);

	// Calculate the threshold values (i.e. energy values) 
	// separate the cumulative energy into the bands
	// of equal energies

	// first find the overall energy
	int data_sum = 0;
	for (i = 0; i < histLength; ++i)
		data_sum += _energyHist[i];
		
	// Identify the required energy step
	double data_step = (double) data_sum / (double) IPEFI1_THRESHOLDS;

	double up_limit		= data_step;
	double cum_energy = 0;
	int		 counter		= 0;

	// Identify the threshold values
	for(i = 0; i < histLength; ++i)
	{
		cum_energy = cum_energy + (double) _energyHist[i];
    
		while(cum_energy > up_limit)
		{
			_fThreshold[counter++] = (double) (i+1);
      up_limit = up_limit + data_step;
		}//end while cum_energy...
	}//end for i...
		
	// Place the last threshold value if necessary
	if (_fThreshold[counter-1] != histLength || counter < IPEFI1_THRESHOLDS)
		_fThreshold[counter] = (double) histLength;

	// Linearly distribute the same threshold values
	// (not the most accurate but it works)
	int same = 0;
	for(i = 0; i < (IPEFI1_THRESHOLDS - 1); ++i)
	{
		if(_fThreshold[i] == _fThreshold[i+1])
			++same;
		else
		{
			if(same > 0)
			{
				double fr = (double) (1.0 / ((double) same + 1.0));
        int co = 1;
        for (j = i-1; j >= i-same; --j,++co)
					_fThreshold[j] = _fThreshold[i] - ((double)co * fr);
			}//end if same...
      same = 0;
		}//end else...
	}//end for i...

	if (same > 0)
	{
		double fr = (double) (1.0 / ((double) same + 1.0));
    int co = 1;
    for (j = (i - 1); j >= (i - same); --j, ++co)
			_fThreshold[j] = _fThreshold[i] - ((double)co * fr);
	}//end if same...

		
	// De-normalise the threshold values
	for (i = 0; i < IPEFI1_THRESHOLDS; ++i)
		_threshold[i] = (int) (_fThreshold[i] / histLength * fmax_energy);

}//end FindThresholdEnergies.

