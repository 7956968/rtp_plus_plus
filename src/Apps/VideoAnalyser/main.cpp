#include "VideoAnalyserPch.h"
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/Conversion.h>
#include <cpputil/FileUtil.h>
#include <cpputil/MakeService.h>
#include <cpputil/ServiceManager.h>
#include <rtp++/application/Application.h>
#include <rtp++/media/AsyncStreamMediaSource.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h264/SvcExtensionHeader.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>
#include <rtp++/media/MediaGeneratorService.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/StreamMediaSource.h>
#include <rtp++/media/VirtualVideoDeviceV2.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
using media::MediaSample;

typedef std::map<std::string, std::vector<uint32_t> > LayerSizeMap_t;

namespace po = boost::program_options;

#ifdef SYNC
// This callback is triggered by the CTRL-C and stops the blocked io_service
void stopIoService(boost::asio::io_service& ioService /*, std::shared_ptr<boost::asio::io_service::work>& pWork*/)
{
  LOG(INFO) << "Stopping IO service.";
#ifdef _WIN32
  // TODO: resetting the io_service does not suffice for some reason?!?
  pWork.reset();
  ioService.stop();
#else
  //pWork.reset();
  ioService.stop();
#endif
}

// This handler is called if the end of the media source is reached. The CTRL-C is triggered to end
// the blocked io_service thread
void handleMediaSourceEof(const boost::system::error_code& ec, boost::asio::io_service& ioService/*, std::shared_ptr<boost::asio::io_service::work>& pWork*/)
{
  LOG(INFO) << "End of file reached.";
#ifdef _WIN32
  pWork.reset();
  ioService.stop();
#else
  // pWork.reset();
  // for some reason raise doesn't kill the process cleanly by simulating a CTRL-C
  // kill does seem to do the trick
  //  raise(SIGINT);
  kill(getpid(), SIGINT);
#endif
}

#else
// This callback is triggered by the CTRL-C and stops the blocked io_service
void stopIoService(ServiceManager& serviceManager)
{
  LOG(INFO) << "Stopping IO service.";
#ifdef _WIN32
  serviceManager.stop();
  // TODO: resetting the io_service does not suffice for some reason?!?
  //pWork.reset();
  //ioService.stop();
#else
  serviceManager.stop();
#endif
}

// This handler is called if the end of the media source is reached. The CTRL-C is triggered to end
// the blocked io_service thread
void handleMediaSourceEof(const boost::system::error_code& ec, ServiceManager& serviceManager)
{
  LOG(INFO) << "End of file reached.";
#ifdef _WIN32
  serviceManager.stop();
  //pWork.reset();
  //ioService.stop();
#else
  // pWork.reset();
  // for some reason raise doesn't kill the process cleanly by simulating a CTRL-C
  // kill does seem to do the trick
  //  raise(SIGINT);
  kill(getpid(), SIGINT);
#endif
}
#endif

// This method writes a NAL unit to the outputstream
void writeH264Nalu(const MediaSample& mediaSample, media::h264::NalUnitType eType, std::ofstream& out, bool bExtractBaseLayerOnly)
{
  if (!bExtractBaseLayerOnly)
  {
    media::h264::H264AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out);
  }
  else
  {
    // strip EL NAL units
    if ((eType != media::h264::NUT_CODED_SLICE_EXT && eType != media::h264::NUT_RESERVED_21))
    {
      media::h264::H264AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out);
    }
  }
}

static unsigned g_AccessUnitCount = 0;
static double g_lastSampleTime = 0.0;

// This callback is triggered for every Access Unit read from the media source
boost::system::error_code handleH264VideoAccessUnit(const std::vector<MediaSample>& mediaSamples, std::ofstream& out, bool bVerbose, bool bExtractBaseLayerOnly, LayerSizeMap_t& mLayerInfo)
{
  using namespace media::h264;
  std::ostringstream ostr;
  bool bAccessUnitContainsSvcNalu = false;
  for (const MediaSample& mediaSample: mediaSamples)
  {
    media::h264::NalUnitType eType = media::h264::getNalUnitType(mediaSample);
    ostr << " NUT: " << eType
         << " (" << mediaSample.getDataBuffer().getSize() << ")";

    /* SVC encoder is not updated to latest version of standard: NUT_RESERVED_21 is SVC IDR */
    if (eType == NUT_PREFIX_NAL_UNIT || eType == NUT_CODED_SLICE_EXT || eType == NUT_RESERVED_21)
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
    }
    if (out.is_open())
      writeH264Nalu(mediaSample, eType, out, bExtractBaseLayerOnly);
  }

  static unsigned g_AccessUnitCount = 1;
  VLOG_IF(2, bVerbose) << "Video AU " << g_AccessUnitCount << " received TS: " << mediaSamples[0].getStartTime()
                       << " Count: " << mediaSamples.size()
                       << ostr.str();

  if (bAccessUnitContainsSvcNalu && bVerbose)
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
  }

  ++g_AccessUnitCount;
  return boost::system::error_code();
}

