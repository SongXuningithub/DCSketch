#ifndef _DCS_H_
#define _DCS_H_

#include<iostream>
#include<array>
#include<vector>
#include<algorithm>
#include<set>
#include<unordered_map>
#include"hashfunc.h"
using namespace std;
#define HASH_SEED 92317
#define SOURCE_FLOW 1
#define DEST_FLOW 2
class DCS_Synopsis{
public:
    static const uint32_t mem_size = 1000;  //kB
    static const uint32_t bucket_num = 23;
    static const uint32_t log_m = 32;
    static const uint32_t r = 3;
    static constexpr double epsi = 0.3;
    static const uint32_t s = mem_size * 1024 * 8.0 / bucket_num / r / (32 * (2*log_m + 1) );
    array<uint32_t,32> patterns;
    array<array<uint32_t,2 * log_m + 1>, bucket_num * r * s> counter_arrays;
    DCS_Synopsis();
    void update(string u, string v);
    vector<pair<uint32_t, uint32_t>> BaseTopk(uint32_t k);
    void GetdSample(uint32_t b, set<pair<uint32_t,uint32_t>>& ds);
    void ReturnSingleton(uint32_t b, uint32_t j, uint32_t k, pair<uint32_t,uint32_t>& uv_pair);
};




#endif