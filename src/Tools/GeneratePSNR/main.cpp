/*
********************************************************************************

NOTE - One of the two copyright statements below may be chosen
       that applies for the software.

********************************************************************************

This software module was originally developed by

Heiko Schwarz    (Fraunhofer HHI),
Tobias Hinz      (Fraunhofer HHI),
Karsten Suehring (Fraunhofer HHI)

in the course of development of the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video
Coding) for reference purposes and its performance may not have been optimized.
This software module is an implementation of one or more tools as specified by
the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding).

Those intending to use this software module in products are advised that its
use may infringe existing patents. ISO/IEC have no liability for use of this
software module or modifications thereof.

Assurance that the originally developed software module can be used
(1) in the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding) once the
ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding) has been adopted; and
(2) to develop the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding):

To the extent that Fraunhofer HHI owns patent rights that would be required to
make, use, or sell the originally developed software module or portions thereof
included in the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding) in a
conforming product, Fraunhofer HHI will assure the ISO/IEC that it is willing
to negotiate licenses under reasonable and non-discriminatory terms and
conditions with applicants throughout the world.

Fraunhofer HHI retains full right to modify and use the code for its own
purpose, assign or donate the code to a third party and to inhibit third
parties from using the code for products that do not conform to MPEG-related
ITU Recommendations and/or ISO/IEC International Standards.

This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2005.

********************************************************************************

COPYRIGHT AND WARRANTY INFORMATION

Copyright 2005, International Telecommunications Union, Geneva

The Fraunhofer HHI hereby donate this source code to the ITU, with the following
understanding:
    1. Fraunhofer HHI retain the right to do whatever they wish with the
       contributed source code, without limit.
    2. Fraunhofer HHI retain full patent rights (if any exist) in the technical
       content of techniques and algorithms herein.
    3. The ITU shall make this code available to anyone, free of license or
       royalty fees.

DISCLAIMER OF WARRANTY

These software programs are available to the user without any license fee or
royalty on an "as is" basis. The ITU disclaims any and all warranties, whether
express, implied, or statutory, including any implied warranties of
merchantability or of fitness for a particular purpose. In no event shall the
contributor or the ITU be liable for any incidental, punitive, or consequential
damages of any kind whatsoever arising from the use of these programs.

This disclaimer of warranty extends to the user of these programs and user's
customers, employees, agents, transferees, successors, and assigns.

The ITU does not represent or warrant that the programs furnished hereunder are
free of infringement of any third-party patents. Commercial implementations of
ITU-T Recommendations, including shareware, may be subject to royalty fees to
patent holders. Information regarding the ITU-T patent policy is available from
the ITU Web site at http://www.itu.int.

THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.

********************************************************************************
*/

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1

