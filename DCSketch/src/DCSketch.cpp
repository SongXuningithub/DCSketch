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

BJKST_Arr::BJKST_Arr()
{
    cout<<"BJKST estimators initializing..."<<endl;
    for(uint32_t counter_pos = 0;counter_pos < k;counter_pos++)
    {
        uint32_t cur_val = static_cast<uint32_t>(static_cast<double>(counter_pos+1)/k*counter_max_int);
        for(uint32_t estimator_pos = 0;estimator_pos < bjkst_arr_size;estimator_pos++)
        {
            set_counter_val(estimator_pos,counter_pos,cur_val);
        }
    }
    // for(uint32_t estimator_pos = 0;estimator_pos < bjkst_arr_size;estimator_pos++)
    // {
    //     for(uint32_t counter_pos = 0;counter_pos < k;counter_pos++)
    //     {
    //         uint32_t cur_val = get_counter(estimator_pos,counter_pos);
    //         cout<<counter_pos<<" : "<<cur_val<<" ";
    //     }
    //     cout<<endl;
    // }
    cout<<"Initialization done."<<endl;
    cout<<"total memory cost: "<<memory<<"kB"<<endl;
    cout<<"memory const per (BJKST)estimator: "<<k*integer_size<<endl;
    cout<<"integers per (BJKST)estimator: "<<k<<endl;
    cout<<"number of (BJKST)estimators: "<<bjkst_arr_size<<endl;
}

uint32_t BJKST_Arr::get_counter(uint32_t estimator_pos,uint32_t counter_pos)
{
    uint32_t counter_lowpart = lowbits_raw[estimator_pos * k + counter_pos];
    uint32_t st_bit = estimator_pos * k * highpart_size + counter_pos * highpart_size;
    uint32_t shift_ = 8 - ((st_bit % 8) + highpart_size);
    uint32_t counter_highpart = static_cast<uint32_t>((highbits_raw[st_bit/8] >> shift_) & highpart_pat);
    uint32_t counter_val = (counter_highpart << 8) + counter_lowpart;
    return counter_val;
}

void BJKST_Arr::get_flow_estimators(string flowid, array<double,BJKST_Arr::k>& estimators)
{
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_LAYER2);
    uint32_t estimator_pos = static_cast<uint32_t>(hashres128[0] >> 32) % bjkst_arr_size; 
    for(size_t i = 0;i < estimators.size();i++)
    {
        estimators[i] = int2dec(get_counter(estimator_pos,i));
    }
}

void BJKST_Arr::set_counter_val(uint32_t estimator_pos,uint32_t counter_pos,uint32_t val)
{
    uint32_t val_lowpart = val & MAX_UINT8;
    lowbits_raw[estimator_pos * k + counter_pos] = val_lowpart;
    uint8_t val_highpart = (val >> 8)&highpart_pat;
    uint32_t st_bit = estimator_pos * k * highpart_size + counter_pos * highpart_size;
    uint32_t shift_ = 8 - ((st_bit % 8) + highpart_size);
    highbits_raw[st_bit/8] &= ~(highpart_pat<<shift_);  //clear high bits
    highbits_raw[st_bit/8] |= (val_highpart<<shift_);
}

// #define BASE 1.004
// #define DEC_MAX 59.61  // 1.004^1024
// #define BASE 1.003
// #define DEC_MAX 21.4859  // 1.003^1024
#define BASE 1.002
#define DEC_MAX 7.7365  // 1.003^1024
// #define BASE 1.004
// #define DEC_MAX 59.61  // 1.003^1024
double BJKST_Arr::int2dec(uint32_t cnt_val)
{
    /*linear interval coding*/
    //return static_cast<double>(cnt_val)/counter_max_int * counter_max_dec;
    /*exponential interval coding*/
    return (pow(BASE,cnt_val + 1) - 1)/(DEC_MAX - 1) * counter_max_dec;
}

uint32_t BJKST_Arr::dec2int(double dec_val)
{
    /*linear interval coding*/
    //return dec_val / counter_max_dec * counter_max_int;
    /*exponential interval coding*/
    dec_val = 1 + dec_val / counter_max_dec * (DEC_MAX - 1);
    double res = log(dec_val)/log(BASE);
    return static_cast<uint32_t>(res);
}

uint32_t BJKST_Arr::get_flow_spread(string flowid,double& flow_max_dec)
{
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_LAYER2);
    uint32_t max_kth_val = 0;
    for(size_t i = 0;i < hash_num;i++)
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
        cur_pos %= bjkst_arr_size;
        uint32_t kth_counter_val = get_counter(cur_pos,k - 1);
        if (kth_counter_val > max_kth_val)
        {
            max_kth_val = kth_counter_val;
        }
    }
    double dec_val = int2dec(max_kth_val);
    uint32_t ret_spread = k/dec_val;
    return ret_spread;
}

