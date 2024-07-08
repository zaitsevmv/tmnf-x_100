//
// Created by Matvey on 04.07.2024.
//

#include "requests.h"

namespace po = boost::program_options;

int main(int argc, char** argv){
//    po::options_description desc("Options");
//    desc.add_options()
//            ("reset,r", "drop all databases");
//    po::variables_map vm;
//    po::store(po::parse_command_line(argc, argv, desc), vm);
//    po::notify(vm);
    requests req;
    req.LoadTemp("/home/temp.txt");
    req.LoadTempLeaderboards("/home/leaderboards.txt");
//
////    req.PrintLeaderboards();
    req.GetNoRecordMaps();
    req.Compare();
    req.MakeLeaderboards();

    req.SaveTemp("/home/temp.txt");
    req.SaveTempLeaderboards("/home/leaderboards.txt");
    req.PrintLeaderboards();
    return 0;
}