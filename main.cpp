//
// Created by Matvey on 04.07.2024.
//

#include <chrono>
#include <ctime>
#include <iostream>

#include "requests.hpp"

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
    std::string dataFile = "data/temp.txt";
    std::string leadersFile = "data/leaderboards.txt";
    std::string frontendFile = "data/data_to_frontend.txt";
    requests req;
    req.LoadTemp(dataFile);
    req.LoadTempLeaderboards(leadersFile);

    auto start = std::chrono::system_clock::now();
    std::time_t startTime = std::chrono::system_clock::to_time_t(start);
    std::cout << "Getting records start time: " << std::ctime(&startTime) << std::endl;
    req.GetNoRecordMaps();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = end-start;
    std::cout << "Elapsed time: " << elapsedTime.count() << std::endl;

    req.Compare();
    
    start = std::chrono::system_clock::now();
    startTime = std::chrono::system_clock::to_time_t(start);
    std::cout << "Getting replays start time: " << std::ctime(&startTime) << std::endl;
    req.MakeLeaderboards();
    end = std::chrono::system_clock::now();
    elapsedTime = end-start;
    std::cout << "Elapsed time: " << elapsedTime.count() << std::endl;

    req.UpdateLeaderboardsNames();

    // req.SaveTemp(dataFile);
    // req.SaveDataForFrontend(frontendFile);
    // req.SaveTempLeaderboards(leadersFile);
    // req.PrintLeaderboards();
    return 0;
}