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
    static const uint32_t HLL_size = register_num * register_size;
    static constexpr double alpha_m = 0.7213/(1+1.079/128); 
    array<uint8_t,register_num> HLL_registers{};
    uint8_t get_leading_zeros(uint32_t bitstr);
    void record_element(uint32_t hashres);
    int get_spread();
};

class Bitmap{
public:
    static const uint32_t bitnum = 500;
    array<uint8_t,bitnum/8> raw{};
    void record_element(uint32_t hashres);
    uint32_t get_unitval(uint32_t bitpos);
    int get_spread();
    void reset();
    uint32_t size(){ return bitnum; }
    Bitmap(){ reset(); }
};

class FLOW{
public:
    string flowid;
    uint32_t flow_spread;
    FLOW(){flow_spread = 0;flowid="";}
};

struct MinHeapCmp
{
    inline bool operator()(const FLOW &x, const FLOW &y)
    {
        return x.flow_spread > y.flow_spread;
    }
};

class bSkt{
public:
// #define HLL_MODE 1
#define Bitmap_MODE 1
    bool DETECT_SUPERSPREADER = false;
    static const uint32_t memory = 1000;  //kB

#ifdef HLL_MODE
    static const uint32_t table_size = memory * 1024 * 8 / 4 /HLL::HLL_size;
    array<array<HLL,table_size>,4> tables;
#endif
#ifdef Bitmap_MODE 
    static const uint32_t table_size = memory * 1024 * 8 / 4 /Bitmap::bitnum;
    array<array<Bitmap,table_size>,4> tables;
#endif
    void process_packet(string flowid,string element);
    void report_superspreaders(vector<IdSpread>& superspreaders);
    uint32_t get_flow_spread(string flowid);
    uint32_t heap_size = 400;
    set<string> inserted;
    vector<FLOW> heap;
};

void bSkt::report_superspreaders(vector<IdSpread>& superspreaders)
{
    superspreaders.clear();
    for(size_t i = 0;i < heap.size();i++)
    {
        superspreaders.push_back(IdSpread(heap[i].flowid, heap[i].flow_spread));
    }
    sort(superspreaders.begin(), superspreaders.end(), IdSpreadComp);
}

#endif