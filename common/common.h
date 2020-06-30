// ======================================================================== //
// Copyright 2019 Ingo Wald                                                 //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

// std
#include <mutex>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <assert.h>
#include <string>
#include <math.h>
#include <cmath>
#include <algorithm>
#ifndef WIN32
#include <sys/time.h>
#else
#define NOMINMAX
#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#endif
#include <vector>
#include <thread>
#include <memory>
// tbb
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>
#include <tbb/task_scheduler_init.h>

// #include "cmake_generated_config_file.h"


#ifndef PRINT
# define PRINT(var) std::cout << #var << "=" << var << "\n";
# define PING std::cout << __FILE__ << "::" << __LINE__ << ": " << __FUNCTION__ << "\n";
#endif

#ifdef __WIN32__
#  define dw2_snprintf sprintf_s
#else
#  define dw2_snprintf snprintf
#endif
  
#define NOTIMPLEMENTED throw std::runtime_error(__PRETTY_FUNCTION__+std::string(": not implemented"));

namespace dw2 {

  template<typename INDEX_T, typename TASK_T>
  inline void parallel_for(INDEX_T nTasks, TASK_T&& taskFunction)
  {
    if (nTasks == 0) return;
    if (nTasks == 1)
      taskFunction(size_t(0));
    else
      tbb::parallel_for(INDEX_T(0), nTasks, std::forward<TASK_T>(taskFunction));
  }
  
  template<typename INDEX_T, typename TASK_T>
  inline void serial_for(INDEX_T nTasks, TASK_T&& taskFunction)
  {
    for (INDEX_T taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
      taskFunction(taskIndex);
    }
  }
  
  /*! added pretty-print function for large numbers, printing 10000000 as "10M" instead */
  inline std::string prettyDouble(const double val) {
    const double absVal = abs(val);
    char result[1000];

    if      (absVal >= 1e+15f) dw2_snprintf(result,1000,"%.1f%c",val/1e18f,'E');
    else if (absVal >= 1e+15f) dw2_snprintf(result,1000,"%.1f%c",val/1e15f,'P');
    else if (absVal >= 1e+12f) dw2_snprintf(result,1000,"%.1f%c",val/1e12f,'T');
    else if (absVal >= 1e+09f) dw2_snprintf(result,1000,"%.1f%c",val/1e09f,'G');
    else if (absVal >= 1e+06f) dw2_snprintf(result,1000,"%.1f%c",val/1e06f,'M');
    else if (absVal >= 1e+03f) dw2_snprintf(result,1000,"%.1f%c",val/1e03f,'k');
    else if (absVal <= 1e-12f) dw2_snprintf(result,1000,"%.1f%c",val*1e15f,'f');
    else if (absVal <= 1e-09f) dw2_snprintf(result,1000,"%.1f%c",val*1e12f,'p');
    else if (absVal <= 1e-06f) dw2_snprintf(result,1000,"%.1f%c",val*1e09f,'n');
    else if (absVal <= 1e-03f) dw2_snprintf(result,1000,"%.1f%c",val*1e06f,'u');
    else if (absVal <= 1e-00f) dw2_snprintf(result,1000,"%.1f%c",val*1e03f,'m');
    else dw2_snprintf(result,1000,"%f",(float)val);
    return result;
  }
  
  inline std::string prettyNumber(const size_t s)
  {
    char buf[100];
    if (s >= (1024LL*1024LL*1024LL*1024LL)) {
      sprintf(buf,"%.2fT",s/(1024.f*1024.f*1024.f*1024.f));
    } else if (s >= (1024LL*1024LL*1024LL)) {
      sprintf(buf,"%.2fG",s/(1024.f*1024.f*1024.f));
    } else if (s >= (1024LL*1024LL)) {
      sprintf(buf,"%.2fM",s/(1024.f*1024.f));
    } else if (s >= (1024LL)) {
      sprintf(buf,"%.2fK",s/(1024.f));
    } else {
      sprintf(buf,"%zi",s);
    }
    return buf;
  }
  
  inline double getCurrentTime()
  {
#ifdef _WIN32
    SYSTEMTIME tp; GetSystemTime(&tp);
    return double(tp.wSecond) + double(tp.wMilliseconds) / 1E3;
#else
    struct timeval tp; gettimeofday(&tp,nullptr);
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6;
#endif
  }

}
