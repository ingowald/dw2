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

#include "Server.h"
#include <thread>
#include "glfwWindow.h"

#include <stdlib.h>

namespace dw2 {

  void usage(const std::string &err)
  {
    if (!err.empty()) {
      std::cout << "Error: " << err << "\n" << "\n";
    }
    std::cout << "usage: ./dw2_service [args]*" << "\n" << "\n";
    std::cout << "w/ args: " << "\n";
    std::cout << "--num-displays|-nd <Nx> <Ny> - num displays (total) in x and y dir" << "\n";
    std::cout << "--width|-w <numDisplays.x>        - num displays in x direction" << "\n";
    std::cout << "--height|-h <numDisplays.y>       - num displays in y direction" << "\n";
    std::cout << "--window-size|-ws <res_x> <res_y> - window size (in pixels)" << "\n";
    std::cout << "--[no-]head-node | -[n]hn         - use / do not use dedicated head node" << "\n";
    std::cout << "--head-node-port | -hnp           - use a specific port for the head node client connections" << "\n";
    std::cout << "--max-frames-in-flight|-fif <n>   - allow up to n frames in flight" << "\n";
    std::cout << "--bezel-width|-bw <Nx> <Ny>       - assume a bezel width (between displays) of Nx and Ny pixels" << "\n";
    std::cout << "--displays-per-node|-dpn <N> <display1> ...<displayN>] " << "\n";
    std::cout << "     (specifies that each physical host in the mpi launch will have" << "\n";
    std::cout << "      N physical displays attached to, with each display specified through" << "\n";
    std::cout << "      three values: first, the X display is on (ie, :0, :1, etc), and the," << "\n";
    std::cout << "       pixel coords (x and y) of the lower-left pixel of that X display, with" << "\n";
    std::cout << "       the i'th rank on any given node running the i'th such display." << "\n";
    std::cout << "       This only makes sense if the mpi launch uses N ranks per display node.)" << "\n";
    exit(!err.empty());
  }

  struct LocalDisplay {
    /*! the X11 display ID */
    int monitorID;
    /*! lower-left pixel coordinates on the x screen that this window
        should be positioned at */
    vec2i lower;
  };
  
  extern "C" int main(int ac, const char **av)
  {
    mpi::initThreaded(ac,av);
    mpi::Comm world(MPI_COMM_WORLD);

    auto error_callback = [](int error, const char* description) {
      fprintf(stderr, "glfw error %d: %s\n", error, description);
    };
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
      fprintf(stderr, "Failed to initialize GLFW\n");
      exit(EXIT_FAILURE);
    }
    Server::Config config;

    /* the following are NOT server config because they do not
       influence the server's logical abstraction of number and
       position of displays - it only influences how ranks map a
       logical display ID (in the server's numDisplays.x*numDisplays.y
       space of logical displays) to a given rank's local X server's X
       displays (:0, :1, etc) and virtual frame buffer (a single X
       display can cover multiple physical display wall displays) */

