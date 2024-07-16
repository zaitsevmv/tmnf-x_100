//
// Created by Matvey on 04.07.2024.
//

#include "requests.h"

//namespace po = boost::program_options;

int main(int argc, char** argv){
//    po::options_description desc("Options");
//    desc.add_options()
//            ("data", po::value<std::string>()->default_value("/home/temp.txt"), "path to data file")
//            ("leaders", po::value<std::string>()->default_value("/home/leaderboards.txt"), "path to leaderboards file")
//            ("front", po::value<std::string>()->default_value("/home/temp.txt"), "path to frontend data file")
//            ("reset", "delete all data");
//    po::variables_map vm;
//    po::store(po::parse_command_line(argc, argv, desc), vm);
//    try {
//        po::notify(vm);
//    } catch (...){
//        return 2;
//    }
//    std::string dataFile = vm["data"].as<std::string>();
//    std::string leadersFile = vm["data"].as<std::string>();
//    std::string frontendFile = vm["data"].as<std::string>();
    std::string dataFile = "/home/temp.txt";
    std::string leadersFile = "/home/leaderboards.txt";
    std::string frontendFile = "/home/data_to_frontend.txt";
    requests req;
    req.LoadTemp(dataFile);
    req.LoadTempLeaderboards(leadersFile);


    req.GetNoRecordMaps();
    req.Compare();
    req.MakeLeaderboards();

    req.SaveTemp(dataFile);
    req.SaveDataForFrontend(frontendFile);
    req.SaveTempLeaderboards(leadersFile);
    req.PrintLeaderboards();
    return 0;
}