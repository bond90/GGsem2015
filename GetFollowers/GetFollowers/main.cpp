//
//  main.cpp
//  GetFollowers
//
//  Created by Gabriele Di Bari on 06/07/15.
//  Copyright (c) 2015 Gabriele Di Bari. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <fstream>
#include <sstream>
#include <curl/curl.h>
#include <iostream>
#include "json11.hpp"

static size_t steam_string(char *buffer, size_t size, size_t nmemb, void *stream)
{
    std::stringstream& sstream = *((std::stringstream*)stream);
    sstream << buffer;
    return nmemb;
}

const char *ckey = "JgrNx9ChveogmOZ0byTkIqemm";
const char *csecret = "0EO2TQRaUgWPxEn3y5FQAtXirHla9rIHFnz3bvnXJ56Mb271nm";
const char *atok = "552673093-Lmmp8roGDEinnpoowetTuazdSxcZvRzPJQ1DtFq0";
const char *atoksecret = "UEOQc2n49L7GBtVDRpzE2i3aSeIH0wf0qs5gVTLTi1QOF";

json11::Json get_json_followers(CURL*  curl,
                                const std::string& str,
                                std::string& err,
                                int count)
{
    //const char *url = "https://stream.twitter.com/1.1/statuses/sample.json";
    //REST api friends/followers
    const char* str_url=
    "https://api.twitter.com/1.1/followers/list.json"
    "?cursor=-1"
    "&skip_status=true"
    "&include_user_entities=false";
    
    //create link
    std::string url(str_url+
                    ("&count="+std::to_string(count))+
                    ("&screen_name="+str));
    
    // URL, POST parameters (not used in this example), OAuth signing method, HTTP method, keys
    char *signedurl = oauth_sign_url2(url.c_str(), NULL, OA_HMAC, "GET", ckey, csecret, atok, atoksecret);
    
    // URL we're connecting to
    curl_easy_setopt(curl, CURLOPT_URL, signedurl);
    
    // User agent we're going to use, fill this in appropriately
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "appname/0.1");
    
    // libcurl will now fail on an HTTP error (>=400)
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    
    // In this case, we're not specifying a callback function for
    // handling received data, so libcURL will use the default, which
    // is to write to the file specified in WRITEDATA
    std::stringstream str_stream;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, steam_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&str_stream);
    
    // Execute the request!
    int curlstatus = curl_easy_perform(curl);
    //error?
    if(curlstatus)
        err =  "curl_easy_perform terminated with status code " + std::to_string(curlstatus)+"\n";
    //parsing
    return json11::Json::parse(str_stream.str(), err);
}

void get_names_from_twitter_json(CURL *curl,
                                 const std::string& name,
                                 std::string& err,
                                 int count,
                                 json11::Json::array& parent_att,
                                 json11::Json::object& parent_obj,
                                 int depth)
{
    if(depth <= 0) return;
    //////////// //////////// //////////// ////////////
    json11::Json uni =  get_json_followers(curl,name,err,count);
    //////////// //////////// //////////// ////////////
    for(auto& user : uni["users"].array_items())
    {
        //child name
        const std::string& child_name=user["screen_name"].string_value();
        //child map
        json11::Json::array child_att;
        json11::Json::object child_obj;
        std::string location_name_tmp=user["location"].string_value();
        //json child
        get_names_from_twitter_json
        (
         curl,
         child_name,
         err,
         count,
         child_att,
         child_obj,
         depth-1
         );
        child_att.push_back(std::move(location_name_tmp));
        child_att.push_back(std::move(child_obj));
        parent_obj.insert({child_name, child_att});
    }
    //debug
    std::cout << name << " have " << parent_obj.size() <<" friends\n";
    //////////// //////////// //////////// ////////////
}

int main(int argc, const char *argv[])
{
    
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    
    std::string err;
    json11::Json::array att;
    std::string location("Perugia, Italy");
    json11::Json::object map;
    
    get_names_from_twitter_json
    (
        curl,           //curl
        "GabrieleTW",   //name
        err,            //errors
        13,             //count
        att,            //attributes
        map,            //map
        2               //depth
     );
    
    att.push_back(std::move(location));
    att.push_back(std::move(map));
    
    //build json
    json11::Json::object GabrieleTWmap;
    GabrieleTWmap.insert({"GabrieleTW", att});
    std::string out;
    json11::Json(GabrieleTWmap).dump(out);
    //std::cout << out;
    std::ofstream file("gabtweet.json", std::ofstream::out);
    file << out;
    //dealloc
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    
    return 0;
}
