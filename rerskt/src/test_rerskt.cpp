#include "mylibpcap.h"
#include "rerskt.h"
#include <iostream>
#include <set>
#include <memory>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <ctime>
using std::unique_ptr;

void write_res(string dataset,string filename,RerSkt& rersketch);
bool per_src_flow = true;
int main()
{
    string dataset = "MAWI";
    if(dataset == "CAIDA"){
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }

    for(size_t i = 1;i <= 1;i++){
        // string filename = "5M_frag (" + to_string(i) + ")";
        string filename = "pkts_frag_0000" + to_string(i);
        // string filename = "Unicauca";
        PCAP_SESSION session(dataset,filename,PCAP_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;
        RerSkt rerskt;
        
        clock_t startTime,endTime;
        startTime = clock();
        while(int status = session.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            // if(per_src_flow)
            //     rerskt.process_flow(srcip,dstip);
            // else
            //     rerskt.process_flow(dstip,srcip);
            
            // if(session.proc_num()%200000 == 0){
            //     // cout<<"process packet "<<session.proc_num()<<endl;
            //     break;
            // }
        }
        endTime = clock();
        cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
        
        // write_res(dataset,filename,rerskt);
    }
    
    return 0;
}

void write_res(string dataset,string filename,RerSkt& rersketch)
{
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../rerskt/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + filename + ".txt");
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
        int estimated_spread = rersketch.get_flow_spread(flowid);
        estimated_spread = estimated_spread > 0 ? estimated_spread : 0;
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
    }
    endTime = clock();
    cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    ifile_hand.close();
    ofile_hand.close();
}

