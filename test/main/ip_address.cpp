#include "ip_address.h"
#include "unity.h"

//==============================================================================

PL::IpV4Address testIpV4Address(1, 2, 3, 4);
std::string testIpV4AddressString = "1.2.3.4";
PL::IpV6Address testIpV6Address(0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F);
std::string testIpV6AddressString = "0001:0203:0405:0607:0809:0A0B:0C0D:0E0F";

//==============================================================================

void TestIpAddress() {
  PL::IpV4Address testIpV4AddressFromString(testIpV4AddressString);
  TEST_ASSERT(testIpV4AddressFromString.u32 == testIpV4Address.u32);
  TEST_ASSERT(testIpV4AddressFromString == testIpV4Address);
  testIpV4AddressFromString.u8[3] = 0;
  TEST_ASSERT(testIpV4AddressFromString != testIpV4Address);
  
  PL::IpV6Address testIpV6AddressFromString(testIpV6AddressString);
  for (int i = 0; i < sizeof(PL::IpV6Address::u8); i++) 
    TEST_ASSERT(testIpV6AddressFromString.u8[i] == testIpV6Address.u8[i]);
  TEST_ASSERT(testIpV6AddressFromString == testIpV6Address);
  testIpV6AddressFromString.u8[3] = 0;
  TEST_ASSERT(testIpV6AddressFromString != testIpV6Address);
}