//
// Created by Matvey on 04.07.2024.
//

#ifndef TMNF_X_100_REQUESTS_H
#define TMNF_X_100_REQUESTS_H

#include "boost/program_options.hpp"

constexpr int mapCount = 1000;

enum trackTag{Normal, Stunt, Maze, Offroad, Laps, Fullspeed, LOL, Tech, SpeedTech, RPG, PressForward, Trial, Grass, All};

class requests {
public:
    void SaveTemp(const std::string& tempFile);
    void ReadTemp(const std::string& tempFile);
    void GetNoRecordMaps();
    void GetReplays();
    void GetNoRecordJSON(const std::string& jsonFile);

    void Compare();

    void PrintSet();
    void PrintMap();
    void PrintWithRecords();
private:
    std::map<int64_t,std::vector<trackTag>> noRecordMaps;
    std::vector<std::tuple<int64_t, bool, std::vector<trackTag>>> allNoRecord;
    std::map<trackTag, std::vector<int>> leaderboards;
    int64_t lastNoRecord;
    size_t lastResponseSize = mapCount;
};


#endif //TMNF_X_100_REQUESTS_H
