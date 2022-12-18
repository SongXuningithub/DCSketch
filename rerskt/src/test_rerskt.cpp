#include "mylibpcap.h"
#include "rerskt.h"
#include <iostream>
#include <set>
#include <memory>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <string>
#include "util.h"

// template<class Estimator>
// void write_res(string dataset,string filename, RerSkt<Estimator>& rersketch, uint32_t tmpmem, double cmratio);
int main() {
    // RerSkt<HLL> rerskt_notuse(200, 0);
    // string dataset = "ZIPF";
    // uint32_t init_mem = 2000;
    // Get_Mem(rerskt_notuse, dataset, init_mem);

    RerSkt<HLL> rerskt_notuse(200, 0);
    string ofile_path = "../../rerskt/output";
    Test_task1(rerskt_notuse, ofile_path, 0);

    return 0;
}

// template<class Estimator>
// void write_res(string dataset, string filename, RerSkt<Estimator>& rersketch, uint32_t tmpmem, double cmratio) {
//     string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
//     string ofile_path = "../../rerskt/output/" + dataset + "/";
//     ifstream ifile_hand;
//     ofstream ofile_hand;

//     ifile_hand = ifstream(ifile_path + filename + ".txt");
//     // ofile_hand = ofstream(ofile_path + to_string(tmpmem) + "_" + filename + ".txt");
//     ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + to_string(cmratio).substr(0,4) + "_" + filename + ".txt");
//     if(!ifile_hand || !ofile_hand){
//         cout<<"fail to open files."<<endl;
//         return;
//     }
//     clock_t startTime,endTime;
//     startTime = clock();
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
//         int estimated_spread = rersketch.get_flow_spread(flowid);
//         estimated_spread = estimated_spread > 0 ? estimated_spread : 0;
//         ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
//     }
//     endTime = clock();
//     cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
//     ifile_hand.close();
//     ofile_hand.close();
// }

