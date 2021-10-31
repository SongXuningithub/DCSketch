#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include "hashfunc.h"
using namespace std;

vector<uint8_t> offline_layer1;
vector<uint32_t> offline_layer2;
uint32_t layer1_len;
uint32_t layer2_len;

vector<uint32_t> spread_arr;
vector<uint32_t> freq_arr;
vector<double> spread_prob_arr;

vector<uint32_t> hll_dist;
vector<double> hll_prob;

bool read_dist(string dataset,vector<uint32_t>& spread_arr,vector<uint32_t>& freq_arr,vector<double>& spread_prob_arr)
{
    ifstream filehd("../../Distribution_Estimation/files/" + dataset + "/0000dist.txt");
    if (!filehd)
    {
        cout<<"unable to open file " + dataset + "/0000dist.txt"<<endl;
        return false;
    }
    double freq_sum = 0;
    while (!filehd.eof())
    {
        uint32_t spread;
        uint32_t freq;
        filehd >> spread;
        filehd >> freq;
        freq_sum += freq;
        spread_arr.push_back(spread);
        freq_arr.push_back(freq);
    }
    filehd.close();
    spread_prob_arr.resize(freq_arr.size());
    for(size_t i = 0;i < spread_prob_arr.size();i++)
    {
        spread_prob_arr[i] = freq_arr[i] / freq_sum;
        //cout << spread_prob_arr[i] << endl;
    }

    return true;
}

bool read_sketch_dist(string dataset, vector<uint32_t>& hll_dist, vector<double>& hll_prob)
{
    ifstream filehd("../../DCSketch/output/" + dataset + "/HLL_dist/0000.txt");
    if (!filehd)
    {
        cout<<"unable to open file " + dataset + "/HLL_dist/0000.txt"<<endl;
        return false;
    }
    string linedata;
    while(true)
    {
        getline(filehd,linedata);
        if(linedata.find("HLL")==0)
            break;
    }
    double freq_sum = 0;
    while(true)
    {
        getline(filehd,linedata);
        if(linedata.find("Bitmap")==0)
            break;
        uint32_t strip_pos = linedata.find(" ");
        uint32_t val = stoi(linedata.substr(0,strip_pos));
        uint32_t freq = stoi(linedata.substr(strip_pos + 1));
        hll_dist.resize(val+1,0);
        hll_dist[val] = freq;
        freq_sum += freq;
    }
    cout<<hll_dist.size()<<endl;
    hll_prob.resize(hll_dist.size());
    for(size_t i = 0;i < hll_dist.size();i++)
    {
        hll_prob[i] = hll_dist[i] / freq_sum;
        // if(hll_prob[i] != 0)
        //     cout << hll_prob[i] << endl;
    }
    return true;
}

bool read_sketch(string dataset)
{
    string ifile_path = "../../DCSketch/metadata/" + dataset + "/";
    ifstream ifile_hand;
    ifile_hand = ifstream(ifile_path + "0000sketch.txt");
    if(!ifile_hand)
    {
        cout<<"fail to open files."<<endl;
        return false;
    }

    string str;
    getline(ifile_hand,str);
    cout << str << endl;
    layer1_len = stoi( str.substr( str.find(" ")+1 ) ) ;
    offline_layer1.resize(layer1_len);
    for(size_t i = 0;i < layer1_len;i++)
    {
        getline(ifile_hand,str);
        uint32_t tmp = stoi(str);
        offline_layer1[i] = tmp;
    }

    getline(ifile_hand,str);
    cout << str << endl;
    layer2_len = stoi( str.substr( str.find(" ")+1 ) ) ;
    offline_layer2.resize(layer2_len);
    for(size_t i = 0;i < layer2_len;i++)
    {
        getline(ifile_hand,str);
        uint32_t tmp = stoi(str);
        offline_layer2[i] = tmp;
    }
    return true;
}

void write_res(string dataset)
{
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../Distribution_Estimation/files/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + "0000.txt");
    ofile_hand = ofstream(ofile_path + "0000MLEres.txt");
    if(!ifile_hand || !ofile_hand)
    {
        cout<<"fail to open files."<<endl;
        return;
    }
#define HASH_SEED_1 92317
    uint32_t minhll = 0;
    while(true)
    {
        if(hll_dist[minhll] == 0)
            minhll++;
        else
            break;
    }
    while(!ifile_hand.eof())
    {
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        ifile_hand >> spread;
        array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_1);
        uint32_t HLL_pos[2];
        HLL_pos[0] = (hashres128[1] >> 32) % layer2_len;
        HLL_pos[1] = static_cast<uint32_t>(hashres128[1]) % layer2_len;
        uint32_t H1 = offline_layer2[HLL_pos[0]];
        uint32_t H2 = offline_layer2[HLL_pos[1]];
        uint32_t len = spread_arr.size();
        double maxprob = 0;
        double maxprob_val;
        for(size_t i = 1;i < len;i++)
        {
            if( H1 - i < minhll || H2 - i < minhll)
                break;
            double tmp_prob = spread_prob_arr[i] * hll_prob[H1 - i] * hll_prob[H2 - i];
            if(tmp_prob > maxprob)
            {
                maxprob = tmp_prob;
                maxprob_val = i;
            }
        }

        ofile_hand << flowid <<" "<<spread<<" "<<maxprob_val<<endl;
    }
    ifile_hand.close();
    ofile_hand.close();
}

int main()
{
    string dataset = "CAIDA";
    read_dist(dataset,spread_arr,freq_arr,spread_prob_arr);
    read_sketch_dist(dataset,hll_dist,hll_prob);
    read_sketch(dataset);
    write_res(dataset);
    return 0;
}