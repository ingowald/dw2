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

#include "CompressedTile.h"
#include <atomic>

#if TURBO_JPEG
#include "turbojpeg.h"
// # include "jpeglib.h"
#define JPEG_QUALITY 75
//100
#endif

namespace dw2 {

  struct PlainTileEncoder : public TileEncoder {
    virtual Mailbox::Message::SP encode(const PlainTile &tile) override
    {
      Mailbox::Message::SP message = std::make_shared<Mailbox::Message>();
      message->resize(sizeof(TileMessageDataHeader)
                      +
                      sizeof(uint32_t)*tile.region.size().product());
      TileMessageDataHeader *header = (TileMessageDataHeader*)message->data();
      header->frameID = tile.frameID;
      header->region  = tile.region;
      header->eye     = tile.eye;
      uint32_t *out = (uint32_t*)&header[+1];
      
      assert(message->size() == sizeof(TileMessageDataHeader)+tile.pixels.size()*sizeof(uint32_t));
      memcpy(out,tile.pixels.data(),tile.pixels.size()*sizeof(uint32_t));
      return message;
    }
  };

  struct PlainTileDecoder : public TileDecoder {
    
    virtual void decode(PlainTile &tile,
                        Mailbox::Message::SP message) override
    {
      TileMessageDataHeader *header = (TileMessageDataHeader*)message->data();
      tile.alloc(header->region,header->eye);
      tile.frameID = header->frameID;
      const vec2i size  = tile.region.size();
      size_t numBytes = size.x*size.y*sizeof(uint32_t);
      assert(message->size() == (sizeof(TileMessageDataHeader)+numBytes));
      uint32_t *out = tile.pixels.data();
      const uint32_t *in = (const uint32_t *)&header[+1];
      memcpy(out,in,numBytes);
    }
  };

#if TURBO_JPEG
  struct JpegTileEncoder : public TileEncoder {
    JpegTileEncoder()
    {
      compressor = tjInitCompress();
      // cinfo.err = jpeg_std_error(&jerr);
      // cinfo.input_components = 4;
      // cinfo.in_color_space   = JCS_EXT_RGBX;
      // jpeg_create_compress(&cinfo);
    }
    
    virtual Mailbox::Message::SP encode(const PlainTile &tile) override
    {
      /* the output that jpeg will create for us */
      // unsigned char *outBuffer = nullptr;
      // size_t         outSize;
      
      // jpeg_mem_dest(&cinfo, &outBuffer, (unsigned long*)&outSize);

      // cinfo.image_width      = tile.size().x;
      // cinfo.image_height     = tile.size().y;
      // cinfo.in_color_space   = JCS_EXT_RGBX;
      // cinfo.input_components = 4;
      // jpeg_set_defaults(&cinfo);
      // cinfo.in_color_space   = JCS_EXT_RGBX;

      // jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE);
      // jpeg_start_compress(&cinfo, TRUE);

      // for (int iy=0;iy<tile.size().y;iy++) {
      //   JSAMPROW line = (JSAMPROW)&tile.pixels[iy*tile.size().x];
      //   jpeg_write_scanlines(&cinfo, &line, 1);
      // }

      // jpeg_finish_compress(&cinfo);

      //! Change to turbo jpeg
      unsigned char *outBuffer = nullptr;
      long unsigned outSize;

      assert(tile.pixels.data());
      int numPixels = tile.size().x * tile.size().y;

      int rc = tjCompress2((tjhandle)compressor, (unsigned char *)tile.pixels.data(),
                           tile.size().x,tile.pitch*sizeof(int),tile.size().y,
                           TJPF_RGBX, 
                           &outBuffer, 
                           &outSize,TJSAMP_420,JPEG_QUALITY,0);

      //std::cout << "compression ratio: " << (float)outSize / (tile.size().x * tile.size().y * 4)  << "\n";

      Mailbox::Message::SP message = std::make_shared<Mailbox::Message>();
      message->resize(outSize+sizeof(TileMessageDataHeader) + sizeof(size_t));
      message->outSize = outSize;
      TileMessageDataHeader *header = (TileMessageDataHeader *)message->data();
      header->frameID = tile.frameID;
      header->region  = tile.region;
      header->eye     = tile.eye;
      memcpy(header+1,outBuffer,outSize);

      free(outBuffer);
      return message;
    }
    
    // static void *createCompressor(){return (void *)tjInitCompress();};

    // static void freeCompressor(void *compressor){tjDestroy((tjhandle)compressor);};

    tjhandle compressor;

    // struct jpeg_compress_struct cinfo;
    // struct jpeg_error_mgr jerr;
  };
  
  struct JpegTileDecoder : public TileDecoder {
    JpegTileDecoder()
    {
      decompressor = tjInitDecompress();
      // cinfo.err = jpeg_std_error(&jerr);
      // jpeg_create_decompress(&cinfo);
    }
    
    virtual void decode(PlainTile &tile,
                        Mailbox::Message::SP message) override
    {
      // static std::mutex sync;
      // std::lock_guard<std::mutex> serial(sync);
      
      TileMessageDataHeader *header = (TileMessageDataHeader *)message->data();
      tile.alloc(header->region,0);
      size_t jpegSize = message ->size()-sizeof(*header);
      int rc = tjDecompress2((tjhandle)decompressor, (unsigned char *)(header+1),
                              message->size()-sizeof(*header),
                              (unsigned char*)tile.pixels.data(),
                              tile.size().x,tile.pitch*sizeof(int), tile.size().y,
                              TJPF_RGBX, 0);

      // jpeg_mem_src(&cinfo, (unsigned char *)(header+1),
      //              message->size()-sizeof(*header));
      
      // jpeg_read_header(&cinfo,true);
      // cinfo.out_color_space   = JCS_EXT_RGBX;
      // jpeg_start_decompress(&cinfo);

      
      // for (int iy=0;iy<tile.size().y;iy++) {
      //   JSAMPROW line = (JSAMPROW)&tile.pixels[iy*tile.size().x];
      //   jpeg_read_scanlines(&cinfo, &line, 1);
      // }

      // jpeg_finish_decompress(&cinfo);
    }
    
    tjhandle decompressor;

    // struct jpeg_decompress_struct cinfo;
    // struct jpeg_error_mgr jerr;
  };
  
  TileEncoder::SP TileEncoder::create() { return std::make_shared<JpegTileEncoder>(); }
  TileDecoder::SP TileDecoder::create() { return std::make_shared<JpegTileDecoder>(); }
#else
  TileEncoder::SP TileEncoder::create() { return std::make_shared<PlainTileEncoder>(); }
  TileDecoder::SP TileDecoder::create() { return std::make_shared<PlainTileDecoder>(); }
#endif
  
} // ::dw2



