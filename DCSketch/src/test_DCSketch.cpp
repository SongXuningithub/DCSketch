#include "DCSketch.h"
#include "MurmurHash3.h"
#include "mylibpcap.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <set>
#include <memory>
#include <algorithm>
#include <unistd.h>
using std::unique_ptr;

void write_perflow_spread(string dataset,string filename,DCSketch& dcsketch);
void write_real_distribution(string dataset,string filename,DCSketch& dcsketch);
void write_sketch(string dataset,string filename,DCSketch& dcsketch);
void write_superspreaders(string dataset,string filename,vector<IdSpread>& superspreaders);

bool per_src_flow = true;
int main()
{
#define OUTPUT_PERFLOW_SPREAD 1
// #define OUTPUT_SUPER_SPREADERS 1
//#define OUTPUT_SKETCH 1
//#define OUTPUT_REAL_DISTRIBUTION 1
    
    string dataset = "MAWI";
    if(dataset == "CAIDA")
    {
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }

    for(size_t i = 1;i <= 2;i++)
    {
        DCSketch dcsketch(1000,0.6);
        string filename = "pkts_frag_0000" + to_string(i);
        // string filename = "5M_frag (" + to_string(i) + ")";
        // string filename = "Unicauca";
        PCAP_SESSION session(dataset,filename,PCAP_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;
        clock_t startTime,endTime;
        startTime = clock();
        while(int status = session.get_packet(cur_packet))
        {
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if (per_src_flow)
                dcsketch.process_element(srcip,dstip);
            else
                dcsketch.process_element(dstip,srcip);
            // if(session.proc_num()%1000000 == 0)
            // {
            //     cout<<"process packet "<<session.proc_num()<<endl;
            // }
        }
        endTime = clock();
        cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
        dcsketch.get_global_info();
    #ifdef OUTPUT_PERFLOW_SPREAD
        write_perflow_spread(dataset,filename,dcsketch);
    #endif
    
    #ifdef OUTPUT_SUPER_SPREADERS
        //uint32_t threshold = 50000;//1000;
        vector<IdSpread> superspreaders;
        startTime = clock();
        dcsketch.report_superspreaders(superspreaders);
        endTime = clock();
        cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
        write_superspreaders(dataset,filename,superspreaders);
    #endif
    }
    return 0;
}

void write_perflow_spread(string dataset,string filename,DCSketch& dcsketch)
{
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../DCSketch/output/PerFlowSpread/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + filename + ".txt");
       
    if(!ifile_hand || !ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
    // clock_t startTime,endTime;
    // startTime = clock();
    bool first_line = true;
    while(!ifile_hand.eof())
    {
        if(first_line)
            first_line = false;
        else 
            ofile_hand << endl;
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        ifile_hand >> spread;
        uint32_t estimated_spread = dcsketch.query_spread(flowid);
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
    }
    // endTime = clock();
    // cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    ifile_hand.close();
    ofile_hand.close();
}

void write_superspreaders(string dataset,string filename,vector<IdSpread>& superspreaders)
{
    string ofile_path = "../../DCSketch/output/SuperSpreaders/" + dataset + "/";
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + filename + ".txt");
    if(!ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
    bool first_line = true;
    for(auto item : superspreaders)
    {
        if(first_line)
            first_line = false;
        else
            ofile_hand << endl;
        ofile_hand << item.flowID << " " << item.spread;
    }
    ofile_hand.close();
}

bool cmp_fun(pair<uint32_t,uint32_t> &a,pair<uint32_t,uint32_t> &b)
{
    return a.first < b.first;
}