    std::vector<LocalDisplay> localDisplaysOnNode = { 0, { 0,0 } };
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg == "--head-node" || arg == "-hn") {
        config.useHeadNode = true;
      } else if (arg == "--head-node-port" || arg == "-hnp") {
        config.headNodePort = atoi(av[++i]);
      } else if(arg == "--control-window" || arg == "-cw"){
        config.hasControlWindow = true;
      } else if(arg == "--control-window-position" || arg == "-cwp"){
        config.controlWindowPosition.x = atoi(av[++i]);
        config.controlWindowPosition.y = atoi(av[++i]);
      } else if(arg == "--control-window-size" || arg == "-cws"){
        config.controlWindowSize.x = atoi(av[++i]);
        config.controlWindowSize.y = atoi(av[++i]);
      } else if (arg == "--stereo" || arg == "-s") {
        config.doStereo = true;
      } else if (arg == "--no-head-node" || arg == "-nhn") {
        config.useHeadNode = false;
      } else if (arg == "--num-displays" || arg == "-nd") {
        assert(i+2<ac);
        config.numDisplays.x = atoi(av[++i]);
        config.numDisplays.y = atoi(av[++i]);
      } else if (arg == "--width" || arg == "-w") {
        assert(i+1<ac);
        config.numDisplays.x = atoi(av[++i]);
      } else if (arg == "--height" || arg == "-h") {
        assert(i+1<ac);
        config.numDisplays.y = atoi(av[++i]);
      } else if (arg == "--window-size" || arg == "-ws") {
        assert(i+1<ac);
        config.windowSize.x = atoi(av[++i]);
        assert(i+1<ac);
        config.windowSize.y = atoi(av[++i]);
      } else if (arg == "--full-screen" || arg == "-fs") {
        config.windowSize = vec2i(-1,-1);
        config.doFullScreen = true;
      } else if (arg == "--max-frames-in-flight" || arg == "-fif" || arg == "-mfif") {
        config.maxFramesInFlight = atoi(av[++i]);
      } else if (arg == "--bezel-width" || arg == "--bezel" || arg == "-bw" || arg == "-b") {
        if (i+2 >= ac) {
          printf("format for --bezel|-b argument is '-b <x> <y>'\n");
          exit(1);
        }
        config.bezelWidth.x = atoi(av[++i]);
        config.bezelWidth.y = atoi(av[++i]);
      } else if (arg == "--port" || arg == "-p") {
        config.desiredInfoPortNum = atoi(av[++i]);
      } else if (arg == "--displays-per-node" || arg == "-dpn") {
        if (i+1 >= ac) usage("not enough data in -dpn parameter");
        int numDisplays = std::atol(av[++i]);
        if (i+1+3*numDisplays != ac)
          usage("not enough (or incorrect) data in -dpn parameter parsing");
        localDisplaysOnNode.resize(numDisplays);
        for (auto &ld : localDisplaysOnNode) {
          ld.monitorID = std::atoi(av[++i]);
          ld.lower.x   = std::atoi(av[++i]);
          ld.lower.y   = std::atoi(av[++i]);
        }
      } else {
        usage("unkonwn arg "+arg);
      } 
    }

    if (config.numDisplays.x < 1) 
      usage("no display wall width specified (--width <w>)");
    if (config.numDisplays.y < 1) 
      usage("no display wall height specified (--heigh <h>)");
    if (world.size() != config.numDisplays.x*config.numDisplays.y+config.useHeadNode)
      throw std::runtime_error("invalid number of ranks for given display/head node config");

    const int displayNo = config.useHeadNode ? world.rank()-1 : world.rank();
    const vec2i displayID(displayNo % config.numDisplays.x, displayNo / config.numDisplays.x);

    char title[1000];
    sprintf(title,"rank %i/%i, display (%i,%i)",
            world.rank(),world.size(),displayID.x,displayID.y);
      
    LocalDisplay localDisplay;    
    if (config.doFullScreen) {
      if (config.useHeadNode) {
        if (world.rank() == 1)
          config.windowSize = GLFWindow::getScreenSize();
        // when using head node, use the window size of first displya, not of head node!
        MPI_CALL(Bcast(&config.windowSize,2,MPI_INT,1,world.comm));
      } else
        // in non-head node mode, all ranks have the same display size
        config.windowSize = GLFWindow::getScreenSize();
    } else {
      // non-fullscreen mode: each physical node might have multiple
      // ranks, each running a different display ...
      std::vector<std::string> nodeNameOfRank(world.size());
      int myNameLen = 0;
      char myName[MPI_MAX_PROCESSOR_NAME];
      MPI_CALL(Get_processor_name(myName,&myNameLen));
      PRINT(myName);
      for (int rank=0;rank<world.size();rank++) {
        int rankNameLen = 0;
        char rankName[MPI_MAX_PROCESSOR_NAME+1];
        if (rank == world.rank()) {
          rankNameLen = myNameLen;
          strcpy(rankName,myName);
        }
        MPI_CALL(Bcast(&rankNameLen,1,MPI_INT,rank,world.comm));
        MPI_CALL(Bcast(rankName,rankNameLen+1,MPI_BYTE,rank,world.comm));
        nodeNameOfRank[rank] = rankName;
      }

      // PRINT(world.rank());
      // for (int i=0;i<world.size();i++) PRINT(nodeNameOfRank[i]);
      
      // now have the names of all ranks, find out the how many'eth
      // rank on our node we are:
      int ourRankOnOurNode = 0;
      for (int i=config.useHeadNode?1:0;i<world.rank();i++)
        if (nodeNameOfRank[i] == nodeNameOfRank[world.rank()])
          ourRankOnOurNode++;
      std::cout << "rank " << world.rank() << " : is the " << ourRankOnOurNode
        << "'th display on node " << nodeNameOfRank[world.rank()] << "..." << "\n";
      std::cout << "local display on node size is : " << localDisplaysOnNode.size() << "\n";
      if (ourRankOnOurNode >= localDisplaysOnNode.size())
        usage("have more ranks running on node than --displays-per-node made us expect!?");
      localDisplay = localDisplaysOnNode[ourRankOnOurNode];
    }
    world.barrier();
    
    Server server(config);
    if(config.hasControlWindow == true){
      const std::string title = "Control Window ";
      GLFWindow *controlWindow = new GLFWindow(config.controlWindowSize, config.controlWindowPosition, 
                                          title, false, config.doStereo, 0);
    }
    std::shared_ptr<GLFWindow> glfWindow
      = std::make_shared<GLFWindow>(config.windowSize,localDisplay.lower,
                                    title,
                                    config.doFullScreen,config.doStereo,localDisplay.monitorID
                                    );
    
    std::thread frameSetter([&]() {
        // wait for new thread to arrive
        size_t frame_id = 0;
        double t_last = -1;
        double t_avg = 0;
        while (1) {
          // std::cout << "#MAIN%%%: waiting for next assembled frame" << "\n";
          FrameBuffer::SP nextFrame = server.waitForNextAssembledFrame();
          glfWindow->setFrameBuffer(nextFrame);
          double t_out = getCurrentTime();

          if (world.rank() == 0) {
            if (t_last < 0.0) {
              t_last = t_out;
            } else {
              double t = t_out - t_last;
              t_avg = (t + frame_id * t_avg) / (frame_id + 1);
              ++frame_id;
              std::cout << "#server: avg frame rate: " << (1.0/t_avg) << "fps" << "\n";
            }
          }
          t_last = t_out;
        }
      });
    glfWindow->run();
    return 0;
  }
    
} // ::dw2
