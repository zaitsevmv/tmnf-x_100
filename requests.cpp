//
// Created by Matvey on 04.07.2024.
//

#include "requests.hpp"

#include <atomic>
#include <boost/json/stream_parser.hpp>
#include <boost/json/impl/parse.ipp>
#include <boost/json/impl/serialize.ipp>
#include <boost/json/value.hpp>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <mutex>
#include <numeric>
#include <thread>
#include <variant>

#include <curl/curl.h>
#include <boost/json/src.hpp>
#include <vector>

using namespace boost;

std::string toString(trackTag tag){
    switch (tag) {
        case Normal:
            return "Normal";
        case Stunt:
            return "Stunt";
        case Maze:
            return "Maze";
        case Offroad:
            return "Offroad";
        case Laps:
            return "Laps";
        case Fullspeed:
            return "Fullspeed";
        case LOL:
            return "LOL";
        case Tech:
            return "Tech";
        case SpeedTech:
            return "SpeedTech";
        case RPG:
            return "RPG";
        case PressForward:
            return "PressForward";
        case Trial:
            return "Trial";
        case Grass:
            return "Grass";
        case All:
            return "All";
    }
    return "";
}

std::string toString(TrackDifficulty difficulty){
    switch (difficulty) {
        case TrackDifficulty::Beginner:
            return "Beginner";
        case TrackDifficulty::Intermediate:
            return "Intermediate";
        case TrackDifficulty::Expert:
            return "Expert";
        case TrackDifficulty::Lunatic:
            return "Lunatic";
    }
    return "";
}

void requests::SaveTemp(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time) << std::endl;
    for(const auto& [id, beaten, tags, difficulty]: allTracks){
        fout << id << ' ' << beaten << ' ' << static_cast<int64_t>(difficulty) << std::endl;
        for(const auto& a: tags){
            fout << a << ' ';
        }
        fout << std::endl;
    }
    fout.close();
}

void requests::LoadTemp(const std::string &tempFile) {
    std::ifstream fin(tempFile);
    std::string time;
    getline(fin, time);
    std::cout << "Last tracks update: " << time << std::endl;
    allTracks.clear();
    int64_t id;
    bool beaten;
    std::string tagsLine;
    std::vector<trackTag> tags;
    fin >> id;
    while(fin >> beaten){
        // int64_t trackDifficulty = 0;
        // if (false) {
        //     fin >> trackDifficulty;
        // }
        int curTag;
        while(fin >> curTag && curTag <= 12){
            tags.push_back(static_cast<trackTag>(curTag));
        }
        allTracks.emplace_back(id, false, tags, static_cast<TrackDifficulty>(TrackDifficulty::Beginner));
        id = curTag;
        tags.clear();
    }
    fin.close();
}

typedef std::variant<std::string, int, int64_t, std::vector<std::string>> param_cell;

struct httpsURLConstructor{
    httpsURLConstructor(const std::string& host, const std::string& target, const std::map<std::string,param_cell>& params):
        host{host}, target{target}, params{params}
    {
        UpdateParams(params);
    }

    void UpdateParams(const std::map<std::string,param_cell>& newParams){
        params = newParams;
        std::string paramsString = "?";
        for(const auto& [key, val]: params){
            paramsString += key + "=" + std::visit([](auto arg){
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>)
                    return arg;
                else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t>)
                    return std::to_string(arg);
                else if constexpr (std::is_same_v<T, std::vector<std::string>>){
                    std::string t;
                    for(const std::string& a: arg){
                        t += a + "%2C";
                    }
                    t.resize(t.size()-3);
                    return t;
                }
            }, val);
            paramsString += "&";
        }
        paramsString.pop_back();
        URL = "https://" + host + target + paramsString;
    }

    std::string GetURL(){
        return URL;
    }
private:
    std::string host;
    std::string target;
    std::map<std::string, param_cell> params;
    std::string URL;
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch (std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
    return newLength;
}

