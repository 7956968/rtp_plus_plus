#pragma once
#include <boost/optional.hpp>
#include <cpputil/IBitStream.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/rfc3611/RtcpXr.h>

namespace rtp_plus_plus
{
namespace rfc3611
{
  /**
   * This class is responsible for creating RTCP XR packets
   */
  class RtcpXrFactory
  {
    public:
      static RtcpXrReportBlock createReceiverRttXrReportBlock(const ReceiverReferenceTimeReportBlock& rrtBlock)
      {
          RtcpXrReportBlock xrBlock(XR_RECEIVER_REFERENCE_TIME_BLOCK, 0);
          uint32_t uiLen = XR_RECEIVER_REFERENCE_TIME_BLOCK_LENGTH << 2;
          Buffer rawReceiverRttXrReportBlock(new uint8_t[uiLen], uiLen);
          OBitStream ob(rawReceiverRttXrReportBlock);
          ob.write(rrtBlock.NtpTimestampMsw, 32);
          ob.write(rrtBlock.NtpTimestampLsw, 32);
          xrBlock.setRawXrBlockContent(rawReceiverRttXrReportBlock);
          return xrBlock;
      }

      static RtcpXrReportBlock createDlrrXrReportBlock(const std::vector<DlrrBlock>& vDllrInfo)
      {
          RtcpXrReportBlock xrBlock(XR_DLRR_BLOCK, 0);
          uint32_t uiLen = (XR_DLRR_SUBBLOCK_LENGTH << 2) * vDllrInfo.size();
          Buffer rawReceiverRttXrReportBlock(new uint8_t[uiLen], uiLen);
          OBitStream ob(rawReceiverRttXrReportBlock);
          for (const DlrrBlock& dllr: vDllrInfo)
          {
            ob.write(dllr.SSRC, 32);
            ob.write(dllr.LastRR, 32);
            ob.write(dllr.DLRR, 32);
          }
          xrBlock.setRawXrBlockContent(rawReceiverRttXrReportBlock);
          return xrBlock;
      }

      static RtcpPacketBase::ptr createXrReport(uint32_t uiSSRC)
      {
        RtcpXr::ptr pXr = RtcpXr::create(uiSSRC);
        return pXr;
      }

      static boost::optional<ReceiverReferenceTimeReportBlock> parseReceiverReferenceTimeReportBlock(const RtcpXrReportBlock& xrBlock)
      {
        if (xrBlock.getBlockType() != XR_RECEIVER_REFERENCE_TIME_BLOCK) return boost::optional<ReceiverReferenceTimeReportBlock>();
        if (xrBlock.getBlockLength() != XR_RECEIVER_REFERENCE_TIME_BLOCK_LENGTH) return boost::optional<ReceiverReferenceTimeReportBlock>();
        Buffer rawXrBlockContent = xrBlock.getRawXrBlockContent();
        if (rawXrBlockContent.getSize() != (XR_RECEIVER_REFERENCE_TIME_BLOCK_LENGTH << 2)) return boost::optional<ReceiverReferenceTimeReportBlock>();

        ReceiverReferenceTimeReportBlock rrtBlock;
        IBitStream in1(rawXrBlockContent);
        in1.read(rrtBlock.NtpTimestampMsw, 32);
        in1.read(rrtBlock.NtpTimestampLsw, 32);
        return boost::optional<ReceiverReferenceTimeReportBlock>(rrtBlock);
      }

      static boost::optional<std::vector<DlrrBlock> > parseDlrrReportBlock(const RtcpXrReportBlock& xrBlock)
      {
        if (xrBlock.getBlockType() != XR_DLRR_BLOCK) return boost::optional<std::vector<DlrrBlock> >();
        Buffer rawXrBlockContent = xrBlock.getRawXrBlockContent();
        uint32_t uiNumberOfBlocks = xrBlock.getBlockLength()/3;
        if (uiNumberOfBlocks == 0) return boost::optional<std::vector<DlrrBlock> >();

        uint32_t uiSize = (xrBlock.getBlockLength() << 2);
        if (uiSize != rawXrBlockContent.getSize()) return boost::optional<std::vector<DlrrBlock> >();

        std::vector<DlrrBlock> vBlocks;
        IBitStream in1(rawXrBlockContent);
        for (std::size_t i = 0; i < uiNumberOfBlocks; ++i)
        {
          DlrrBlock block;
          in1.read(block.SSRC, 32);
          in1.read(block.LastRR, 32);
          in1.read(block.DLRR, 32);
          vBlocks.push_back(block);
        }
        return boost::optional<std::vector<DlrrBlock> >(vBlocks);
      }
  };
}

}
