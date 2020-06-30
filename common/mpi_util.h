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

#ifdef MPI_FOUND

// ours
#include "common.h"
// mpi
#include <mpi.h>

#define MPI_CALL(a) MPI_##a;

namespace dw2 {
  namespace mpi {

    struct Comm;

    template<typename T>
    inline void read_t(Comm *comm, int rank, T &t);
    
    template<typename T>
    inline void write_t(Comm *comm, int rank, const T &t);
      
    /*! abstraction for a mpi communicator */
    struct Comm {
      Comm(MPI_Comm comm=MPI_COMM_NULL) : comm(comm) {}

      inline Comm dup() const
      {
        MPI_Comm duped;
        MPI_CALL(Comm_dup(comm,&duped));
        return Comm(duped);
      }
      
      inline void barrier() const
      {
        MPI_CALL(Barrier(comm));
      }
      
      inline int size() const
      {
        int size;
        MPI_CALL(Comm_size(comm,&size));
        return size;
      }
      
      inline int rank() const
      {
        int rank;
        MPI_CALL(Comm_rank(comm,&rank));
        return rank;
      }

      template<typename T>
      inline T read(int rank) { T t; read_t(this,rank,t); return t; }
      
      template<typename T>
      inline void write(int rank, const T &t) { write_t(this,rank,t); }
      
      MPI_Comm comm;
    };


      template<typename T>
      inline void read_t(Comm *comm, int rank, T &t) {
        MPI_CALL(Recv(&t,sizeof(T),MPI_BYTE,rank,/*tag:*/0,comm->comm,MPI_STATUS_IGNORE));
      }

      template<>
      inline void read_t(Comm *comm, int rank, std::string &t) {
        int len;
        read_t(comm,rank,len);
        t = std::string(len,' ');
        MPI_CALL(Recv((void *)t.data(),len,MPI_BYTE,rank,/*tag:*/0,comm->comm,MPI_STATUS_IGNORE));
      }
    
    template<typename T>
    inline void write_t(Comm *comm, int rank, const T &t) {
      MPI_CALL(Send(&t,sizeof(T),MPI_BYTE,rank,/*tag:*/0,comm->comm));
    }
    
    // template<>
    inline void write_t(Comm *comm, int rank, const std::string &t) {
      write_t(comm,rank,(int)t.size());
      MPI_CALL(Send((void*)t.data(),t.size(),MPI_BYTE,rank,/*tag:*/0,comm->comm));
    }

    
    /*! initialize MPI in threaded mode, and error-exit out if this
        isn't possible */
    void initThreaded(int &ac, const char **&av);
  }
}

#endif

