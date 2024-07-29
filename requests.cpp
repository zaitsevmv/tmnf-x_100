//
// Created by Matvey on 04.07.2024.
//

#include "requests.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <variant>

#include "curl/curl.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

namespace pt = boost::property_tree;

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

void requests::SaveTemp(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time) << std::endl;
    for(const auto& [id, beaten, tags]: allTracks){
        fout << id << ' ' << beaten << std::endl;
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
        int curTag;
        while(fin >> curTag && curTag <= 12){
            tags.push_back(static_cast<trackTag>(curTag));
        }
        allTracks.emplace_back(id, beaten, tags);
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
            {{"fields", std::vector<std::string>{"TrackId", "Tags"}},
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
            json_out.open("/home/response.json", std::ios_base::out);
            json_out << readBuffer << std::endl;
            readBuffer.clear();
            json_out.close();
            std::fstream json_in("/home/response.json");
            std::string abc;
            json_in >> abc;
            if(abc.find("\"More\"") >= abc.size()){
                continue;
            }
            GetNoRecordJSON("/home/response.json");
            params =
                    {{"fields", std::vector<std::string>{"TrackId", "Tags"}},
                     {"count", mapCount},
                     {"inhasrecord", 0},
                     {"after", lastNoRecord}};
            uc.UpdateParams(params);
        }
        std::remove("/home/response.json");
        noRec = noRecordTracks.size();
        std::cout << "Got " << noRecordTracks.size() <<  " tracks." << std::endl;
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void requests::GetNoRecordJSON(const std::string& jsonFile) {
    std::ifstream file(jsonFile);
    pt::ptree p;
    pt::read_json(file, p);
    for (const auto& [key, value]: p) {
        if(key == "Results"){
            lastResponseSize = value.size();
            for (const auto& a: value) {
                std::pair<int64_t,std::vector<trackTag>> newTrack;
                for (const auto& [k, val]: a.second) {
                    if(k == "TrackId"){
                        newTrack.first = val.get_value<int64_t>();
                        lastNoRecord = newTrack.first;
                    } else if(k == "Tags"){
                        std::vector<trackTag> tags;
                        for(const auto& t: val){
                            tags.push_back(static_cast<trackTag>(t.second.get_value<int>()));
                        }
                        newTrack.second = tags;
                        noRecordTracks.insert(newTrack);
                    }
                }
            }
            noRecordTracks.erase(0);
            return;
        }
    }
}

void requests::PrintSet() {
    for(const auto& a: noRecordTracks){
        std::cout << a.first;
        for(const auto& v: a.second){
            std::cout << ' ' << v;
        }
        std::cout << std::endl;
    }
    std::cout << noRecordTracks.size() << std::endl;
}

void requests::PrintMap() {
    for(const auto& [id, beaten, tags]: allTracks){
        std::cout << id << ' ' << beaten << " [ ";
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
        for(auto& [id, tags]: noRecordTracks){
            allTracks.emplace_back(id, false, tags);
        }
        return;
    }
    if(!extraTracks.empty()){
        for(auto& [id, tags]: extraTracks){
            allTracks.emplace_back(id, false, tags);
        }
        return;
    }
    int totalBeaten{0};
    int newBeaten{0};
    for(auto& [id, beaten, tags]: allTracks){
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
    for(auto& [id, tags]: noRecordTracks){
        allTracks.emplace_back(id, false, tags);
        newMaps++;
    }
    std::cout << "New maps: " << newMaps << std::endl << "################" << std::endl;
}

void requests::PrintWithRecords() {
    int totalRecords = 0;
    for(const auto& [id, beaten, tags]: allTracks){
        if(beaten){
            std::cout << id << ' ' << beaten << " [ ";
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
    if(leaderboards.empty()){
        for (int a = Normal; a >= All; a++){
            leaderboards.emplace(static_cast<trackTag>(a), std::map<int64_t, std::pair<std::string, int>>());
        }
    }
    std::cout << "Replays to check: " << tracksToCheck.size() << std::endl;
    std::cout << "Getting replays." << std::endl;
    for(const auto& a: tracksToCheck){
        GetReplaysFromMap(a);
    }
    std::cout << "Got replays." << std::endl;
}

std::pair<int64_t, std::string> GetFinisherIdName(const std::string &jsonFile) {
    std::ifstream file(jsonFile);
    pt::ptree p;
    pt::read_json(file, p);
    for (const auto& [key, value]: p) {
        if(key == "Results"){
            for(const auto& a: value) {
                for(const auto& b: a.second) {
                    int64_t userId{0};
                    std::string finisherName;
                    for(const auto& [k, val]: b.second) {
                        if(k == "UserId"){
                            userId = val.get_value<int64_t>();
                        }
                        if(k == "Name"){
                            finisherName = val.get_value<std::string>();
                            for(auto& a: finisherName){
                                if(a == ' '){
                                    a = '_';
                                }
                            }
                        }
                        if(!finisherName.empty() && userId > 0){
                            return {userId, finisherName};
                        }
                    }
                }
            }
        }
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

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        httpsURLConstructor uc(host, target, params);
        while(true){
            curl_easy_setopt(curl, CURLOPT_URL, uc.GetURL().c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if(res != CURLE_OK){
                std::cerr << curl_easy_strerror(res) << "curl_easy_perform() failed: %s\n" << std::endl;
                continue;
            }

            std::fstream json_out;
            json_out.open("/home/response_replay.json", std::ios_base::out);
            json_out << readBuffer << std::endl;
            readBuffer.clear();
            json_out.close();
            std::fstream json_in("/home/response_replay.json");
            std::string abc;
            json_in >> abc;
            if(abc.find("\"Type\"") < abc.size()){
                return;
            }
            if(abc.find("\"More\"") >= abc.size()){
                continue;
            }
            auto finisher_id_name = GetFinisherIdName("/home/response_replay.json");
            UpdateLeaderboards(trackId, finisher_id_name.second, finisher_id_name.first);
            break;
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void requests::UpdateLeaderboards(const int64_t trackId, const std::string &finisherName, const int64_t finisherId) {
    for(const auto& [id, beaten, tags]: allTracks){
        if(id == trackId){
            if(leaderboards[All].find(finisherId) != leaderboards[All].end()){
                leaderboards[All][finisherId].second++;
                leaderboards[All][finisherId].first = finisherName;
            } else{
                leaderboards[All].emplace(finisherId, std::make_pair(finisherName, 1));
            }
            for(const auto& a: tags){
                if(leaderboards[a].contains(finisherId)){
                    leaderboards[a][finisherId].second++;
                    leaderboards[a][finisherId].first = leaderboards[All][finisherId].first;
                } else{
                    leaderboards[a].emplace(finisherId, std::make_pair(finisherName, 1));
                }
            }
            return;
        }
    }
}

void requests::UpdateLeaderboardsNames() {
    for(const auto& [id, name_count]: leaderboards[All]){
        for(int tag = Normal; tag < All; tag++){
            if(leaderboards[static_cast<trackTag>(tag)][id].first != name_count.first){
                leaderboards[static_cast<trackTag>(tag)][id].first = name_count.first;
            }
        }
    }
}

void requests::SaveTempLeaderboards(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time) << std::endl;
    for(const auto& [id, subTable]: leaderboards){
        fout << id << ' ' << subTable.size() << std::endl;
        for(const auto& [playerId, player]: subTable){
            fout << playerId << ' ' << player.first << ' ' << player.second << std::endl;
        }
    }
    fout.close();
}

void requests::LoadTempLeaderboards(const std::string &tempFile) {
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
            leaderboards[static_cast<trackTag>(tag)].emplace(playerId, std::make_pair(playerName, playerCount));
        }
    }
    fin.close();
}

void requests::PrintLeaderboards() {
    for(const auto& [tag, subTable]: leaderboards){
        std::vector<std::pair<int64_t, std::pair<std::string, int>>> data(subTable.begin(), subTable.end());
        std::sort(data.begin(), data.end(),[](const auto& a, const auto& b){
            return a.second.second > b.second.second;
        });
        if(tag == All){
            int count = 0;
            for(const auto& a: data){
                count += a.second.second;
            }
            std::cout << "\nLeaderboard entries: " << count << std::endl;
        }
        data.resize(10);
        std::cout << "###############\n" << toString(tag) << "\n###############" << std::endl;
        for(const auto& [playerId, player]: data){
            std::cout << player.first << ' ' << player.second << std::endl;
        }
    }
}

void requests::SaveDataForFrontend(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time);
    fout << noRec << std::endl;
    fout << oldRecords.size()+tracksToCheck.size() << std::endl;
    for(const auto& [tag, subTable]: leaderboards){
        std::vector<std::pair<int64_t, std::pair<std::string, int>>> data(subTable.begin(), subTable.end());
        std::sort(data.begin(), data.end(),[](const auto& a, const auto& b){
            return a.second.second > b.second.second;
        });
        data.resize(10);
        fout << toString(tag) << std::endl;
        for(const auto& [playerId, player]: data){
            if(player.second == 0) break;
            fout << player.first << ' ' << player.second << std::endl;
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
                    {{"fields", std::vector<std::string>{"TrackId", "Tags"}},
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
            json_out.open("/home/response.json", std::ios_base::out);
            json_out << readBuffer << std::endl;
            readBuffer.clear();
            json_out.close();
            std::fstream json_in("/home/response.json");
            std::string abc;
            json_in >> abc;
            if(abc.find("\"More\"") >= abc.size()){
                continue;
            }
            GetNoRecordJSON("/home/response.json");
            std::cout << noRecordTracks.size() << std::endl;
        }
        std::remove("/home/response.json");
        std::cout << "Got extra tracks." << std::endl;
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void requests::LoadExtra(const std::string &tempFile) {
    std::ifstream fin(tempFile);
    int64_t trackId = 0;
    while(fin >> trackId){
        extraTracks.emplace(trackId, std::vector<trackTag>());
    }
    for(const auto& a: allTracks){
        if(extraTracks.contains(get<0>(a)))
            extraTracks.erase(get<0>(a));
    }
    std::cout << "Extra tracks: " << extraTracks.size();
    fin.close();
}