void requests::GetNoRecordMaps() {
    const std::string host = "tmnf.exchange";
    const std::string target = "/api/tracks";
    std::map<std::string, param_cell> params =
            {{"fields", std::vector<std::string>{"TrackId", "Tags", "Difficulty"}},
             {"count", mapCount} ,
             {"inhasrecord", 0}};

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        httpsURLConstructor uc(host, target, params);
        std::cout << "Getting tracks." << std::endl;
        while(mapCount <= lastResponseSize){
            curl_easy_setopt(curl, CURLOPT_URL, uc.GetURL().c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if(res != CURLE_OK){
                std::cerr << curl_easy_strerror(res) << "curl_easy_perform() failed: %s\n" << std::endl;
                continue;
            }

            std::fstream json_out;
            json_out.open("data/response.json", std::ios_base::out);
            json_out << readBuffer << std::endl;
            readBuffer.clear();
            json_out.close();
            std::fstream json_in("data/response.json");
            std::string abc;
            json_in >> abc;
            if(abc.find("\"More\"") >= abc.size()){
                continue;
            }
            GetNoRecordJSON("data/response.json");
            params =
                    {{"fields", std::vector<std::string>{"TrackId", "Tags", "Difficulty"}},
                     {"count", mapCount},
                     {"inhasrecord", 0},
                     {"after", lastNoRecord}};
            uc.UpdateParams(params);
        }
        std::remove("data/response.json");
        noRec = noRecordTracks.size();
        std::cout << "Got " << noRecordTracks.size() <<  " tracks." << std::endl;
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void requests::GetNoRecordJSON(const std::string& jsonFile) {
    std::ifstream file(jsonFile);
    json::value jv = boost::json::parse(file);
    json::value resultsValue = jv.at("Results");
    lastResponseSize = resultsValue.as_array().size();
    for (const auto& result: resultsValue.as_array()) {
        TrackStruct newTrack;
        json::value trackId = result.at("TrackId");
        newTrack.trackId = trackId.as_int64();
        lastNoRecord = newTrack.trackId;

        json::value trackTags = result.at("Tags");
        std::vector<trackTag> tags;
        for(const auto& tag: trackTags.as_array()){
            tags.push_back(static_cast<trackTag>(tag.as_int64()));
        }

        json::value trackDifficulty = result.at("Difficulty");
        newTrack.difficulty = static_cast<TrackDifficulty>(trackDifficulty.as_int64());
        newTrack.tags = tags;
        noRecordTracks.emplace(newTrack.trackId, newTrack);
    }
    noRecordTracks.erase(0);
}

void requests::PrintSet() {
    for(const auto& [id, track]: noRecordTracks){
        std::cout << id;
        for(const auto& tag: track.tags){
            std::cout << ' ' << tag;
        }
        std::cout << std::endl;
    }
    std::cout << noRecordTracks.size() << std::endl;
}

void requests::PrintMap() {
    for(const auto& [id, beaten, tags, difficulty]: allTracks){
        std::cout << id << ' ' << beaten << ' ' << static_cast<int64_t>(difficulty) << " [ ";
        for(const auto& a: tags){
            std::cout << a << ' ';
        }
        std::cout << "]" << std::endl;
    }
    std::cout << allTracks.size() << std::endl;
}

void requests::Compare() {
    std::cout << std::endl;
    if(allTracks.empty()){
        for(auto& [id, track]: noRecordTracks){
            allTracks.emplace_back(id, false, track.tags, track.difficulty);
        }
        return;
    }
    if(!extraTracks.empty()){
        for(auto& [id, track]: extraTracks){
            allTracks.emplace_back(id, false, track.tags, track.difficulty);
        }
        return;
    }
    int totalBeaten{0};
    int newBeaten{0};
    for(auto& [id, beaten, tags, difficulty]: allTracks){
        if(!beaten && !noRecordTracks.contains(id)){
            beaten = true;
            newBeaten++;
            tracksToCheck.push_back(id);
        }else if(beaten){
            oldRecords.emplace(id);
            totalBeaten ++;
        }
        noRecordTracks.erase(id);
    }
    totalBeaten+=newBeaten;
    std::cout << "################\nTotal beaten: " << totalBeaten << std::endl
        << "New beaten: " << newBeaten << std::endl;
    int newMaps = 0;
    for(auto& [id, track]: noRecordTracks){
        allTracks.emplace_back(id, false, track.tags, track.difficulty);
        newMaps++;
    }
    std::cout << "New maps: " << newMaps << std::endl << "################" << std::endl;
}

void requests::PrintWithRecords() {
    int totalRecords = 0;
    for(const auto& [id, beaten, tags, difficulty]: allTracks){
        if(beaten){
            std::cout << id << ' ' << beaten << ' ' << static_cast<int64_t>(difficulty) << " [ ";
            for(const auto& a: tags){
                std::cout << a << ' ';
            }
            std::cout << "]" << std::endl;
            totalRecords++;
        }
    }
    std::cout << totalRecords << std::endl;
}

void requests::MakeLeaderboards() {
    std::cout << "Replays to check: " << tracksToCheck.size() << std::endl;
    std::cout << "Getting replays." << std::endl;
    uint batchSize = 32;
    std::vector<std::thread> batchThreads(batchSize);
    std::vector<std::atomic_flag> mapStatus(tracksToCheck.size());
    for (auto j = 0ul; j < batchSize; j++) {
        batchThreads[j] = std::thread([this, j, &mapStatus]{
            size_t currentMap = 0;
            while (currentMap < tracksToCheck.size()) {
                if (mapStatus[currentMap].test_and_set()) {
                    currentMap++;
                    continue;
                }
                GetReplaysFromMap(tracksToCheck[currentMap]);
                currentMap++;
            }
        });
    }
    for (auto& th: batchThreads) {
        th.join();
    }
    std::cout << "Got replays." << std::endl;
}

std::pair<int64_t, std::string> GetFinisherIdName(const std::string &jsonString) {
    json::value jv = json::parse(jsonString);
    json::value results = jv.at("Results");
    for(const auto& res: results.as_array()) {
        json::value trackFinisher = res.at("User");
        int64_t userId = trackFinisher.at("UserId").as_int64();
        std::string finisherName = trackFinisher.at("Name").as_string().c_str();
        for(auto& c: finisherName){
            if(c == ' '){
                c = '_';
            }
        }
        if(!finisherName.empty() && userId > 0){
            return {userId, finisherName};
        }
        break;
    }
    return {0, ""};
}

void requests::GetReplaysFromMap(const int64_t trackId) {
    const std::string host = "tmnf.exchange";
    const std::string target = "/api/replays";
    std::map<std::string, param_cell> params =
            {{"fields", std::vector<std::string>{"User.UserId", "User.Name"}},
             {"trackId", trackId},
             {"count", 1}};

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        httpsURLConstructor uc(host, target, params);
        int attempts = 0;
        while(attempts < 3){
            curl_easy_setopt(curl, CURLOPT_URL, uc.GetURL().c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if(res != CURLE_OK){
                std::cerr << curl_easy_strerror(res) << "curl_easy_perform() failed" << std::endl;
                continue;
            }
            if(readBuffer.find("\"Type\"") < readBuffer.size() || readBuffer.find("\"type\"") < readBuffer.size()){
                return;
            }
            if(readBuffer.find("\"More\"") >= readBuffer.size()){
                attempts++;
                continue;
            }
            auto finisher_id_name = GetFinisherIdName(readBuffer);
            UpdateLeaderboards(trackId, finisher_id_name.second, finisher_id_name.first);
            break;
        }
        curl_easy_cleanup(curl);
    }
}

void requests::UpdateLeaderboards(const int64_t trackId, const std::string &finisherName, const int64_t finisherId) {
    for(const auto& [id, beaten, tags, difficulty]: allTracks){
        if(id == trackId){
            std::lock_guard<std::mutex> lock(leaderboardsMutex);
            if(auto iter = leaderboardsByTag[All].find(finisherId); iter != leaderboardsByTag[All].end()){
                iter->second.finishedMaps++;
                iter->second.playerName = finisherName;
                iter->second.playerId = finisherId;
            } else{
                leaderboardsByTag[All].emplace(finisherId, Player{
                    .playerName = finisherName, .playerId = finisherId, .finishedMaps = 1
                });
            }
            for(const auto& tag: tags){
                if(auto iter = leaderboardsByTag[tag].find(finisherId); iter != leaderboardsByTag[tag].end()){
                    iter->second.finishedMaps++;
                    iter->second.playerName = finisherName;
                    iter->second.playerId = finisherId;
                } else{
                    leaderboardsByTag[tag].emplace(finisherId, Player{
                        .playerName = finisherName, .finishedMaps = 1
                    });
                }
            }
            if(auto iter = leaderboardsByDifficulty[difficulty].find(finisherId); iter != leaderboardsByTag[All].end()){
                iter->second.finishedMaps++;
                iter->second.playerName = finisherName;
                iter->second.playerId = finisherId;
            } else{
                leaderboardsByDifficulty[difficulty].emplace(finisherId, Player{
                    .playerName = finisherName, .finishedMaps = 1
                });
            }
            return;
        }
    }
}

void requests::UpdateLeaderboardsNames() {
    for(const auto& [id, name_count]: leaderboardsByTag[All]){
        for(int tag = Normal; tag < All; tag++){
            leaderboardsByTag[static_cast<trackTag>(tag)][id].playerName = name_count.playerName;
        }

        for(int64_t difficulty = static_cast<int64_t>(TrackDifficulty::Beginner); difficulty <= static_cast<int64_t>(TrackDifficulty::Lunatic); difficulty++){
            leaderboardsByDifficulty[static_cast<TrackDifficulty>(difficulty)][id].playerName = name_count.playerName;
        }
    }
}

void requests::SaveTempLeaderboardsByTag(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time) << std::endl;
    for(const auto& [id, subTable]: leaderboardsByTag){
        fout << id << ' ' << subTable.size() << std::endl;
        for(const auto& [playerId, player]: subTable){
            fout << playerId << ' ' << player.playerName << ' ' << player.finishedMaps << std::endl;
        }
    }
    fout.close();
}

void requests::SaveTempLeaderboardsByDifficulty(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time) << std::endl;
    for(const auto& [id, subTable]: leaderboardsByDifficulty){
        fout << static_cast<int64_t>(id) << ' ' << subTable.size() << std::endl;
        for(const auto& [playerId, player]: subTable){
            fout << playerId << ' ' << player.playerName << ' ' << player.finishedMaps << std::endl;
        }
    }
    fout.close();
}

