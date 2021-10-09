#include "DCSketch.h"
#include "MurmurHash3.h"
#include "mylibpcap.h"
#include <iostream>
#include <fstream>
#include <set>
#include <memory>
#include <algorithm>
#include <unistd.h>
using std::unique_ptr;

void write_res(string filename,DCSketch& dcsketch);

int main()
{
    DCSketch dcsketch;
    //string filename = "imc_merge_0000";
    string filename = "CAIDA_frag_0000";
    //string filename = "pkts_frag_00000";
    PCAP_SESSION session(filename);
    IP_PACKET cur_packet;
    string srcip,dstip;
    
    while(int status = session.get_packet(cur_packet))
    {
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        dcsketch.process_element(srcip,dstip);
        if(session.proc_num()%1000000 == 0)
        {
            cout<<"process packet "<<session.proc_num()<<endl;
        }
    }
    dcsketch.update_mean_error();
    write_res(filename,dcsketch);
    
    //dcsketch.layer3.report_superspreaders();
    return 0;
}

void write_res(string filename,DCSketch& dcsketch)
{
    // string ifile_path = "../../get_groundtruth/truth/IMC/";
    // string ofile_path = "../../DCSketch/output/IMC/";
    string ifile_path = "../../get_groundtruth/truth/CAIDA/";
    string ofile_path = "../../DCSketch/output/CAIDA/";
    // string ifile_path = "../../get_groundtruth/truth/MAWI/";
    // string ofile_path = "../../DCSketch/output/MAWI/";
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
        uint32_t estimated_spread = dcsketch.query_spread(flowid);
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread<<endl;
    }
    cout<<"mean error in layer1:" << dcsketch.L1_mean_error<<endl;
    cout<<"mean error in layer2:" << dcsketch.L2_mean_error<<endl;
    ifile_hand.close();
    ofile_hand.close();
}

