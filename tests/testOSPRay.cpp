#include <iostream>
#include "dw2_client.h"
// #include "include/dw2.h"

// OSPRay
#include <mpiCommon/MPICommon.h>
#include "ospcommon/vec.h"
#include "ospcommon/box.h"
#include "ospray/ospray_cpp/Device.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/Camera.h"
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Renderer.h"
#include "ospray/ospray_cpp/Geometry.h"
#include "ospray/ospray_cpp/Model.h"


#include "testOSPRay.h"

namespace dw2{
    using namespace ospcommon;
    using namespace ospray::cpp;

    extern int dbg_rank;
  
    int mpi_rank = -1;
    int mpi_size = -1;

    void usage(const std::string &errMsg, int rank)
    {
        if (rank == 0) {

        if (errMsg != "")
            std::cerr << "Error: " << errMsg << "\n" << "\n";
        
        std::cout << "Usage: ./testFrameRenderer <host> <port> <args>*" << "\n";
        }
        exit(errMsg != "");
    }

    extern "C" int main(int ac, const char **av){

        //! Initialize OSPRay and Load OSPRay Display Wall Module
        if(ospInit(&ac, av) != OSP_NO_ERROR){
            throw std::runtime_error("Cannot Initialize OSPRay");
        }else{
            std::cout << "Initializa OSPRay Successfully" << "\n";
        }
        //! Load Module
        ospLoadModule("mpi");
        ospLoadModule("wall");

        const int mpi_rank = mpicommon::world.rank;
        const int world_size = mpicommon::world.size;
	    const int worker_size = mpicommon::worker.size;
        dbg_rank = mpi_rank;
	    std::cout << "world size " << world_size << "\n";
	    std::cout << "worker size " << worker_size << "\n";

        /*! various options to configure test renderer, so we can stress
                different pieces of the communication pattern */
        int    numFramesToRender = -1;//100;
        size_t usleepPerFrame    = 0;
        bool   barrierPerFrame   = false;
        
        // /*! if randomizeowner is false, each tile gets done by same rank
        //     every frame with pretty much same numebr of tiles for rank
        //     every frame; if it is true we will randomlzie this, which may
        //     also change the number of tiles any rank produces */
        // bool        randomizeOwner  = false;
        std::string hostName = "";
        int         port = 0;


        bool connectToWall = true;

        for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "--barrier-per-frame")
            barrierPerFrame = true;
        else if (arg == "--no-wall")
          connectToWall = false;
        else if (arg[0] == '-')
            usage("un-recognized cmd-line argument '"+arg+"'",mpi_rank);
        else if (hostName == "")
            hostName = arg;
        else if (port == 0)
            port = stoi(arg);
        else
            usage("both hostname and port are already specified ... !?",mpi_rank);
        }

        if (connectToWall && hostName == "")
            usage("no hostname specified...",mpi_rank);
        if (connectToWall && port == 0)
            usage("no port specified...",mpi_rank);
        
        //! Receive wall info first, need the wallSize for camera aspect
        dw2_info_t info;
        if (mpi_rank == 0)
            std::cout << "querying wall info from " << hostName << ":" << port << "\n";

        if (dw2_query_info(&info, hostName.c_str(), port) != DW2_OK) {
          if (mpi_rank == 0)
            std::cerr << "could not connect to display wall ..." << "\n";
          exit(1);
        }
        const vec2i wallSize = (const vec2i&)info.totalPixelsInWall;
        const int hasControlWindow = (const int&)info.hasControlWindow;
        const vec2i controlWindowSize = (const vec2i&)info.controlWindowSize;
        // const osp::vec2i wallSize{23040, 5760};
        // const osp::vec2i wallSize{1024, 1024};
        if(hasControlWindow)
            std::cout << "Debug: control window size : " << controlWindowSize.x << " " << controlWindowSize.y << "\n";
       
        // mpicommon::world.barrier();

        //! Create Geometry: load some random particals for testing
        float box = 5000;
        float cam_pos[] = {100, 62, 200};
        float cam_up[] = {0, 1, 0};
        float cam_target[] = {152.0f, 62.0f, 62.f};
        float cam_view[] = {cam_target[0]-cam_pos[0], cam_target[1] - cam_pos[1], cam_target[2] - cam_pos[2]};
        OSPGeometry geom;
        std::vector<Particle> random_atoms;
        std::vector<float> random_colors;
        //! Renderer
        OSPRenderer renderer = ospNewRenderer("scivis");
        constructParticles(random_atoms, random_colors, box);
        //new spheres geometry
        geom = commitParticles(random_atoms, random_colors, renderer); // create and setup model and mesh
        //! Create Model
        OSPModel model = ospNewModel();
        ospAddGeometry(model, geom);
        ospCommit(model); 
        //! Camera
        OSPCamera camera = ospNewCamera("perspective");
        ospSet1f(camera, "aspect", wallSize.x / (float)wallSize.y);
        ospSet3fv(camera, "pos", cam_pos);
        ospSet3fv(camera, "up", cam_up);
        ospSet3fv(camera, "dir", cam_view);
        ospCommit(camera);
        // For distributed rendering we must use the MPI raycaster
        //! Lights
        OSPLight ambient_light = ospNewLight(renderer, "AmbientLight");
        ospSet1f(ambient_light, "intensity", 0.35f);
        ospSetVec3f(ambient_light, "color", osp::vec3f{174.f / 255.0f, 218.0f / 255.f, 1.0f});
        ospCommit(ambient_light);
        OSPLight directional_light0 = ospNewLight(renderer, "DirectionalLight");
        ospSet1f(directional_light0, "intensity",0.5f);
        //ospSetVec3f(directional_light0, "direction", osp::vec3f{80.f, 25.f, 35.f});
        ospSetVec3f(directional_light0, "direction", osp::vec3f{-1.f, -1.0f, -0.8f});
        ospSetVec3f(directional_light0, "color", osp::vec3f{1.0f, 232.f / 255.0f, 166.0f / 255.f});
        ospCommit(directional_light0);
        OSPLight directional_light1 = ospNewLight(renderer, "DirectionalLight");
        ospSet1f(directional_light1, "intensity", 0.04f);
        ospSetVec3f(directional_light1, "direction", osp::vec3f{0.f, -1.f, 0.f});
        ospSetVec3f(directional_light1, "color", osp::vec3f{1.0f, 1.0f, 1.0f});
        ospCommit(directional_light1);
        std::vector<OSPLight> light_list {ambient_light, directional_light0, directional_light1} ;
        OSPData lights = ospNewData(light_list.size(), OSP_OBJECT, light_list.data());
        ospCommit(lights);
        //! Setup the parameters for the renderer
        ospSet1i(renderer, "spp", 1);
        ospSet1f(renderer, "shadowsEnabled", 1);
        ospSet1f(renderer, "aoSamples", 1);
        ospSetVec4f(renderer, "bgColor", osp::vec4f{1.f, 1.f, 1.f, 1.f});
        ospSetObject(renderer, "model", model);
        ospSetObject(renderer, "camera", camera);
        ospSetObject(renderer, "lights", lights);
        ospCommit(renderer);

        

        //! FrameBuffer
        const osp::vec2i img_size{wallSize.x, wallSize.y};
        //! Pixel Op frame buffer 
        OSPFrameBuffer Pframebuffer = ospNewFrameBuffer(img_size, OSP_FB_NONE, OSP_FB_COLOR);
        //! Regular frame buffer
        OSPFrameBuffer framebuffer = ospNewFrameBuffer(img_size, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
        ospFrameBufferClear(framebuffer, OSP_FB_COLOR); 

        //! Set PixelOp parameters 
        if (connectToWall) {
          OSPPixelOp pixelOp = ospNewPixelOp("wall");
          ospSetString(pixelOp, "hostName", hostName.c_str());
          // ospSet1i(pixelOp, "barrierPerFrame", 0);
          ospSet1i(pixelOp, "port", port);
          OSPData wallInfoData = ospNewData(sizeof(dw2_info_t),
              OSP_RAW, &info, OSP_DATA_SHARED_BUFFER);
          ospCommit(wallInfoData);
          ospSetData(pixelOp, "wallInfo", wallInfoData);
          ospCommit(pixelOp);
          ospSetPixelOp(Pframebuffer, pixelOp);
        }

        int frameID = 0;

        double avgTime = 0;
        //!Render
        while(1){
            double lastTime = getSysTime();
            ospRenderFrame(Pframebuffer, renderer, OSP_FB_COLOR);
            // ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);
            double thisTime = getSysTime() - lastTime;

            avgTime = (thisTime + frameID * avgTime) / (frameID + 1); 
            frameID++;
            std::cout << "Frame Rate  = " << 1.f / avgTime << "\n";
            //double thisTime = getSysTime();
            //std::cout << "offload frame rate = " << 1.f / (thisTime - lastTime) << "\n";
             cam_pos[1] += 1.0;
             cam_view[1] = cam_target[1] - cam_pos[1];
             ospSet3fv(camera, "pos", cam_pos);
             ospSet3fv(camera, "dir", cam_view);
             ospCommit(camera); 
        } 

#if 0
        //!Debug: save framebuffer to image
        if(mpicommon::world.rank == 0){
            uint32_t* fb = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
            const std::string file = "ospray-sphere0.png";
            stbi_write_png(file.c_str(), img_size.x, img_size.y, 4, fb, img_size.x * 4);
            // writePPM("test.ppm", &image_size, fb);
            std::cout << "Image saved to 'ospray-sphere.png'\n";
            ospUnmapFrameBuffer(fb, framebuffer);
        }
#endif
        // Clean up all our objects
        ospFreeFrameBuffer(framebuffer);
        ospFreeFrameBuffer(Pframebuffer);
        ospRelease(renderer);
        ospRelease(camera);
        ospRelease(model);
        return 0;

        

    }


}// end dw2
