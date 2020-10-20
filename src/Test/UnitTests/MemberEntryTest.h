#pragma once
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3550/MemberEntry.h>

#define MEMBER_ENTRY_TEST_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace test
{

class MemberEntryTest
{
public:
  static void test()
  {
    test_sequenceNumberProbation1();
    test_sequenceNumberClassification();
  }

  static void test_sequenceNumberClassification()
  {
    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "MAX dropout test";
    // Max drop out test
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = 1;
      member.initSequence(uiSN++);
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      // probation is over
      uiSN += rfc3550::MAX_DROPOUT;
      member.update_seq(uiSN++);
      // this should cause a re-init
      member.update_seq(uiSN++);
      BOOST_CHECK_EQUAL(member.received, 1);
    }
    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "Wrap around test";
    // Wrap around 1: normal wrap around
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = UINT16_MAX - 2;
      member.initSequence(uiSN++);
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      // probation is over
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      BOOST_CHECK_EQUAL(member.cycles, 1 << 16);
    }
    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "Wrap around test2";
    // Wrap around 2: big jump on wrap around
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = UINT16_MAX - 2;
      member.initSequence(uiSN++);
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      // probation is over
      uiSN += 100;
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      BOOST_CHECK_EQUAL(member.cycles, 1 << 16);
    }

    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "Dup test 1";
    // DUP test
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = 100;
      member.initSequence(uiSN++);
      // Late arrival
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      member.update_seq(uiSN);
      // DUP
      member.update_seq(uiSN);
      BOOST_CHECK_EQUAL(member.m_uiProbation, 0);
    }

    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "Dup test 2";
    // DUP test
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = 100;
      member.initSequence(uiSN++);
      // Late arrival
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      member.update_seq(uiSN++);
      member.update_seq(uiSN);
      // DUP
      member.update_seq(uiSN - 1);
      BOOST_CHECK_EQUAL(member.m_uiProbation, 0);
    }

    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "OLD DUP test";
    // DUP test
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = 100;
      member.initSequence(uiSN++);
      for (size_t i = 0; i < 100; ++i)
      {
        member.update_seq(uiSN + i);
      }
      // OLD DUP
      member.update_seq(180);
      BOOST_CHECK_EQUAL(member.m_uiProbation, 0);
    }

    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "MAX misorder test";
    // Max misorder test
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = 100;
      uint16_t uiLateSN;
      member.initSequence(uiSN++);
      for (size_t i = 0; i < 100; ++i)
      {
        if (i != 50)
          member.update_seq(uiSN + i);
        else
        {
          uiLateSN = uiSN + i;
          VLOG(5) << "Simulating late packet: " << uiLateSN;
        }
      }
      // Late arrival
      member.update_seq(uiLateSN);
      BOOST_CHECK_EQUAL(member.m_uiProbation, 0);
    }

    VLOG(MEMBER_ENTRY_TEST_LOG_LEVEL) << "MAX misorder test2";
    // Max misorder test: packet is later than max misorder
    {
      rfc3550::MemberEntry member;
      uint16_t uiSN = 100;
      member.initSequence(uiSN++);
      for (size_t i = 0; i < 150; ++i)
      {
        member.update_seq(uiSN + i);
      }
      // Late arrival
      member.update_seq(110);
      BOOST_CHECK_EQUAL(member.m_uiProbation, 0);
    }
  }

  // test probation with incrementing RTP SN numbers
  static void test_sequenceNumberProbation1()
  {
    rfc3550::MemberEntry member;
    uint16_t uiSN = 12345;
    member.initSequence(uiSN);
    BOOST_CHECK_EQUAL(member.m_uiProbation, rfc3550::MIN_SEQUENTIAL);
    member.update_seq(uiSN + 1);
    BOOST_CHECK_EQUAL(member.m_uiProbation, rfc3550::MIN_SEQUENTIAL - 1);
    member.update_seq(uiSN + 2);
    BOOST_CHECK_EQUAL(member.m_uiProbation, 0);
  }

  // test probation with non-incrementing RTP SN numbers
  static void test_sequenceNumberProbation2()
  {
    rfc3550::MemberEntry member;
    uint16_t uiSN = 12345;
    member.initSequence(uiSN);
    BOOST_CHECK_EQUAL(member.m_uiProbation, rfc3550::MIN_SEQUENTIAL);
    member.update_seq(uiSN + 2);
    BOOST_CHECK_EQUAL(member.m_uiProbation, rfc3550::MIN_SEQUENTIAL - 1);
    member.update_seq(uiSN + 1);
    BOOST_CHECK_EQUAL(member.m_uiProbation, rfc3550::MIN_SEQUENTIAL - 1);
    member.update_seq(uiSN + 3);
    BOOST_CHECK_EQUAL(member.m_uiProbation, rfc3550::MIN_SEQUENTIAL - 1);
    member.update_seq(uiSN + 4);
    BOOST_CHECK_EQUAL(member.m_uiProbation, 0);
  }

};

} // test
} // rtp_plus_plus
