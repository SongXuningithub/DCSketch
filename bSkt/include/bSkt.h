#ifndef _BSKT_H_
#define _BSKT_H_

#include<iostream>
#include<array>
#include<string>
#include<math.h>
#include<vector>
#include<algorithm>
#include<set>
#include "hashfunc.h"
#include "util.h"
using namespace std;

#define HASH_SEED_1 92317
#define HASH_SEED_2 37361 
#define HASH_SEED_3 52813

class HLL{
public:
    static const uint32_t register_num = 128;
    static const uint32_t register_size = 5;
    static const uint32_t size = register_num * register_size;
    static const uint32_t HLL_size = register_num * register_size;
    static constexpr double alpha_m = 0.7213/(1+1.079 / 128); 
    array<uint8_t,register_num> HLL_registers{};
    uint8_t get_leading_zeros(uint32_t bitstr);
    void record_element(uint32_t hashres);
    int get_spread();
    double memory_utilization();
};

class Bitmap{
public:
    static const uint32_t bitnum = 5000;
    static const uint32_t size = bitnum;
    array<uint8_t,bitnum/8> raw{};
    void record_element(uint32_t hashres);
    uint32_t get_unitval(uint32_t bitpos);
    int get_spread();
    void reset();
    // uint32_t size(){ return bitnum; }
    Bitmap(){ reset(); }
    double memory_utilization();
};

class MultiResBitmap {
public:
    static constexpr double sigma = 0.2;
    static const uint32_t b = 0.6367 / (sigma * sigma);
    static const uint32_t LEN = b;
    static const uint32_t b_hat = 2 * b;
    static const uint32_t C = 120000;
    inline static uint32_t c; // = log2(C / (2.6744 * b)); //== 9.47
    static constexpr double setmax_ratio = 0.9311;
    inline static uint32_t mrbitmap_size;  // = b * (c - 1) + b_hat;
    static const uint32_t size = 240;
    vector<vector<uint8_t>> bitmaps;
    inline static bool init_flag = false;

public:
    MultiResBitmap(); 
    static void shared_param_Init();
    static uint32_t get_size();
    uint32_t get_ones_num(uint32_t layer);
    void record_element(uint32_t hashres);
    int get_spread();
    double memory_utilization();
};


class FLOW{
public:
    string flowid;
    uint32_t flow_spread;
    FLOW(){flow_spread = 0;flowid="";}
};

struct MinHeapCmp{
    inline bool operator()(const FLOW &x, const FLOW &y){
        return x.flow_spread > y.flow_spread;
    }
};

template<class Estimator>
class bSkt{
public:
    //bSkt
    bool DETECT_SUPERSPREADER = false;
    uint32_t memory;  //kB
    uint32_t table_size;   //memory * 1024 * 8 / 4 /Estimator::size;
    vector<vector<Estimator>> tables;

    void process_packet(string flowid,string element);
    void report_superspreaders(vector<IdSpread>& superspreaders);
    bSkt(uint32_t memory_, double unused);
    uint32_t get_flow_cardinality(string flowid);
    uint32_t heap_size = 400;
    set<string> inserted;
    vector<FLOW> heap;

    vector<double> get_utilization();
};

template<class Estimator>
vector<double> bSkt<Estimator>::get_utilization(){
    vector<double> bins(100);
    auto tab = tables[0];
    double res;
    for(auto estimator : tab) {
        uint32_t bin_idx = static_cast<uint32_t>(100 * estimator.memory_utilization());
        bin_idx = bin_idx < 100 ? bin_idx : 100;
        bins[bin_idx]++;
    }
    for (size_t i = 0;i < bins.size();i++) {
        bins[i] = bins[i] / tab.size();
    }
    for (size_t i = 1;i < bins.size();i++) {
        bins[i] = bins[i] + bins[i - 1];
    }
    return bins;
}

template<class Estimator>
bSkt<Estimator>::bSkt(uint32_t memory_, double unused) : memory(memory_), table_size(memory * 1024 * 8 / 4 /Estimator::size), 
tables(4){
    tables[0].resize(table_size);
    tables[1].resize(table_size);
    tables[2].resize(table_size);
    tables[3].resize(table_size);
    cout << "tables[0].size(): " << tables[0].size() << endl;
}

template<class Estimator>
void bSkt<Estimator>::report_superspreaders(vector<IdSpread>& superspreaders){
    superspreaders.clear();
    for(size_t i = 0;i < heap.size();i++){
        superspreaders.push_back(IdSpread(heap[i].flowid, heap[i].flow_spread));
    }
    sort(superspreaders.begin(), superspreaders.end(), IdSpreadComp);
}

#endif