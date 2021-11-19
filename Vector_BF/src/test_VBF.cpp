#include "Vector_BF.h"
#include "mylibpcap.h"

void write_res(string dataset,string filename,vector<string>& superspreaders);

int main()
{
    Vector_Bloom_Filter vbf(512);
    string dataset = "MAWI"; 
    //string filename = "imc_merge_0000";
    //string filename = "CAIDA_frag_0000";
    string filename = "pkts_frag_00000";
    //string filename = "Dataset-Unicauca";
    PCAP_SESSION session(dataset,filename,PCAP_FILE);
    IP_PACKET cur_packet;
    string srcip,dstip;
    
    while(int status = session.get_packet(cur_packet))
    {
        srcip = cur_packet.get_srcip();
        dstip = cur_packet.get_dstip();
        array<uint8_t,4> srcip_tuple;
        for(size_t i = 0;i < 4;i++)
        {
            srcip_tuple[i] = cur_packet.srcdot[i];
        }
        vbf.process_packet(srcip,srcip_tuple,dstip);
        if(session.proc_num()%1000000 == 0)
        {
            cout<<"process packet "<<session.proc_num()<<endl;
        }
    }
    uint32_t threshold = 400;
    vbf.calc_Z(threshold);
    vector<string> superspreaders;
    vbf.Detect_Superpoint(superspreaders);
    
    write_res(dataset, filename, superspreaders);
    return 0;
}

void write_res(string dataset,string filename,vector<string>& superspreaders)
{
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../Vector_BF/output/" + dataset + "/";
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