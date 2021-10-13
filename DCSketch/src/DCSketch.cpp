#include <iostream>
#include "DCSketch.h"
#include <malloc.h>
#include <bitset>
#include <vector>
#include <algorithm>

namespace metadata{
    uint32_t bits_bias;
    uint32_t uint32_pos;
    uint32_t inner_bias;
    int shift_;
};

Bitmap_Arr::Bitmap_Arr()
{
    using namespace L1_Param;
    cout<<"Layer1: Bitmap array initializing:"<<endl;

    for(uint8_t i = 0;i < bitmap_size;i++)
    {
        patterns[i] = 1 << (bitmap_size - i - 1);
    }
    double ln_bmsize = log(bitmap_size);
    double ln_bmsize_minu1 = log(bitmap_size - 1);
    for(uint8_t i = 1;i <= bitmap_size;i++)
    {
        spreads[i] = ( ln_bmsize - log(i) ) / (ln_bmsize - ln_bmsize_minu1);
    }
    cout<<"Layer1 total memory cost(bit): "<<8*sizeof(raw)<<" bits"<<endl;
    cout<<"mempry cost(bit) per bitmap: "<<bitmap_size<<endl;
    cout<<"number of bitmaps in layer 1: "<<bitmap_num<<endl;
    cout<<"Layer1: Bitmap array initialized."<<endl;
    cout<<endl;
}

uint8_t Bitmap_Arr::get_bitmap(uint32_t bitmap_pos)
{
    using namespace metadata;
#define FULL_PAT static_cast<uint8_t>(255>>2)

    bits_bias = bitmap_pos * L1_Param::bitmap_size;
    uint32_pos =  bits_bias/ 32;
    inner_bias = bits_bias % 32;
    shift_ = 31 - (inner_bias + L1_Param::bitmap_size - 1);
    uint8_t res;
    if(shift_ >= 0)
    {
        res = static_cast<uint8_t>(raw[uint32_pos] >> shift_);
    }
    else
    {
        res = static_cast<uint8_t>( (raw[uint32_pos] << (-shift_)) + (raw[uint32_pos + 1] >> (32 + shift_)) );
    }
    res &= FULL_PAT;
    return res;
}

bool Bitmap_Arr::check_bitmap_full(uint8_t input_bitmap)
{
    for(uint8_t i=0;i<L1_Param::bitmap_size;i++)
    {
        if( (input_bitmap | (1 << i)) == FULL_PAT)
            return true;
    }
    return false;
}

bool Bitmap_Arr::add_element(uint32_t bit_pos)
{
    using namespace metadata;
    uint32_t temp = inner_bias + bit_pos;
    if(temp <= 31)
    {
        raw[uint32_pos] |= 1<<(31-temp); 
    }
    else
    {
        temp -= 32;
        raw[uint32_pos + 1] |= 1<<(31-temp); 
    }
    return false;
}

bool Bitmap_Arr::process_element(string flowid,string element)
{
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_LAYER1);
    array<uint32_t,L1_Param::hash_num> L1_pos;
    for(uint8_t i = 0;i < L1_Param::hash_num;i++)
    {
        uint32_t cur_pos;
        if(i%2==0)
        {
            cur_pos = static_cast<uint32_t>(hashres128[i/2]>>32);
        }
        else
        {
            cur_pos = static_cast<uint32_t>(hashres128[i/2]);
        }
        cur_pos %= L1_Param::bitmap_num;
        L1_pos[i] = cur_pos;
    }
    uint32_t hashres32 = str_hash32(element,HASH_SEED_LAYER1);
    uint32_t update_pos = L1_pos[(hashres32>>16) % L1_Param::hash_num];
    uint8_t bitmap_state = get_bitmap(update_pos);
#ifdef DEBUG_LAYER1
    cout<<"before changed: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
    bool all_full = true;
    if(!check_bitmap_full(bitmap_state))
    {
        uint32_t update_bit = static_cast<uint16_t>(hashres32)%L1_Param::bitmap_size;
        add_element(update_bit);
#ifdef DEBUG_LAYER1
        bitmap_state = get_bitmap(update_pos);
        cout<<"change bit: "<<update_bit<<endl;
        cout<<"after changed: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
        all_full = false;
    }
    else
    {
#ifdef DEBUG_LAYER12
        cout<<"ONE bitmap_state: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
        for(uint8_t i = 0;i < L1_Param::hash_num;i++)
        {
            if(L1_pos[i] == update_pos)
                continue;
            bitmap_state = get_bitmap(L1_pos[i]);

#ifdef DEBUG_LAYER12
            cout<<"ANOTHER bitmap_state: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
            if(!check_bitmap_full(bitmap_state))
            {
                all_full = false;
                break;
            }
        }
    }
    return all_full;
}

