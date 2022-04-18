#include "Vector_BF.h"
#include "mylibpcap.h"
#include <ctime>
#include <unordered_map>

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem);
bool per_src_flow = true;
int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};
    
    string dataset = "KAGGLE";
    if(dataset == "CAIDA"){
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }

    vector<uint32_t> mems{500, 1000, 1500, 2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        Vector_Bloom_Filter vbf(tmpmem);
 
        string filename = datasets[dataset][0];
        PCAP_SESSION session(dataset,filename,CSV_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;
        clock_t startTime,endTime;
        startTime = clock();
        while(int status = session.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if(per_src_flow){
                array<uint8_t,4> srcip_tuple;
                for(size_t i = 0;i < 4;i++)
                    srcip_tuple[i] = cur_packet.srcdot[i];
                vbf.process_packet(srcip,srcip_tuple,dstip);
            } else {
                array<uint8_t,4> dstip_tuple;
                for(size_t i = 0;i < 4;i++)
                    dstip_tuple[i] = cur_packet.dstdot[i];
                vbf.process_packet(dstip,dstip_tuple,srcip);
            }
        }
        endTime = clock();
        cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

        uint32_t threshold = 600;
        vbf.calc_Z(threshold);
        vector<IdSpread> superspreaders;
        
        startTime = clock();
        vbf.Detect_Superpoint(superspreaders);
        endTime = clock();
        cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
        write_res(dataset, filename, superspreaders,tmpmem);
    }

    return 0;
}

void write_res(string dataset,string filename,vector<IdSpread>& superspreaders,uint32_t tmpmem){
    string ofile_path = "../../Vector_BF/output/" + dataset + "/";
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