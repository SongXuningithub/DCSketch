#ifndef _DCSKETCH_H_
#define _DCSKETCH_H_

#include "hashfunc.h"
#include<iostream>
#include<cmath>
#include<string>
#include<array>
#include<memory>
#include<vector>
#include<set>
#include<unordered_map>
using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::array;
using std::unordered_map;

// #define DEBUG_LAYER12 3
//#define DEBUG_LAYER2 3
// #define DEBUG_LAYER1 1

#define MAX_UINT8 255
#define MAX_UINT16 65535
#define MAX_UINT32 4294967295

/*
Layer1:     A "special" sketch which consists of lots of small bitmaps(rather than counters), which are
            used to record spreads of small flows. Flows which have "filled" all its hashed bitmaps will
            resort to layer2 to record them.  
*/

// namespace L1_Param{
//     const uint32_t memory = 400;      //kB
//     const uint32_t bitmap_size = 6;      //bits
//     const uint32_t bitmap_num = memory * 1024 * 8 / bitmap_size;
//     const uint32_t hash_num = 2;
//     static constexpr double max_spread = 9.8275; //L1_Param::bitmap_size * log(L1_Param::bitmap_size);
// };

class Bitmap_Arr{
public:
    static const uint32_t memory = 400;      //kB
    static const uint32_t bitmap_size = 6;      //bits
    static const uint32_t bitmap_num = memory * 1024 * 8 / bitmap_size;
    static const uint32_t hash_num = 2;
    uint32_t raw[memory*1024*8/32];
    array<uint8_t,bitmap_size> patterns;
    array<double,bitmap_size + 1> spreads;
#define BITMAP_FULL_FLAG 435
    Bitmap_Arr();
    uint8_t get_bitmap(uint32_t bitmap_pos);
    bool check_bitmap_full(uint8_t input_bitmap);
    bool add_element(uint32_t bit_pos);
    bool process_element(string flowid,string element);
    uint32_t get_spread(string flowid);
    uint32_t get_spread(uint32_t pos);
};

/*
Layer 2:    A sketch which consists of HyperLogLog estimators. In another word, the counter array in canonical
            sketches is replaced with HyperLogLog estimator array in our Layer2 sketch. We use Layer2 to record
            Medium-sized flows.
*/

class HLL_Arr{
public:
    static const uint32_t memory = 300;      //kB
#define HASH_SEED_1 92317
#define HASH_SEED_2 37361 
    static const uint32_t register_num = 32;
    static const uint32_t register_size = 4;
    static const uint32_t HLL_size = register_num * register_size;
    static const uint32_t HLL_num = memory * 1024 * 8 / HLL_size;
    static constexpr double alpha_m = 0.673; 
    static constexpr double alpha_m_sqm = alpha_m * register_num * register_num; 
    static constexpr double LC_thresh = 2.5 * register_num; 
    array<uint8_t,memory * 1024 * 8 / 8> HLL_raw{};
    
    array<double,1<<register_size> exp_table;

    uint32_t get_leading_zeros(uint32_t bitstr);
    uint32_t get_counter_val(uint32_t HLL_pos,uint32_t bucket_pos);
    void set_counter_val(uint32_t HLL_pos,uint32_t bucket_pos,uint32_t val_);
    void process_packet(string flowid,string elementid);
    uint32_t get_spread(string flowid);
    uint32_t get_spread(uint32_t pos);

    class Table_Entry{
    public:
        string flowid;
        array<uint8_t,2> selected_counters;
    };
    static const uint32_t table_mem = 20; //KB
    static const uint32_t tab_size = table_mem * 1024 * 8 / (4 + 4 + 32);
    vector<Table_Entry> hash_table; 
    void insert_hashtab(string flowid, array<uint32_t,2> HLL_pos, uint64_t hahsres64);
    void report_superspreaders(uint32_t threshold, set<string>& superspreaders);
    
    HLL_Arr()
    {
        hash_table.resize(tab_size);
        for(size_t i = 0;i < exp_table.size();i++)
        {
            exp_table[i] = pow(2.0, 0.0 - i);
        }
    }
};

/*
Layer 3:    A hash table which store <key,value> pairs where the key is the flow ID and the value is 
            its BJKST array. Each unit(or integer) of the array has 16 bits, which is more than that(
            10 bits) in Layer2. So the resolution of Layer3 is higher and valid estimation range is 
            larger.
*/

