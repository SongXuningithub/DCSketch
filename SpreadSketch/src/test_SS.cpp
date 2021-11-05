#include "SS.h"
#include "mylibpcap.h"
#include <fstream>

void write_res(string dataset,string filename,set<string>& superspreaders);

int main()
{
    SpreadSketch ss(1024);
    string dataset = "MAWI"; 
    //string filename = "imc_merge_0000";
    //string filename = "CAIDA_frag_0000";
    string filename = "pkts_frag_00000";
    PCAP_SESSION session(dataset,filename);
    IP_PACKET cur_packet;
    string srcip,dstip;
    
    while(int status = session.get_packet(cur_packet))
    {
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        ss.update(srcip,dstip);
        if(session.proc_num()%1000000 == 0)
        {
            cout<<"process packet "<<session.proc_num()<<endl;
        }
    }

    set<string> superspreaders;
    uint32_t threshold = 1000;
    ss.output_superspreaders(threshold,superspreaders);
    write_res(dataset, filename, superspreaders);
    return 0;
}

string trans2ipformat(uint32_t val)
{
    string ret = "";
    for(size_t i = 0;i < 4;i++)
    {
        val = val >> (8 * i);
        string tmp = to_string(val & 255);
        while(tmp.size() != 3)
        {
            tmp = "0" + tmp;
        }
        ret = tmp + ret; 
    }
    return ret;
}

void write_res(string dataset,string filename,set<string>& superspreaders)
{
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../SpreadSketch/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + filename.substr(filename.size() - 4) + ".txt");
    if(!ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
    for(auto item : superspreaders)
    {
        ofile_hand << item <<endl;
    }
    ofile_hand.close();
}