#ifndef _RERSKT_H_
#define _RERSKT_H_

#include<iostream>
#include<array>
#include<string>
#include<math.h>
#include<vector>
#include<algorithm>
#include<set>
#include"hashfunc.h"
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
    void record_element(string element,uint32_t unit_pos);
    static int get_spread(array<uint8_t,register_num> virtual_HLL);
    int get_spread();
    void set_unit(uint32_t pos,uint32_t val);
    void reset();
    uint32_t get_unitval(uint32_t pos){ return HLL_registers[pos]; }
    uint32_t size(){ return register_num; }
    HLL(){ reset(); }
};

class Bitmap{
public:
    static const uint32_t bitnum = 5000;
    array<uint8_t,bitnum/8> raw{};
    void record_element(uint32_t unit_pos);
    uint32_t get_unitval(uint32_t bitpos);
    int get_spread();
    void set_unit(uint32_t pos,uint32_t val);
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
    inline bool operator()(const FLOW &x, const FLOW &y){
        return x.flow_spread > y.flow_spread;
    }
};

class RerSkt{
public:
#define HLL_MODE 1
// #define Bitmap_MODE 1
    bool DETECT_SUPERSPREADER = false;
    static const uint32_t memory = 1000;  //kB
    
#ifdef HLL_MODE
    static const uint32_t table_size = memory * 1024 * 8 / 2 /HLL::HLL_size;
    array<HLL,table_size> table1;
    array<HLL,table_size> table2;
#endif
#ifdef Bitmap_MODE
    static const uint32_t table_size = memory * 1024 * 8 / 2 /Bitmap::bitnum;
    array<Bitmap,table_size> table1;
    array<Bitmap,table_size> table2;
#endif
    void process_flow(string flowid,string element);
    int get_flow_spread(string flowid);
    uint32_t heap_size = 300;
    vector<FLOW> heap;
    set<string> inserted;
    RerSkt();
};

RerSkt::RerSkt(){
    for(size_t i = 0; i < table1.size();i++){
        table1[i].reset();
        table2[i].reset();
    }
}



#endif