void HLL_Arr::insert_hashtab(string flowid, array<uint32_t,2> HLL_pos, uint64_t hahsres64)
{
    uint32_t hashres32 = hahsres64 >> 32;         //high 32 bits of initial hash result which is 64 bits
    uint32_t table_pos1 = (hashres32 >> 16) % tab_size;     //high 16 bits
    uint32_t table_pos2 = (hashres32 & MAX_UINT16) % tab_size;  //low 16 bits
    hashres32 = hahsres64 & MAX_UINT32;           //low 32 bits of initial hash result which is 64 bits
    uint32_t innerpos1 = hashres32 & 15;
    uint32_t innerpos2 = ((hashres32 >> 4) & 15) + 16;
    uint32_t selected_val1 = min(get_counter_val(HLL_pos[0],innerpos1), get_counter_val(HLL_pos[1],innerpos1));
    uint32_t selected_val2 = min(get_counter_val(HLL_pos[0],innerpos2), get_counter_val(HLL_pos[1],innerpos2));

    if(hash_table[table_pos1].flowid == "" || hash_table[table_pos1].flowid == flowid)
    {
        hash_table[table_pos1].flowid = flowid;
        hash_table[table_pos1].selected_counters[0] = selected_val1;
        hash_table[table_pos1].selected_counters[1] = selected_val2;
        return;
    }
    else if(hash_table[table_pos2].flowid == "" || hash_table[table_pos2].flowid == flowid)
    {
        hash_table[table_pos2].flowid = flowid;
        hash_table[table_pos2].selected_counters[0] = selected_val1;
        hash_table[table_pos2].selected_counters[1] = selected_val2;
        return;
    }

    // double tmp1 = pow(2.0, 0.0 - hash_table[table_pos1].selected_counters[0] ) + pow(2.0, 0.0 - hash_table[table_pos1].selected_counters[1] );  
    // double tmp2 = pow(2.0, 0.0 - hash_table[table_pos2].selected_counters[0] ) + pow(2.0, 0.0 - hash_table[table_pos2].selected_counters[1] ); 
    // double local_hllval = pow(2.0, 0.0 - selected_val1) + pow(2.0, 0.0 - selected_val2);
    double tmp1 = exp_table[hash_table[table_pos1].selected_counters[0]] + exp_table[hash_table[table_pos1].selected_counters[1]];
    double tmp2 = exp_table[hash_table[table_pos2].selected_counters[0]] + exp_table[hash_table[table_pos2].selected_counters[1]]; 
    double local_hllval = exp_table[selected_val1] + exp_table[selected_val2];
    if(tmp1 > tmp2)
    {
        if(tmp1 >= local_hllval)
        {
            hash_table[table_pos1].flowid = flowid;
            hash_table[table_pos1].selected_counters[0] = selected_val1;
            hash_table[table_pos1].selected_counters[1] = selected_val2;
        }
    }
    else
    {
        if(tmp2 >= local_hllval)
        {
            hash_table[table_pos2].flowid = flowid;
            hash_table[table_pos2].selected_counters[0] = selected_val1;
            hash_table[table_pos2].selected_counters[1] = selected_val2;
        }
    }
}

void HLL_Arr::report_superspreaders(uint32_t threshold, set<string>& superspreaders)
{
    superspreaders.clear();
    set<string> checked_flows;
    for(size_t i = 0;i < tab_size;i++)
    {
        string tmp_flowid = hash_table[i].flowid;
        if(checked_flows.find(tmp_flowid) != checked_flows.end())
        {
            continue;
        } 
        else
        {
            checked_flows.insert(tmp_flowid);
            uint32_t esti_card = get_spread(tmp_flowid); 
            if(19 + esti_card >= threshold)
            {
                superspreaders.insert(tmp_flowid);
            }
        }
    }
}

class HLL{
public:
#define HASH_SEED_1 92317
#define HASH_SEED_2 37361 
    static const uint32_t register_num = 128;
    static const uint32_t register_size = 5;
    static const uint32_t HLL_size = register_num * register_size;
    static constexpr double alpha_m = 0.7213/(1+1.079/128); 
    array<uint8_t,register_num> HLL_registers{};
    uint8_t get_leading_zeros(uint32_t bitstr);
    void process_flow(string flowid);
    uint32_t get_cardinality();
};

class DCSketch{
public:
    //online part
    Bitmap_Arr layer1;
    HLL_Arr layer2;
    HLL FLOW_CARD;
    HLL ELEM_CARD;
    //DCSketch(){}
    void update_global_HLL(string flowid,string element);
    uint32_t L1_mean_error;
    uint32_t L2_mean_error;
    void process_element(string flowid,string element);
    uint32_t query_spread(string flowid);
    void update_mean_error();

    // //offline part
    // vector<uint8_t> offline_layer1;
    // vector<uint32_t> offline_layer2;
    // DCSketch(string dataset,string filename);
};



#endif
