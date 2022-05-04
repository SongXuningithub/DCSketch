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

template<class Estimator>
void write_res(string dataset, string filename, bSkt<Estimator>& bskt, uint32_t tmpmem);
// void write_superspreaders(string dataset, string filename, vector<IdSpread>& superspreaders);

int main()
{
    string dataset = "CAIDA_SUB";
    // vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000};
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    vector<uint32_t> mems{2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        uint32_t filenum = 11;
        for(size_t i = 0;i < filenum;i++){
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            bSkt<Bitmap> bskt(tmpmem);
            
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                bskt.process_packet(flowID, elemID);
                if(filehandler.proc_num()%1000000 == 0){
                    cout<<"process packet "<<filehandler.proc_num()<<endl;
                }
            }
            endTime = clock();
            cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            // string flow_id;
            // while(bool iseof = checker.query_flow(flow_id))
            // {
            //     int flow_spread = rerskt.get_flow_spread(flow_id);
            //     flow_spread = max(0,flow_spread);
            //     checker.record_result((uint32_t)flow_spread);
            //     //checker.record_full_result(flow_spread,dcsketch.layer1);
            // }
        #ifdef OUTPUT_PER_FLOW_SPREAD
            write_res(dataset, filehandler.get_filename(), bskt, tmpmem);
        #endif

        #ifdef OUTPUT_SUPER_SPREADERS
            vector<IdSpread> superspreaders;
            startTime = clock();
            bskt.report_superspreaders(superspreaders);
            endTime = clock();
            cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_superspreaders(dataset, filename, superspreaders);
        #endif
        }
    }
    return 0;
}

template<class Estimator>
void write_res(string dataset, string filename, bSkt<Estimator>& bskt, uint32_t tmpmem){
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../bSkt/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + to_string(tmpmem) + "_" + filename + ".txt");
    if(!ifile_hand || !ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }

    bool first_line = true;
    while(!ifile_hand.eof()){
        if(first_line)
            first_line = false;
        else 
            ofile_hand << endl;
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        ifile_hand >> spread;
        int estimated_spread = bskt.get_flow_spread(flowid);
        estimated_spread = estimated_spread > 0 ? estimated_spread : 0;
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
    }
    ifile_hand.close();
    ofile_hand.close();
}

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