void requests::LoadTempLeaderboardsByTag(const std::string &tempFile) {
    std::ifstream fin(tempFile);
    std::string time;
    getline(fin, time);
    std::cout << "Last leaderboards update: " << time << std::endl;
    int tag;
    int n{0};
    while(fin >> tag){
        fin >> n;
        for(int i = 0; i < n; i++){
            int64_t playerId;
            std::string playerName;
            int playerCount;
            fin >> playerId >> playerName >> playerCount;
            leaderboardsByTag[static_cast<trackTag>(tag)].emplace(playerId, Player{
                .playerName = playerName, .playerId = playerId, .finishedMaps = playerCount
            });
        }
    }
    fin.close();
}

void requests::LoadTempLeaderboardsByDifficulty(const std::string &tempFile) {
    std::ifstream fin(tempFile);
    std::string time;
    getline(fin, time);
    std::cout << "Last leaderboards update: " << time << std::endl;
    int64_t difficulty = 0;
    int n{0};
    while(fin >> difficulty){
        fin >> n;
        for(int i = 0; i < n; i++){
            int64_t playerId;
            std::string playerName;
            int playerCount;
            fin >> playerId >> playerName >> playerCount;
            leaderboardsByDifficulty[static_cast<TrackDifficulty>(difficulty)].emplace(playerId, Player{
                .playerName = playerName, .playerId = playerId, .finishedMaps = playerCount
            });
        }
    }
    fin.close();
}

