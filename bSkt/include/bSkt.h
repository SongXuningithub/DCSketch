#ifndef _BSKT_H_
#define _BSKT_H_

#include<iostream>
#include<array>
#include<string>
#include<math.h>
#include<vector>
#include<algorithm>
#include "hashfunc.h"
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
    void process_element(uint32_t hashres);
    int get_spread();
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
    bool DETECT_SUPERSPREADER = true;
    static const uint32_t memory = 1000;  //kB
    static const uint32_t table_size = memory * 1024 * 8 / 4 /HLL::HLL_size;
    array<array<HLL,table_size>,4> tables;
    void process_packet(string flowid,string element);
    uint32_t get_flow_spread(string flowid);
    uint32_t heap_size = 300;
    vector<FLOW> heap;
};



#endif