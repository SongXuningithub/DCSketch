#include "MurmurHash3.h"
#include "mylibpcap.h"
#include "rerskt.h"
#include <iostream>
#include <set>
#include <memory>
#include <algorithm>
#include <fstream>
#include <unistd.h>
using std::unique_ptr;

void write_res(string filename,RerSkt& rersketch);

int main()
{
    //string filename = "CAIDA_frag_0000";
    string filename = "imc_merge_0000";
    PCAP_SESSION session(filename);
    IP_PACKET cur_packet;
    string srcip,dstip;
    RerSkt rerskt;
    while(int status = session.get_packet(cur_packet))
    {
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        rerskt.process_flow(srcip,dstip);
        if(session.proc_num()%1000000 == 0)
        {
            cout<<"process packet "<<session.proc_num()<<endl;
        }
    }
    // string flow_id;
    // while(bool iseof = checker.query_flow(flow_id))
    // {
    //     int flow_spread = rerskt.get_flow_spread(flow_id);
    //     flow_spread = max(0,flow_spread);
    //     checker.record_result((uint32_t)flow_spread);
    //     //checker.record_full_result(flow_spread,dcsketch.layer1);
    // }
    write_res(filename,rerskt);
    return 0;
}

void write_res(string filename,RerSkt& rersketch)
{
    // string ifile_path = "../../get_groundtruth/truth/CAIDA/";
    // string ofile_path = "../../rerskt/output/CAIDA/";
    string ifile_path = "../../get_groundtruth/truth/IMC/";
    string ofile_path = "../../rerskt/output/IMC/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename.substr(filename.size() - 4) + ".txt");
    ofile_hand = ofstream(ofile_path + filename.substr(filename.size() - 4) + ".txt");
    if(!ifile_hand || !ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
    while(!ifile_hand.eof())
    {
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        ifile_hand >> spread;
        int estimated_spread = rersketch.get_flow_spread(flowid);
        estimated_spread = estimated_spread > 0 ? estimated_spread : 0;
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread<<endl;
    }
    ifile_hand.close();
    ofile_hand.close();
}

