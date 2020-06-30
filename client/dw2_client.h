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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  struct dw2_info_t {
    /*! total (logical) pixels in the wall, across all displays,
        _including_ bezels */
    int32_t totalPixelsInWall[2];
    int32_t numDisplays[2];
    int32_t hasControlWindow;
    int32_t controlWindowSize[2];
  };

  typedef enum { DW2_OK = 0, DW2_ERROR } dw2_rc;

  /*! query information on that given address; can be done as often as
      desired before connecting, and does not require a connect. This
      allows an app to query the vailability and/or size of a wall
      before even creating the render nodes. */
  dw2_rc dw2_query_info(dw2_info_t *info,
                        const char *hostName,
                        const int   port);

  /*! connect to the service at host:port, together with the given
      number of peers. Note the number of peers specified here must
      match how many nodes will join in in this dw2_connect call, as
      well as in how many nodes will join in the begin/end frame
      calls.  */
  dw2_rc dw2_connect(const char *hostName, int port,
                     int numPeers);
  
  void dw2_disconnect();

  void dw2_begin_frame();
  
  void dw2_end_frame();

  /*! send a tile that goes to position (x0,y0) and has size (sizeX,
      sizeY), with given array of pixels. */
  void dw2_send_rgba(int x0, int y0, int sizeX, int sizeY,
                     /*! pitch: the increment (in uints) between each
                         line and the next in the pixel[] array */
                     int pitch,
                     const uint32_t *pixel);

  
#ifdef __cplusplus
}
#endif
