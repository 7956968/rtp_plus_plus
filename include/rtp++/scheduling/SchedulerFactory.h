#pragma once
#include <cpputil/GenericParameters.h>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/scheduling/AckBasedRtpScheduler.h>
#include <rtp++/scheduling/DistributedScheduler.h>
#include <rtp++/scheduling/NadaScheduler.h>
#include <rtp++/scheduling/PacedRtpScheduler.h>
#include <rtp++/scheduling/RandomScheduler.h>
#include <rtp++/scheduling/RoundRobinScheduler.h>
#include <rtp++/scheduling/RtpScheduler.h>
#include <rtp++/scheduling/ScreamScheduler.h>
#include <rtp++/scheduling/XyzScheduler.h>
#include <rtp++/TransmissionManager.h>

namespace rtp_plus_plus
{

class SchedulerFactory
{
public:

  // schedulers
  static const uint32_t BASE_SP_SCHEDULER = 0;    // simple scheduler
  static const uint32_t PACED_SP_SCHEDULER = 1;   // paced scheduler
  static const uint32_t ACK_SP_SCHEDULER = 2;     // ack-based scheduler
  static const uint32_t SCREAM_SP_SCHEDULER = 3;  // scream-based scheduler
  static const uint32_t NADA_SP_SCHEDULER = 4;  // NADA-based scheduler
  static const uint32_t BASE_MP_SCHEDULER = 10;   // round-robin scheduler
  static const uint32_t RND_MP_SCHEDULER = 11;    // rnd scheduler
  static const uint32_t XYZ_MP_SCHEDULER = 12;    // distribution-based scheduler
  static const uint32_t DST_MP_SCHEDULER = 13;    // distribution-based scheduler

  static std::unique_ptr<RtpScheduler> create(RtpSession::ptr pRtpSession,
                                              TransmissionManager& transmissionManager,
                                              ICooperativeCodec* pCooperative,
                                              uint32_t uiType,
                                              boost::asio::io_service& ioService,
                                              const GenericParameters& applicationParameters,
                                              uint16_t uiFlows = 0, const std::string& sSchedulingParameter = "")
  {
    std::unique_ptr<RtpScheduler> pScheduler;
    switch (uiType)
    {
      case BASE_SP_SCHEDULER:
      {
        VLOG(2) << "Scheduler factory: creating RtpScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new RtpScheduler(pRtpSession));
        break;
      }
      case PACED_SP_SCHEDULER:
      {
        VLOG(2) << "Scheduler factory: creating RtpScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new PacedRtpScheduler(pRtpSession, ioService ));
        break;
      }
      case ACK_SP_SCHEDULER:
      {
        auto fps = applicationParameters.getUintParameter(app::ApplicationParameters::video_fps);
        auto minKbps = applicationParameters.getUintParameter(app::ApplicationParameters::video_min_kbps);
        auto maxKbps = applicationParameters.getUintParameter(app::ApplicationParameters::video_max_kbps);
        VLOG(2) << "Scheduler factory: creating AckBasedRtpScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new AckBasedRtpScheduler(pRtpSession, transmissionManager, pCooperative, ioService, *fps, *minKbps, *maxKbps, sSchedulingParameter ));
        break;
      }
#ifdef ENABLE_SCREAM
      case SCREAM_SP_SCHEDULER:
      {
        auto fps = applicationParameters.getUintParameter(app::ApplicationParameters::video_fps);
        auto minKbps = applicationParameters.getUintParameter(app::ApplicationParameters::video_min_kbps);
        auto maxKbps = applicationParameters.getUintParameter(app::ApplicationParameters::video_max_kbps);
        VLOG(2) << "Scheduler factory: creating ScreamScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new ScreamScheduler(pRtpSession, transmissionManager, pCooperative, ioService, *fps, *minKbps * 1000, *maxKbps * 1000));
        break;
      }
#endif
      case NADA_SP_SCHEDULER:
      {
        auto fps = applicationParameters.getUintParameter(app::ApplicationParameters::video_fps);
        auto minKbps = applicationParameters.getUintParameter(app::ApplicationParameters::video_min_kbps);
        auto maxKbps = applicationParameters.getUintParameter(app::ApplicationParameters::video_max_kbps);
        VLOG(2) << "Scheduler factory: creating NadaScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new NadaScheduler(pRtpSession, transmissionManager, pCooperative, ioService, *fps, *minKbps * 1000, *maxKbps * 1000));
        break;
      }
      case BASE_MP_SCHEDULER:
      {
        VLOG(2) << "Scheduler factory: creating RoundRobinScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new RoundRobinScheduler(pRtpSession, uiFlows));
        break;
      }
      case RND_MP_SCHEDULER:
      {
        VLOG(2) << "Scheduler factory: creating RandomScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new RandomScheduler(pRtpSession, uiFlows));
        break;
      }
      case XYZ_MP_SCHEDULER:
      {
        VLOG(2) << "Scheduler factory: creating XyzScheduler";
        // this could fail if the format string is invalid
        pScheduler = std::unique_ptr<RtpScheduler>(XyzScheduler::create(sSchedulingParameter, pRtpSession, uiFlows));
        break;
      }
      case DST_MP_SCHEDULER:  // TODO
      {
        VLOG(2) << "Scheduler factory: creating DistributedScheduler";
        pScheduler = std::move(DistributedScheduler::create(sSchedulingParameter, pRtpSession, uiFlows));
        break;
      }
    }
    if (!pScheduler)
    {
      VLOG(2) << "Scheduler factory: creating default MPRTP scheduler - RoundRobinScheduler";
      // default schedulers
      if (pRtpSession->isMpRtpSession())
      {
        pScheduler = std::unique_ptr<RtpScheduler>(new RoundRobinScheduler(pRtpSession, uiFlows));
      }
      else
      {
        VLOG(2) << "Scheduler factory: creating default single path scheduler - RtpScheduler";
        pScheduler = std::unique_ptr<RtpScheduler>(new RtpScheduler(pRtpSession));
      }
    }
    return pScheduler;
  }
};

} // rtp_plus_plus
