#include "../thprac/src/thprac/th06nc/ecl.cpp"
#include <fstream>
#include <string>
#include <iostream>
namespace THPrac::TH06NC {
uintptr_t imageBase{};THPracParam thPracParam{};int shot{};
int MenuShot(){return shot;}
}
int main(int argc,char** argv) {
using namespace THPrac::TH06NC;
using namespace THPrac::TH06;
if(argc<2)return 1;
imageBase=(uintptr_t)VirtualAlloc(nullptr,0x1000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
struct Case {int stage,section;const char* name;};
Case cases[]={
{1,TH06_ST1_MID1,"TH06_ST1_MID1"},
{1,TH06_ST1_MID2,"TH06_ST1_MID2"},
{1,TH06_ST1_BOSS1,"TH06_ST1_BOSS1"},
{1,TH06_ST1_BOSS2,"TH06_ST1_BOSS2"},
{1,TH06_ST1_BOSS3,"TH06_ST1_BOSS3"},
{1,TH06_ST1_BOSS4,"TH06_ST1_BOSS4"},
{2,TH06_ST2_MID1,"TH06_ST2_MID1"},
{2,TH06_ST2_BOSS1,"TH06_ST2_BOSS1"},
{2,TH06_ST2_BOSS2,"TH06_ST2_BOSS2"},
{2,TH06_ST2_BOSS3,"TH06_ST2_BOSS3"},
{2,TH06_ST2_BOSS4,"TH06_ST2_BOSS4"},
{2,TH06_ST2_BOSS5,"TH06_ST2_BOSS5"},
{3,TH06_ST3_MID1,"TH06_ST3_MID1"},
{3,TH06_ST3_MID2,"TH06_ST3_MID2"},
{3,TH06_ST3_BOSS1,"TH06_ST3_BOSS1"},
{3,TH06_ST3_BOSS2,"TH06_ST3_BOSS2"},
{3,TH06_ST3_BOSS3,"TH06_ST3_BOSS3"},
{3,TH06_ST3_BOSS4,"TH06_ST3_BOSS4"},
{3,TH06_ST3_BOSS5,"TH06_ST3_BOSS5"},
{3,TH06_ST3_BOSS6,"TH06_ST3_BOSS6"},
{3,TH06_ST3_BOSS7,"TH06_ST3_BOSS7"},
{4,TH06_ST4_BOOKS,"TH06_ST4_BOOKS"},
{4,TH06_ST4_MID1,"TH06_ST4_MID1"},
{4,TH06_ST4_BOSS1,"TH06_ST4_BOSS1"},
{4,TH06_ST4_BOSS2,"TH06_ST4_BOSS2"},
{4,TH06_ST4_BOSS3,"TH06_ST4_BOSS3"},
{4,TH06_ST4_BOSS4,"TH06_ST4_BOSS4"},
{4,TH06_ST4_BOSS5,"TH06_ST4_BOSS5"},
{4,TH06_ST4_BOSS6,"TH06_ST4_BOSS6"},
{4,TH06_ST4_BOSS7,"TH06_ST4_BOSS7"},
{5,TH06_ST5_MID1,"TH06_ST5_MID1"},
{5,TH06_ST5_MID2,"TH06_ST5_MID2"},
{5,TH06_ST5_BOSS1,"TH06_ST5_BOSS1"},
{5,TH06_ST5_BOSS2,"TH06_ST5_BOSS2"},
{5,TH06_ST5_BOSS3,"TH06_ST5_BOSS3"},
{5,TH06_ST5_BOSS4,"TH06_ST5_BOSS4"},
{5,TH06_ST5_BOSS5,"TH06_ST5_BOSS5"},
{5,TH06_ST5_BOSS6,"TH06_ST5_BOSS6"},
{6,TH06_ST6_MID1,"TH06_ST6_MID1"},
{6,TH06_ST6_MID2,"TH06_ST6_MID2"},
{6,TH06_ST6_BOSS1,"TH06_ST6_BOSS1"},
{6,TH06_ST6_BOSS2,"TH06_ST6_BOSS2"},
{6,TH06_ST6_BOSS3,"TH06_ST6_BOSS3"},
{6,TH06_ST6_BOSS4,"TH06_ST6_BOSS4"},
{6,TH06_ST6_BOSS5,"TH06_ST6_BOSS5"},
{6,TH06_ST6_BOSS6,"TH06_ST6_BOSS6"},
{6,TH06_ST6_BOSS7,"TH06_ST6_BOSS7"},
{6,TH06_ST6_BOSS8,"TH06_ST6_BOSS8"},
{6,TH06_ST6_BOSS9,"TH06_ST6_BOSS9"},
{7,TH06_ST7_MID1,"TH06_ST7_MID1"},
{7,TH06_ST7_MID2,"TH06_ST7_MID2"},
{7,TH06_ST7_MID3,"TH06_ST7_MID3"},
{7,TH06_ST7_END_NS1,"TH06_ST7_END_NS1"},
{7,TH06_ST7_END_S1,"TH06_ST7_END_S1"},
{7,TH06_ST7_END_NS2,"TH06_ST7_END_NS2"},
{7,TH06_ST7_END_S2,"TH06_ST7_END_S2"},
{7,TH06_ST7_END_NS3,"TH06_ST7_END_NS3"},
{7,TH06_ST7_END_S3,"TH06_ST7_END_S3"},
{7,TH06_ST7_END_NS4,"TH06_ST7_END_NS4"},
{7,TH06_ST7_END_S4,"TH06_ST7_END_S4"},
{7,TH06_ST7_END_NS5,"TH06_ST7_END_NS5"},
{7,TH06_ST7_END_S5,"TH06_ST7_END_S5"},
{7,TH06_ST7_END_NS6,"TH06_ST7_END_NS6"},
{7,TH06_ST7_END_S6,"TH06_ST7_END_S6"},
{7,TH06_ST7_END_NS7,"TH06_ST7_END_NS7"},
{7,TH06_ST7_END_S7,"TH06_ST7_END_S7"},
{7,TH06_ST7_END_NS8,"TH06_ST7_END_NS8"},
{7,TH06_ST7_END_S8,"TH06_ST7_END_S8"},
{7,TH06_ST7_END_S9,"TH06_ST7_END_S9"},
{7,TH06_ST7_END_S10,"TH06_ST7_END_S10"},
};
int failures=0,total=0;
for(auto test:cases)for(int dialogue=0;dialogue<2;++dialogue)for(shot=0;shot<4;++shot) {
std::ifstream f(std::string(argv[1])+"/ecldata"+std::to_string(test.stage)+".ecl",std::ios::binary);
std::vector<unsigned char> data((std::istreambuf_iterator<char>(f)),{});
*reinterpret_cast<void**>(imageBase+0xa6eb78)=data.data();
thPracParam={};thPracParam.stage=test.stage-1;thPracParam.section=test.section;thPracParam.dlg=dialogue;
if(!ApplyEcl()) {++failures;std::cout<<test.name<<" dlg="<<dialogue<<" shot="<<shot<<" missing="<<std::hex<<EclFailureOffset()<<std::dec<<"\n";}
++total;
}
std::cout<<"total="<<total<<" failures="<<failures<<"\n";
return failures?2:0;
}
