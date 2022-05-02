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

Bitmap_Arr::Bitmap_Arr(uint32_t memory_){
    memory = memory_;
    bitmap_num = memory * 1024 * 8 / bitmap_size;
    raw.resize(memory*1024*8/32);
    for(size_t i = 0;i < raw.size();i++)
        raw[i] = 0;
    for(size_t i = 0;i < bitmap_size;i++)
        patterns[i] = 1 << (bitmap_size - i - 1);
    
    double ln_bmsize = log(bitmap_size);
    double ln_bmsize_minu1 = log(bitmap_size - 1);

    for(size_t i = 1;i <= bitmap_size;i++)
        spreads[i] = ( ln_bmsize - log(i) ) / (ln_bmsize - ln_bmsize_minu1);
    spreads[0] = spreads[1]; 
    capacity = floor(spreads[1] * 2);

    // cout<< "The capacity of layer1 is " << capacity << endl;
    cout<< "The number of LC(bitmap)s in layer 1: " << bitmap_num << endl;
}

uint16_t Bitmap_Arr::get_bitmap(uint32_t bitmap_pos){
    using namespace metadata;
    bits_bias = bitmap_pos * bitmap_size;
    uint32_pos =  bits_bias / 32;
    inner_bias = bits_bias % 32;
    shift_ = 31 - (inner_bias + bitmap_size - 1);
    uint16_t res;
    if(shift_ >= 0) 
        res = static_cast<uint16_t>(raw[uint32_pos] >> shift_);
    else
        res = static_cast<uint16_t>( (raw[uint32_pos] << (-shift_)) + (raw[uint32_pos + 1] >> (32 + shift_)) );
    res &= FULL_PAT;
    return res;
}

uint32_t zero_pos;
bool has_zero = false;

int Bitmap_Arr::check_bitmap_full(uint16_t input_bitmap){
    if( input_bitmap == FULL_PAT ){
        //cout<<"FAST return."<<endl;
        return bitmap_size;  
    }
    
    for(size_t i=0;i<bitmap_size;i++){
        if( (patterns[i] & input_bitmap) == 0){
            if(has_zero)
                return -1;
            else
                has_zero = true;
            zero_pos = i;
        }   
    }
    return zero_pos;
}

bool Bitmap_Arr::set_bit(uint32_t bit_pos){
    using namespace metadata;
    uint32_t temp = inner_bias + bit_pos;
    if(temp <= 31)
        raw[uint32_pos] |= 1<<(31-temp); 
    else{
        temp -= 32;
        raw[uint32_pos + 1] |= 1<<(31-temp); 
    }
    return false;
}

bool Bitmap_Arr::process_packet(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element){
    array<uint32_t,2> L1_pos;
    L1_pos[0] = static_cast<uint32_t>(hash_flowid[0]>>32) % bitmap_num;
    L1_pos[1] = static_cast<uint32_t>(hash_flowid[0]) % bitmap_num;
    uint32_t hashres32 = static_cast<uint32_t>(hash_element[0] >> 32);
    uint32_t update_pos = L1_pos[(hashres32>>16) % 2];
    uint16_t tmp_bitmap = get_bitmap(update_pos);
    bool all_full = true;
    int empty_pos = check_bitmap_full(tmp_bitmap);
    if(empty_pos == -1){
        uint32_t update_bit = static_cast<uint16_t>(hashres32) % bitmap_size;
        set_bit(update_bit);
        all_full = false;
    } else {   //the hashed bitmap(linear-counting) has been full, so we check the other one.
#ifdef DEBUG_LAYER12
        cout<<"ONE bitmap_state: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
        if(empty_pos != bitmap_size)
            set_bit(empty_pos);   //set the last zero bit to 1
        for(size_t i = 0;i < 2;i++){
            if(L1_pos[i] == update_pos)
                continue;
            tmp_bitmap = get_bitmap(L1_pos[i]);
#ifdef DEBUG_LAYER12
            cout<<"ANOTHER bitmap_state: "<<std::bitset<6>(bitmap_state)<<endl;
#endif
            empty_pos = check_bitmap_full(tmp_bitmap);
            if(empty_pos == -1){
                all_full = false;
                break;
            }
             if(empty_pos != bitmap_size)
                set_bit(empty_pos);   //set the last zero bit to 1
        }
    }
    return all_full;
}

