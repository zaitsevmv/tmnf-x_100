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
    req.ReadTemp("/home/temp.txt");
    req.PrintWithRecords();
//    req.GetNoRecordMaps();
//    req.Compare();
//    req.SaveTemp("/home/temp.txt");
    return 0;
}