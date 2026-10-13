#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

static std::uint32_t be32(const std::vector<unsigned char>& b,std::size_t p){return (b[p]<<24)|(b[p+1]<<16)|(b[p+2]<<8)|b[p+3];}
static std::uint32_t variable(const std::vector<unsigned char>& b,std::size_t& p){std::uint32_t v=0;unsigned char c=0;do{if(p>=b.size())throw 1;c=b[p++];v=(v<<7)|(c&0x7f);}while(c&0x80);return v;}
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"usage: recorded_midi_inspection file.mid\n";return 2;}std::ifstream in(argv[1],std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(in)),{});if(b.size()<22||std::string(reinterpret_cast<char*>(b.data()),4)!="MThd"){std::cerr<<"invalid MIDI file\n";return 2;}
    std::size_t p=8+be32(b,4);bool note=false,down=false,up=false,duplicate=false;std::map<std::tuple<std::uint32_t,int,int>,int> noteOns;
    while(p+8<=b.size()&&std::string(reinterpret_cast<char*>(b.data()+p),4)=="MTrk"){
        const auto end=p+8+be32(b,p+4);p+=8;std::uint32_t tick=0;int running=0;
        while(p<end&&p<b.size()){
            tick+=variable(b,p);int status=b[p];if(status&0x80){++p;running=status;}else status=running;
            if(status==0xff){if(p>=end)break;++p;const auto n=variable(b,p);p+=n;continue;}if(status==0xf0||status==0xf7){const auto n=variable(b,p);p+=n;continue;}
            const int type=status&0xf0,channel=status&15;if(p>=end)break;const int d1=b[p++];int d2=0;if(type!=0xc0&&type!=0xd0){if(p>=end)break;d2=b[p++];}
            if(type==0x90&&d2>0){note=true;if(++noteOns[{tick,channel,d1}]>1)duplicate=true;}
            if(type==0xb0&&d1==64){if(d2>=64)down=true;else up=true;}
        }p=end;
    }
    if(!note){std::cerr<<"note-on missing\n";return 1;}if(!down||!up){std::cerr<<"CC64 down/up missing\n";return 1;}if(duplicate){std::cerr<<"duplicate Note On at identical channel/pitch/time\n";return 1;}
    std::cout<<"PASS: notes, CC64=127, CC64=0, no duplicate Note On\n";
}
