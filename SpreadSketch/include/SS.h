#ifndef SPREAD_SKETCH_H
#define SPREAD_SKETCH_H

#include <iostream>
#include <math.h>
#include <vector>
using namespace std;

uint32_t get_leading_zeros(uint32_t bitstr);

class MultiResBitmap{
public:
    static constexpr double sigma = 0.1;
    static const uint32_t b = 0.6367 / (sigma * sigma);
    static const uint32_t b_hat = 2 * b;
    static const uint32_t C = 120000;
    static const uint32_t c = 2 + 10;   //log2(C / (2.6744 * b)); == 9.47
    static constexpr double setmax_ratio = 0.7981;
    vector<vector<uint8_t>> V;
    MultiResBitmap();    
    uint32_t get_ones_num(uint32_t layer);
    void update(uint32_t hashval);
    uint32_t get_cardinality();
};

class SS_Bucket{
public:
    uint32_t K;
    uint32_t L;
    MultiResBitmap mrbitmap;
};

class SpreadSketch{
public:
    static const uint32_t r = 2;
    static const uint32_t w = 10;
    vector<vector<SS_Bucket>> bkt_table;
    SpreadSketch();
};

SpreadSketch::SpreadSketch()
{
    bkt_table.resize(r);
    for(size_t i = 0;i < r;i++)
    {
        bkt_table[i].resize(w);
    }
}

#endif