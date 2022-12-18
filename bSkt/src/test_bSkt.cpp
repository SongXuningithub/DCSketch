#include "hashfunc.h"
#include "mylibpcap.h"
#include "bSkt.h"
#include <iostream>
#include <set>
#include <memory>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <ctime>
using std::unique_ptr;

#define OUTPUT_PER_FLOW_SPREAD 1
// #define OUTPUT_SUPER_SPREADERS 1

// template<class Estimator>
// void write_res(string dataset, string filename, bSkt<Estimator>& bskt, uint32_t tmpmem, double cmratio);
// void write_superspreaders(string dataset, string filename, vector<IdSpread>& superspreaders);

int main()
{
    // bSkt<HLL> bskt(200, 0);
    // string dataset = "ZIPF";
    // uint32_t init_mem = 8000;
    // Get_Mem(bskt, dataset, init_mem);

    bSkt<HLL> bskt(200, 0);
    string ofile_path = "../../bSkt/output";
    Test_task1(bskt, ofile_path, 0);
    return 0;
}

// template<class Estimator>
// void write_res(string dataset, string filename, bSkt<Estimator>& bskt, uint32_t tmpmem, double cmratio){
//     string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
//     string ofile_path = "../../bSkt/output/" + dataset + "/";
//     ifstream ifile_hand;
//     ofstream ofile_hand;
//     ifile_hand = ifstream(ifile_path + filename + ".txt");
//     // ofile_hand = ofstream(ofile_path + to_string(tmpmem) + "_" + filename + ".txt");
//     ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + to_string(cmratio).substr(0,4) + "_" + filename + ".txt");
//     if(!ifile_hand || !ofile_hand){
//         cout<<"fail to open files."<<endl;
//         return;
//     }

//     bool first_line = true;
//     while(!ifile_hand.eof()){
//         if(first_line)
//             first_line = false;
//         else 
//             ofile_hand << endl;
//         string flowid;
//         uint32_t spread;
//         ifile_hand >> flowid;
//         ifile_hand >> spread;
//         int estimated_spread = bskt.get_flow_spread(flowid);
//         estimated_spread = estimated_spread > 0 ? estimated_spread : 0;
//         ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
//     }
//     ifile_hand.close();
//     ofile_hand.close();
// }

// void write_superspreaders(string dataset, string filename, vector<IdSpread>& superspreaders){
//     string ofile_path = "../../bSkt/SuperSpreader/" + dataset + "/";
//     ifstream ifile_hand;
//     ofstream ofile_hand;
//     ofile_hand = ofstream(ofile_path + filename + ".txt");
//     if(!ofile_hand){
//         cout<<"fail to open files."<<endl;
//         return;
//     }
//     bool first_line = true;
//     for(auto item : superspreaders){
//         if(first_line)
//             first_line = false;
//         else
//             ofile_hand << endl;    
//         ofile_hand << item.flowID << " " << item.spread;
//     }
//     ofile_hand.close();
// }