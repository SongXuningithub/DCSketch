#include "vHLL.h"
#include "mylibpcap.h"
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
bool per_src_flow = true;
int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};

    string dataset = "CAIDA";
    if(dataset == "CAIDA"){
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }
    vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        for(size_t i = 0;i < datasets[dataset].size();i++){   
            string filename = datasets[dataset][i];
            PCAP_SESSION session(dataset,filename,PCAP_FILE);
            IP_PACKET cur_packet;
            string srcip,dstip;
            vHLL virhll(tmpmem);
            
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = session.get_packet(cur_packet)){
                srcip = cur_packet.get_srcip();
                dstip = cur_packet.get_dstip();
                if(per_src_flow)
                    virhll.process_packet(srcip,dstip);
                else
                    virhll.process_packet(dstip,srcip);
                // if(session.proc_num()%2000000 == 0){
                //     // cout<<"process packet "<<session.proc_num()<<endl;
                //     break;
                // }
            }
            endTime = clock();
            // cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            cout << "n: " << virhll.get_spread(virhll.global_HLL)<<endl; 
            write_res(dataset,filename,virhll,tmpmem);
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

