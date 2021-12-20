/*********************************************************************************************************************
Copyright (c) 2020 RoboSense
All rights reserved

By downloading, copying, installing or using the software you agree to this license. If you do not agree to this
license, do not download, install, copy or use the software.

License Agreement
For RoboSense LiDAR SDK Library
(3-clause BSD License)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the names of the RoboSense, nor Suteng Innovation Technology, nor the names of other contributors may be used
to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************************************************/

#pragma once

#include <arpa/inet.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <functional>
#include <chrono>
#include <mutex>

namespace robosense
{
namespace lidar
{

#pragma pack(push, 1)

typedef struct
{
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t ms;
  uint16_t us;
} RSTimestampYMD;

typedef struct
{
  uint8_t sec[6];
  uint32_t us;
} RSTimestampUTC;

typedef struct 
{
  uint8_t sec[6];
  uint8_t ss[4];
} RSTimestampUTC2;

typedef struct
{
  uint8_t tt[2];
} RSTemperature;

#pragma pack(pop)

inline uint64_t parseTimeUTCWithNs(const RSTimestampUTC2* tsUtc)
{
  // sec
  uint64_t sec = 0;
  for (int i = 0; i < 6; i++)
  {
    sec <<= 8;
    sec += tsUtc->sec[i];
  }

  uint64_t ns = 0;
  for (int i = 0; i < 4; i++)
  {
    ns <<= 8;
    ns += tsUtc->ss[i]; 
  }

  return (sec * 1000000 + ns/1000);
}

inline uint64_t parseTimeUTCWithUs(const RSTimestampUTC2* tsUtc)
{
  // sec
  uint64_t sec = 0;
  for (int i = 0; i < 6; i++)
  {
    sec <<= 8;
    sec += tsUtc->sec[i];
  }

  uint64_t us = 0;
  for (int i = 0; i < 4; i++)
  {
    us <<= 8;
    us += tsUtc->ss[i]; 
  }

  return (sec * 1000000 + us);
}

inline uint64_t parseTimeUTCWithMs(const RSTimestampUTC2* tsUtc)
{
  // sec
  uint64_t sec = 0;
  for (int i = 0; i < 6; i++)
  {
    sec <<= 8;
    sec += tsUtc->sec[i];
  }

  // ms
  uint64_t ms = tsUtc->ss[0]; 
  ms <<= 8;
  ms += tsUtc->ss[1];

  // us
  uint64_t us = tsUtc->ss[2]; 
  us <<= 8;
  us += tsUtc->ss[3];

  return (sec * 1000000 + ms * 1000 + us);
}

inline uint64_t parseTimeYMD(const RSTimestampYMD* tsYmd)
{
  std::tm stm;
  memset(&stm, 0, sizeof(stm));

  // since 2000 in robosense YMD, and since 1900 in struct tm
  stm.tm_year = tsYmd->year + 2000 - 1900; 
  // since 1 in robosense YMD, and since 0 in struct tm
  stm.tm_mon = tsYmd->month - 1; 
  // since 1 in both robosense YMD and struct tm
  stm.tm_mday = tsYmd->day; 
  stm.tm_hour = tsYmd->hour;
  stm.tm_min = tsYmd->minute;
  stm.tm_sec = tsYmd->second;
  time_t sec = std::mktime(&stm);

  uint64_t ms = ntohs(tsYmd->ms);
  uint64_t us = ntohs(tsYmd->us);

#if 0
  std::cout << "tm_year:" << stm.tm_year 
    << ", tm_mon:" << stm.tm_mon 
    << ", ms:" << ms 
    << ", us:" << us 
    << std::endl;
#endif

  return (sec * 1000000 + ms * 1000 + us);
}

inline uint64_t getTimeHost(void)
{
  std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
  std::chrono::system_clock::duration t_s = t.time_since_epoch();

  std::chrono::duration<uint64_t, std::ratio<1l, 1000000l>> t_us = 
  std::chrono::duration_cast<std::chrono::duration<uint64_t, std::ratio<1l, 1000000l>>>(t_s);
  return t_us.count();
}

inline int16_t parseTemp(const RSTemperature* tmp)
{
  // | lsb | padding | neg | msb |
  // |  5  |    3    |  1  |  7  | (in bits)
  uint8_t lsb = tmp->tt[0] >> 3;
  uint8_t msb = tmp->tt[1] & 0x7F;
  uint8_t neg = tmp->tt[1] & 0x80;

  int16_t t = ((uint16_t)msb << 5) + lsb;
  if (neg) t = -t;

  return t;
}

}  // namespace lidar
}  // namespace robosense