uint32_t Bitmap_Arr::get_spread(string flowid, array<uint64_t,2>& hash_flowid, uint32_t error_){
    array<uint32_t,2> L1_pos;
    L1_pos[0] = static_cast<uint32_t>(hash_flowid[0]>>32) % bitmap_num;
    L1_pos[1] = static_cast<uint32_t>(hash_flowid[0]) % bitmap_num;

    double min_spread = 1000;
    bool all_full = true;
    for(size_t i = 0;i < L1_pos.size();i++){
        uint16_t tmp_bitmap = get_bitmap(L1_pos[i]);
        size_t zeros_num = 0;
        for(size_t bit_pos = 0;bit_pos < bitmap_size;bit_pos++){
            if( (tmp_bitmap & patterns[bit_pos]) == 0)
                zeros_num++;
        }
        min_spread = min(spreads[zeros_num] , min_spread);
        if(zeros_num > 1)
            all_full = false;
    }
    if(all_full)
        return BITMAP_FULL_FLAG;
    uint32_t ans = round((min_spread - error_) * 2);
    if (ans < 1)
        return 1;
    return ans;

    // if (min_spread > error_)
    //     return round((min_spread - error_) * 2);
    // else
    //     return 1;
}

HLL_Arr::HLL_Arr(uint32_t memory_){
    memory = memory_;
    HLL_num = memory * 1024 * 8 / HLL_size;
    HLL_raw.resize(memory * 1024 * 8 / 8);
    cout << "The number of HLLs in layer 2: " << HLL_num << endl;
    for(size_t i = 0;i <= HLL_raw.size();i++)
        HLL_raw[i] = 0;
    hash_table.resize(tab_size);
    for(size_t i = 0;i < exp_table.size();i++)
        exp_table[i] = pow(2.0, 0.0 - i);
    if (register_num == 32)
        alpha_m = 0.697; 
    else if (register_num == 64)
        alpha_m = 0.709;
    else if (register_num >= 128)
        alpha_m = 0.7213/(1 + 1.079/register_num);
    alpha_m_sqm = alpha_m * register_num * register_num; 
    LC_thresh = 2.5 * register_num; 
}

uint32_t HLL_Arr::get_counter_val(uint32_t HLL_pos,uint32_t bucket_pos){
    uint32_t uint8_pos = HLL_pos * (register_num >> 1) + bucket_pos / 2;
    if(bucket_pos % 2 == 0)
        return HLL_raw[uint8_pos] >> 4;       //high 4 bits
    else
        return HLL_raw[uint8_pos] & 15;       //low 4 bits
}

void HLL_Arr::set_counter_val(uint32_t HLL_pos,uint32_t bucket_pos,uint32_t val_){
    uint32_t uint8_pos = HLL_pos * (register_num >> 1) + bucket_pos / 2;
    if(bucket_pos % 2 == 0){
        HLL_raw[uint8_pos] &= 15;            //keep the low 4 bits unchanged 
        HLL_raw[uint8_pos] |= static_cast<uint8_t>(val_) << 4;      //set the high 4 bits
    } else {
        HLL_raw[uint8_pos] &= 240;            //keep the high 4 bits unchanged 
        HLL_raw[uint8_pos] |= static_cast<uint8_t>(val_);
    }
}

void HLL_Arr::process_packet(string flowid, array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element){
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
    if(bucket_val < rou_x){
        set_counter_val(update_pos,bucket_pos,rou_x);
        if (bucket_pos < Table_Entry::selected_num) {
            //operation on hash table
            uint8_t selected_sum1 = get_counter_val(HLL_pos[0],0)+get_counter_val(HLL_pos[0],1)+get_counter_val(HLL_pos[0],2)+get_counter_val(HLL_pos[0],3);
            uint8_t selected_sum2 = get_counter_val(HLL_pos[1],0)+get_counter_val(HLL_pos[1],1)+get_counter_val(HLL_pos[1],2)+get_counter_val(HLL_pos[1],3);
            uint8_t selected_sum = min(selected_sum1,selected_sum2);
            insert_hashtab(flowid,selected_sum,hash_flowid[0]);
        }
    }
}

