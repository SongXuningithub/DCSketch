#ifndef _RERSKT_H_
#define _RERSKT_H_

#include<iostream>
#include<array>
#include<string>
#include<math.h>
#include"MurmurHash3.h"
using namespace std;

#define HASH_SEED_1 92317
#define HASH_SEED_2 37361 
#define HASH_SEED_3 52813

uint32_t str_hash32(string input_str,uint32_t hashseed)
{
    uint32_t hash_res;
    MurmurHash3_x86_32 ( input_str.c_str(), input_str.length(),hashseed, &hash_res );
    return hash_res;
}

array<uint64_t,2> str_hash128(string input_str,uint32_t hashseed)
{
    uint64_t hash_res[2];
    MurmurHash3_x64_128 ( input_str.c_str(), input_str.length(), hashseed, hash_res );
    return array<uint64_t,2>{hash_res[0],hash_res[1]};
}

class HLL{
public:
    static const uint32_t register_num = 128;
    static const uint32_t register_size = 5;
    static const uint32_t HLL_size = register_num * register_size;
    static constexpr double alpha_m = 0.7213/(1+1.079/128); 
    array<uint8_t,register_num> HLL_registers{};
    uint8_t get_leading_zeros(uint32_t bitstr);
    void process_element(string element,uint32_t unit_pos);
    static int get_spread(array<uint8_t,register_num> virtual_HLL);
};

class RerSkt{
public:
    static const uint32_t memory = 1000;  //kB
    static const uint32_t table_size = memory * 1024 * 8 / 2 /HLL::HLL_size;
    array<HLL,table_size> table1;
    array<HLL,table_size> table2;
    void process_flow(string flowid,string element);
    int get_flow_spread(string flowid);
};



#endif