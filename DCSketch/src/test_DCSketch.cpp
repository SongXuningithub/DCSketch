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
void write_HLL_distribution(string filename,DCSketch& dcsketch);
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
    cout<<"number of flows: "<<dcsketch.FLOW_CARD.get_cardinality()<<endl;
    cout<<"number of elements: "<<dcsketch.ELEM_CARD.get_cardinality()<<endl;
    write_res(filename,dcsketch);
    write_HLL_distribution(filename,dcsketch);
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

    ifile_hand.close();
    ofile_hand.close();
}

bool cmp_fun(pair<uint32_t,uint32_t> &a,pair<uint32_t,uint32_t> &b)
{
    return a.first < b.first;
}

void write_HLL_distribution(string filename,DCSketch& dcsketch)
{
    // string ifile_path = "../../get_groundtruth/truth/IMC/";
    // string ofile_path = "../../DCSketch/output/IMC/";
    string ifile_path = "../../get_groundtruth/truth/CAIDA/";
    string ofile_path = "../../DCSketch/output/CAIDA/HLL_dist/";
    // string ifile_path = "../../get_groundtruth/truth/MAWI/";
    // string ofile_path = "../../DCSketch/output/MAWI/HLL_dist/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename.substr(filename.size() - 4) + ".txt");
    ofile_hand = ofstream(ofile_path + filename.substr(filename.size() - 4) + ".txt");
    if(!ifile_hand || !ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
    unordered_map<uint32_t,uint32_t> grd_truth;
    while(!ifile_hand.eof())
    {
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        ifile_hand >> spread;
        if(flowid.length() < 4)
            break;
        if(grd_truth.find(spread) != grd_truth.end())
            grd_truth[spread]++;
        else
            grd_truth.insert(pair<uint32_t,uint32_t>(spread,1));
    }

    set<uint32_t> keys;
    for(auto iter = grd_truth.begin();iter != grd_truth.end();iter++)
    {
        keys.insert(iter->first);
    }
    ofile_hand<<"real distribution"<<endl;
    for(auto spread : keys)
    {
        ofile_hand << spread << " " << grd_truth[spread] << endl;
    }

    unordered_map<uint32_t,uint32_t> HLL_dist;
    for(size_t i = 0;i < dcsketch.layer2.HLL_num;i++)
    {
        uint32_t cur_val = dcsketch.layer2.get_spread(i);
        if(HLL_dist.find(cur_val) != HLL_dist.end())
            HLL_dist[cur_val]++;
        else
            HLL_dist.insert(pair<uint32_t,uint32_t>(cur_val,1));
    }
    keys.clear();
    for(auto iter = HLL_dist.begin();iter != HLL_dist.end();iter++)
    {
        keys.insert(iter->first);
    }
    ofile_hand<<"HLL value distribution"<<endl;
    for(auto hllval : keys)
    {
        ofile_hand << hllval << " " << HLL_dist[hllval] << endl;
    }

    unordered_map<uint32_t,uint32_t> Bitmap_dist;
    for(size_t i = 0;i < dcsketch.layer1.bitmap_num;i++)
    {
        uint32_t cur_val = dcsketch.layer1.get_spread(i);
        if(Bitmap_dist.find(cur_val) != Bitmap_dist.end())
            Bitmap_dist[cur_val]++;
        else
            Bitmap_dist.insert(pair<uint32_t,uint32_t>(cur_val,1));
    }
    keys.clear();
    for(auto iter = Bitmap_dist.begin();iter != Bitmap_dist.end();iter++)
    {
        keys.insert(iter->first);
    }
    ofile_hand<<"Bitmap value distribution"<<endl;
    for(auto hllval : keys)
    {
        ofile_hand << hllval << " " << Bitmap_dist[hllval] << endl;
    }
    ifile_hand.close();
    ofile_hand.close();
}

