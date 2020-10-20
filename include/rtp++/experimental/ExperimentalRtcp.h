#pragma once
#include <cstdint>
#include <boost/make_shared.hpp>
#include <rtp++/experimental/Nada.h>
#include <rtp++/experimental/Scream.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4585/RtcpFb.h>

namespace rtp_plus_plus {
namespace experimental {

/**
 * @brief The RtcpScreamFb class
 */
class RtcpScreamFb : public rfc4585::RtcpFb
{
public:
  typedef boost::shared_ptr<RtcpScreamFb> ptr;

  // default named constructor
  static ptr create()
  {
    return boost::make_shared<RtcpScreamFb>();
  }

  static ptr create(uint32_t uiJiffy, uint32_t uiHighestSN, uint32_t uiCumuLoss, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpScreamFb>(uiJiffy, uiHighestSN, uiCumuLoss, uiSenderSSRC, uiSourceSSRC);
  }

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr createRaw( uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords )
  {
    return boost::make_shared<RtcpScreamFb>(uiVersion, uiPadding, uiLengthInWords);
  }

  explicit RtcpScreamFb()
    :RtcpFb(TL_FB_SCREAM_JIFFY, rfc4585::PT_RTCP_GENERIC_FEEDBACK)
  {
    setLength(rfc4585::BASIC_FB_LENGTH + 3);
  }

  explicit RtcpScreamFb(uint32_t uiJiffy, uint32_t uiHighestSN, uint32_t uiCumuLoss, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_SCREAM_JIFFY, rfc4585::PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC),
      m_uiJiffy(uiJiffy),
      m_uiHighestSNReceived(uiHighestSN),
      m_uiLost(uiCumuLoss)
  {
    setLength(rfc4585::BASIC_FB_LENGTH + 3);
  }

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  explicit RtcpScreamFb(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords)
    :RtcpFb(uiVersion, uiPadding, TL_FB_SCREAM_JIFFY, rfc4585::PT_RTCP_GENERIC_FEEDBACK, uiLengthInWords),
      m_uiJiffy(0),
      m_uiHighestSNReceived(0),
      m_uiLost(0)
  {

  }

  uint32_t getJiffy() const { return m_uiJiffy; }
  void setJiffy(uint32_t uiJiffy) { m_uiJiffy = uiJiffy; }

  uint32_t getHighestSNReceived() const { return m_uiHighestSNReceived; }
  void setHighestSNReceived(uint32_t uiHighestSNReceived){ m_uiHighestSNReceived = uiHighestSNReceived; }

  uint32_t getLost() const { return m_uiLost; }
  void setLost(uint32_t uiLost) { m_uiLost = uiLost; }

protected:
  virtual void writeFeedbackControlInformation(OBitStream& ob) const
  {
    ob.write(m_uiJiffy, 32);
    ob.write(m_uiHighestSNReceived, 32);
    ob.write(m_uiLost, 32);
  }

  virtual void readFeedbackControlInformation(IBitStream& ib)
  {
    // NB: if the length is less than 2, the following code
    // will cause the application to consume all CPU
    // before running out of memory and crashing
    assert(getLength() >= 2);
    ib.read(m_uiJiffy, 32);
    ib.read(m_uiHighestSNReceived, 32);
    ib.read(m_uiLost, 32);
  }

private:
  uint32_t m_uiJiffy;
  uint32_t m_uiHighestSNReceived;
  uint32_t m_uiLost;
};

/**
 * @brief The RtcpNadaFb class
 */
class RtcpNadaFb : public rfc4585::RtcpFb
{
public:
  typedef boost::shared_ptr<RtcpNadaFb> ptr;

  // default named constructor
  static ptr create()
  {
    return boost::make_shared<RtcpNadaFb>();
  }

  static ptr create(int64_t x_n, uint32_t ui_r_recv, uint32_t ui_rmode, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpNadaFb>(x_n, ui_r_recv, ui_rmode, uiSenderSSRC, uiSourceSSRC);
  }

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr createRaw( uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords )
  {
    return boost::make_shared<RtcpNadaFb>(uiVersion, uiPadding, uiLengthInWords);
  }

  explicit RtcpNadaFb()
    :RtcpFb(TL_FB_NADA, rfc4585::PT_RTCP_GENERIC_FEEDBACK)
  {
    setLength(rfc4585::BASIC_FB_LENGTH + 4);
  }

  explicit RtcpNadaFb(int64_t x_n, uint32_t ui_r_recv, uint32_t ui_rmode, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_NADA, rfc4585::PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC),
      m_i_x_n(x_n),
      m_ui_r_recv(ui_r_recv),
      m_ui_rmode(ui_rmode)
  {
    setLength(rfc4585::BASIC_FB_LENGTH + 4);
  }

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  explicit RtcpNadaFb(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords)
    :RtcpFb(uiVersion, uiPadding, TL_FB_NADA, rfc4585::PT_RTCP_GENERIC_FEEDBACK, uiLengthInWords),
      m_i_x_n(0),
      m_ui_r_recv(0),
      m_ui_rmode(0)
  {

  }

  int64_t get_x_n() const { return m_i_x_n; }
  void set_x_n(uint64_t x_n) { m_i_x_n = x_n; }

  uint32_t get_r_recv() const { return m_ui_r_recv; }
  void set_r_reecv(uint32_t ui_r_recv){ m_ui_r_recv = ui_r_recv; }