bool BJKST_Arr::process_element(string flowid,string element)
{
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_LAYER2);
    //uint32_t estimator_pos = static_cast<uint32_t>(hashres128[0] >> 32) % bjkst_arr_size; 
    array<uint32_t,hash_num> L2_pos;
    for(size_t i = 0;i < hash_num;i++)
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
        cur_pos %= bjkst_arr_size;
        L2_pos[i] = cur_pos;
    }

    std::vector<uint32_t> update_pos;
    array<uint32_t,hash_num> pos_vals;
    uint32_t max_val = 0;
    for(size_t i = 0;i < hash_num;i++)
    {
        pos_vals[i] = get_counter(L2_pos[i],k - 1);
        if(pos_vals[i] > max_val)
        {
            max_val = pos_vals[i];
        }
    }
    for(size_t i = 0;i < hash_num;i++)
    {
        if(pos_vals[i] == max_val)
        {
            update_pos.push_back(L2_pos[i]); 
        }
    }
    for(size_t i = 0;i < update_pos.size();i++)
    {
        uint32_t cur_pos = update_pos[i];
        uint32_t hashres32 = str_hash32(element,HASH_SEED_LAYER2);
        double decval = (double)hashres32/4294967295;
        if(decval >= counter_max_dec)  //it must be larger than the kth val in any BJKST estimators
            return false;
        uint32_t intval = dec2int(decval); //decval * counter_max_int;

        int insert_pos = 56446;
        bool insertable = true;
        for(int i = k - 1;i >= 0;i--)
        {
            uint32_t curval = get_counter(cur_pos,i);
            if(intval < curval)
            {
                insert_pos = i;
                continue;
            }
            if(intval == curval)
            {
                insertable = false;
                break;
            }
            if(intval > curval)
            {
                break;
            }
        }
        if(!insertable || insert_pos >= k)
            return false;
        for(int j = k - 2;j >= insert_pos;j--)
        {
            set_counter_val(cur_pos,j+1,get_counter(cur_pos,j));
        }
        if(insert_pos < k)
            set_counter_val(cur_pos,insert_pos,intval);
    }
    return true;
}

void SS_Table::report_superspreaders()
{
    for(size_t i = 0;i < ss_table.size();i++)
    {
        cout<<ss_table[i].flowid<<"  "<<ss_table[i].get_spread()<<endl;
    }
}

int SS_Table::process_flow(string flowid,string element,double max_dec)
{
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_LAYER3);
    array<uint32_t,subtab_num> poses;
    poses[0] = (hashres128[0]>>32) % subtab_size;
    poses[1] = static_cast<uint32_t>(hashres128[0]) % subtab_size;
    poses[2] = (hashres128[1]>>32) % subtab_size;
    uint32_t try_tab =  static_cast<uint32_t>(hashres128[0]) % subtab_num;
    uint32_t esti_pos;
    uint32_t min_pos;
    double min_val = 2;
    bool find_empty = false;
    for(size_t i = 0;i < subtab_num;i++)
    {
        esti_pos = offsets[(try_tab + i)%subtab_num] + poses[(try_tab + i)%subtab_num];
        if(ss_table[esti_pos].flowid == flowid)
        {
            uint32_t hashres32 = str_hash32(element,HASH_SEED_LAYER3);
            double decval = static_cast<double>(hashres32)/MAX_UINT32;
            ss_table[esti_pos].update(decval);
            return -1;
        }
        if(ss_table[esti_pos].flowid == "")
        {
            find_empty = true;
            break;
        }
        double tmp_val = BJKST_Estimator::int2dec(ss_table[esti_pos].minimumk[BJKST_Estimator::k-1]);
        if(tmp_val < min_val)
        {
            min_val = tmp_val;
            min_pos = esti_pos;
        }
    }
    if(!find_empty)
    {
        if(max_dec < min_val)  //the processed flow is larger than the minimum flow among those hashed ones 
            return min_pos;   //so we expel the flow with the minimum flow and replace it with the processed flow.
        else
            return -1;
    }
    else
    {
        return esti_pos;   //position which is empty
    }
}

bool SS_Table::insert_flow(string flowid,uint32_t insert_pos,array<double,BJKST_Estimator::k> decvals)
{
    ss_table[insert_pos].flowid = flowid;
    for(size_t i = 0;i < decvals.size();i++)
    {
        ss_table[insert_pos].minimumk[i] = BJKST_Estimator::dec2int(decvals[i]);
    }
    return true;
}

