#include "Vector_BF.h"
#include "mylibpcap.h"
#include <ctime>

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders);
bool per_src_flow = false;
int main()
{
    Vector_Bloom_Filter vbf(1024);
    string dataset = "CAIDA"; 
    //string filename = "imc_merge_0000";
    string filename = "5M_frag (1)";
    // string filename = "pkts_frag_00001";
    //string filename = "Dataset-Unicauca";
    PCAP_SESSION session(dataset,filename,PCAP_FILE);
    IP_PACKET cur_packet;
    string srcip,dstip;
    
    while(int status = session.get_packet(cur_packet))
    {
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        if(per_src_flow)
        {
            array<uint8_t,4> srcip_tuple;
            for(size_t i = 0;i < 4;i++)
            {
                srcip_tuple[i] = cur_packet.srcdot[i];
            }
            vbf.process_packet(srcip,srcip_tuple,dstip);
        }
        else
        {
            array<uint8_t,4> dstip_tuple;
            for(size_t i = 0;i < 4;i++)
            {
                dstip_tuple[i] = cur_packet.dstdot[i];
            }
            vbf.process_packet(dstip,dstip_tuple,srcip);
        }
        if(session.proc_num()%1000000 == 0)
        {
            cout<<"process packet "<<session.proc_num()<<endl;
        }
    }
    uint32_t threshold = 600;
    vbf.calc_Z(threshold);
    vector<IdSpread> superspreaders;
    clock_t startTime,endTime;
    startTime = clock();
    vbf.Detect_Superpoint(superspreaders);
    endTime = clock();
    cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    write_res(dataset, filename, superspreaders);
    return 0;
}

void write_res(string dataset,string filename,vector<IdSpread>& superspreaders)
{
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../Vector_BF/output/" + dataset + "/";
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