uint32_t HLL_Arr::get_spread(string flowid, array<uint64_t,2>& hash_flowid, uint32_t error_) {
    array<uint32_t,2> HLL_pos;
    HLL_pos[0] = static_cast<uint32_t>(hash_flowid[1] >> 32) % HLL_num;
    HLL_pos[1] = static_cast<uint32_t>(hash_flowid[1]) % HLL_num;

    double res_1;
    double sum_ = 0;
    uint32_t V_ = 0;
    for(size_t i = 0;i < register_num;i++){
        uint32_t tmpval = get_counter_val(HLL_pos[0],i);
        sum_ += exp_table[tmpval];
        if(tmpval == 0)
            V_++;
    }
    res_1 = alpha_m_sqm / sum_;
    if(res_1 <= LC_thresh){
        if(V_ > 0)
            res_1 = register_num * log(register_num / (double)V_);
    }

    double res_2;
    sum_ = 0;
    V_ = 0;
    for(size_t i = 0;i < register_num;i++){
        uint32_t tmpval = get_counter_val(HLL_pos[1],i);
        sum_ += exp_table[tmpval];
        if(tmpval == 0)
            V_++;
    }
    res_2 = alpha_m_sqm / sum_;
    if(res_2 <= LC_thresh){
        if(V_ > 0)
            res_2 = register_num * log(register_num / (double)V_);
    }
    double min_spread = min(res_1,res_2);
    uint32_t ans = round((min_spread - error_) * 2);
    if (ans < 1)
        return 1;
    return ans;

    // if(min_spread > error_)
    //     return round(2 * (min_spread - error_));
    // else
    //     return 0;
}

void HLL_Arr::insert_hashtab(string flowid, uint8_t selected_sum, uint64_t hahsres64){
    uint32_t hashres32 = hahsres64 >> 32;         //high 32 bits of initial hash result which is 64 bits
    uint32_t table_pos1 = (hashres32 >> 16) % tab_size;     //high 16 bits
    uint32_t table_pos2 = (hashres32 & MAX_UINT16) % tab_size;  //low 16 bits

    if(hash_table[table_pos1].flowid == "" || hash_table[table_pos1].flowid == flowid){
        hash_table[table_pos1].flowid = flowid;
        hash_table[table_pos1].selected_sum = selected_sum;
        return;
    }
    else if(hash_table[table_pos2].flowid == "" || hash_table[table_pos2].flowid == flowid){
        hash_table[table_pos2].flowid = flowid;
        hash_table[table_pos2].selected_sum = selected_sum;
        return;
    }

    uint8_t tmp1 = hash_table[table_pos1].selected_sum;
    uint8_t tmp2 = hash_table[table_pos2].selected_sum; 
    if(tmp1 > tmp2){
        if(selected_sum >= tmp2){
            hash_table[table_pos2].flowid = flowid;
            hash_table[table_pos2].selected_sum = selected_sum;
        }
    } else {
        if(selected_sum >= tmp1){
            hash_table[table_pos1].flowid = flowid;
            hash_table[table_pos1].selected_sum = selected_sum;
        }
    }
}

void DCSketch::report_superspreaders(vector<IdSpread>& superspreaders){
    superspreaders.clear();
    set<string> checked_flows;
    for(size_t i = 0;i < layer2.tab_size;i++){
        string tmp_flowid = layer2.hash_table[i].flowid;
        if(checked_flows.find(tmp_flowid) != checked_flows.end())
            continue;
        else{
            checked_flows.insert(tmp_flowid);
            //array<uint64_t,2> hash_flowid = str_hash128(tmp_flowid,HASH_SEED_1);
            uint32_t esti_card = query_spread(tmp_flowid); 
            superspreaders.push_back( IdSpread(tmp_flowid,esti_card) );
        }
    }
    sort(superspreaders.begin(), superspreaders.end(), IdSpreadComp);
}

