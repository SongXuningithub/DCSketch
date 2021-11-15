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

void write_perflow_spread(string dataset,string filename,DCSketch& dcsketch);
void write_real_distribution(string dataset,string filename,DCSketch& dcsketch);
void write_sketch(string dataset,string filename,DCSketch& dcsketch);
void write_superspreaders(string dataset,string filename,set<string>& superspreaders);
int main()
{
#define OUTPUT_PERFLOW_SPREAD 1
#define OUTPUT_SUPER_SPREADERS 1
//#define OUTPUT_SKETCH 1
//#define OUTPUT_REAL_DISTRIBUTION 1
    DCSketch dcsketch;
    string dataset = "KAGGLE";
    //string filename = "imc_merge_0000";
    //string filename = "CAIDA_frag_0000";
    string filename = "Dataset-Unicauca";
    PCAP_SESSION session(dataset,filename,CSV_FILE);
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

#ifdef OUTPUT_PERFLOW_SPREAD
    write_perflow_spread(dataset,filename,dcsketch);
#endif

#ifdef OUTPUT_SUPER_SPREADERS
    uint32_t threshold = 1000;
    set<string> superspreaders;
    dcsketch.layer2.report_superspreaders(threshold, superspreaders);
    write_superspreaders(dataset,filename,superspreaders);
#endif
    
#ifdef OUTPUT_SKETCH
    write_sketch(dataset,filename,dcsketch);
#endif

#ifdef OUTPUT_REAL_DISTRIBUTION
write_real_distribution(dataset,filename,dcsketch);
#endif
    return 0;
}

void write_perflow_spread(string dataset,string filename,DCSketch& dcsketch)
{
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../DCSketch/output/PerFlowSpread/" + dataset + "/";
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

void write_sketch(string dataset,string filename,DCSketch& dcsketch)
{
    string ofile_path = "../../DCSketch/output/MetaData/" + dataset + "/";
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + filename.substr(filename.size() - 4) + "sketch.txt");
    if(!ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
    uint32_t layer1_len = dcsketch.layer1.bitmap_num;
    ofile_hand<<"layer1: "<<layer1_len<<endl;
    for(size_t i = 0;i < layer1_len;i++)
    {
        ofile_hand << dcsketch.layer1.get_spread(i) << endl;
    }
    uint32_t layer2_len = dcsketch.layer2.HLL_num;
    ofile_hand<<"layer2: "<<layer2_len << endl;
    for(size_t i = 0;i < layer2_len;i++)
    {
        ofile_hand << dcsketch.layer2.get_spread(i) << endl;
    }
    ofile_hand.close();


    ofile_hand = ofstream(ofile_path + filename.substr(filename.size() - 4) + "sketch_dist.txt");
    if(!ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
    unordered_map<uint32_t,uint32_t> HLL_dist;
    set<uint32_t> possible_vals;
    for(size_t i = 0;i < dcsketch.layer2.HLL_num;i++)
    {
        uint32_t cur_val = dcsketch.layer2.get_spread(i);
        if(HLL_dist.find(cur_val) != HLL_dist.end())
            HLL_dist[cur_val]++;
        else
            HLL_dist.insert(pair<uint32_t,uint32_t>(cur_val,1));
    }
    possible_vals.clear();
    for(auto iter = HLL_dist.begin();iter != HLL_dist.end();iter++)
    {
        possible_vals.insert(iter->first);
    }
    ofile_hand<<"HLL value distribution"<<endl;
    for(auto hllval : possible_vals)
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
    possible_vals.clear();
    for(auto iter = Bitmap_dist.begin();iter != Bitmap_dist.end();iter++)
    {
        possible_vals.insert(iter->first);
    }
    ofile_hand<<"Bitmap value distribution"<<endl;
    for(auto hllval : possible_vals)
    {
        ofile_hand << hllval << " " << Bitmap_dist[hllval] << endl;
    }
    ofile_hand.close();
}

void write_superspreaders(string dataset,string filename,set<string>& superspreaders)
{
    string ofile_path = "../../DCSketch/output/SuperSpreaders/" + dataset + "/";
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

bool cmp_fun(pair<uint32_t,uint32_t> &a,pair<uint32_t,uint32_t> &b)
{
    return a.first < b.first;
}

void write_real_distribution(string dataset,string filename,DCSketch& dcsketch)
{
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../DCSketch/output/MetaData/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename.substr(filename.size() - 4) + ".txt");
    ofile_hand = ofstream(ofile_path + filename.substr(filename.size() - 4) + "real_dist.txt");
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
    ifile_hand.close();
    ofile_hand.close();
}




