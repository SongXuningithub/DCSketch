#include "cds.h"
#include "mylibpcap.h"
#include <ctime>
#include <unordered_map>

// #define DETECT_SUPERSPREADERS 1
#define DETECT_SUPERCHANGES 1


void write_ss(string dataset, string filename, vector<IdSpread>& superspreaders);
void write_sc(string dataset, vector<IdSpread>& superchanges);
bool per_src_flow = true;
int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};

    string dataset = "KAGGLE", filename;
    if(dataset == "CAIDA"){
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }

#ifdef DETECT_SUPERSPREADERS
    CDS cds(1024);
    filename = datasets[dataset][0];
    PCAP_SESSION session(dataset,filename,CSV_FILE);
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
        if(session.proc_num()%1000000 == 0)
            cout<<"process packet "<<session.proc_num()<<endl;
    }
    endTime = clock();
    cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

    vector<IdSpread> superspreaders;
    startTime = clock();
    cds.DetectSuperSpreaders(superspreaders);
    endTime = clock();
    cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    write_ss(dataset, filename, superspreaders);
    return 0;
#endif


#ifdef DETECT_SUPERCHANGES
    CDS cds1(1024);
    filename = datasets[dataset][0];
    PCAP_SESSION session1(dataset,filename,CSV_FILE);
    IP_PACKET cur_packet;
    string srcip,dstip;

    while(int status = session1.get_packet(cur_packet)){
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        if(per_src_flow)
            cds1.update(srcip,dstip);
        else
            cds1.update(dstip,srcip);
    }

    CDS cds2(1024);
    filename = datasets[dataset][1];
    PCAP_SESSION session2(dataset,filename,CSV_FILE);

    while(int status = session2.get_packet(cur_packet)){
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        if(per_src_flow)
            cds2.update(srcip,dstip);
        else
            cds2.update(dstip,srcip);
    }

    vector<IdSpread> superchanges;
    cds2.DetectSuperChanges(cds1, superchanges);
    write_sc(dataset, superchanges);
    return 0;

#endif
}

void write_ss(string dataset,string filename,vector<IdSpread>& superspreaders){
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../CDS/output/SuperSpreaders/" + dataset + "/";
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

void write_sc(string dataset, vector<IdSpread>& superchanges){
    string ofile_path = "../../CDS/output/SuperChanges/";
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + dataset + ".txt");
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