#ifndef _V_HLL_
#define _V_HLL_

#include<vector>
#include<iostream>
#include<array>
#include<algorithm>
#include"hashfunc.h"
#include"util.h"
#include<set>
using namespace std;

/*FLOW is used for heap*/
class FLOW{
public:
    string flowid;
    uint32_t flow_spread;
    FLOW(){flow_spread = 0;flowid="";}
};

struct MinHeapCmp
{
    inline bool operator()(const FLOW &x, const FLOW &y){
        return x.flow_spread > y.flow_spread;
    }
};

class vHLL{
public:
    uint32_t memory;
    static const uint32_t HLL_size = 128;
    static const uint32_t glb_HLL_size = 1024;
    static const uint32_t register_size = 5;
    uint32_t register_num;
    vector<uint32_t> raw;
    vector<uint8_t> global_HLL;
    static const uint32_t heap_size = 300;
    vector<FLOW> heap;
    set<string> inserted;
    bool DETECT_SUPERSPREADER = false;
    vHLL(uint32_t mem);
#define HASH_SEED_1 92317
#define HASH_SEED_2 37361 
    void process_packet(string flowID, string elementID);
    uint8_t get_register(uint32_t reg_pos);
    void set_register(uint32_t reg_pos, uint8_t val);
    uint32_t get_spread(vector<uint8_t> virtual_HLL);
    int get_spread(string flowID);
};



#endif