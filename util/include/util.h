#ifndef _UTILITY_H_
#define _UTILITY_H_

#include<iostream>
#include<bitset>
using namespace std;

struct IdSpread{
public:
    string flowID;
    uint32_t spread;
    IdSpread(string str,uint32_t s){flowID = str; spread = s;}
};

bool IdSpreadComp(IdSpread& a, IdSpread& b){
    return a.spread > b.spread;
}

string Uint32toIPstr(uint32_t val){
    string ret = "";
    for(size_t i = 0;i < 4;i++){
        uint8_t tmpval = (val >> (i * 8)) & 255;
        string tmpstr = to_string(tmpval);
        ret = (string(3 - tmpstr.length(), '0') + tmpstr) + ret;
    }
    return ret;
}

uint32_t IPstrtoUint32(string IPstr){
    uint32_t ret = 0;
    for(size_t i = 0;i < 4;i++){
        uint32_t tmp = stoi(IPstr.substr(i*3,3));
        ret = (ret << 8) + tmp;
    }
    return ret;
}

inline uint32_t get_one_num(uint8_t val){
    bitset<8> tmp(val);
    return tmp.count();
}

uint32_t get_leading_zeros(uint32_t bitstr){
    uint32_t tmp = 2147483648;   //1<<31
    for(size_t i = 0;i < 32;i++){
        if((bitstr & tmp) != 0)
            return i;
        tmp >>= 1;
    }
    return 32;
}

#endif