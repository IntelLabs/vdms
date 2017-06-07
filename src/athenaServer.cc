#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "chrono/Chrono.h"
#include "AthenaDemoHLS.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Please specify the query" << std::endl;
        exit(0);
    }

    // AthenaDemoHLS demo1("./tcia_pmgd", AthenaDemoHLS::TDB,
    //                     "/home/luisremi/visual_storm/hls_loader/tdb_dir/hls/");
    AthenaDemoHLS demo2("./tcia_pmgd", AthenaDemoHLS::EXT4,
                        "./images_hls/pngs/");

    if (std::atoi(argv[1]) == 1) {
        if (argc < 3){
            std::cout << "Query 1: Specify [drug | radiation]" << std::endl;
            exit(0);
        }
        std::cout << std::endl << "Running Query 1..." << std::endl;
        demo2.runQuery1(argv[2]);
    }
    else if (std::atoi(argv[1]) == 2) {
        if (argc < 4){
            std::cout << "Query 2: Specify age range" << std::endl;
            exit(0);
        }
        std::cout << std::endl << "Running Query 2..." << std::endl;
        demo2.runQuery2(std::atoi(argv[2]), std::atoi(argv[3]) );
    }
    else if (std::atoi(argv[1]) == 3) {
        if (argc < 3){
            std::cout << "Query 3: Specify drug name" << std::endl;
            exit(0);
        }
        std::cout << std::endl << "Running Query 3..." << std::endl;
        demo2.runQuery3(argv[2]);
        // demo1.runQuery3("Temozolomide");
    }
    else if (std::atoi(argv[1]) == 4) {
        std::string patient = "TCGA-02-0070";
        if (argc < 3){
            std::cout << "Query 4: Specify patient id" << std::endl;
            exit(0);
        }
        std::cout << std::endl << "Running Query 4..." << std::endl;
        std::cout << "VCL using TileDB" << std::endl;
        // demo1.runQuery4(argv[2]);
        std::cout << "VCL using PNG/Ext4" << std::endl;
        demo2.runQuery4(argv[2]);
    }
    else if (std::atoi(argv[1]) == 5) {
        std::string patient = "TCGA-02-0070";
        if (argc < 3){
            std::cout << "Query 5: Specify patient id" << std::endl;
            exit(0);
        }
        std::cout << std::endl << "Running Query 5..." << std::endl;
        std::cout << "VCL using TileDB" << std::endl;
        // demo1.runQuery5(argv[2]);
        std::cout << "VCL using PNG/Ext4" << std::endl;
        demo2.runQuery5(argv[2]);
    }
    else if (std::atoi(argv[1]) == 6) {
        std::string patient = "TCGA-02-0070";
        if (argc < 3){
            std::cout << "Query 6: Specify patient id" << std::endl;
            exit(0);
        }
        std::cout << std::endl << "Running Query 6..." << std::endl;
        std::cout << "VCL using TileDB" << std::endl;
        // demo1.runQuery6(argv[2]);
        std::cout << "VCL using PNG/Ext4" << std::endl;
        demo2.runQuery6(argv[2]);
    }


    return 0;
}
