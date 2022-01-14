#include <iostream>
#include "DCSketch.h"
#include <malloc.h>
#include <bitset>
#include <vector>
#include <algorithm>
#include <fstream>

namespace metadata{
    uint32_t bits_bias;
    uint32_t uint32_pos;
    uint32_t inner_bias;
    int shift_;
};

Bitmap_Arr::Bitmap_Arr()
{
    cout<<"Layer1: Bitmap array initializing:"<<endl;
    for(size_t i = 0;i < raw.size();i++)
    {
        raw[i] = 0;
    }
    for(size_t i = 0;i < bitmap_size;i++)
    {
        patterns[i] = 1 << (bitmap_size - i - 1);
    }
    double ln_bmsize = log(bitmap_size);
    double ln_bmsize_minu1 = log(bitmap_size - 1);
    spreads[0] = ( ln_bmsize - log(1) ) / (ln_bmsize - ln_bmsize_minu1);
    for(size_t i = 1;i <= bitmap_size;i++)
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
    bits_bias = bitmap_pos * bitmap_size;
    uint32_pos =  bits_bias/ 32;
    inner_bias = bits_bias % 32;
    shift_ = 31 - (inner_bias + bitmap_size - 1);
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

uint32_t zero_pos;
uint32_t tmp_arr[6] = {1<<5,1<<4,1<<3,1<<2,1<<1,1};
int Bitmap_Arr::check_bitmap_full(uint8_t input_bitmap)
{
    if( input_bitmap == FULL_PAT )
    {
        //cout<<"FAST return."<<endl;
        return 6;
    }
    bool has_zero = false;
    for(size_t i=0;i<bitmap_size;i++)
    {
        if( (tmp_arr[i] & input_bitmap) == 0)
        {
            if(has_zero)
                return -1;
            else
                has_zero = true;
            zero_pos = i;
        }   
    }
    return zero_pos;
}

// uint32_t tmp_arr[6] = {1<<5,1<<4,1<<3,1<<2,1<<1,1};
// int Bitmap_Arr::check_bitmap_full(uint8_t input_bitmap)
// {
//     for(size_t i=0;i<bitmap_size;i++)
//     {
//         if( (input_bitmap | tmp_arr[i] ) == FULL_PAT)
//             return true;
//     }
//     return false;
// }

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

bool Bitmap_Arr::process_packet(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element)
{
    array<uint32_t,hash_num> L1_pos;
    L1_pos[0] = static_cast<uint32_t>(hash_flowid[0]>>32) % bitmap_num;
    L1_pos[1] = static_cast<uint32_t>(hash_flowid[0]) % bitmap_num;
    uint32_t hashres32 = static_cast<uint32_t>(hash_element[0] >> 32);
    uint32_t update_pos = L1_pos[(hashres32>>16) % hash_num];
    uint8_t bitmap_state = get_bitmap(update_pos);
// #ifdef DEBUG_LAYER1
//     cout<<"before changed: "<<std::bitset<6>(bitmap_state)<<endl;
// #endif
    bool all_full = true;
    int tmp_status = check_bitmap_full(bitmap_state);
    if(tmp_status == -1)
    {
        uint32_t update_bit = static_cast<uint16_t>(hashres32) % bitmap_size;
        add_element(update_bit);
// #ifdef DEBUG_LAYER1
//         bitmap_state = get_bitmap(update_pos);
//         cout<<"change bit: "<<update_bit<<endl;
//         cout<<"after changed: "<<std::bitset<6>(bitmap_state)<<endl;
// #endif
        all_full = false;
    }
    else
    {
#ifdef DEBUG_LAYER12
        cout<<"ONE bitmap_state: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
        if(tmp_status < 6)
        {
            //cout<<"ONE bitmap_state before_change: "<<std::bitset<6>(bitmap_state)<<endl;
            add_element(tmp_status);
            // uint8_t bitmap_state = get_bitmap(update_pos);
            // cout<<"ONE bitmap_state after_change: "<<std::bitset<6>(bitmap_state)<<endl;
        }
        for(size_t i = 0;i < hash_num;i++)
        {
            if(L1_pos[i] == update_pos)
                continue;
            bitmap_state = get_bitmap(L1_pos[i]);
#ifdef DEBUG_LAYER12
            cout<<"ANOTHER bitmap_state: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
            tmp_status = check_bitmap_full(bitmap_state);
            if(tmp_status == -1)
            {
                all_full = false;
                break;
            }
            if(tmp_status < 6)
            {
                // cout<<"ONE bitmap_state before_change: "<<std::bitset<6>(bitmap_state)<<endl;
                add_element(tmp_status);
                // uint8_t bitmap_state = get_bitmap(update_pos);
                // cout<<"ONE bitmap_state after_change: "<<std::bitset<6>(bitmap_state)<<endl;
                //add_element(tmp_status);
            }
        }
    }
    return all_full;
}

double Bitmap_Arr::get_spread(string flowid, array<uint64_t,2>& hash_flowid)
{
    array<uint32_t,hash_num> L1_pos;
    L1_pos[0] = static_cast<uint32_t>(hash_flowid[0]>>32) % bitmap_num;
    L1_pos[1] = static_cast<uint32_t>(hash_flowid[0]) % bitmap_num;

    double flow_spread = 30;
    bool all_full = true;
    for(size_t i = 0;i < L1_pos.size();i++)
    {
        uint8_t bitmap_state = get_bitmap(L1_pos[i]);
        uint8_t zeros_num = 0;
        for(size_t bit_pos = 0;bit_pos < bitmap_size;bit_pos++)
        {
            if( (bitmap_state & patterns[bit_pos]) == 0)
                zeros_num++;
        }
        flow_spread = min(spreads[zeros_num] , flow_spread);
        if(zeros_num > 1)
            all_full = false;
    }
    if(all_full)
        return BITMAP_FULL_FLAG;
    return flow_spread;
}

array<uint32_t,2> Bitmap_Arr::get2bitmap_zeronum(array<uint64_t,2>& hash_flowid)
{
    array<uint32_t,hash_num> L1_pos;
    L1_pos[0] = static_cast<uint32_t>(hash_flowid[0]>>32) % bitmap_num;
    L1_pos[1] = static_cast<uint32_t>(hash_flowid[0]) % bitmap_num;
    array<uint32_t,2> ret;
    for(size_t i = 0;i < 2;i++)
    {
        uint8_t bitmap_state = get_bitmap(L1_pos[i]);
        uint8_t zeros_num = 0;
        for(size_t bit_pos = 0;bit_pos < bitmap_size;bit_pos++)
        {
            if( (bitmap_state & patterns[bit_pos]) == 0)
                zeros_num++;
        }
        ret[i] = zeros_num;
    }
    return ret;
}

uint32_t Bitmap_Arr::get_spread(uint32_t pos)
{
    uint8_t bitmap_state = get_bitmap(pos);
    uint8_t zeros_num = 0;
    for(size_t bit_pos = 0;bit_pos < bitmap_size;bit_pos++)
    {
        if( (bitmap_state & patterns[bit_pos]) == 0)
            zeros_num++;
    }
    double flow_spread = spreads[zeros_num];
    return static_cast<uint32_t>(flow_spread);
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

uint32_t get_leading_zeros(uint32_t bitstr)
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

void HLL_Arr::process_packet(string flowid, array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element)
{
    //operation on hyperloglog sketch
    uint32_t hashres32 = static_cast<uint32_t>(hash_element[0]);
    array<uint32_t,2> HLL_pos;
    HLL_pos[0] = static_cast<uint32_t>(hash_flowid[1] >> 32) % HLL_num;
    HLL_pos[1] = static_cast<uint32_t>(hash_flowid[1]) % HLL_num;
    uint32_t bucket_pos = hashres32 & (register_num - 1); //use the last 4 bits to locate the bucket to update
    uint32_t rou_x = get_leading_zeros(hashres32) + 1;
    uint32_t update_pos;
    if(hash_element[0]>>63)
        update_pos = HLL_pos[0];
    else
        update_pos = HLL_pos[1];
    rou_x = rou_x <= 15 ? rou_x : 15;
    uint32_t bucket_val = get_counter_val(update_pos,bucket_pos);
    if(bucket_val < rou_x)
    {
        set_counter_val(update_pos,bucket_pos,rou_x);
        // if(heavypart_active == false)
        //     return;
        if (bucket_pos < Table_Entry::selected_num) 
        {
            //operation on hash table
            uint8_t selected_sum1 = get_counter_val(HLL_pos[0],0)+get_counter_val(HLL_pos[0],1)+get_counter_val(HLL_pos[0],2)+get_counter_val(HLL_pos[0],3);
            uint8_t selected_sum2 = get_counter_val(HLL_pos[1],0)+get_counter_val(HLL_pos[1],1)+get_counter_val(HLL_pos[1],2)+get_counter_val(HLL_pos[1],3);
            uint8_t selected_sum = min(selected_sum1,selected_sum2);
            insert_hashtab(flowid,selected_sum,hash_flowid[0]);
        }
    }
}

uint32_t HLL_Arr::get_spread(string flowid, array<uint64_t,2>& hash_flowid)
{
    array<uint32_t,2> HLL_pos;
    HLL_pos[0] = static_cast<uint32_t>(hash_flowid[1] >> 32) % HLL_num;
    HLL_pos[1] = static_cast<uint32_t>(hash_flowid[1]) % HLL_num;

    double res_1;
    double sum_ = 0;
    uint32_t V_ = 0;
    for(size_t i = 0;i < register_num;i++)
    {
        uint32_t tmpval = get_counter_val(HLL_pos[0],i);
        sum_ += exp_table[tmpval];
        if(tmpval == 0)
            V_++;
    }
    res_1 = alpha_m_sqm / sum_;
    if(res_1 <= LC_thresh)
    {
        if(V_ > 0)
            res_1 = register_num * log(register_num / (double)V_);
    }

    double res_2;
    sum_ = 0;
    V_ = 0;
    for(size_t i = 0;i < register_num;i++)
    {
        uint32_t tmpval = get_counter_val(HLL_pos[1],i);
        sum_ += exp_table[tmpval];
        if(tmpval == 0)
            V_++;
    }
    res_2 = alpha_m_sqm / sum_;
    if(res_2 <= LC_thresh)
    {
        if(V_ > 0)
            res_2 = register_num * log(register_num / (double)V_);
    }
    return static_cast<uint32_t>(min(res_1,res_2));
}

uint32_t HLL_Arr::get_spread(uint32_t pos)
{
    double sum_ = 0;
    uint32_t V_ = 0;
    for(size_t i = 0;i < register_num;i++)
    {
        uint32_t tmpval = get_counter_val(pos,i);
        //sum_ += 1.0 / (1 << tmpval);
        sum_ += exp_table[tmpval];
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

array<uint32_t,2> HLL_Arr::get2hll_vals(array<uint64_t,2>& hash_flowid)
{
    array<uint32_t,2> HLL_pos;
    HLL_pos[0] = static_cast<uint32_t>(hash_flowid[1] >> 32) % HLL_num;
    HLL_pos[1] = static_cast<uint32_t>(hash_flowid[1]) % HLL_num;

    double res_1;
    double sum_ = 0;
    uint32_t V_ = 0;
    for(size_t i = 0;i < register_num;i++)
    {
        uint32_t tmpval = get_counter_val(HLL_pos[0],i);
        sum_ += exp_table[tmpval];
        if(tmpval == 0)
            V_++;
    }
    res_1 = alpha_m_sqm / sum_;
    if(res_1 <= LC_thresh)
    {
        if(V_ > 0)
            res_1 = register_num * log(register_num / (double)V_);
    }

    double res_2;
    sum_ = 0;
    V_ = 0;
    for(size_t i = 0;i < register_num;i++)
    {
        uint32_t tmpval = get_counter_val(HLL_pos[1],i);
        sum_ += exp_table[tmpval];
        if(tmpval == 0)
            V_++;
    }
    res_2 = alpha_m_sqm / sum_;
    if(res_2 <= LC_thresh)
    {
        if(V_ > 0)
            res_2 = register_num * log(register_num / (double)V_);
    }

    array<uint32_t,2> ret;
    ret[0] = static_cast<uint32_t>(res_1);
    ret[1] = static_cast<uint32_t>(res_2);
    return ret;
}

void HLL_Arr::insert_hashtab(string flowid, uint8_t selected_sum, uint64_t hahsres64)
{
    uint32_t hashres32 = hahsres64 >> 32;         //high 32 bits of initial hash result which is 64 bits
    uint32_t table_pos1 = (hashres32 >> 16) % tab_size;     //high 16 bits
    uint32_t table_pos2 = (hashres32 & MAX_UINT16) % tab_size;  //low 16 bits


    if(hash_table[table_pos1].flowid == "" || hash_table[table_pos1].flowid == flowid)
    {
        hash_table[table_pos1].flowid = flowid;
        hash_table[table_pos1].selected_sum = selected_sum;
        return;
    }
    else if(hash_table[table_pos2].flowid == "" || hash_table[table_pos2].flowid == flowid)
    {
        hash_table[table_pos2].flowid = flowid;
        hash_table[table_pos2].selected_sum = selected_sum;
        return;
    }

    uint8_t tmp1 = hash_table[table_pos1].selected_sum;
    uint8_t tmp2 = hash_table[table_pos2].selected_sum; 
    if(tmp1 > tmp2)
    {
        if(selected_sum >= tmp2)
        {
            hash_table[table_pos2].flowid = flowid;
            hash_table[table_pos2].selected_sum = selected_sum;
        }
    }
    else
    {
        if(selected_sum >= tmp1)
        {
            hash_table[table_pos1].flowid = flowid;
            hash_table[table_pos1].selected_sum = selected_sum;
        }
    }
}

void DCSketch::report_superspreaders(vector<IdSpread>& superspreaders)
{
    superspreaders.clear();
    set<string> checked_flows;
    for(size_t i = 0;i < layer2.tab_size;i++)
    {
        string tmp_flowid = layer2.hash_table[i].flowid;
        if(checked_flows.find(tmp_flowid) != checked_flows.end())
        {
            continue;
        } 
        else
        {
            checked_flows.insert(tmp_flowid);
            //array<uint64_t,2> hash_flowid = str_hash128(tmp_flowid,HASH_SEED_1);
            uint32_t esti_card = query_spread(tmp_flowid); 
            superspreaders.push_back( IdSpread(tmp_flowid,esti_card) );
        }
    }
    sort(superspreaders.begin(), superspreaders.end(), IdSpreadComp);
}

void Global_HLLs::update_layer1(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element)
{
    //mumber of distinct flows
    uint32_t hashres_32 = static_cast<uint32_t>(hash_flowid[0]);
    uint8_t lz_num = get_leading_zeros(hashres_32);
    uint32_t reg_pos = hashres_32 % register_num;
    Layer1_flows[reg_pos] = max( static_cast<uint8_t>(lz_num + 1), Layer1_flows[reg_pos]);
    //number of distinct <flow,element> pairs
    hashres_32 = static_cast<uint32_t>(hash_element[0]);
    lz_num = get_leading_zeros(hashres_32);
    reg_pos = hashres_32 % register_num;
    Layer1_elements[reg_pos] = max(static_cast<uint8_t>(lz_num + 1) , Layer1_elements[reg_pos]);
}

void Global_HLLs::update_layer2(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element)
{
    //mumber of distinct flows
    uint32_t hashres_32 = static_cast<uint32_t>(hash_flowid[0]);
    uint8_t lz_num = get_leading_zeros(hashres_32);
    uint32_t reg_pos = hashres_32 % register_num;
    Layer2_flows[reg_pos] = max(static_cast<uint8_t>(lz_num + 1) , Layer2_flows[reg_pos]);
    //number of distinct <flow,element> pairs
    hashres_32 = static_cast<uint32_t>(hash_element[0]);
    lz_num = get_leading_zeros(hashres_32);
    reg_pos = hashres_32 % register_num;
    Layer2_elements[reg_pos] = max(static_cast<uint8_t>(lz_num + 1) , Layer2_elements[reg_pos]);
}

uint32_t Global_HLLs::get_cardinality(array<uint8_t,register_num>& HLL_registers)
{
    double inv_sum = 0;
    for(size_t i = 0;i < HLL_registers.size();i++)
    {
        inv_sum += pow(2,0-(int)HLL_registers[i]);
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

array<uint8_t,Global_HLLs::register_num> Global_HLLs::HLL_union(array<uint8_t,register_num>& HLL_registers1,array<uint8_t,register_num>& HLL_registers2)
{
    array<uint8_t,register_num> vsketch;
    for(size_t i = 0;i < HLL_registers1.size();i++)
    {
        vsketch[i] = max(HLL_registers1[i] , HLL_registers2[i]);
    }
    return vsketch;
}

uint32_t Global_HLLs::get_number_flows(uint32_t layer)
{
    if(layer == LAYER1)
    {
        return get_cardinality(Layer1_flows);
    }
    else
    {
        return get_cardinality(Layer2_flows);
    }
}

uint32_t Global_HLLs::get_number_elements(uint32_t layer)
{
    if(layer == LAYER1)
    {
        return get_cardinality(Layer1_elements);
    }
    else
    {
        return get_cardinality(Layer2_elements);
    }
}

void DCSketch::process_element(string flowid,string element)
{
    array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    array<uint64_t,2> hash_element = str_hash128(flowid + element,HASH_SEED_2);
    bool layer1_full = layer1.process_packet(hash_flowid,hash_element);
    if(!layer1_full)
    {
        global_hlls.update_layer1(hash_flowid,hash_element);
        return;
    }
    layer2.process_packet(flowid,hash_flowid,hash_element);
    global_hlls.update_layer2(hash_flowid,hash_element);
}

void DCSketch::update_mean_error()
{
    layer1_flows = global_hlls.get_number_flows(LAYER1);
    layer2_flows = global_hlls.get_number_flows(LAYER2);
    layer1_elements = global_hlls.get_number_elements(LAYER1);
    layer2_elements = global_hlls.get_number_elements(LAYER2);
    if(layer1_flows > layer1.thresh_ratio * layer1.bitmap_num)
        layer1_err_remove = true;
    else
        layer1_err_remove = false;
    if(layer2_flows > layer2.thresh_ratio * layer2.HLL_num)
        layer2_err_remove = true;
    else
        layer2_err_remove = false;
    
    if(layer1_err_remove)
        L1_mean_error = Error_RMV[layer1_elements/layer1.bitmap_num];
    else
        L1_mean_error = 0;
    if(layer2_err_remove)
        L2_mean_error = Error_RMV[layer2_elements/layer2.HLL_num];//global_hlls.get_number_elements(LAYER2) * 2 / layer2.HLL_num;
    else
        L2_mean_error = 0;
    cout<<"layer1 flows: "<<layer1_flows<<"  layer1 elements: "<<global_hlls.get_number_elements(LAYER1)<<endl;
    cout<<"layer2 flows: "<<layer2_flows<<"  layer2 elements: "<<global_hlls.get_number_elements(LAYER2)<<endl;
    cout<<"layer1_err_remove: "<<layer1_err_remove<<" L1_mean_error: "<<L1_mean_error<<endl;
    cout<<"layer2_err_remove: "<<layer2_err_remove<<" L2_mean_error: "<<L2_mean_error<<endl;
    auto vsketch_1 = global_hlls.HLL_union(global_hlls.Layer1_flows , global_hlls.Layer2_flows);
    auto vsketch_2 = global_hlls.HLL_union(global_hlls.Layer1_elements , global_hlls.Layer2_elements);
    cout<<"number of flows: " <<global_hlls.get_cardinality(vsketch_1) << endl;
    cout<<"number of elements: " <<global_hlls.get_cardinality(vsketch_2) << endl;
    return;
}

uint32_t DCSketch::query_spread(string flowid)
{
    array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    double spread_layer1 = layer1.get_spread(flowid,hash_flowid);
    int ret;
    if(spread_layer1 != BITMAP_FULL_FLAG)
    {
        ret = round((spread_layer1 - L1_mean_error) * 2);
    }
    else
    {
        int spread_layer2 = layer2.get_spread(flowid,hash_flowid);
        ret = 2 * (spread_layer2 - L2_mean_error) + (19 - L1_mean_error * 2);
    }
    if(ret <= 0)
        ret = 1;
    return ret;
}


uint32_t DCSketch::query_spread_offline(string flowid)
{
    array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    array<uint32_t,2> bm_zero_num = layer1.get2bitmap_zeronum(hash_flowid);
    if (bm_zero_num[0] == 1 && bm_zero_num[1] == 1) //both 2 bitmaps are full
    {
        array<uint32_t,2> hll_vals = layer2.get2hll_vals(hash_flowid);
        if(hll_vals[0] > 2*hll_vals[1] || hll_vals[1] > 2*hll_vals[0])
            return 19 + 2*min(hll_vals[0],hll_vals[1]);
        size_t low_bound = (hll_vals[0] + hll_vals[1]) * (1 - 0.1856 * 2);
        size_t up_bound = (hll_vals[0] + hll_vals[1]) * (1 + 0.1856 * 2);
        double max_prob = 0;
        uint32_t ret_spread = mle_esti.find_max(hll_vals, low_bound, up_bound);
        return 19 + ret_spread;
    }
    else
    {
        return query_spread(flowid);
    }
}

// DCSketch::DCSketch(string dataset,string filename)
// {
//     offline = true;
//     string ifile_path = "../../DCSketch/metadata/" + dataset + "/";
//     ifstream ifile_hand;
//     ifile_hand = ifstream(ifile_path + filename.substr(filename.size() - 4) + "sketch.txt");
//     if(!ifile_hand)
//     {
//         cout<<"fail to open files."<<endl;
//         return;
//     }
//     offline_layer1.resize(layer1.bitmap_num);
//     offline_layer2.resize(layer2.HLL_num);
//     string str;
//     getline(ifile_hand,str);
//     cout << str << endl;
//     for(size_t i = 0;i < offline_layer1.size();i++)
//     {
//         ifile_hand >> offline_layer1[i];
//     }
//     getline(ifile_hand,str);
//     cout << str << endl;
//     for(size_t i = 0;i < offline_layer2.size();i++)
//     {
//         ifile_hand >> offline_layer2[i];
//     }
// }

// double max_prob = 0;
// uint32_t ret_spread = 1;
// for(size_t spread = 1;spread < 19;spread++)
// {
//     double prob_spread = 0;
//     for (size_t bm1_s = 0;bm1_s <= spread;bm1_s++)
//     {
//         double cond_p1 = mle_esti.prob_cond_bm(bm_zero_num[0], bm1_s);
//         double cond_p2 = mle_esti.prob_cond_bm(bm_zero_num[1], spread - bm1_s);
//         double bm1_s_prob = mle_esti.prob_s1_val(bm1_s,spread);
//         prob_spread += cond_p1 * cond_p2 * bm1_s_prob;
//     }
//     if(prob_spread > max_prob)
//     {
//         max_prob = prob_spread;
//         ret_spread = spread;
//     }
// }
// return ret_spread;