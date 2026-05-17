//
// Created by Matvey on 04.07.2024.
//

#ifndef TMNF_X_100_REQUESTS_H
#define TMNF_X_100_REQUESTS_H

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "boost/program_options.hpp"

constexpr int mapCount = 1000;

enum trackTag{Normal, Stunt, Maze, Offroad, Laps, Fullspeed, LOL, Tech, SpeedTech, RPG, PressForward, Trial, Grass, All};

enum class TrackDifficulty: int64_t {
    Beginner = 0, Intermediate = 1, Expert = 2, Lunatic = 3, Unknown = 10
};

class requests {
public:
    void SaveTemp(const std::string& tempFile);
    void LoadTemp(const std::string& tempFile);
    void GetNoRecordMaps();
    void GetAllMapsForDifficulty();
    void GetReplaysFromMap(const int64_t trackId);
    void GetNoRecordJSON(const std::string& jsonFile);
    void GetAllMapsForDifficultyJSON(const std::string& jsonFile);

    void LoadExtra(const std::string& tempFile);
    void AddExtra();

    void Compare();
    void MakeLeaderboards();
    void UpdateLeaderboards(int64_t trackId, const std::string& finisherName, int64_t finisherId);
    void UpdateLeaderboardsNames();
    void SaveTempLeaderboardsByTag(const std::string& tempFile);
    void SaveTempLeaderboardsByDifficulty(const std::string& tempFile);
    void SaveDataForFrontendByTag(const std::string& tempFile);
    void SaveDataForFrontendByDifficulty(const std::string& tempFile);
    void LoadTempLeaderboardsByTag(const std::string& tempFile);
    void LoadTempLeaderboardsByDifficulty(const std::string& tempFile);

    void PrintSet();
    void PrintMap();
    void PrintLeaderboards();
    void PrintWithRecords();
private:
    struct TrackStruct {
        int64_t trackId = 0;
        TrackDifficulty difficulty;
        std::vector<trackTag> tags;
        bool beaten = false;
    };

    struct Player {
        std::string playerName;
        int64_t playerId = 0;
        int64_t finishedMaps = 0;
    };

    std::unordered_map<int64_t, TrackStruct> noRecordTracks;
    std::unordered_map<int64_t, TrackStruct> allTracksIfNeeded;
    std::unordered_map<int64_t, TrackStruct> allTracks;

    using leaderboardValue = std::unordered_map<int64_t, Player>;
    std::unordered_map<trackTag, leaderboardValue> leaderboardsByTag;
    std::unordered_map<TrackDifficulty, leaderboardValue> leaderboardsByDifficulty;
    std::vector<int64_t> tracksToCheck;
    std::set<int64_t> oldRecords;
    std::unordered_map<int64_t, TrackStruct> extraTracks;
    int64_t lastNoRecord;
    int noRec = 0;
    size_t lastResponseSize = mapCount;

    std::mutex leaderboardsMutex;
};


#endif //TMNF_X_100_REQUESTS_H
