#include "DCSketch.h"
#include "MurmurHash3.h"
#include "mylibpcap.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <set>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unistd.h>
using std::unique_ptr;

#define OUTPUT_PERFLOW_SPREAD 1
// #define OUTPUT_SUPER_SPREADERS 1
// #define OUTPUT_SUPER_CHANGES 1

void write_perflow_spread(string dataset,string filename,DCSketch& dcsketch,uint32_t tmpmem);
void write_real_distribution(string dataset,string filename,DCSketch& dcsketch);
void write_sketch(string dataset,string filename,DCSketch& dcsketch);
void write_superspreaders(string dataset,string filename,vector<IdSpread>& superspreaders,uint32_t tmpmem);
void WriteSuperChanges(string dataset, vector<IdSpread>& superchanges, uint32_t tmpmem);

int main()
{
    string dataset = "CAIDA_SUB";

#ifndef OUTPUT_SUPER_CHANGES
    // vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000};
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    vector<uint32_t> mems{2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        uint32_t filenum = 11;
        for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
            DCSketch dcsketch(tmpmem, 0.6);
            FILE_HANDLER filehandler(dataset, i);

            string flowID, elemID;
            clock_t startTime,endTime;
            startTime = clock();
            // set<pair<string,string>> layer1_items;
            // set<pair<string,string>> layer2_items;
            // set<string> layer2_flows;

            while(int status = filehandler.get_item(flowID, elemID)){
                dcsketch.process_element(flowID, elemID);
                if(filehandler.proc_num()%1000000 == 0)
                    cout<<"process packet "<<filehandler.proc_num()<<endl;
            }
            // set<pair<string,string>> inter_items; 
            // set_intersection(layer1_items.begin(), layer1_items.end(), layer2_items.begin(), layer2_items.end(),
            //     inserter(inter_items, inter_items.begin()));
            // cout << "inter_items: " << inter_items.size() << " layer2_flows: " << layer2_flows.size() << " average " <<
            // static_cast<double>(inter_items.size()) / layer2_flows.size()  <<endl;

            endTime = clock();
            cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            dcsketch.get_global_info();
        #ifdef OUTPUT_PERFLOW_SPREAD
            write_perflow_spread(dataset, filehandler.get_filename(), dcsketch, tmpmem);
        #endif
        
        #ifdef OUTPUT_SUPER_SPREADERS
            vector<IdSpread> superspreaders;
            startTime = clock();
            dcsketch.report_superspreaders(superspreaders);
            endTime = clock();
            cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_superspreaders(dataset, filename, superspreaders, tmpmem);
        #endif
        }
    }
    
#else
    uint32_t superchange_thresh = 100;
    // uint32_t superchange_thresh = 1000;
    vector<uint32_t> mems{500, 1000, 1500, 2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        
        /*---first epoch---*/
        DCSketch dcsketch1(tmpmem,0.6);
        string filename = datasets[dataset][0];
        PCAP_SESSION session1(dataset,filename,CSV_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;
        while(int status = session1.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if (per_src_flow)
                dcsketch1.process_element(srcip,dstip);
            else
                dcsketch1.process_element(dstip,srcip);
            /**/
            if(session1.proc_num() == 1788648)
                break;
            /**/
        }
        dcsketch1.get_global_info();
        vector<IdSpread> superspreaders1;
        dcsketch1.report_superspreaders(superspreaders1);

        /*---second epoch---*/
        DCSketch dcsketch2(tmpmem,0.6);
        filename = datasets[dataset][0];  /*KAGGLE*/
        PCAP_SESSION session2(dataset,filename,CSV_FILE);
        while(int status = session2.get_packet(cur_packet)){
            /**/
            if(session2.proc_num() <= 1788648)
                continue;
            /**/
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if (per_src_flow)
                dcsketch2.process_element(srcip,dstip);
            else
                dcsketch2.process_element(dstip,srcip);
        }
        dcsketch2.get_global_info();
        vector<IdSpread> superspreaders2;
        dcsketch2.report_superspreaders(superspreaders2);

        /*---detect super changes---*/
        vector<IdSpread> superchanges;
        set<string> inserted;
        for(size_t i = 0;i < superspreaders1.size();i++){
            auto tmp = superspreaders1[i].flowID;
            uint32_t change_val = abs((int)dcsketch2.query_spread(tmp) - (int)dcsketch1.query_spread(tmp));
            if(change_val >= superchange_thresh){
                if(inserted.find(tmp) == inserted.end()){
                    superchanges.push_back(IdSpread(tmp,change_val));
                    inserted.insert(tmp);
                }
            }
        }
        for(size_t i = 0;i < superspreaders2.size();i++){
            auto tmp = superspreaders2[i].flowID;
            uint32_t change_val = abs((int)dcsketch2.query_spread(tmp) - (int)dcsketch1.query_spread(tmp));
            if(change_val >= superchange_thresh){
                if(inserted.find(tmp) == inserted.end()){
                    superchanges.push_back(IdSpread(tmp,change_val));
                    inserted.insert(tmp);
                }
            }
        }
        sort(superchanges.begin(), superchanges.end(), IdSpreadComp);
        // WriteSuperChanges(dataset, superchanges,tmpmem);
    }

#endif
    return 0;
}

void write_perflow_spread(string dataset,string filename,DCSketch& dcsketch,uint32_t tmpmem){
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../DCSketch/output/PerFlowSpread/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + to_string(tmpmem) + "_" + filename + ".txt");
       
    if(!ifile_hand || !ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    // clock_t startTime,endTime;
    // startTime = clock();
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
        uint32_t estimated_spread = dcsketch.query_spread(flowid);
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
    }
    // endTime = clock();
    // cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    ifile_hand.close();
    ofile_hand.close();
}

void write_superspreaders(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem){
    string ofile_path = "../../DCSketch/output/SuperSpreaders/" + dataset + "/";
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" +  filename + ".txt");
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

void WriteSuperChanges(string dataset, vector<IdSpread>& superchanges, uint32_t tmpmem){
    string ofile_path = "../../DCSketch/output/SuperChanges/" + dataset + "/";
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + dataset + ".txt");
    if(!ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    bool first_line = true;
    for(auto item : superchanges){
        if(first_line)
            first_line = false;
        else
            ofile_hand << endl;
        ofile_hand << item.flowID << " " << item.spread;
    }
    ofile_hand.close();
}


void write_meta(string dataset,string filename,DCSketch& dcsketch,uint32_t tmpmem){
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../DCSketch/output/MetaData/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + to_string(tmpmem) + "_" + filename + ".txt");
       
    if(!ifile_hand || !ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    // clock_t startTime,endTime;
    // startTime = clock();
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
        uint32_t estimated_spread = dcsketch.query_spread(flowid);
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
    }
    // endTime = clock();
    // cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    ifile_hand.close();
    ofile_hand.close();
}