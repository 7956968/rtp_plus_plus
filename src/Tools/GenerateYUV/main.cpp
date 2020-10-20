#ifdef _WIN32
#include <cmath>
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdio.h>
#include "math.h"
#include <rtp++/Version.h>
#include <cpputil/Conversion.h>

using namespace std;

#define SPEEDUP

#ifdef _WIN32
unsigned int log2( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return ans ;
}
#endif

int main(int argc, char **argv)
{
  cout << "GenerateYUV rtp++ version: " << RTP_PLUS_PLUS_VERSION_STRING << endl;

  if(argc<3)
  {
    cerr << "Usage of the GenerateYUV: ./generateYUV inputFile NumFramesInOrigFile width height numlayer IDRperiod GOPsize outputfile YUVlayer1 ... YUVlayer_numlayer"<<endl;
  }
  else
  {
    for (int i = 0; i < argc; ++i)
    {
      cerr << argv[i] << " ";
    }
    cerr << endl;
  }

  stringstream strValue;
  strValue << argv[5];
  unsigned int numlayer;
  strValue >> numlayer;
  strValue.str(std::string());
  strValue.clear();
  strValue << argv [2];
  unsigned int numFramesInOrigFile;
  strValue >> numFramesInOrigFile;
  strValue.clear();
  strValue << argv [3];
  unsigned int width;
  strValue >> width;
  strValue.clear();
  strValue << argv[4];
  unsigned int height;
  strValue >> height;
  strValue.clear();
  unsigned int IDRperiod;
  strValue << argv[6];
  strValue >> IDRperiod;
  strValue.clear();
  unsigned int GOPsize;
  strValue << argv[7];
  strValue >> GOPsize;
  unsigned int refFrames = log2(GOPsize);

  if(GOPsize > 4)
  {
    cerr << "Error, it only works for GOP sizes up to 4: " << GOPsize << endl;
    return -1;
  }

  const char* outputFile = argv[8];

  int *decReflayer = new int[refFrames];
  for ( int i = 0; i < refFrames; i++)
    decReflayer[i] = -1;

  if (numlayer + 9 != argc)
  {
    cerr << "The number of YUV files does not coincide with the numlayers or some parameters have not been provided. Type --help"<< endl;
    return -1;
  }

#if 1
  const unsigned int frameSize = width*height*1.5;
  char* freezeFrame = new char[frameSize];
  for (int i=0; i<frameSize; i++)
    freezeFrame[i]=127;
  char** readFrames = new char*[numlayer];
  for (int i = 0; i < numlayer; ++i)
  {
    readFrames[i] = new char[frameSize];
  }
#else
  unsigned int frameSize = width * height * 1.5;
  char freezeFrame[frameSize];
  for (int i=0; i<frameSize; i++)
    freezeFrame[i]=127;

  char readFrames[numlayer][frameSize];
#endif

  string line;
  ifstream myfile(argv[1]);
  ifstream *myYUVs = new ifstream [numlayer];

  for(int i=0; i < numlayer; i++)
  {
    myYUVs[i].open(argv[9 + i],ios::in|ios::binary);
    if(!myYUVs[i].good())
      cerr << "Problem opening YUV" << argv[9 + i] << endl;

  }

  ofstream outfile(argv[8], ios_base::out | ios_base::binary);
  ofstream logfile("generateYUV.log", ios_base::out);

  if(myfile.good())
  {
    getline(myfile,line);
    myfile.close();
    unsigned int posInTraceFile = 0;
    unsigned int offsetInOriginalFile = 0;//values used when loop

    string s;
    int j=0;

    while (posInTraceFile != line.length())
    {
      offsetInOriginalFile=posInTraceFile%numFramesInOrigFile;
      if (offsetInOriginalFile == 0 && posInTraceFile!=0)
      {
        for(int i=0; i < numlayer; i++)
        {
#if 1
          myYUVs[i].seekg(0, ios::beg);
#else
          myYUVs[i].close();
          myYUVs[i].open(argv[9+i],ios::in|ios::binary);
#endif
          if(!myYUVs[i].good())
            cerr << "Problem opening YUV" << argv[9+i] << endl;
        }
      }
      offsetInOriginalFile++;
      s = line.substr (posInTraceFile++,1);

      bool dummy;
      j = convert<int>(s, dummy);
#if 0
      strValue.clear();
      strValue << s;
      strValue >> j;
#endif

      if( (offsetInOriginalFile-1) % IDRperiod == 0 )
      {
        decReflayer[0] = j-1;
      }
      else if((offsetInOriginalFile-1) % GOPsize == 0 )
      {
        decReflayer[0] = (decReflayer[0] >= j-1) * (j - 1) + (decReflayer[0] < j - 1) * decReflayer[0];
        j = decReflayer[0] + 1;
      }
      else if( (offsetInOriginalFile - 1) % GOPsize == 1)
      {
        j=j*(j<=decReflayer[0]+1)+(decReflayer[0]+1)*(j>decReflayer[0]+1);
      }
      else if( (offsetInOriginalFile-1) % GOPsize == 2 )
      {
        j=j*(j<=decReflayer[0]+1)+(decReflayer[0]+1)*(j>decReflayer[0]+1);
        decReflayer[1]=j-1;
      }
      else
      {
        j=j*(j<=decReflayer[1]+1)+(decReflayer[1]+1)*(j>decReflayer[0]+1);
      }

      for (int i=0; i < numlayer; i++)
      {
        myYUVs[i].read(readFrames[i], frameSize);
      }

      if( j==0 )
      {
        logfile << "Freeze Frame - pos: " << posInTraceFile-1 << endl;
        outfile.write(freezeFrame, frameSize);
      }
      else
      {
        logfile << "Frame " << posInTraceFile-1 << " received and decoded at layer: " << j-1 <<endl;
        // speed-up
#ifdef SPEEDUP
        memcpy(freezeFrame, readFrames[j-1], frameSize);
#else
        for (int i = 0; i < frameSize; i++)
          freezeFrame[i] = readFrames[j-1][i];
#endif
        outfile.write(readFrames[j-1], frameSize);
      }
    }

    logfile << "YUV file generated successfully";

    outfile.close();
    logfile.close();

#if 1
    for (int i = 0; i < numlayer; ++i)
    {
      delete[] readFrames[i];
    }
    delete[] readFrames;
#endif
    return 0;
  }
  else
  {
    cerr << "Error opening the file" << endl;
    return -1;
  }
}
