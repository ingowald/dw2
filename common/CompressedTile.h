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

#include "vec.h"
#include "Mailbox.h"
//std
#include <vector>

namespace dw2 {

  /*! the header we will find in any tile message - it's the sernders
      job to make sure that's the case. inhertif from
      timestampedmessage to get the frameID we need for tile sorting,
      then add the tile region and eye info */
  struct TileMessageDataHeader : public TimeStampedMailbox::TileStampedMessageHeader {
    box2i region;
    int eye;
  };
  
  /*! a plain, uncompressed tile */
  struct PlainTile 
  {
    typedef std::shared_ptr<PlainTile> SP;
    
    void alloc(const box2i &region, int eye)
      // : pitch(tileSize.x),
      //   pixel(new uint32_t [tileSize.x*tileSize.y])1
    {
      this->region = region;
      this->eye = eye;
      pixels.resize(region.size().x*region.size().y);
      pitch = region.size().x;
    }

    inline vec2i size() const { return region.size(); }

    /*! region of pixels that this tile corresponds to */
    box2i     region;
    /*! number of ints in pixel[] buffer from one y to y+1 */
    int       pitch { 0 };
    /*! which eye this goes to (if stereo) */
    int       eye   { 0 };
    /*! the frame that this tile belongs to */
    int       frameID { -1 };
    /*! pointer to buffer of pixels; this buffer is 'pitch' int-sized pixels wide */
    std::vector<uint32_t> pixels;
  };



  struct TileEncoder {
    typedef std::shared_ptr<TileEncoder> SP;
    
    /*! create for one thread to ues */
    static TileEncoder::SP create();

    virtual Mailbox::Message::SP encode(const PlainTile &tile) = 0;

  };

  struct TileDecoder {
    typedef std::shared_ptr<TileDecoder> SP;
    
    /*! create for one thread to ues */
    static TileDecoder::SP create();

    virtual void decode(PlainTile &plain, Mailbox::Message::SP message) = 0;
  };
  
  // Mailbox::Message::SP encode(void *compressor, const PlainTile &tile);
  
  // void decodeTile(PlainTile &tile,
  //                 void *decompressor,
  //                 Mailbox::Message::SP message);

  // 
  // void *createDecompressor();
  // 
  // void freeDecompressor(void *);
    
} // ::dw2
