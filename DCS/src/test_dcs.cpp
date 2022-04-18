#include"dcs.h"
#include "mylibpcap.h"
#include <ctime>

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders,uint32_t tmpmem);
bool per_src_flow = true;
int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};
    
    string dataset = "MAWI"; 

    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    vector<uint32_t> mems{1000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        DCS_Synopsis dcs(tmpmem);
        if(dataset == "CAIDA"){
            per_src_flow = false;
            cout<<"per_src_flow = false"<<endl;
        }
        string filename = datasets[dataset][0];
        PCAP_SESSION session(dataset,filename,PCAP_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;
        clock_t startTime,endTime;
        startTime = clock();
        while(int status = session.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if(per_src_flow)
                dcs.update(srcip,dstip);
            else
                dcs.update(dstip,srcip);
            // if(session.proc_num()%1000000 == 0)
            //     cout<<"process packet "<<session.proc_num()<<endl;
        }
        endTime = clock();
        cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

        vector<IdSpread> superspreaders;
        startTime = clock();
        dcs.BaseTopk(superspreaders, 300);
        endTime = clock();
        cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
        // write_res(dataset, filename, superspreaders,tmpmem);
    }

    
    return 0;
}

void write_res(string dataset,string filename,vector<IdSpread>& superspreaders,uint32_t tmpmem){
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../DCS/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + filename + ".txt");
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