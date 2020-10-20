#include "H265AnalyserPch.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/FileUtil.h>
#include <cpputil/MakeService.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>
#include <rtp++/media/MediaGeneratorService.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/StreamMediaSource.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfchevc/Rfchevc.h>
#include <rtp++/util/ApplicationUtil.h>

using namespace std;

namespace po = boost::program_options;

// This callback is triggered by the CTRL-C and stops the blocked io_service
void stopIoService(boost::asio::io_service& ioService, std::shared_ptr<boost::asio::io_service::work>& pWork)
{
  LOG(INFO) << "Stopping IO service.";
#ifdef _WIN32
  // TODO: resetting the io_service does not suffice for some reason?!?
  pWork.reset();
  ioService.stop();
#else
  pWork.reset();
  ioService.stop();
#endif
}

// This handler is called if the end of the media source is reached. The CTRL-C is triggered to end
// the blocked io_service thread
void handleMediaSourceEof(const boost::system::error_code& ec, boost::asio::io_service& ioService, std::shared_ptr<boost::asio::io_service::work>& pWork)
{
  LOG(INFO) << "End of file reached.";
#ifdef _WIN32
  pWork.reset();
  ioService.stop();
#else
  pWork.reset();
  // for some reason raise doesn't kill the process cleanly by simulating a CTRL-C
  // kill does seem to do the trick
  //  raise(SIGINT);
  kill(getpid(), SIGINT);
#endif
}

// This method writes a NAL unit to the outputstream
void writeNalu(const MediaSample& mediaSample, media::h265::NalUnitType eType, std::ofstream& out, bool bExtractBaseLayerOnly)
{
  //if (!bExtractBaseLayerOnly)
  //{
    media::h265::writeMediaSampleNaluToStream(mediaSample, out);
  /*}
  else
  {
    // strip EL NAL units
    if ((eType != media::h264::NUT_CODED_SLICE_EXT && eType != media::h264::NUT_RESERVED_21))
    {
      media::h264::writeMediaSampleNaluToStream(mediaSample, out);
    }
  }*/
}

// This callback is triggered for every Access Unit read from the media source
boost::system::error_code handleVideoAccessUnit(const std::vector<MediaSample>& mediaSamples, std::ofstream& out, bool bVerbose, bool bExtractBaseLayerOnly)
{
  using namespace media::h265;
  std::ostringstream ostr;
  //bool bAccessUnitContainsSvcNalu = false;
  for (const MediaSample& mediaSample: mediaSamples)
  {
    media::h265::NalUnitType eType = media::h265::getNalUnitType(mediaSample);
    ostr << " NUT: " << eType
         << " (" << mediaSample.getDataBuffer().getSize() << ")";

    /* SVC encoder is not updated to latest version of standard: NUT_RESERVED_21 is SVC IDR */
    /*if (eType == NUT_PREFIX_NAL_UNIT || eType == NUT_CODED_SLICE_EXT || eType == NUT_RESERVED_21)
    {
      bAccessUnitContainsSvcNalu = true;
      boost::optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(mediaSample);
      if (pSvcHeader)
      {
        ostr << " SVC idr_flag: " << (int)pSvcHeader->idr_flag << " DQT: " << (int)pSvcHeader->dependency_id << ":" << (int)pSvcHeader->quality_id << ":" << (int)pSvcHeader->temporal_id;
      }
      else
      {
        ostr << " MVC";
      }
    }*/
    if (out.is_open())
      writeNalu(mediaSample, eType, out, false/*bExtractBaseLayerOnly*/);
  }

  static unsigned g_AccessUnitCount = 1;
  VLOG_IF(2, bVerbose) << "Video AU " << g_AccessUnitCount << " received TS: " << mediaSamples[0].getStartTime()
                       << " Count: " << mediaSamples.size()
                       << ostr.str();

  /*if (bAccessUnitContainsSvcNalu && bVerbose)
  {
    ostr.clear();
    ostr.str("");
    std::vector<MediaSample> vBaseLayerSamples;
    std::vector<MediaSample> vEnhancementLayerSamples;
    // split media samples into base and enhancement layer ones before calling packetise
    media::h264::svc::splitMediaSamplesIntoBaseAndEnhancementLayers(mediaSamples, vBaseLayerSamples, vEnhancementLayerSamples);

    ostr << "BL: ";
    for (const MediaSample& mediaSample: vBaseLayerSamples)
    {
      media::h264::NalUnitType eType = media::h264::getNalUnitType(mediaSample);
      ostr << eType << " ";
    }
    VLOG(2) << ostr.str();
    ostr.clear();
    ostr.str("");
    ostr << "EL: ";
    for (const MediaSample& mediaSample: vEnhancementLayerSamples)
    {
      media::h264::NalUnitType eType = media::h264::getNalUnitType(mediaSample);
      ostr << eType << " ";
    }
    VLOG(2) << ostr.str();
  }*/

  ++g_AccessUnitCount;
  return boost::system::error_code();
}

// This callback is triggered for every NAL unit read from the media source
boost::system::error_code handleVideoSample(const MediaSample& mediaSample, std::ofstream& out, bool bVerbose, bool bExtractBaseLayerOnly)
{
  media::h265::NalUnitType eType = media::h265::getNalUnitType(mediaSample);
  VLOG_IF(2, bVerbose) << "Video received TS: " << mediaSample.getStartTime()
                       << " Size: " << mediaSample.getDataBuffer().getSize()
                       << " NAL Unit Type: " << eType << " " << toString(eType);

  if (out.is_open())
    writeNalu(mediaSample, eType, out, bExtractBaseLayerOnly);
  return boost::system::error_code();
}

