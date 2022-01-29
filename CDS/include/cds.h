#ifndef _CDS_H_
#define _CDS_H_

#include<iostream>
#include<vector>
#include<array>
#include<math.h>
#include<algorithm>
#include<set>
#include"hashfunc.h"
using namespace std;

#define HASH_SEED 92317

class CDS
{
public:
    // static const uint32_t m_1 = 16067; 
    // static const uint32_t m_2 = 16369; 
    // static const uint32_t m_3 = 16361;
    array<uint64_t,3> mi = {16067, 16369, 16361};
    array<uint32_t,2> Mi = {mi[1],mi[0]};
    uint32_t M = Mi[0] * Mi[1];
    array<uint32_t,2> Mi_inverse;

    uint32_t v;
    static const uint32_t H = 2;
    static const uint32_t H_plus = 1;
    vector<vector<uint8_t>> data;
    vector<uint32_t> cols1;  //indexes of super columns
    vector<uint32_t> cols2;
    static constexpr double phi = 0.001;
    uint32_t thresh;
    CDS(uint32_t mem); //kB
    
    static const uint64_t n = (static_cast<uint64_t>(1) << 32) - 1;
    static const uint64_t p = 4294967311;   //fixed prime larger than n.

    uint32_t hash_funs(uint32_t i, uint32_t x);
    void update(string flowid, string element);
    uint32_t get_cardinality(vector<uint8_t> vec);
    uint32_t get_cardinality(uint32_t x);
    void FindSuperCols();
    void GetInverse();

    void DetectSuperSpreaders(set<uint32_t>& superspreaders);

};




#endif