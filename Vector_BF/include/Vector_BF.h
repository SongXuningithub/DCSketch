#ifndef VECTOR_BLOOM_FILTER_H
#define VECTOR_BLOOM_FILTER_H

#include <iostream>
#include <vector>
#include <math.h>
#include <array>
#include "hashfunc.h"
using namespace std;

class BF_Table{
public:
    uint32_t rows = 0;
    vector<vector<uint8_t>> raw;
    vector<uint32_t> order_nums;
    BF_Table(){}
    void resize(uint32_t rows_,uint32_t m_);
    void append(uint32_t order_num,vector<uint8_t> rowdata);
    void update(uint32_t row,uint32_t col);
    void process_packet(string srcip,string dstip);
};


class Vector_Bloom_Filter{
public:
    uint32_t m;
    uint32_t Z;
    array<BF_Table,5> tables;
    Vector_Bloom_Filter(uint32_t m_);
#define HASH_SEED 92317
    uint32_t VBF_hash_1to5(uint32_t hash_num,array<uint8_t,4> srcip_tuple,string srcip_str);
    uint32_t VBF_hash_f(string srcip,string dstip);
    void process_packet(string srcip,array<uint8_t,4> srcip_tuple,string dstip);
    uint32_t compare_tailhead(uint32_t num1,uint32_t num2);
    uint32_t compare_tailhead_rev(uint32_t num1,uint32_t num2);
    uint32_t merge(uint32_t num1,uint32_t num2,uint32_t flag);
    uint32_t get_zero_num(vector<uint8_t> vec);
    void calc_Z(uint32_t threshold);
    void Merge_String(BF_Table& input1,BF_Table& input2,BF_Table& output);
    void Generate_IP(BF_Table& input1,BF_Table& input2,BF_Table& output);
    void Detect_Superpoint(vector<string>& superspreaders);
};

#endif