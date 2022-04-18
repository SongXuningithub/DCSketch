#include "cds.h"
#include "mylibpcap.h"
#include <ctime>
#include <unordered_map>

#define DETECT_SUPERSPREADERS 1
// #define DETECT_SUPERCHANGES 1


void write_ss(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem);
void write_sc(string dataset, vector<IdSpread>& superchanges, uint32_t tmpmem);
bool per_src_flow = true;
int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};

    string dataset = "MAWI";
    if(dataset == "CAIDA"){
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }

#ifdef DETECT_SUPERSPREADERS
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    vector<uint32_t> mems{1000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        CDS cds(tmpmem);
        string filename = datasets[dataset][0];
        PCAP_SESSION session(dataset,filename, PCAP_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;
        clock_t startTime,endTime;
        startTime = clock();
        while(int status = session.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if(per_src_flow)
                cds.update(srcip,dstip);
            else
                cds.update(dstip,srcip);
        }
        endTime = clock();
        cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

        vector<IdSpread> superspreaders;
        startTime = clock();
        cds.DetectSuperSpreaders(superspreaders);
        endTime = clock();
        cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
        // write_ss(dataset, filename, superspreaders,tmpmem);
    }
    
    return 0;
#endif


#ifdef DETECT_SUPERCHANGES
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    vector<uint32_t> mems{2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;

        CDS cds1(tmpmem);
        string filename = datasets[dataset][0];
        PCAP_SESSION session1(dataset,filename,PCAP_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;

        while(int status = session1.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if(per_src_flow)
                cds1.update(srcip,dstip);
            else
                cds1.update(dstip,srcip);
            /**/
            if(session1.proc_num() == 1788648)
                break;
            /**/
        }

        CDS cds2(tmpmem);
        filename = datasets[dataset][0]; /*KAGGLE*/
        PCAP_SESSION session2(dataset,filename,PCAP_FILE);

        while(int status = session2.get_packet(cur_packet)){
            /**/
            if(session2.proc_num() <= 1788648)
                continue;
            /**/
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if(per_src_flow)
                cds2.update(srcip,dstip);
            else
                cds2.update(dstip,srcip);
        }

        vector<IdSpread> superchanges;
        cds2.DetectSuperChanges(cds1, superchanges);
        sort(superchanges.begin(), superchanges.end(), IdSpreadComp);
        write_sc(dataset, superchanges, tmpmem);
    
    }
    return 0;

#endif
}

void write_ss(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem){
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../CDS/output/SuperSpreaders/" + dataset + "/";
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

void write_sc(string dataset, vector<IdSpread>& superchanges, uint32_t tmpmem){
    string ofile_path = "../../CDS/output/SuperChanges/" + dataset + "/";
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