  uint32_t get_rmode() const { return m_ui_rmode; }
  void set_rmode(uint32_t ui_rmode) { m_ui_rmode = ui_rmode; }

protected:
  virtual void writeFeedbackControlInformation(OBitStream& ob) const
  {
    uint32_t uiMsw = 0;
    uint32_t uiLsw = 0;
    RtpTime::split((int64_t)m_i_x_n, uiMsw, uiLsw);
    ob.write(uiMsw, 32);
    ob.write(uiLsw, 32);
    ob.write(m_ui_r_recv, 32);
    ob.write(m_ui_rmode, 32);
  }

  virtual void readFeedbackControlInformation(IBitStream& ib)
  {
    // NB: if the length is less than 2, the following code
    // will cause the application to consume all CPU
    // before running out of memory and crashing
    assert(getLength() >= 2);
    //
    uint32_t uiMsw = 0;
    uint32_t uiLsw = 0;
    ib.read(uiMsw, 32);
    ib.read(uiLsw, 32);
    m_i_x_n = RtpTime::join(uiMsw, uiLsw);
    ib.read(m_ui_r_recv, 32);
    ib.read(m_ui_rmode, 32);
  }

private:
  // aggregate congestion signal
  int64_t m_i_x_n;
  // kbps
  uint32_t m_ui_r_recv;
  // rmode
  uint32_t m_ui_rmode;
};

/**
 * @brief The RtcpRembFb class
 */
class RtcpRembFb
{
public:
  static const uint32_t REMB_ID = ('R' << 24) | ('E' << 16) | ('M' << 8) | 'B';

  static boost::optional<RtcpRembFb> parseFromAppData(const std::string& sAppData)
  {
    IBitStream ib(sAppData);
    uint32_t uiExp = 0;
    uint32_t uiMantissa = 0;
    uint32_t uiUniqueId = 0;
    uint32_t uiNumSsrcs = 0;
    uint32_t uiBps = 0;
    uint32_t uiSenderSsrc = 0;
    ib.read(uiUniqueId, 32);
    if (uiUniqueId == REMB_ID)
    {
      ib.read(uiNumSsrcs, 8);
      ib.read(uiExp, 6);
      ib.read(uiMantissa, 18);
      uiBps = uiMantissa << uiExp;
      ib.read(uiSenderSsrc, 32);
      VLOG(12) << "REMB: " << uiUniqueId << " 'REMB': " << REMB_ID ;
      return boost::optional<RtcpRembFb>(RtcpRembFb(uiBps, uiNumSsrcs, uiSenderSsrc));
    }
    else
    {
      VLOG(2) << "Failed to match REMB: " << uiUniqueId << " 'REMB': " << REMB_ID ;
      return boost::optional<RtcpRembFb>();
    }
  }

  explicit RtcpRembFb()
    :m_uiUniqueId(REMB_ID),
      m_uiBps(0),
      m_uiNumSsrcs(0),
      m_uiSsrcFeedback(0)
  {

  }

  explicit RtcpRembFb(uint32_t uiBps, uint32_t uiNumSsrcs, uint32_t uiSsrcFb)
    :m_uiUniqueId(REMB_ID),
      m_uiBps(uiBps),
      m_uiNumSsrcs(uiNumSsrcs),
      m_uiSsrcFeedback(uiSsrcFb)
  {

  }

  uint32_t getBps() const { return m_uiBps; }
  void setBps(uint32_t uiBps) { m_uiBps = uiBps; }

  uint32_t getNumSsrcs() const { return m_uiNumSsrcs; }
  void setNumSsrcs(uint32_t uiNumSsrcs) { m_uiNumSsrcs = uiNumSsrcs; }

  uint32_t getSsrcFb() const { return m_uiSsrcFeedback; }
  void setSsrcFb(uint32_t uiSsrcFb) { m_uiSsrcFeedback = uiSsrcFb; }

  std::string toString() const
  {
    OBitStream ob(12);
    ob.write(m_uiUniqueId, 32);
    ob.write(m_uiNumSsrcs, 8);
    uint8_t uiExp = 0;
    uint32_t uiMantissa = 0;
    ComputeMantissaAnd6bitBase2Exponent(m_uiBps, 18, &uiMantissa, &uiExp);
    ob.write(uiExp, 6);
    ob.write(uiMantissa, 18);
    ob.write(m_uiSsrcFeedback, 32);
    return ob.data().toStdString();
  }

private:

  // from chromium codebase
  void ComputeMantissaAnd6bitBase2Exponent(uint32_t input_base10,
                                           uint8_t bits_mantissa,
                                           uint32_t* mantissa,
                                           uint8_t* exp) const {
    // input_base10 = mantissa * 2^exp
    assert(bits_mantissa <= 32);
    uint32_t mantissa_max = (1 << bits_mantissa) - 1;
    uint8_t exponent = 0;
    for (uint32_t i = 0; i < 64; ++i) {
      if (input_base10 <= (mantissa_max << i)) {
        exponent = i;
        break;
      }
    }
    *exp = exponent;
    *mantissa = (input_base10 >> exponent);
  }

  uint32_t m_uiUniqueId;
  uint32_t m_uiBps;
  uint32_t m_uiNumSsrcs;
  uint32_t m_uiSsrcFeedback;
};


} // experimental
} // rtp_plus_plus