uint32_t Bitmap_Arr::get_spread(string flowid)
{
    using namespace L1_Param;
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_LAYER1);
    array<uint32_t,hash_num> L1_pos;
    for(uint8_t i = 0;i < L1_pos.size();i++)
    {
        uint32_t cur_pos;
        if(i%2==0)
        {
            cur_pos = static_cast<uint32_t>(hashres128[i/2]>>32);
        }
        else
        {
            cur_pos = static_cast<uint32_t>(hashres128[i/2]);
        }
        cur_pos %= bitmap_num;
        L1_pos[i] = cur_pos;
    }
    double flow_spread = 0;
    bool all_full = true;
    for(uint8_t i = 0;i < L1_pos.size();i++)
    {
        uint8_t bitmap_state = get_bitmap(L1_pos[i]);
        uint8_t zeros_num = 0;
        for(uint8_t bit_pos = 0;bit_pos < bitmap_size;bit_pos++)
        {
            if( (bitmap_state & patterns[bit_pos]) == 0)
                zeros_num++;
        }
        flow_spread += spreads[zeros_num];
        if(zeros_num > 1)
            all_full = false;
    }
    if(all_full)
        return BITMAP_FULL_FLAG;
    return static_cast<uint8_t>(flow_spread);
}

template<class T>
uint32_t TYPE_MAX()
{
    if (typeid(T) == typeid(uint8_t))
        return MAX_UINT8;
    if (typeid(T) == typeid(uint16_t))
        return MAX_UINT16;
    if (typeid(T) == typeid(uint32_t))
        return MAX_UINT32;
    return 0;
}

uint32_t HLL_Arr::get_leading_zeros(uint32_t bitstr)
{
    for(size_t i = 1;i <= 32;i++)
    {
        if( ((bitstr<<i)>>i) != bitstr )
            return i - 1;
    }
    return 32;
}

uint32_t HLL_Arr::get_counter_val(uint32_t HLL_pos,uint32_t bucket_pos)
{
    uint32_t uint8_pos = HLL_pos * (register_num >> 1) + bucket_pos / 2;
    if(bucket_pos % 2 == 0)
        return HLL_raw[uint8_pos] >> 4;       //high 4 bits
    else
        return HLL_raw[uint8_pos] & 15;       //low 4 bits
}

void HLL_Arr::set_counter_val(uint32_t HLL_pos,uint32_t bucket_pos,uint32_t val_)
{
    uint32_t uint8_pos = HLL_pos * (register_num >> 1) + bucket_pos / 2;
    if(bucket_pos % 2 == 0)
    {
        HLL_raw[uint8_pos] &= 15;            //keep the low 4 bits unchanged 
        HLL_raw[uint8_pos] |= static_cast<uint8_t>(val_) << 4;      //set the high 4 bits
    }
    else
    {
        HLL_raw[uint8_pos] &= 240;            //keep the high 4 bits unchanged 
        HLL_raw[uint8_pos] |= static_cast<uint8_t>(val_);
    }
}

void HLL_Arr::process_packet(string flowid,string elementid)
{
    uint32_t hashres32 = str_hash32(elementid,HASH_SEED_2);
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_1);
    uint32_t HLL_pos;
    if((hashres32 & register_num) == 0)    //use the 6th bit to determine which bucket to update
        HLL_pos = (hashres128[1] >> 32) % HLL_num;
    else
        HLL_pos = static_cast<uint32_t>(hashres128[1]) % HLL_num;
    //uint32_t HLL_pos = hashres32 % HLL_num;
    uint32_t bucket_pos = hashres32 & (register_num - 1); //use the last 4 bits to locate the bucket to update
    uint32_t rou_x = get_leading_zeros(hashres32) + 1;
    rou_x = rou_x <= 15 ? rou_x : 15;
    uint32_t bucket_val = get_counter_val(HLL_pos,bucket_pos);
    if(bucket_val < rou_x)
    {
        set_counter_val(HLL_pos,bucket_pos,rou_x);
    }
}

uint32_t HLL_Arr::get_spread(uint32_t pos)
{
    double sum_ = 0;
    uint32_t V_ = 0;
    for(size_t i = 0;i < register_num;i++)
    {
        uint32_t tmpval = get_counter_val(pos,i);
        sum_ += 1.0 / (1 << tmpval);
        if(tmpval == 0)
            V_++;
    }
    double ret;
    ret = alpha_m_sqm / sum_;
    if(ret <= LC_thresh)
    {
        if(V_ > 0)
            ret = register_num * log(register_num / (double)V_);
    }
    return static_cast<uint32_t>(ret);
}

