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

// ours
#include "../common/mpi_util.h"
// mpi
#include <mpi.h>

#define MPI_CALL(a) MPI_##a;

namespace dw2 {
  namespace mpi {

    /*! initialize MPI in threaded mode, and error-exit out if this
      isn't possible */
    void initThreaded(int &ac, const char **&av)
    {
      int requested = MPI_THREAD_MULTIPLE;
      int provided  = 0;
      MPI_CALL(Init_thread(&ac,(char***)&av,requested,&provided));
      if (provided < requested)
        throw std::runtime_error("Could not initialize MPI - does your mpi support MPI_THREAD_MULTIPLE?");
    }
    
  }
}