void requests::PrintLeaderboards() {
    for(const auto& [tag, subTable]: leaderboardsByTag){
        std::vector<Player> playerData;
        playerData.reserve(subTable.size());
        for (const auto& [id, player]: subTable) {
            playerData.push_back(player);
        }
        std::sort(playerData.begin(), playerData.end(),[](const Player& a, const Player& b){
            return a.finishedMaps > b.finishedMaps;
        });
        if(tag == All){
            int count = std::accumulate(playerData.begin(), playerData.end(), 0, [](int init, const Player& pl) {return init + pl.finishedMaps;});
            std::cout << "\nLeaderboard entries: " << count << std::endl;
        }
        playerData.resize(10);
        std::cout << "###############\n" << toString(tag) << "\n###############" << std::endl;
        for(const auto& [playerId, playerName, finishedMaps]: playerData){
            std::cout << playerName << ' ' << finishedMaps << std::endl;
        }
    }
}

void requests::SaveDataForFrontendByTag(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time);
    fout << noRec << std::endl;
    fout << oldRecords.size()+tracksToCheck.size() << std::endl;
    for(const auto& [tag, subTable]: leaderboardsByTag){
        std::vector<Player> playerData;
        playerData.reserve(subTable.size());
        for (const auto& [id, player]: subTable) {
            playerData.push_back(player);
        }
        std::sort(playerData.begin(), playerData.end(),[](const Player& a, const Player& b){
            return a.finishedMaps > b.finishedMaps;
        });
        playerData.resize(10);
        fout << toString(tag) << std::endl;
        for(const auto& [playerId, playerName, finishedMaps]: playerData){
            if(finishedMaps == 0) break;
            fout << playerName << ' ' << finishedMaps << std::endl;
        }
        fout << "=" << std::endl;
    }
    fout.close();
}

