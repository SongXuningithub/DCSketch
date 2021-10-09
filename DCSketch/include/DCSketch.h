#ifndef _DCSKETCH_H_
#define _DCSKETCH_H_

#include "hashfunc.h"
#include<iostream>
#include<cmath>
#include<string>
#include<array>
#include<memory>
#include<vector>
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

namespace L1_Param{
    const uint32_t memory = 400;      //kB
    const uint32_t bitmap_size = 6;      //bits
    const uint32_t bitmap_num = memory * 1024 * 8 / bitmap_size;
    const uint32_t hash_num = 2;
    static constexpr double max_spread = 9.8275; //L1_Param::bitmap_size * log(L1_Param::bitmap_size);
};

class Bitmap_Arr{
private:
    uint32_t raw[L1_Param::memory*1024*8/32];
    array<uint8_t,L1_Param::bitmap_size> patterns;
    array<double,L1_Param::bitmap_size + 1> spreads;
public:
#define BITMAP_FULL_FLAG 435
    Bitmap_Arr();
    uint8_t get_bitmap(uint32_t bitmap_pos);
    bool check_bitmap_full(uint8_t input_bitmap);
    bool add_element(uint32_t bit_pos);
    bool process_element(string flowid,string element);
    uint32_t get_spread(string flowid);
};

/*
Layer 2:    A sketch which consists of BJKST estimators. In another word, the counter array in canonical
            sketches is replaced with BJKST estimator array in our Layer2 sketch. We use Layer2 to record
            Medium-sized flows.
*/

class BJKST_Arr{
public:
    static const uint32_t memory = 300;      //kB
    static const uint32_t k = 15;      
    static const uint32_t integer_size = 10;      //bits
    static const uint32_t lowpart_size = 8;
    static const uint32_t highpart_size = integer_size - lowpart_size;
    const uint8_t highpart_pat = (1 << highpart_size) - 1;
    static const uint32_t lowbits_arr_size = memory * 1024 * 8 / integer_size;
    static const uint32_t highbits_arr_size = lowbits_arr_size/(lowpart_size/highpart_size) + 1;
    const uint32_t bjkst_arr_size = lowbits_arr_size/k;
    static const uint32_t hash_num = 2;
    double init_spread = L1_Param::hash_num * L1_Param::max_spread;
    double counter_max_dec = k/init_spread;
    const uint32_t counter_max_int = (1<<integer_size) - 1;
    array<uint8_t,lowbits_arr_size> lowbits_raw{};
    array<uint8_t,highbits_arr_size> highbits_raw{};
    uint8_t ts[highbits_arr_size];

    BJKST_Arr();
    uint32_t get_counter(uint32_t estimator_pos,uint32_t counter_pos);
    double int2dec(uint32_t cnt_val);
    uint32_t dec2int(double dec_val);
    uint32_t get_flow_spread(string flowid,double& flow_max_dec);
    void get_flow_estimators(string flowid,array<double,BJKST_Arr::k>& estimators);
    void set_counter_val(uint32_t estimator_pos,uint32_t counter_pos,uint32_t val);
    bool process_element(string flowid,string element);
};

/*
Layer 3:    A hash table which store <key,value> pairs where the key is the flow ID and the value is 
            its BJKST array. Each unit(or integer) of the array has 16 bits, which is more than that(
            10 bits) in Layer2. So the resolution of Layer3 is higher and valid estimation range is 
            larger.
*/

class SS_Table  //Super-Spreader Table
{
public:
    class BJKST_Estimator{
    public:
        static const uint32_t k = BJKST_Arr::k;
        string flowid = "";
        array<uint16_t,k> minimumk{};
        void update(uint32_t intval);
        void update(double decval);
        static uint32_t dec2int(double decval){
            uint32_t intval = MAX_UINT16 * decval / max_dec_val;
            return intval;
        }
        static double int2dec(uint32_t intval){
            double decval = max_dec_val * intval / MAX_UINT16;
            return decval;
        }
        uint32_t get_spread(){
            uint32_t ret_spread = k/int2dec(minimumk[k-1]);
            return ret_spread;
        }
    };
    static const uint32_t subtab_num = 3;
    static const uint32_t memory = subtab_num * 10;  //kB
    static const uint32_t estimator_size = 16 * BJKST_Estimator::k + 32;
    static const uint32_t threshold = 300;
    static constexpr double max_dec_val = (double)BJKST_Estimator::k/threshold;
    static const uint32_t table_size = memory * 1024 * 8 / estimator_size / 3 * 3;
    static const uint32_t subtab_size = table_size / 3;
    static const array<uint32_t,subtab_num> offsets;
    array<BJKST_Estimator,table_size> ss_table;
    uint32_t get_flow_spread(string flowid);
    int process_flow(string flowid,string element,double max_dec);
    bool insert_flow(string flowid,uint32_t insert_pos,array<double,BJKST_Estimator::k> decvals);
    void report_superspreaders();
};
const array<uint32_t,SS_Table::subtab_num> SS_Table::offsets={0, SS_Table::subtab_size, SS_Table::subtab_size*2};
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
    Bitmap_Arr layer1;
    BJKST_Arr layer2;
    SS_Table layer3;
    HLL L1_ELEM_CARD;
    HLL L2_FLOW_CARD;
    HLL L2_ELEM_CARD;
    uint32_t L1_mean_error;
    uint32_t L2_mean_error;
    void update_L1_card(string flowid,string element);
    void update_L2_card(string flowid,string element);
    void process_element(string flowid,string element);
    uint32_t query_spread(string flowid);
    void update_mean_error();
};

#endif