void Global_HLLs::update_layer1(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element){
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

void Global_HLLs::update_layer2(array<uint64_t,2>& hash_flowid, array<uint64_t,2>& hash_element){
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

uint32_t Global_HLLs::get_cardinality(array<uint8_t,register_num>& HLL_registers){
    double inv_sum = 0;
    uint32_t TmpHLLSize = HLL_registers.size();
    for(size_t i = 0;i < TmpHLLSize;i++)
        inv_sum += pow(2,0-(int)HLL_registers[i]);
    double E = alpha_m * TmpHLLSize * TmpHLLSize / inv_sum;
    if(E <= 2.5 * TmpHLLSize){
        uint32_t zeros_num = 0;
        for(size_t i = 0;i < TmpHLLSize;i++){
            if(HLL_registers[i] == 0)
                zeros_num++;
        }
        E = TmpHLLSize * log((double)TmpHLLSize/zeros_num);
    }
    return E;
}

array<uint8_t,Global_HLLs::register_num> Global_HLLs::HLL_union(array<uint8_t,register_num>& HLL_registers1,array<uint8_t,register_num>& HLL_registers2){
    array<uint8_t,register_num> vsketch;
    for(size_t i = 0;i < HLL_registers1.size();i++)
        vsketch[i] = max(HLL_registers1[i] , HLL_registers2[i]);
    return vsketch;
}

uint32_t Global_HLLs::get_number_flows(uint32_t layer){
    if(layer == LAYER1)
        return get_cardinality(Layer1_flows);
    else
        return get_cardinality(Layer2_flows);
}

uint32_t Global_HLLs::get_number_elements(uint32_t layer){
    if(layer == LAYER1)
        return get_cardinality(Layer1_elements);
    else
        return get_cardinality(Layer2_elements);
}

uint32_t DCSketch::process_element(string flowid,string element) {
    array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    array<uint64_t,2> hash_element = str_hash128(flowid + element,HASH_SEED_2);
    bool layer1_full = layer1.process_packet(hash_flowid,hash_element);
    if(!layer1_full){
        global_hlls.update_layer1(hash_flowid,hash_element);
        return 1;
    }
    layer2.process_packet(flowid,hash_flowid,hash_element);
    global_hlls.update_layer2(hash_flowid,hash_element);
    return 2;
}

void DCSketch::get_global_info() {
    layer1_flows = global_hlls.get_number_flows(LAYER1);
    layer2_flows = global_hlls.get_number_flows(LAYER2);
    layer1_elements = global_hlls.get_number_elements(LAYER1);
    layer2_elements = global_hlls.get_number_elements(LAYER2);

    if(layer1_flows > layer1.thresh_ratio * layer1.bitmap_num)
        L1_mean_error = Error_RMV[layer1_elements/layer1.bitmap_num];
    if(layer2_flows > layer2.thresh_ratio * layer2.HLL_num)
        L2_mean_error = Error_RMV[layer2_elements/layer2.HLL_num];//global_hlls.get_number_elements(LAYER2) * 2 / layer2.HLL_num;
    
    cout<<"L1_mean_error: "<<L1_mean_error<<"  L2_mean_error: "<<L2_mean_error<<endl;

    auto vsketch_1 = global_hlls.HLL_union(global_hlls.Layer1_flows , global_hlls.Layer2_flows);
    auto vsketch_2 = global_hlls.HLL_union(global_hlls.Layer1_elements , global_hlls.Layer2_elements);
    int overlapping_Bias = layer2_elements + layer1_elements - (int)global_hlls.get_cardinality(vsketch_2);
    cout << "overlapping_Bias: " << overlapping_Bias << "  average overlapping bias: " << overlapping_Bias / static_cast<double>(layer2_flows) << endl;
#ifdef DEBUG_OUTPUT
    cout<<"layer1 flows: "<<layer1_flows<<"  layer1 elements: "<<layer1_elements<<endl;
    cout<<"layer2 flows: "<<layer2_flows<<"  layer2 elements: "<<layer2_elements<<endl;
    cout<<"L1_mean_error: "<<L1_mean_error<<"  L2_mean_error: "<<L2_mean_error<<endl;
    cout<<"total number of flows: " <<global_hlls.get_cardinality(vsketch_1) << endl;
    cout<<"total number of elements: " <<global_hlls.get_cardinality(vsketch_2) << endl;
#endif
    return;
}

uint32_t DCSketch::query_spread(string flowid){
    array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    uint32_t spread_layer1 = layer1.get_spread(flowid, hash_flowid, L1_mean_error);
    uint32_t ret;
    if(spread_layer1 != BITMAP_FULL_FLAG)
        ret = spread_layer1;
    else {
        uint32_t spread_layer2 = layer2.get_spread(flowid, hash_flowid, L2_mean_error);
        ret = spread_layer2 + layer1.capacity - 2 * L1_mean_error;
    }
    return ret;
}

array<double,2> DCSketch::GetLoadFactor(){
    double L1factor = (double)layer1_flows / layer1.bitmap_num;
    double L2factor = (double)layer2_flows / layer2.HLL_num;
    return array<double,2>{L1factor,L2factor};
}
