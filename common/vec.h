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

#include "common.h"

namespace dw2 {

  inline int divRoundUp(int a, int b)
  { return (a+(b-1))/b; }
  inline size_t divRoundUp(size_t a, size_t b)
  { return (a+(b-1))/b; }
    
  // ==================================================================
  // vec
  // ==================================================================
  
  struct vec2i {
    inline vec2i() {}
    inline vec2i(int i) : x(i), y(i) {}
    inline vec2i(int x, int y) : x(x), y(y) {}
    inline int product() const { return x*y; }
    int x, y;
  };

  inline vec2i operator-(const vec2i &a, const vec2i &b) { return { a.x - b.x, a.y - b.y }; }
  inline vec2i operator+(const vec2i &a, const vec2i &b) { return { a.x + b.x, a.y + b.y }; }
  inline vec2i operator*(const vec2i &a, const vec2i &b) { return { a.x * b.x, a.y * b.y }; }
  
  inline vec2i divRoundUp(const vec2i &a, const vec2i &b)
  { return vec2i{ divRoundUp(a.x,b.x), divRoundUp(a.y,b.y) }; }


  // ==================================================================
  // box
  // ==================================================================
  
  struct box2i {
    inline box2i()
      : lower(INT_MAX), upper(INT_MIN)
    {}
    
    inline box2i(const vec2i &lo, const vec2i &hi)
      : lower(lo), upper(hi)
    {}

    inline bool overlaps(const box2i &other) const
    {
      if (upper.x <= other.lower.x) return false;
      if (upper.y <= other.lower.y) return false;
      if (lower.x >= other.upper.x) return false;
      if (lower.y >= other.upper.y) return false;
      return true;
    }
    
    inline vec2i size() const { return upper - lower; }
    
    vec2i lower, upper;
  };

  inline std::ostream &operator<<(std::ostream &o, const vec2i &v)
  { o << "(" << v.x << "," << v.y << ")"; return o; }

  inline std::ostream &operator<<(std::ostream &o, const box2i &v)
  { o << "{" << v.lower << "-" << v.upper << "}"; return o; }

  inline bool operator==(const vec2i &a, const vec2i &b)
  {
    return a.x == b.x && a.y == b.y;
  }

}