boost::system::error_code handleH265VideoAccessUnit(const std::vector<MediaSample>& mediaSamples, std::ofstream& out, bool bVerbose, bool bExtractBaseLayerOnly, LayerSizeMap_t& mLayerInfo)
{
  using namespace media::h265;
  std::ostringstream ostr1;
  for (size_t i = 0; i < mediaSamples.size(); ++i)
  {
    const MediaSample& mediaSample = mediaSamples[i];

    std::ostringstream ostr;
    media::h265::NalUnitType eType;
    uint32_t uiLayerId = 0;
    uint32_t uiTemporalId = 0;
    media::h265::getNalUnitInfo(mediaSample, eType, uiLayerId, uiTemporalId);

    // commenting this out to have standard print out for H.264 and H.265
#if 0
    VLOG_IF(2, bVerbose) << "Video AU " << g_AccessUnitCount
                         << " (" << i + 1 << "/" << mediaSamples.size()
                         << ") TS: " << mediaSamples[i].getStartTime()
                         << " NUT: " << eType
                         << " Size: " << mediaSample.getDataBuffer().getSize()
                         << " LT: " << uiLayerId << ":" << uiTemporalId
                         << " NUT: " << toString(getNalUnitType(mediaSample))
                            ;
#else
    ostr1 << " NUT: " << media::h265::toString(eType)
         << " (" << mediaSample.getDataBuffer().getSize() << ")";

#endif

    if (out.is_open())
      media::h265::H265AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out);

    ostr.clear();
    ostr.str("");
    ostr << uiLayerId << ":" << uiTemporalId;
    mLayerInfo[ostr.str()].push_back(mediaSample.getDataBuffer().getSize());
  }

  VLOG_IF(2, bVerbose) << "Video AU " << g_AccessUnitCount << " received TS: " << mediaSamples[0].getStartTime()
                       << " Count: " << mediaSamples.size()
                       << ostr1.str();

  ++g_AccessUnitCount;
  g_lastSampleTime = mediaSamples[0].getStartTime();
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
    std::string sMediaType;
    std::string sMediaSink;
    uint32_t uiFps = 0;
    bool bLimitRate = true;
    bool bAu = true;
    bool bVerbose = false;
    bool bExtractBaseLayer = false;
    // flag to allow application to write video meta data to files
    bool bOutputVideoMetaData = false;

    // check program configuration
    // Declare the supported options.
    po::options_description desc("Allowed options");

    desc.add_options()
        ("help,h", "produce help message")
        ("source,s", po::value<std::string>(&sMediaSource)->default_value("stdin"), "media source = [stdin, <<file_name>>]")
        ("format", po::value<std::string>(&sMediaType)->default_value(rfc6184::H264), "media format = [h264 | h265]")
        ("fps,f", po::value<uint32_t>(&uiFps)->default_value(30), "frames per second")
        ("limit-rate,l", po::bool_switch(&bLimitRate)->default_value(false), "Limit frame rate to fps")
        ("vvv,v", po::bool_switch(&bVerbose)->default_value(false), "Verbose [false]")
        ("au", po::value<bool>(&bAu)->default_value(true), "Process Access Unit [true]")
        ("outfile,o", po::value<std::string>(&sMediaSink)->default_value(""), "Outfile")
        ("extractBl,x", po::bool_switch(&bExtractBaseLayer)->default_value(false), "Extract base layer only (for SVC streams) to output file")
        ("output-meta-data,m", po::bool_switch(&bOutputVideoMetaData)->default_value(false), "Output video meta data to files (file source only)")
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

    ApplicationUtil::printApplicationInfo(argc, argv);

    // this application only supports h264 and h265 for now
    // the command line parameter takes precedence
    if (sMediaType != "")
    {
      std::transform(sMediaType.begin(), sMediaType.end(), sMediaType.begin(), ::toupper);
      if (sMediaType != rfc6184::H264 && sMediaType != rfc6190::H264_SVC && sMediaType != rfchevc::H265)
      {
        LOG(ERROR) << "Unsupported media format: " << sMediaType;
        return 1;
      }
    }
    else
    {
      // try and infer from file extension
      std::string sExt = boost::filesystem::extension(sMediaSource);
      if (sExt == ".265")
      {
        sMediaType = rfchevc::H265;
      }
      else if (sExt == ".svcbin")
      {
        sMediaType = rfc6190::H264_SVC;
      }
      else
      {
        // default and accounts for .264 
        LOG(WARNING) << "Format not specified: assuming H264";
        sMediaType = rfc6184::H264;
      }
    }

    std::ofstream out1;
    if (sMediaSink != "")
      out1.open(sMediaSink.c_str(), std::ios_base::out | std::ios_base::binary);

    LayerSizeMap_t mLayerInfo;
    ServiceManager serviceManager;

    if (sMediaSource != "stdin")
    {
      // update the log file: we want to be able to parse this file
      google::SetLogDestination(google::GLOG_INFO, (sMediaSource + ".INFO").c_str());
      google::SetLogDestination(google::GLOG_WARNING, (sMediaSource + ".WARNING").c_str());
      google::SetLogDestination(google::GLOG_ERROR, (sMediaSource + ".ERROR").c_str());

      // use VirtualVideoDeviceV2 to replace AsyncStreamMediaSource
      std::shared_ptr<media::VirtualVideoDeviceV2> pVideoDevice = media::VirtualVideoDeviceV2::create(serviceManager.getIoService(), sMediaSource, sMediaType, uiFps, bLimitRate, false);

      if (pVideoDevice)
      {
        if (sMediaType == rfc6184::H264 || sMediaType == rfc6190::H264_SVC)
          pVideoDevice->setReceiveAccessUnitCB(boost::bind(&handleH264VideoAccessUnit, _1, boost::ref(out1), bVerbose, bExtractBaseLayer, boost::ref(mLayerInfo)));
        else if (sMediaType == rfchevc::H265)
          pVideoDevice->setReceiveAccessUnitCB(boost::bind(&handleH265VideoAccessUnit, _1, boost::ref(out1), bVerbose, bExtractBaseLayer, boost::ref(mLayerInfo)));
        else
          assert(false);
        pVideoDevice->setCompletionHandler(boost::bind(&handleMediaSourceEof, _1,  boost::ref(serviceManager)));

        uint32_t uiServiceId;
        bool bSuccess = serviceManager.registerService(boost::bind(&media::VirtualVideoDeviceV2::start, pVideoDevice.get()),
                                                       boost::bind(&media::VirtualVideoDeviceV2::stop, pVideoDevice.get()),
                                                       uiServiceId);

        if (bSuccess)
        {
          // the user can now specify the handlers for incoming samples.
          startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
                         boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));

          if (bOutputVideoMetaData)
          {
            FileUtil::writeFile("video.source",sMediaSource, false);
            FileUtil::writeFile("video.fps",toString(uiFps), false);
            FileUtil::writeFile("video.aus",toString(g_AccessUnitCount), false);
            // calculate approximate bitrate
            std::ifstream in(sMediaSource);
            in.seekg(0, std::ios_base::end);
            size_t size = in.tellg();
            double dTotalDuration = g_lastSampleTime + (1.0/uiFps);
            double dBitrate = (size/125.0)/dTotalDuration;
            FileUtil::writeFile("video.duration",toString(dTotalDuration), false);
            FileUtil::writeFile("video.bitrate",toString(static_cast<uint32_t>(dBitrate + 0.5)), false);
            FileUtil::writeFile("video.size",toString(size), false);

            // check the media layer info
            if (mLayerInfo.empty())
            {
              VLOG(2) << "TODO: No layer info collected.";
            }
            else
            {
              // calculate total size based on video data e.g. without start codes
              uint32_t uiTotalSize = 0;
              for (auto it = mLayerInfo.begin(); it != mLayerInfo.end(); ++it)
              {
                std::vector<uint32_t>& sizes = it->second;
                uiTotalSize += std::accumulate(sizes.begin(), sizes.end(), 0);
              }
              for (auto it = mLayerInfo.begin(); it != mLayerInfo.end(); ++it)
              {
                std::vector<uint32_t>& sizes = it->second;
                uint32_t uiLayerSum = std::accumulate(sizes.begin(), sizes.end(), 0);
                double dRatio = uiLayerSum*100.0/size;
                VLOG(2) << "Layer " << it->first << " Size: " << uiLayerSum << "/" << uiTotalSize << " Ratio: " << dRatio;
                ostringstream ostr;
                ostr << "video.layer" << it->first << ".size";
                FileUtil::writeFile(ostr.str(),toString(uiLayerSum), false);
              }
            }
          }
        }

      }
      else
      {
        LOG(WARNING) << "Failed to create video device";
        return 1;
      }
    }
    else
    {
      LOG(INFO) << "Expecting input on stdin";
      std::istream* in = &std::cin;

      media::AsyncStreamMediaSource source(serviceManager.getIoService(), boost::ref(*in), sMediaType, 10000, false, 0, uiFps, bLimitRate);
      if (sMediaType == rfc6184::H264 || sMediaType == rfc6190::H264_SVC)
        source.setReceiveAccessUnitCB(boost::bind(&handleH264VideoAccessUnit, _1, boost::ref(out1), bVerbose, bExtractBaseLayer, mLayerInfo));
      else
        source.setReceiveAccessUnitCB(boost::bind(&handleH265VideoAccessUnit, _1, boost::ref(out1), bVerbose, bExtractBaseLayer, mLayerInfo));

      // completion handler
      source.setCompletionHandler(boost::bind(&handleMediaSourceEof, _1,  boost::ref(serviceManager)));

      // register media session with manager
      uint32_t uiServiceId;
      bool bSuccess = serviceManager.registerService(boost::bind(&media::AsyncStreamMediaSource::start, boost::ref(source)),
                                                     boost::bind(&media::AsyncStreamMediaSource::stop, boost::ref(source)),
                                                     uiServiceId);

      if (bSuccess)
      {
        // the user can now specify the handlers for incoming samples.
        startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
                       boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));
      }
    }

    // Close files
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
  return 1;
}