uint32_t SS_Table::get_flow_spread(string flowid)
{
    array<uint64_t,2> hashres128 = str_hash128(flowid,HASH_SEED_LAYER3);
    array<uint32_t,subtab_num> poses;
    poses[0] = (hashres128[0]>>32) % subtab_size;
    poses[1] = static_cast<uint32_t>(hashres128[0]) % subtab_size;
    poses[2] = (hashres128[1]>>32) % subtab_size;
    uint32_t ret_spread = 0;
    uint32_t esti_pos;
    for(size_t i = 0;i < subtab_num;i++)
    {
        esti_pos = offsets[i] + poses[i];
        if(ss_table[esti_pos].flowid == flowid)
        {
            ret_spread = ss_table[esti_pos].get_spread();
        }
    }
    return ret_spread;
}

void SS_Table::BJKST_Estimator::update(uint32_t intval)
{
    int insert_pos = 10000;
    bool insertable = true;
    for(int i = k - 1;i >= 0;i--)
    {
        uint32_t curval = minimumk[i];
        if(intval < curval)
        {
            insert_pos = i;
            continue;
        }
        if(intval == curval)
        {
            insertable = false;
            break;
        }
        if(intval > curval)
        {
            break;
        }
    }
    if(!insertable || insert_pos >= k)
        return;
    for(int j = k - 2;j >= insert_pos;j--)
    {
        minimumk[j + 1] = minimumk[j];
    }
    if(insert_pos < k)
        minimumk[insert_pos] = intval;
}

void SS_Table::BJKST_Estimator::update(double decval)
{
    update(dec2int(decval)); 
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
    update_L2_card(flowid,element);
    if(!layer1_full)
    {
        update_L1_card(flowid,element);
        return;
    }
       
    layer2.process_element(flowid,element);
    //update_L2_card(flowid);
    double flow_max_dec;
    uint32_t cur_spread = layer2.get_flow_spread(flowid, flow_max_dec);
    if(cur_spread > layer3.threshold)
    {
        int insert_pos = layer3.process_flow(flowid,element,flow_max_dec);
        if(insert_pos == -1)
            return;
        array<double,15> estimators;
        layer2.get_flow_estimators(flowid,estimators);
        layer3.insert_flow(flowid,insert_pos,estimators);
    }
}

void DCSketch::update_mean_error()
{
    L1_mean_error = L1_ELEM_CARD.get_cardinality() / L1_Param::bitmap_num;
    if(L1_mean_error > L1_Param::max_spread)
        L1_mean_error = (int)L1_Param::max_spread;
    //L2_mean_error = L2_ELEM_CARD.get_cardinality() / L2_FLOW_CARD.get_cardinality() * (L2_FLOW_CARD.get_cardinality() - 1) / layer2.bjkst_arr_size;
    uint32_t l2_ele_card = L2_ELEM_CARD.get_cardinality();
    uint32_t l2_flow_card = L2_FLOW_CARD.get_cardinality();
    uint32_t l1_card = L1_ELEM_CARD.get_cardinality();
    L2_mean_error = (l2_ele_card - l1_card) * ((double)l2_flow_card / layer2.bjkst_arr_size - 1)/l2_flow_card + L1_mean_error * 2;
    //L2_mean_error = (L2_ELEM_CARD.get_cardinality() - L1_CARD.get_cardinality()) * (L2_FLOW_CARD.get_cardinality() - 1) / L2_FLOW_CARD.get_cardinality() + L1_mean_error;
    return;
}

uint32_t DCSketch::query_spread(string flowid)
{
    uint32_t spread_layer1 = layer1.get_spread(flowid);
    if(spread_layer1 != BITMAP_FULL_FLAG)
    {
        double mean_error = L1_mean_error;
        return (spread_layer1 - 2 * mean_error) > 1 ? (spread_layer1 - 2 * mean_error) : 1;
    }
    double mean_error = L2_mean_error;
    uint32_t spread_layer3 = layer3.get_flow_spread(flowid);
    if(spread_layer3 != 0)
        //return spread_layer3;
        return (spread_layer3 - mean_error) > 1 ? (spread_layer3 - mean_error) : 1;
    double tmp_val;
    uint32_t spread_layer2 = layer2.get_flow_spread(flowid,tmp_val);
    //return spread_layer2;
    return (spread_layer2 - mean_error) > 1 ? (spread_layer2 - mean_error) : 1;
}