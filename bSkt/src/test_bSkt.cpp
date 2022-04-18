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

// #define OUTPUT_PER_FLOW_SPREAD 1
// #define OUTPUT_SUPER_SPREADERS 1

void write_res(string dataset,string filename,bSkt& rersketch);
void write_superspreaders(string dataset, string filename, vector<IdSpread>& superspreaders);

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
        bSkt bskt;
        
        clock_t startTime,endTime;
        startTime = clock();
        while(int status = session.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if (per_src_flow)
                bskt.process_packet(srcip,dstip);
            else
                bskt.process_packet(dstip,srcip);
            // if(session.proc_num()%2000000 == 0){
            //     //cout<<"process packet "<<session.proc_num()<<endl;
            //     break;
            // }     
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
        write_res(dataset,filename,bskt);
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
    return 0;
}

void write_res(string dataset,string filename,bSkt& bskt){
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../bSkt/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + filename + ".txt");
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

void write_superspreaders(string dataset, string filename, vector<IdSpread>& superspreaders){
    string ofile_path = "../../bSkt/SuperSpreader/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + filename + ".txt");
    if(!ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    bool first_line = true;
    for(auto item : superspreaders){
        if(first_line)
            first_line = false;
        else
            ofile_hand << endl;    
        ofile_hand << item.flowID << " " << item.spread;
    }
    ofile_hand.close();
}