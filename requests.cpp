//
// Created by Matvey on 04.07.2024.
//

#include "requests.h"

#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <variant>

#include "curl/curl.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

namespace pt = boost::property_tree;

void requests::SaveTemp(const std::string &tempFile) {
    std::ofstream fout(tempFile);
    auto currentTime = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
    fout << std::ctime(&time) << std::endl;
    for(const auto& [id, beaten, tags]: allNoRecord){
        fout << id << ' ' << beaten << std::endl;
        for(const auto& a: tags){
            fout << a << ' ';
        }
        fout << std::endl;
    }
    fout.close();
}

void requests::ReadTemp(const std::string &tempFile) {
    std::ifstream fin(tempFile);
    std::string time;
    getline(fin, time);
    std::cout << "Previous update: " << time << std::endl;
    allNoRecord.clear();
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
        allNoRecord.emplace_back(id, beaten, tags);
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
            std::cout << noRecordMaps.size() << std::endl;
        }
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
                        noRecordMaps.insert(newTrack);
                    }
                }
            }
            noRecordMaps.erase(0);
            return;
        }
    }
}

void requests::PrintSet() {
    for(const auto& a: noRecordMaps){
        std::cout << a.first;
        for(const auto& v: a.second){
            std::cout << ' ' << v;
        }
        std::cout << std::endl;
    }
    std::cout << noRecordMaps.size() << std::endl;
}

void requests::Compare() {
    std::cout << std::endl;
    if(allNoRecord.empty()){
        for(auto& [id, tags]: noRecordMaps){
            allNoRecord.emplace_back(id, false, tags);
        }
        return;
    }
    int totalBeaten = 0;
    for(auto& [id, beaten, tags]: allNoRecord){
        if(!beaten && !noRecordMaps.contains(id)){
            beaten = true;
        }
        if(beaten){
            totalBeaten ++;
        }
    }
    std::cout << "################" << std::endl << totalBeaten << std::endl;
    std::cout << "################" << std::endl;
}

void requests::PrintMap() {
    for(const auto& [id, beaten, tags]: allNoRecord){
        std::cout << id << ' ' << beaten << " [ ";
        for(const auto& a: tags){
            std::cout << a << ' ';
        }
        std::cout << "]" << std::endl;
    }
    std::cout << allNoRecord.size() << std::endl;
}

void requests::PrintWithRecords() {
    int totalRecords = 0;
    for(const auto& [id, beaten, tags]: allNoRecord){
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
