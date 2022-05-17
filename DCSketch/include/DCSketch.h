#ifndef _DCSKETCH_H_
#define _DCSKETCH_H_

#include "hashfunc.h"
#include "util.h"
#include<iostream>
#include<bitset>
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

//#define DEBUG_LAYER12 3
//#define DEBUG_LAYER2 3
// #define DEBUG_LAYER1 1
// #define DEBUG_OUTPUT 1

#define MAX_UINT8 255
#define MAX_UINT16 65535
#define MAX_UINT32 4294967295
uint32_t get_leading_zeros(uint32_t bitstr);
/*
TO DO:
(1) uint8 --> uint16 for LC
(2) bitset for LC
*/


/*
Layer1:     A "special" sketch which consists of lots of small bitmaps(rather than counters), which are
            used to record spreads of small flows. Flows which have "filled" all its hashed bitmaps will
            resort to layer2 to record them.  
*/

class Bitmap_Arr{
public:
    uint32_t memory;// = memory_size * 3 / 5;      //kB
    static const uint32_t bitmap_size = 15;      //bits
    uint32_t bitmap_num;
    vector<uint32_t> raw;  
    array<uint16_t,bitmap_size> patterns;
    array<double,bitmap_size + 1> spreads;
    static const uint16_t FULL_PAT  = (1 << bitmap_size) - 1;
    int capacity;
    static constexpr double thresh_ratio = 1.256 / 2;  //error removal
#define BITMAP_FULL_FLAG -1

    Bitmap_Arr(uint32_t memory_);    
    uint16_t get_bitmap(uint32_t bitmap_pos);
    bool check_bitmap_full(uint16_t input_bitmap);
    bool check_flow_full(array<uint64_t,2>& hash_flowid);
    bool set_bit(uint32_t bit_pos);
    bool process_packet(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element);
    int get_spread(string flowid, array<uint64_t,2>& hash_flowid, uint32_t error_);
};

/*
Layer 2:    A sketch which consists of HyperLogLog estimators. In another word, the counter array in canonical
            sketches is replaced with HyperLogLog estimator array in our Layer2 sketch. We use Layer2 to record
            Medium-sized flows.
*/
static uint32_t TE_NUM = 0;
class HLL_Arr{
public:
    uint32_t memory; //kB
#define HASH_SEED_1 92317
#define HASH_SEED_2 37361 
    static const uint32_t register_num = 64;
    static const uint32_t register_size = 4;
    static const uint32_t HLL_size = register_num * register_size;
    uint32_t HLL_num;
    double alpha_m, alpha_m_sqm, LC_thresh; 
    vector<uint8_t> HLL_raw;
    vector<uint8_t> reg_sums;
    array<double,1<<register_size> exp_table;
    static constexpr double thresh_ratio = 2.103 / 2;

    HLL_Arr(uint32_t memory_);
    uint32_t get_counter_val(uint32_t HLL_pos,uint32_t bucket_pos);
    void set_counter_val(uint32_t HLL_pos,uint32_t bucket_pos,uint32_t val_);
    void process_packet(string flowid, array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element);
    int get_spread(string flowid, array<uint64_t,2>& hash_flowid, uint32_t error_);

    class Table_Entry{
    public:
        string flowid;
        uint8_t min_reg_sum;
        Table_Entry():flowid(""), min_reg_sum(0){}
    };
    static const uint32_t table_mem = 10; //KB
    static const uint32_t tab_size = table_mem * 1024 * 8 / (8 + 32);
    vector<Table_Entry> hash_table; 
    void insert_hashtab(string flowid, uint8_t selected_sum, uint64_t hahsres64);
};

class Global_HLLs{
public:
    static const uint32_t register_num = 1024;
    static const uint32_t register_size = 5;
    // static const uint32_t HLL_size = register_num * register_size;
    static constexpr double alpha_m = 0.7213/(1+1.079/register_num); 
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


Global_HLLs::Global_HLLs(){
    for(size_t i = 0;i < register_num;i++){
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
    
    int layer1_flows, layer2_flows, layer1_elements, layer2_elements;
    uint32_t L1_mean_error = 0, L2_mean_error = 0;
    array<uint32_t,2001> Error_RMV;

    DCSketch(uint32_t memory_size, double layer1_ratio);
    uint32_t process_element(string flowid,string element);
    uint32_t query_spread(string flowid);
    void report_superspreaders(vector<IdSpread>& superspreaders);
    void get_global_info();
    array<double,2> GetLoadFactor();
};


DCSketch::DCSketch(uint32_t memory_size, double layer1_ratio): 
layer1(memory_size * layer1_ratio), layer2(memory_size * (1 - layer1_ratio)){

    string ifile_name = "../../DCSketch/support/error_removal.txt";
    ifstream ifile_hand;
    ifile_hand = ifstream(ifile_name);
    if(!ifile_hand){
        cout<<"fail to open support files."<<endl;
        return;
    }
    while(!ifile_hand.eof()){
        uint32_t ratio;
        uint32_t error_val;
        ifile_hand >> ratio;
        ifile_hand >> error_val;
        Error_RMV[ratio] = error_val;
    }
}
#endif
