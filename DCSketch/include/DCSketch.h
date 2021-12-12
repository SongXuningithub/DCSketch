#ifndef _DCSKETCH_H_
#define _DCSKETCH_H_

#include "hashfunc.h"
#include "MAP.h"
#include<iostream>
#include<cmath>
#include<string>
#include<fstream>
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
uint32_t get_leading_zeros(uint32_t bitstr);

/*
Layer1:     A "special" sketch which consists of lots of small bitmaps(rather than counters), which are
            used to record spreads of small flows. Flows which have "filled" all its hashed bitmaps will
            resort to layer2 to record them.  
*/

class Bitmap_Arr{
public:
    static const uint32_t memory = 400;      //kB
    static const uint32_t bitmap_size = 6;      //bits
    static const uint32_t bitmap_num = memory * 1024 * 8 / bitmap_size;
    static const uint32_t hash_num = 2;
    uint32_t raw[memory*1024*8/32];
    array<uint8_t,bitmap_size> patterns;
    array<double,bitmap_size + 1> spreads;
#define BITMAP_FULL_FLAG 435.0
    Bitmap_Arr();
    static constexpr double thresh_ratio = 1.256 / 2;
    
    uint8_t get_bitmap(uint32_t bitmap_pos);
    array<uint32_t,2> get2bitmap_zeronum(array<uint64_t,2>& hash_flowid);
    bool check_bitmap_full(uint8_t input_bitmap);
    bool add_element(uint32_t bit_pos);
    bool process_packet(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element);
    double get_spread(string flowid, array<uint64_t,2>& hash_flowid);
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
    static constexpr double thresh_ratio = 2.103 / 2;
    array<double,1<<register_size> exp_table;

    uint32_t get_counter_val(uint32_t HLL_pos,uint32_t bucket_pos);
    void set_counter_val(uint32_t HLL_pos,uint32_t bucket_pos,uint32_t val_);
    void process_packet(string flowid, array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element);
    uint32_t get_spread(string flowid, array<uint64_t,2>& hash_flowid);
    uint32_t get_spread(uint32_t pos);
    array<uint32_t,2> get2hll_vals(array<uint64_t,2>& hash_flowid);
    class Table_Entry{
    public:
        string flowid;
        uint8_t selected_sum;
        static const uint32_t selected_num = 4;
        //array<uint8_t,2> selected_counters;
    };
    static const uint32_t table_mem = 50; //KB
    static const uint32_t tab_size = table_mem * 1024 * 8 / (8 + 32);
    vector<Table_Entry> hash_table; 
    void insert_hashtab(string flowid, uint8_t selected_sum, uint64_t hahsres64);
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

class Global_HLLs{
public:
    static const uint32_t register_num = 128;
    static const uint32_t register_size = 5;
    static const uint32_t HLL_size = register_num * register_size;
    static constexpr double alpha_m = 0.7213/(1+1.079/128); 
    array<uint8_t,register_num> Layer1_flows{};
    array<uint8_t,register_num> Layer1_elements{};
    array<uint8_t,register_num> Layer2_flows{};
    array<uint8_t,register_num> Layer2_elements{};
    Global_HLLs();
    void update_layer1(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element);
    void update_layer2(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element);
    void process_flow(string flowid);
#define LAYER1 1
#define LAYER2 2
    uint32_t get_cardinality(array<uint8_t,register_num>& HLL_registers);
    uint32_t get_number_flows(uint32_t layer);
    uint32_t get_number_elements(uint32_t layer);
    array<uint8_t,register_num> HLL_union(array<uint8_t,register_num>& HLL_registers1,array<uint8_t,register_num>& HLL_registers2);
};


Global_HLLs::Global_HLLs()
{
    for(size_t i = 0;i < register_num;i++)
    {
        Layer1_flows[i] = 0;
        Layer1_elements[i] = 0;
        Layer2_flows[i] = 0;
        Layer2_elements[i] = 0;
    }
}

class DCSketch{
public:
    Bitmap_Arr layer1;
    HLL_Arr layer2;
    Global_HLLs global_hlls;
    array<uint32_t,2001> Error_RMV;
    bool layer1_err_remove;
    bool layer2_err_remove;
    uint32_t layer1_flows;
    uint32_t layer2_flows;
    uint32_t layer1_elements;
    uint32_t layer2_elements;
    uint32_t L1_mean_error;
    uint32_t L2_mean_error;
    DCSketch();
    void process_element(string flowid,string element);
    uint32_t query_spread(string flowid);
    uint32_t query_spread_offline(string flowid);
    void update_mean_error();
    //Offline
    MAP map_esti;
    void offline_init();
};


void DCSketch::offline_init()
{
    map_esti.MAP_Init(layer1_flows, layer2_flows, layer1_elements, layer2_elements, layer1.bitmap_num, layer2.HLL_num);
}


DCSketch::DCSketch()
{
    string ifile_name = "../../DCSketch/support/error_removal.txt";
    ifstream ifile_hand;
    ifile_hand = ifstream(ifile_name);
    if(!ifile_hand)
    {
        cout<<"fail to open support files."<<endl;
        return;
    }
    while(!ifile_hand.eof())
    {
        uint32_t ratio;
        uint32_t error_val;
        ifile_hand >> ratio;
        ifile_hand >> error_val;
        Error_RMV[ratio] = error_val;
    }
}



#endif