uint32_t HLL_Arr::get_spread(string flowid)
{
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_1);
    array<uint32_t,2> HLL_pos;
    HLL_pos[0] = (hashres128[1] >> 32) % HLL_num;
    HLL_pos[1] = static_cast<uint32_t>(hashres128[1]) % HLL_num;
    array<double,2> res;
    for(size_t p = 0; p < 2;p++)
    {
        double sum_ = 0;
        uint32_t V_ = 0;
        for(size_t i = 0;i < register_num;i++)
        {
            uint32_t tmpval = get_counter_val(HLL_pos[p],i);
            sum_ += 1.0 / (1 << tmpval);
            if(tmpval == 0)
                V_++;
        }
        res[p] = alpha_m_sqm / sum_;
        if(res[p] <= LC_thresh)
        {
            if(V_ > 0)
                res[p] = register_num * log(register_num / (double)V_);
        }
    }
    double minres = res[0] < res[1] ? res[0] : res[1];
    return static_cast<uint32_t>(minres * 2);
}


uint8_t HLL::get_leading_zeros(uint32_t bitstr)
{
    for(size_t i = 1;i <= 32;i++)
    {
        if( ((bitstr<<i)>>i) != bitstr )
            return i - 1;
    }
    return 32;
}

void HLL::process_flow(string flowid)
{
    uint32_t hashres_32 = str_hash32(flowid,HASH_SEED_1);
    uint8_t lz_num = get_leading_zeros(hashres_32) + 1;
    hashres_32 = str_hash32(flowid,HASH_SEED_2);
    uint32_t reg_pos = hashres_32 % register_num;
    HLL_registers[reg_pos] = max(lz_num , HLL_registers[reg_pos]);
}

uint32_t HLL::get_cardinality()
{
    double inv_sum = 0;
    for(size_t i = 0;i < HLL_registers.size();i++)
    {
        inv_sum += pow(2,0-HLL_registers[i]);
    }
    double E = alpha_m * 128 * 128 / inv_sum;
    if(E <= 2.5 * 128)
    {
        uint32_t zeros_num = 0;
        for(size_t i = 0;i < HLL_registers.size();i++)
        {
            if(HLL_registers[i] == 0)
                zeros_num++;
        }
        E = 128 * log((double)128/zeros_num);
    }
    return E;
}

void DCSketch::update_L1_card(string flowid,string element)
{
    L1_ELEM_CARD.process_flow(flowid + element);
}

void DCSketch::update_L2_card(string flowid,string element)
{
    L2_FLOW_CARD.process_flow(flowid);
    L2_ELEM_CARD.process_flow(flowid + element);
}

void DCSketch::process_element(string flowid,string element)
{
    bool layer1_full = layer1.process_element(flowid,element);
    if(!layer1_full)
    {
        update_L1_card(flowid,element);
        return;
    }
    layer2.process_packet(flowid,element);
    update_L2_card(flowid,element);
}

void DCSketch::update_mean_error()
{
    // L1_mean_error = L1_ELEM_CARD.get_cardinality() / L1_Param::bitmap_num;
    // if(L1_mean_error > L1_Param::max_spread)
    //     L1_mean_error = (int)L1_Param::max_spread;
    // //L2_mean_error = L2_ELEM_CARD.get_cardinality() / L2_FLOW_CARD.get_cardinality() * (L2_FLOW_CARD.get_cardinality() - 1) / layer2.bjkst_arr_size;
    uint32_t l2_ele_card = L2_ELEM_CARD.get_cardinality();
    uint32_t l2_flow_card = L2_FLOW_CARD.get_cardinality();
    cout<<"l2_flow_card: "<<l2_flow_card<<endl;
    cout<<"l2_ele_card: "<<l2_ele_card<<endl;
    uint32_t l1_card = L1_ELEM_CARD.get_cardinality();
    L1_mean_error = l1_card / L1_Param::bitmap_num;
    if(L1_mean_error > L1_Param::max_spread)
        L1_mean_error = (int)L1_Param::max_spread;
    L2_mean_error = 833;//426;
    //L2_mean_error = (L2_ELEM_CARD.get_cardinality() - L1_CARD.get_cardinality()) * (L2_FLOW_CARD.get_cardinality() - 1) / L2_FLOW_CARD.get_cardinality() + L1_mean_error;
    return;
}

uint32_t DCSketch::query_spread(string flowid)
{
    uint32_t spread_layer1 = layer1.get_spread(flowid);
    if(spread_layer1 != BITMAP_FULL_FLAG)
    {
        return spread_layer1;
    }
    int spread_layer2 = layer2.get_spread(flowid);
    spread_layer2 -= (int)L2_mean_error * 2;
    int ret = spread_layer2 + 19 - L1_mean_error * 2;
    if(ret <= 0)
        ret = 1;
    return ret;
}