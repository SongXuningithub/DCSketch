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

void write_res(string dataset,string filename,vHLL& virhll,uint32_t tmpmem);

int main()
{
    string dataset = "CAIDA_SUB";
    // vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000};
    vector<uint32_t> mems{2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        uint32_t filenum = 11;
        for(size_t i = 0;i < filenum;i++){   
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            vHLL virhll(tmpmem);
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                virhll.process_packet(flowID, elemID);
                if(filehandler.proc_num()%1000000 == 0){
                    // cout<<"process packet "<<filehandler.proc_num()<<endl;
                    break;
                }
            }
            endTime = clock();
            // cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            cout << "n: " << virhll.get_spread(virhll.global_HLL)<<endl; 
            write_res(dataset, filehandler.get_filename(), virhll, tmpmem);
        }
    }
    
    return 0;
}

void write_res(string dataset,string filename,vHLL& virhll,uint32_t tmpmem){
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../vHLL/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + filename + ".txt");
    if(!ifile_hand || !ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    clock_t startTime,endTime;
    startTime = clock();
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
        int estimated_spread = virhll.get_spread(flowid);
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
    }
    endTime = clock();
    cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    ifile_hand.close();
    ofile_hand.close();
}

