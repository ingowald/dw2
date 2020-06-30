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

/*! iw: note this code is literally copied (and then adapted) from
  embree's original common/sys/network.h, under following license */

// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#else
#define SOCKET int
#endif

namespace dw2 {
  namespace sock {
    
    /*! type for a socket */
    using socket_t = struct opaque_socket_t*;

    /*! exception thrown when other side disconnects */
    struct Disconnect : public std::exception
    {
#ifdef _WIN32
      virtual const char* what() const override
#else
        virtual const char* what() const noexcept override
#endif
      { return "network disconnect"; }
    };

    int getPortOf(socket_t sockfd);

    SOCKET getFileDescriptor(socket_t sock);
    
    /*! creates a socket bound to a port */
    socket_t bind(unsigned short port);

    /*! listens for an incoming connection and accepts that connection */
    socket_t listen(socket_t sockfd);

    /*! initiates a connection */
    socket_t connect(const char* host, unsigned short port);

    /*! read data from the socket */
    void read(socket_t socket, void* data, size_t bytes);

    /*! read the available data on the socket, up to 'bytes' bytes.
      Returns the number of bytes read. */
    size_t read_some(socket_t socket, void* data,
                     const size_t bytes);

    /*! write data to the socket */
    void write(socket_t socket, const void* data, size_t bytes);

    /*! flushes the write buffer */
    void flush(socket_t socket);

    /*! close a socket */
    void close(socket_t socket);

    // /*! reads a bool from the socket */
    // bool read_bool(socket_t socket);

    // /*! reads a character from the socket */
    // char read_char(socket_t socket);

    // /*! reads an integer from the socket */
    // int read_int(socket_t socket);

    // /*! reads a float from the socket */
    // float read_float(socket_t socket);

    // /*! reads a string from the socket */
    // std::string read_string(socket_t socket);

    // /*! writes a bool to the socket */
    // void write(socket_t socket, bool value);

    // /*! writes a character to the socket */
    // void write(socket_t socket, char value);

    // /*! writes an integer to the socket */
    // void write(socket_t socket, int value);

    // /*! writes a float to the socket */
    // void write(socket_t socket, float value);

    // /*! writes a string to the socket */
    // void write(socket_t socket, const std::string& str);
  }

  template<typename T>
  inline void read(sock::socket_t socket, T &t)
  {
    sock::read(socket,&t,sizeof(t));
  }

  template<typename T>
  T read(sock::socket_t socket) { T t; read(socket,t); return t; }

  template<typename T>
  inline void write(sock::socket_t socket, const T &t)
  {
    sock::write(socket,&t,sizeof(t));
  }

  template<>
  inline void read(sock::socket_t socket, std::string &s)
  {
    int len;
    read(socket,len);
    s = std::string(len,' ');
    sock::read(socket,(void *)s.data(),len);
  }

  template<>
  inline void write(sock::socket_t socket, const std::string &s)
  {
    int len = s.size();
    write(socket,len);
    sock::write(socket,(void *)s.data(),len);
  }
  
  std::string getHostName();
}// ::ospcommon