/**
 * 2013.06.26 Ralf Globisch: modifications to loop original YUV file
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtp++/Version.h>
#include <cstdint>
#include <fstream>
#include <iostream>

typedef struct
{
  int             width;
  int             height;
  unsigned char*  data;
} ColorComponent;

typedef struct
{
  ColorComponent lum;
  ColorComponent cb;
  ColorComponent cr;
} YuvFrame;



void createColorComponent( ColorComponent* cc )
{
  if( ! ( cc->data = new unsigned char[cc->width * cc->height]))
  {
    fprintf(stderr, "\nERROR: memory allocation failed!\n\n");
    exit(-1);
  }
}

void deleteColorComponent( ColorComponent* cc )
{
  delete[] cc->data;
  cc->data = NULL;
}



void createFrame( YuvFrame* f, int width, int height )
{
  f->lum.width = width;    f->lum.height  = height;     createColorComponent( &f->lum );
  f->cb .width = width/2;  f->cb .height  = height/2;   createColorComponent( &f->cb  );
  f->cr .width = width/2;  f->cr .height  = height/2;   createColorComponent( &f->cr  );
}

void deleteFrame( YuvFrame* f )
{
  deleteColorComponent( &f->lum );
  deleteColorComponent( &f->cb  );
  deleteColorComponent( &f->cr  );
}

#define FF
void readColorComponent( ColorComponent* cc, FILE* file)
{
  unsigned int size   = cc->width*cc->height;
  unsigned int rsize;
  rsize = fread( cc->data, sizeof(unsigned char), size, file );
  if( size != rsize )
  {
    fprintf(stderr, "\nERROR: while reading from input file! To be read: %d Actual read: %d\n\n", size, rsize);
    perror ("The following error occurred");
    exit(-1);
  }
}

void readColorComponent( ColorComponent* cc, std::ifstream& in1)
{
  static int i = 0;
  static uint64_t total = 0;

  std::streamsize size   = cc->width*cc->height;
  std::streamsize rsize;
  in1.read((char*)cc->data, size);
  rsize = in1.gcount();
  ++i;
  total += rsize;
#if 0
  std::cerr << i << " (" << rsize << " " << total << ") ";
#endif
  if( size != rsize )
  {
    fprintf(stderr, "\nERROR: while reading frame %d from input file! To be read: %d Actual read: %d\n\n", i, size, rsize);
    exit(-1);
  }
}

void writeColorComponent( ColorComponent* cc, FILE* file, int downScale )
{
  int outwidth  = cc->width   >> downScale;
  int outheight = cc->height  >> downScale;
  int wsize;

  for( int i = 0; i < outheight; i++ )
  {
    wsize = fwrite( cc->data+i*cc->width, sizeof(unsigned char), outwidth, file );

    if( outwidth != wsize )
    {
      fprintf(stderr, "\nERROR: while writing to output file!\n\n");
      exit(-1);
    }
  }
}

double psnr( ColorComponent& rec, ColorComponent& org)
{
  unsigned char*  pOrg  = org.data;
  unsigned char*  pRec  = rec.data;
  double          ssd   = 0;
  int             diff;

  for  ( int r = 0; r < rec.height; r++ )
  {
    for( int c = 0; c < rec.width;  c++ )
    {
      diff  = pRec[c] - pOrg[c];
      ssd  += (double)( diff * diff );
    }
    pRec   += rec.width;
    pOrg   += org.width;
  }

  if( ssd == 0.0 )
  {
    return 99.99;
  }
  return ( 10.0 * log10( (double)rec.width * (double)rec.height * 65025.0 / ssd ) );
}

void getPSNR( double& psnrY, double& psnrU, double& psnrV, YuvFrame& rcFrameOrg, YuvFrame& rcFrameRec )
{
  psnrY = psnr( rcFrameRec.lum, rcFrameOrg.lum );
  psnrU = psnr( rcFrameRec.cb,  rcFrameOrg.cb  );
  psnrV = psnr( rcFrameRec.cr,  rcFrameOrg.cr  );
}

void readFrame( YuvFrame* f, FILE* file )
{
  readColorComponent( &f->lum, file );
  readColorComponent( &f->cb,  file );
  readColorComponent( &f->cr,  file );
}

void readFrame( YuvFrame* f, std::ifstream& in1 )
{
  readColorComponent( &f->lum, in1 );
  readColorComponent( &f->cb,  in1 );
  readColorComponent( &f->cr,  in1 );
}

void print_usage_and_exit( int test, const char* name, const char* message = 0 )
{
  if( test )
  {
    if( message )
    {
      fprintf ( stderr, "\nERROR: %s\n", message );
    }
    fprintf (   stderr, "\nUsage: %s <w> <h> <org> <rec> [<t> [<skip> [<strm> <fps> ]]] [-r]\n\n", name );
    fprintf (   stderr, "\t    w : original width  (luma samples)\n" );
    fprintf (   stderr, "\t    h : original height (luma samples)\n" );
    fprintf (   stderr, "\t  org : original file\n" );
    fprintf (   stderr, "\t  rec : reconstructed file\n" );
    fprintf (   stderr, "\t    t : number of temporal downsampling stages (default: 0)\n" );
    fprintf (   stderr, "\t skip : number of frames to skip at start      (default: 0)\n" );
    fprintf (   stderr, "\t strm : coded stream\n" );
    fprintf (   stderr, "\t fps  : frames per second\n" );
    fprintf (   stderr, "\t -r   : return Luma psnr (default: return -1 when failed and 0 otherwise)\n" );
    fprintf (   stderr, "\n" );
    exit    (   -1 );
  }
}


int main(int argc, char *argv[])
{
  fprintf ( stderr,  "GeneratePSNR rtp++ version: %s", RTP_PLUS_PLUS_VERSION_STRING );


#if 0
  std::ifstream intest("reconstructed.yuv", std::ios_base::in | std::ios_base::binary);
  //int width = 1280;
  //int height = 720;
  int frames = 1800;
  int frame_size=1280*720*1.5;
  unsigned char* pBuffer = new unsigned char[frame_size];
  uint64_t total = 0;
  std::cerr << "Reading frames: ";
#if 0
  for (size_t i = 0; i<frames; ++i)
  {
    if (intest.good())
    {
      intest.read((char*)pBuffer, frame_size);
      std::streamsize r = intest.gcount();
      if (r != frame_size)
      {
       std::cerr << "Error reading frame " << i << " to be read: " << frame_size << " Actual read: " << r << std::endl;
        return -1;
      }
      else
      {
        std::cerr << i << " (" << r << ") ";
      }
      total += r;
    }
  }
#else
  int i = 0;
  while (intest.good())
  {
    intest.read((char*)pBuffer, frame_size);
    std::streamsize r = intest.gcount();
    ++i;
    total += r;
    std::cerr << i << " (" << r << " " << total << ") ";
  }
#endif
  intest.close();
  std::cerr << "Done: " << total << std::endl;
  return 0;
#endif

  int     acc = 10000;

  //===== input parameters =====
  int           stream          = 0;
  unsigned int  width           = 0;
  unsigned int  height          = 0;
  unsigned int  temporal_stages = 0;
  unsigned int  skip_at_start   = 0;
  double        fps             = 0.0;
#ifndef FF
  FILE*         org_file        = 0;
  FILE*         rec_file        = 0;
  FILE*         str_file        = 0;
#endif

  //===== variables =====
  unsigned int  index, skip, skip_between, sequence_length;
  int           py, pu, pv, br;
  double        bitrate = 0.0;
  double        psnrY, psnrU, psnrV;
  YuvFrame      cOrgFrame, cRecFrame;
  double        AveragePSNR_Y = 0.0;
  double        AveragePSNR_U = 0.0;
  double        AveragePSNR_V = 0.0;
  int		      	currarg = 5;
  int			      rpsnr   = 0;


  //===== read input parameters =====
  print_usage_and_exit((argc < 5 || (argc > 10 )), argv[0]);
  width             = atoi  ( argv[1] );
  height            = atoi  ( argv[2] );

#ifdef FF
  std::ifstream inOrig(argv[3], std::ios_base::in | std::ios_base::binary);
  std::ifstream inRecon(argv[4], std::ios_base::in | std::ios_base::binary);
  std::ifstream inStream(argv[5], std::ios_base::in | std::ios_base::binary);
#else
  org_file          = fopen ( argv[3], "rb" );
  rec_file          = fopen ( argv[4], "rb" );
#endif


  if(( argc >=  6 ) && strcmp( argv[5], "-r" ) )
  {
    temporal_stages = atoi  ( argv[5] );
    currarg++;
  }
  if(( argc >=  7 ) && strcmp( argv[6], "-r" ) )
  {
    skip_at_start   = atoi  ( argv[6] );
    currarg++;
  }
  if(( argc >= 9 ) && strcmp( argv[7], "-r" ) )
  {
#ifndef FF
    str_file        = fopen ( argv[7], "rb" );
#endif
    print_usage_and_exit(!strcmp( argv[8], "-r" ), argv[0]);
    fps             = atof  ( argv[8] );
    stream          = 1;
    currarg+=2;
  }

  if(currarg < argc )
  {
    if(!strcmp( argv[currarg], "-r" ))
      rpsnr=1;
    else
      print_usage_and_exit (true,argv[0],"Bad number of argument!" );
  }


  //===== check input parameters =====
#ifdef FF
  print_usage_and_exit  ( ! inOrig.good(),                                       argv[0], "Cannot open original file!" );
  print_usage_and_exit  ( ! inRecon.good(),                                       argv[0], "Cannot open reconstructed file!" );
  print_usage_and_exit  ( ! inStream.good() && stream,                             argv[0], "Cannot open stream!" );
#else
  print_usage_and_exit  ( ! org_file,                                       argv[0], "Cannot open original file!" );
  print_usage_and_exit  ( ! rec_file,                                       argv[0], "Cannot open reconstructed file!" );
  print_usage_and_exit  ( ! str_file && stream,                             argv[0], "Cannot open stream!" );
#endif

  print_usage_and_exit  ( fps <= 0.0 && stream,                             argv[0], "Unvalid frames per second!" );

  //======= get number of frames and stream size =======
#ifdef FF
  inOrig.seekg(0, std::ios_base::end);
  uint64_t osize = inOrig.tellg();
  inOrig.seekg(0, std::ios_base::beg);

  inRecon.seekg(0, std::ios_base::end);
  uint64_t rsize = inRecon.tellg();
  inRecon.seekg(0, std::ios_base::beg);

#else
  fseek(    rec_file, 0, SEEK_END );
  fseek(    org_file, 0, SEEK_END );
  size_t rsize = ftell( rec_file );
  size_t osize = ftell( org_file );
  fseek(    rec_file, 0, SEEK_SET );
  fseek(    org_file, 0, SEEK_SET );
#endif

  // The sequence length determines for which frames the PSNR is calculated
  // This will be modified to always use the reconstructed file as a reference
  // Note that the reconstructed file may be many times larger than the original
  // file. In that case the original file should be looped
  uint64_t sequence_length_original = 0;
  uint64_t index_original = 0;
  if (rsize < osize)
    sequence_length = (unsigned int)((double)rsize/(double)((width*height*3)/2));
  else if (rsize > osize)
  {
    sequence_length_original = (unsigned int)((double)osize/(double)((width*height*3)/2));
    sequence_length = (unsigned int)((double)rsize/(double)((width*height*3)/2));
  }
  else
    sequence_length = (unsigned int)((double)osize/(double)((width*height*3)/2));

  if( stream )
  {
#ifdef FF
    inStream.seekg(0, std::ios_base::end);
    uint64_t ssize = inStream.tellg();
    bitrate       = (double)ssize * 8.0 / 1000.0 / ( (double)(sequence_length << temporal_stages) / fps );
    inStream.seekg(0, std::ios_base::beg);
#else
    fseek(  str_file, 0, SEEK_END );
    bitrate       = (double)ftell(str_file) * 8.0 / 1000.0 / ( (double)(sequence_length << temporal_stages) / fps );
    fseek(  str_file, 0, SEEK_SET );
#endif
  }
  skip_between    = ( 1 << temporal_stages ) - 1;

  //===== initialization ======
  createFrame( &cOrgFrame, width, height );
  createFrame( &cRecFrame, width, height );

  //===== loop over frames =====
  for( skip = skip_at_start, index = 0 ;
       index < sequence_length;
       index++, skip = skip_between )
  {
#ifdef FF
    inOrig.seekg(skip*width*height*3/2, std::ios_base::cur);
    readFrame       ( &cOrgFrame, inOrig );
    readFrame       ( &cRecFrame, inRecon );
#else
    fseek( org_file, skip*width*height*3/2, SEEK_CUR);
    readFrame       ( &cOrgFrame, org_file );
    readFrame       ( &cRecFrame, rec_file );
#endif

    getPSNR         ( psnrY, psnrU, psnrV, cOrgFrame, cRecFrame);
    AveragePSNR_Y +=  psnrY;
    AveragePSNR_U +=  psnrU;
    AveragePSNR_V +=  psnrV;

    py = (int)floor( acc * psnrY + 0.5 );
    pu = (int)floor( acc * psnrU + 0.5 );
    pv = (int)floor( acc * psnrV + 0.5 );
    fprintf(stdout,"%d\t""%d,%04d""\t""%d,%04d""\t""%d,%04d""\n",index,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);

    // special case: loop original sequence
    if (sequence_length_original != 0)
    {
      if (index_original == sequence_length_original - 1)
      {
#ifdef FF
        inOrig.seekg(0, std::ios_base::beg);
#else
        fseek(    org_file, 0, SEEK_SET );
#endif
        index_original = 0;
      }
      else
      {
        ++index_original;
      }
    }
  }
  fprintf(stdout,"\n");

  py = (int)floor( acc * AveragePSNR_Y / (double)sequence_length + 0.5 );
  pu = (int)floor( acc * AveragePSNR_U / (double)sequence_length + 0.5 );
  pv = (int)floor( acc * AveragePSNR_V / (double)sequence_length + 0.5 );
  br = (int)floor( acc * bitrate                                 + 0.5 );
  if( stream )
  {
    fprintf(stderr,"%d,%04d""\t""%d,%04d""\t""%d,%04d""\t""%d,%04d""\n",br/acc,br%acc,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
    fprintf(stdout,"%d,%04d""\t""%d,%04d""\t""%d,%04d""\t""%d,%04d""\n",br/acc,br%acc,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
  }
  else
  {
    fprintf(stderr,"total\t""%d,%04d""\t""%d,%04d""\t""%d,%04d""\n",py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
    fprintf(stdout,"total\t""%d,%04d""\t""%d,%04d""\t""%d,%04d""\n",py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
  }

  fprintf(stdout, "\n");


  //===== finish =====
  deleteFrame( &cOrgFrame );
  deleteFrame( &cRecFrame );

#ifdef FF
  inOrig.close();
  inRecon.close();
  inStream.close();
#else
  fclose     ( org_file   );
  fclose     ( rec_file   );
  if( stream )
  {
    fclose   ( str_file   );
  }
#endif

  return (rpsnr*py);
  //return 0;
}