/**
  * This appication logs information about the processed H.264 stream and can be used
  * as a debugging tool
  */
int main(int argc, char** argv)
{
  google::InitGoogleLogging(argv[0]);

  try
  {
    std::string sMediaSource;
    std::string sMediaSink;
    uint32_t uiFps = 0;
    bool bLimitRate = true;
    bool bAu = true;
    bool bVerbose = false;
    bool bExtractBaseLayer = false;

    // check program configuration
    // Declare the supported options.
    po::options_description desc("Allowed options");

    desc.add_options()
        ("help,h", "produce help message")
        ("source,s", po::value<std::string>(&sMediaSource)->default_value("stdin"), "media source = [stdin, <<file_name>>]")
        ("fps,f", po::value<uint32_t>(&uiFps)->default_value(30), "frames per second [30]")
        ("limit-rate,l", po::value<bool>(&bLimitRate)->default_value(true), "Limit frame rate to fps [true]")
        ("vvv,v", po::bool_switch(&bVerbose)->default_value(false), "Verbose [false]")
        ("au", po::value<bool>(&bAu)->default_value(true), "Process Access Unit [true]")
        ("outfile,o", po::value<std::string>(&sMediaSink)->default_value(""), "Outfile")
        ("extractBl,x", po::bool_switch(&bExtractBaseLayer)->default_value(false), "Extract base layer only (for SVC streams) to output file [false]")
        ;

    po::positional_options_description p;
    p.add("source", 0);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      ostringstream ostr;
      ostr << desc;
      LOG(ERROR) << ostr.str();
      return 1;
    }

    // An H.265 stream can be read from file or from stdin
    bool bStdin = false;
    std::ifstream in1;

    if (sMediaSource == "stdin")
    {
      LOG(INFO) << "Expecting input on stdin";
      bStdin = true;
    }
    else
    {
      if (!ApplicationUtil::initialiseFileMediaSource(in1, sMediaSource))
      {
        return -1;
      }
      else
      {
        // update the log file: we want to be able to parse this file
        google::SetLogDestination(google::GLOG_INFO, (sMediaSource + ".INFO").c_str());
        google::SetLogDestination(google::GLOG_WARNING, (sMediaSource + ".WARNING").c_str());
        google::SetLogDestination(google::GLOG_ERROR, (sMediaSource + ".ERROR").c_str());
      }
    }

    // print cmd arguments to log file
    std::ostringstream ostr;
    for (int i = 0; i < argc; ++i)
      ostr << argv[i] << " ";
    LOG(INFO) << ostr.str();

    // event loop
    boost::asio::io_service io_service;
    std::shared_ptr<boost::asio::io_service::work> pWork(new boost::asio::io_service::work(io_service));

    // If the following services are declared before the io_service their desctructors are not invoked for some
    // reason. Moving the declations after the io_service AND resetting the shared_ptr's
    MediaGeneratorServicePtr pVideoService;
    StreamMediaSourceServicePtr pStreamVideoService;

    std::ofstream out1;
    if (sMediaSink != "")
      out1.open(sMediaSink.c_str(), std::ios_base::out | std::ios_base::binary);

    media::StreamMediaSource::Mode eMode = (bAu) ? media::StreamMediaSource::AccessUnit : media::StreamMediaSource::SingleMediaSample;
    if (bStdin)
    {
      pStreamVideoService = makeService<media::StreamMediaSource>(boost::ref(std::cin), rfchevc::H265, 10000, false, 0, uiFps, bLimitRate, eMode);
      if (bAu)
        pStreamVideoService->get()->setReceiveAccessUnitCB(boost::bind(&handleVideoAccessUnit, _1, boost::ref(out1), bVerbose, bExtractBaseLayer));
      else
        pStreamVideoService->get()->setReceiveMediaCB(boost::bind(&handleVideoSample, _1, boost::ref(out1), bVerbose, bExtractBaseLayer));
      pStreamVideoService->setCompletionHandler(boost::bind(&handleMediaSourceEof, _1,  boost::ref(io_service), pWork));
    }
    else
    {
      pStreamVideoService = makeService<media::StreamMediaSource>(boost::ref(in1), rfchevc::H265, 10000, false, 0, uiFps, bLimitRate, eMode);
      if (bAu)
        pStreamVideoService->get()->setReceiveAccessUnitCB(boost::bind(&handleVideoAccessUnit, _1, boost::ref(out1), bVerbose, bExtractBaseLayer));
      else
        pStreamVideoService->get()->setReceiveMediaCB(boost::bind(&handleVideoSample, _1, boost::ref(out1), bVerbose, bExtractBaseLayer));
      pStreamVideoService->setCompletionHandler(boost::bind(&handleMediaSourceEof, _1,  boost::ref(io_service), pWork));
    }

    if (pVideoService)
      pVideoService->start();
    if (pStreamVideoService)
      pStreamVideoService->start();

    // the user can now specify the handlers for incoming samples.
    startEventLoop(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)),
                   boost::bind(&stopIoService, boost::ref(io_service), pWork));

    LOG(INFO) << "Stopping video service";
    if (pVideoService && pVideoService->isRunning())
      pVideoService->stop();
    if (pStreamVideoService && pStreamVideoService->isRunning())
      pStreamVideoService->stop();

    // On windows the shared_ptr are not allocated if they are declared before the io_service for some reason
    // and the application just hangs
    pVideoService.reset();
    pStreamVideoService.reset();

    // Close files
    if (in1.is_open()) in1.close();
    if (out1.is_open()) out1.close();

    LOG(INFO) << "Done";
    return 0;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: " << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: " << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown exception!!!";
  }
}
