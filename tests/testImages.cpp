#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "dw2_client.h"
#include "../common/vec.h"
#include "../common/mpi_util.h"

namespace dw2{
    extern int dbg_rank;
    
    int mpi_rank = -1;
    int mpi_size = -1;

    void usage(const std::string &errMsg, int rank)
    {
        if (rank == 0) {

        if (errMsg != "")
            std::cerr << "Error: " << errMsg << std::endl << std::endl;
            std::cout << "Usage: ./testFrameRenderer <host> <port> <args>*" << std::endl;
        }
        exit(errMsg != "");
    }

    extern "C" int main(int ac, char **av){
        MPI_CALL(Init(&ac,&av));
        MPI_CALL(Comm_rank(MPI_COMM_WORLD,&mpi_rank));
        MPI_CALL(Comm_size(MPI_COMM_WORLD,&mpi_size));
        dbg_rank = mpi_rank;

        /*! various options to configure test renderer, so we can stress
            different pieces of the communication pattern */
        int    numFramesToRender = -1;
        size_t usleepPerFrame    = 0;
        bool   barrierPerFrame   = false;
        
        int w,h;
        std::string png;
            /*! if randomizeowner is false, each tile gets done by same rank
        every frame with pretty much same numebr of tiles for rank
        every frame; if it is true we will randomlzie this, which may
        also change the number of tiles any rank produces */
        bool        randomizeOwner  = false;
        std::string hostName = "";
        int         port = 0;
        int s = 32;

        for (int i = 1; i < ac; i++) {
            const std::string arg = av[i];
            if (arg == "-w")
                w = std::atoi(av[++i]);
            else if (arg == "-h")
                h = std::atoi(av[++i]);
            else if (arg == "--barrier-per-frame")
                barrierPerFrame = true;         
            else if (arg == "-hostname")
                hostName = av[++i];
            else if (arg == "-port")
                port = std::atoi(av[++i]);
            else if (arg == "-tile-size")
                s = std::atoi(av[++i]);
            else
                png = arg;
        }   
        std::cout << "Png image info : file " << png << " width " << w << " height " << h << std::endl;
        std::cout << "tile size = " << s << std::endl;
        vec2i  tileSize          = { s, s };
        // Load image
        int n;
        unsigned char *data = stbi_load(png.c_str(), &w, &h, &n, 4);

        // // Debug
        // for(int i = 10000; i < 15000; i++){
        //     std::cout << (int)data[i] << " ";
        // }
        // std::cout << "\n";
        
        //! got wall info
        dw2_info_t info;
        if (mpi_rank == 0)
            std::cout << "querying wall info from " << hostName << ":" << port << std::endl;
        if (dw2_query_info(&info,hostName.c_str(),port) != DW2_OK) {
            if (mpi_rank == 0)
                std::cerr << "could not connect to display wall ..." << std::endl;
            exit(1);
        }
        const vec2i wallSize = (const vec2i&)info.totalPixelsInWall;
        // vec2i wallSize; 
        // wallSize.x = 23040; wallSize.y = 5760;
        
        if(wallSize.x != w || wallSize.y != h){
            std::cout << "wall size = (" << wallSize.x << ", " << wallSize.y << ")" << "\n";
            std::cout << "image size = (" << w << ", " << h << ")" << "\n";
            std::cerr << "wall size is not equal to image size ... " << std::endl;
            exit(1);
        }

        dw2_connect(hostName.c_str(),port,mpi_size);
        std::vector<double> beginFrameWaitTime;
        std::vector<double> writeTileTime;
        std::vector<double> sendTileTime;
        std::vector<double> totalTime;
        // static double t_last = -1.;
        double t_last = getCurrentTime();

        for (int frameID=0; frameID!=numFramesToRender; frameID++){
            // std::cout << "node " << mpi_rank << " rendering frame " << frameID << "\n";
            double t_start = getCurrentTime();
            dw2_begin_frame();
            double t_end = getCurrentTime();
            // std::cout << "node " << mpi_rank << " begin frame wait time " << t_end - t_start << "s" << "\n";
            // beginFrameWaitTime.push_back(t_end - t_start);

            vec2i numTiles = divRoundUp(wallSize, tileSize);

            parallel_for(numTiles.x*numTiles.y,[&](int tileID){
                double ts_write = getCurrentTime();
                int tileOwner
                    = (randomizeOwner
                    ? ((tileID * 13 * 17 + 0x123ULL) * 11 *3 + 0x4343ULL)
                    : tileID)
                    % mpi_size;
                if (mpi_rank != tileOwner) return;

                const int tile_x  = tileID % numTiles.x;
                const int tile_y  = tileID / numTiles.x;
                const int begin_x = tile_x * tileSize.x;
                const int begin_y = tile_y * tileSize.y;
                const int end_x   = std::min(begin_x+tileSize.x,wallSize.x);
                const int end_y   = std::min(begin_y+tileSize.y,wallSize.y);

                uint32_t pixel[tileSize.x*tileSize.y];
                //! copy pixel from image
          
                for (int iy = begin_y;iy < end_y; iy++)
                    for (int ix = begin_x; ix < end_x;ix++) {
                        int local_x = ix-begin_x;
                        int local_y = iy-begin_y;
                        int global_idx = ix + wallSize.x * iy;
                        int local_idx = local_x + tileSize.x * local_y;
                        int r = data[global_idx * 4 + 0];
                        int g = data[global_idx * 4 + 1];
                        int b = data[global_idx * 4 + 2];
                        pixel[local_idx] = (r) | (g<<8) | (b<<16) | (r << 24);
                    }
                
                double ts_send = getCurrentTime();

                // std::cout << "node " << mpi_rank << " time of writing one tile " << ts_send - ts_write << " s" << "\n";
                // writeTileTime.push_back(ts_send - ts_write);
                dw2_send_rgba(begin_x,begin_y,
                                end_x-begin_x,
                                end_y-begin_y,
                                /* pitch */tileSize.x,
                                pixel);       
                double te_send = getCurrentTime();
                // std::cout << "node " << mpi_rank << " time of sending one tile " << te_send - ts_send << " s" << "\n";
                // sendTileTime.push_back(te_send - ts_send);
            });
        double t_out = getCurrentTime();
        // std::cout << "sending time for each frame on each render node : " << t_out - t_last << "\n";
        // std::cout << "#client: avg frame rate: " << (1.f/ (t_out - t_last)) << "fps" << "\n";
        // totalTime.push_back(t_out - t_last);
        // if (t_last < 0.) {
        //     t_last = t_out;
        //   }
        //   static double t_avg = 0.f;
        //   if (t_last >= 0) {
        //     double t = t_out - t_last;
        //     if (t_avg == 0.f)
        //       t_avg = t;
        //     else
        //       t_avg = (9.*t_avg + t)/10.;
        //     std::cout << "#client: avg frame rate: " << (1.f/t_avg) << "fps" << std::endl;
        //   }
        t_last = t_out;

      if (barrierPerFrame){          
            MPI_CALL(Barrier(MPI_COMM_WORLD));
      }
    //   //print statistic info
    //     double all = 0;
    //     for(size_t i = 0; i < beginFrameWaitTime.size(); i++){
    //         all += beginFrameWaitTime[i];
    //     }
    //     double a = all / beginFrameWaitTime.size();
    //     all = 0;
    //     for(size_t i = 0; i < writeTileTime.size(); i++){
    //         all += writeTileTime[i];
    //     }
    //     double b = all / writeTileTime.size();
    //     all = 0;
    //     for(size_t i = 0; i < sendTileTime.size(); i++){
    //         all += sendTileTime[i];
    //     }
    //     double c = all / sendTileTime.size();
    //     all = 0;
    //     for(size_t i = 0; i < totalTime.size(); i++){
    //         all += totalTime[i];
    //     }
    //     double d = all / totalTime.size();

    //     std::cout << "node #" << mpi_rank << " rendering frame " << frameID 
    //                                     << " Dw begin frame wait average time: " <<  a << "s"
    //                                     << " write tiles average time: " <<  b << "s" 
    //                                     << " send tiles average time: " << c << "s"
    //                                     << " total time for one render node: " << d << "s"
    //                                     << "\n";
      dw2_end_frame();
      
        // beginFrameWaitTime.clear();
        // writeTileTime.clear();
        // sendTileTime.clear();
        // totalTime.clear();
        usleep(usleepPerFrame);
    }

    return 0;
    }
}
