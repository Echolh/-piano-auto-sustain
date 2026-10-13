#include "RecorderConfig.h"

#include <fstream>
#include <optional>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace pas {
namespace {
std::optional<std::string> stringValue(const std::string& text,const std::string& key)
{
    const auto marker='"'+key+'"';auto pos=text.find(marker);if(pos==std::string::npos)return std::nullopt;
    pos=text.find(':',pos+marker.size());if(pos==std::string::npos)return std::nullopt;
    const auto first=text.find('"',pos+1);if(first==std::string::npos)return std::nullopt;
    const auto last=text.find('"',first+1);if(last==std::string::npos)return std::nullopt;
    return text.substr(first+1,last-first-1);
}
std::optional<int> intValue(const std::string& text,const std::string& key)
{
    const auto marker='"'+key+'"';auto pos=text.find(marker);if(pos==std::string::npos)return std::nullopt;
    pos=text.find(':',pos+marker.size());if(pos==std::string::npos)return std::nullopt;
    try{return std::stoi(text.substr(pos+1));}catch(...){return std::nullopt;}
}
std::optional<bool> boolValue(const std::string& text,const std::string& key)
{
    const auto marker='"'+key+'"';auto pos=text.find(marker);if(pos==std::string::npos)return std::nullopt;
    pos=text.find(':',pos+marker.size());if(pos==std::string::npos)return std::nullopt;
    const auto value=text.substr(pos+1,6);if(value.find("true")!=std::string::npos)return true;if(value.find("false")!=std::string::npos)return false;return std::nullopt;
}
}

RecorderConfig RecorderConfigStore::load() const
{
    RecorderConfig result;if(!std::filesystem::exists(path))return result;
    std::ifstream input(path,std::ios::binary);std::string text((std::istreambuf_iterator<char>(input)),{});
    if(text.empty()||text.front()!='{'||text.find('}')==std::string::npos){
        input.close();
        auto backup=path;backup += ".corrupt";std::error_code ec;std::filesystem::remove(backup,ec);std::filesystem::rename(path,backup,ec);return result;
    }
    const auto sourceSchema=intValue(text,"schemaVersion").value_or(0);
    if(auto value=stringValue(text,"inputIdentifier"))result.inputIdentifier=*value;
    if(auto value=stringValue(text,"outputIdentifier"))result.outputIdentifier=*value;
    if(auto value=stringValue(text,"language"))result.language=*value;
    if(auto value=intValue(text,"theme"))result.theme=*value;
    if(auto value=intValue(text,"pedalPolicy"))result.pedalPolicy=static_cast<HardwarePedalPolicy>(*value);
    if(sourceSchema>=5){
        if(auto value=boolValue(text,"startWithWindows"))result.startWithWindows=*value;
        if(auto value=boolValue(text,"closeToTray"))result.closeToTray=*value;
        if(auto value=boolValue(text,"quietWhenOperational"))result.quietWhenOperational=*value;
    }
    result.schemaVersion=5;return result;
}

bool RecorderConfigStore::save(const RecorderConfig& config) const
{
    std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);auto temporary=path;temporary += ".tmp";
    std::ofstream output(temporary,std::ios::binary|std::ios::trunc);if(!output)return false;
    output<<"{\n  \"schemaVersion\": 5,\n  \"inputIdentifier\": \""<<config.inputIdentifier
          <<"\",\n  \"outputIdentifier\": \""<<config.outputIdentifier
          <<"\",\n  \"pedalPolicy\": "<<static_cast<int>(config.pedalPolicy)
          <<",\n  \"theme\": "<<config.theme<<",\n  \"language\": \""<<config.language
          <<"\",\n  \"startWithWindows\": "<<(config.startWithWindows?"true":"false")
          <<",\n  \"closeToTray\": "<<(config.closeToTray?"true":"false")
          <<",\n  \"quietWhenOperational\": "<<(config.quietWhenOperational?"true":"false")<<"\n}\n";
    output.flush();if(!output)return false;output.close();
#ifdef _WIN32
    return MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    std::filesystem::rename(temporary,path,ec);return !ec;
#endif
}
}