void requests::SaveDataForFrontendByDifficulty(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time);
    fout << noRec << std::endl;
    fout << oldRecords.size()+tracksToCheck.size() << std::endl;
    for(const auto& [difficulty, subTable]: leaderboardsByDifficulty){
        std::vector<Player> playerData;
        playerData.reserve(subTable.size());
        for (const auto& [id, player]: subTable) {
            playerData.push_back(player);
        }
        std::sort(playerData.begin(), playerData.end(),[](const Player& a, const Player& b){
            return a.finishedMaps > b.finishedMaps;
        });
        playerData.resize(10);
        fout << toString(difficulty) << std::endl;
        for(const auto& [playerId, playerName, finishedMaps]: playerData){
            if(finishedMaps == 0) break;
            fout << playerName << ' ' << finishedMaps << std::endl;
        }
        fout << "=" << std::endl;
    }
    fout.close();
}

void requests::AddExtra() {
    const std::string host = "tmnf.exchange";
    const std::string target = "/api/tracks";

    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        std::cout << "Getting extra tracks." << std::endl;
        for(const auto& [id, tags]: extraTracks){
            std::map<std::string, param_cell> params =
                    {{"fields", std::vector<std::string>{"TrackId", "Tags", "Difficulty"}},
                     {"count", 1} ,
                     {"id", id}};
            httpsURLConstructor uc(host, target, params);
            curl_easy_setopt(curl, CURLOPT_URL, uc.GetURL().c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if(res != CURLE_OK){
                std::cerr << curl_easy_strerror(res) << "curl_easy_perform() failed: %s\n" << std::endl;
                continue;
            }

            std::fstream json_out;
            json_out.open("data/response.json", std::ios_base::out);
            json_out << readBuffer << std::endl;
            readBuffer.clear();
            json_out.close();
            std::fstream json_in("data/response.json");
            std::string abc;
            json_in >> abc;
            if(abc.find("\"More\"") >= abc.size()){
                continue;
            }
            GetNoRecordJSON("data/response.json");
            std::cout << noRecordTracks.size() << std::endl;
        }
        std::remove("data/response.json");
        std::cout << "Got extra tracks." << std::endl;
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void requests::LoadExtra(const std::string &tempFile) {
    std::ifstream fin(tempFile);
    int64_t trackId = 0;
    while(fin >> trackId){
        extraTracks.emplace(trackId, TrackStruct{});
    }
    for(const auto& a: allTracks){
        if(extraTracks.contains(get<0>(a))) {
            extraTracks.erase(get<0>(a));
        }
    }
    std::cout << "Extra tracks: " << extraTracks.size();
    fin.close();
}
