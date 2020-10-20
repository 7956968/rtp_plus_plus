#include "SdpWriterPch.h"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <rtp++/application/Application.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;

namespace po = boost::program_options;

/**
 * @brief   Sample application to test SessionDescription::toString method
 * @author  Ralf Globisch
 */
int main(int argc, char** argv)
{
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  ApplicationUtil::printApplicationInfo(argc, argv);

  try
  {
    std::string sSdp;

    // check program configuration  
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
      ("help", "produce help message")
      ("sdp", po::value<std::string>(&sSdp), "SDP file")
      ;

    po::positional_options_description p;
    p.add("sdp", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) 
    {
      ostringstream ostr;
      ostr << desc;
      LOG(WARNING) << ostr.str();
      return 1;
    }

    boost::optional<rfc4566::SessionDescription> sdp = ApplicationUtil::readSdp(sSdp, true);

    if (!sdp)
    {
      LOG(WARNING) << "Failed to parse SDP: " << sSdp;
      return -1;
    }

    LOG(INFO) << "SDP: " << "\n" << sdp->toString();
    return 0;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown exception!!!";
  }

  return 0;
}

