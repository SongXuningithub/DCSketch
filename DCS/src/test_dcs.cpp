#include"dcs.h"
#include "mylibpcap.h"
#include <ctime>

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders);
bool per_src_flow = true;
int main()
{
    DCS_Synopsis dcs(1024);
    string dataset = "KAGGLE"; 
    if(dataset == "CAIDA")
    {
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }
    // string filename = "5M_frag (1)";
    // string filename = "pkts_frag_00001";
    string filename = "Unicauca";
    PCAP_SESSION session(dataset,filename,CSV_FILE);
    IP_PACKET cur_packet;
    string srcip,dstip;
    clock_t startTime,endTime;
    startTime = clock();
    while(int status = session.get_packet(cur_packet))
    {
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        if(per_src_flow)
            dcs.update(srcip,dstip);
        else
            dcs.update(dstip,srcip);
        if(session.proc_num()%1000000 == 0)
        {
            cout<<"process packet "<<session.proc_num()<<endl;
        }
    }
    endTime = clock();
    cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

    vector<IdSpread> superspreaders;
    startTime = clock();
    dcs.BaseTopk(superspreaders, 300);
    endTime = clock();
    cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    write_res(dataset, filename, superspreaders);
    return 0;
}

void write_res(string dataset,string filename,vector<IdSpread>& superspreaders)
{
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../DCS/output/" + dataset + "/";
    ifstream ifile_hand;
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