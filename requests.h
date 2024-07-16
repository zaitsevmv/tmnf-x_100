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
    void LoadTemp(const std::string& tempFile);
    void GetNoRecordMaps();
    void GetReplaysFromMap(const int64_t trackId);
    void GetNoRecordJSON(const std::string& jsonFile);

    void LoadExtra(const std::string& tempFile);
    void AddExtra();

    void Compare();
    void MakeLeaderboards();
    void UpdateLeaderboards(int64_t trackId, const std::string& finisherName, const int64_t finisherId);
    void SaveTempLeaderboards(const std::string& tempFile);
    void SaveDataForFrontend(const std::string& tempFile);
    void LoadTempLeaderboards(const std::string& tempFile);

    void PrintSet();
    void PrintMap();
    void PrintLeaderboards();
    void PrintWithRecords();
private:
    std::map<int64_t,std::vector<trackTag>> noRecordTracks;
    std::vector<std::tuple<int64_t, bool, std::vector<trackTag>>> allTracks;
    std::map<trackTag, std::map<int64_t, std::pair<std::string, int>>> leaderboards;
    std::vector<int64_t> tracksToCheck;
    std::set<int64_t> oldRecords;
    std::map<int64_t,std::vector<trackTag>> extraTracks;
    int64_t lastNoRecord;
    int noRec = 0;
    size_t lastResponseSize = mapCount;
};


#endif //TMNF_X_100_REQUESTS_H
