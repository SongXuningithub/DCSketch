#include "vHLL.h"
#include "mylibpcap.h"
#include "util.h"
#include <iostream>
#include <set>
#include <memory>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <unordered_map>
#include <ctime>
using std::unique_ptr;

// void write_res(string dataset, string filename, vHLL& virhll, uint32_t tmpmem, double cmratio);

int main(){
    vHLL virhll(200, 0);
    Get_Mem(virhll);
    
    // string dataset = "FACEBOOK";
    // uint32_t mem = 1000;
    // vector<double> cm_ratios{0.05, 0.15, 0.25, 0.35, 0.45, 0.55}; //0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 
    // for(auto cmratio : cm_ratios){
    //     cout << "CarMon ratio: " << cmratio << endl;
    //     uint32_t filenum = 1;
    //     for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
    //         vHLL virhll(mem, cmratio);
    //         FILE_HANDLER filehandler(dataset, i);
    //         string flowID, elemID;
    //         clock_t startTime,endTime;
    //         startTime = clock();
    //         while(int status = filehandler.get_item(flowID, elemID)){
    //             virhll.process_packet(flowID, elemID);
    //             if(filehandler.proc_num()%10000000 == 0){
    //                 cout<<"process packet "<<filehandler.proc_num()<<endl;
    //                 // break;
    //             }
    //         }
    //         endTime = clock();
    //         cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    //         write_res(dataset, filehandler.get_filename(), virhll, mem, cmratio);
    //     }
    // }

    // vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000};
    // // vector<uint32_t> mems{2000};
    // for(auto tmpmem : mems){
    //     cout << "memory: " << tmpmem << endl;
    //     uint32_t filenum = 1;
    //     for(size_t i = 0;i < filenum;i++){   
    //         FILE_HANDLER filehandler(dataset, i);
    //         string flowID, elemID;
    //         vHLL virhll(tmpmem);
    //         clock_t startTime,endTime;
    //         startTime = clock();
    //         while(int status = filehandler.get_item(flowID, elemID)){
    //             virhll.process_packet(flowID, elemID);
    //             if(filehandler.proc_num()%1000000 == 0){
    //                 // cout<<"process packet "<<filehandler.proc_num()<<endl;
    //                 break;
    //             }
    //         }
    //         endTime = clock();
    //         // cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    //         cout << "n: " << virhll.get_spread(virhll.global_HLL)<<endl; 
    //         write_res(dataset, filehandler.get_filename(), virhll, tmpmem);
    //     }
    // }
    
    return 0;
}

// void write_res(string dataset, string filename, vHLL& virhll, uint32_t tmpmem, double cmratio){
//     string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
//     string ofile_path = "../../vHLL/output/" + dataset + "/";
//     ifstream ifile_hand;
//     ofstream ofile_hand;
//     ifile_hand = ifstream(ifile_path + filename + ".txt");
//     // ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + filename + ".txt");
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
//         int estimated_spread = virhll.get_flow_spread(flowid);
//         ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
//     }
//     endTime = clock();
//     cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
//     ifile_hand.close();
//     ofile_hand.close();
// }

