#include "rerskt.h"

uint8_t HLL::get_leading_zeros(uint32_t bitstr)
{
    for(size_t i = 1;i <= 32;i++)
    {
        if( ((bitstr<<i)>>i) != bitstr )
            return i - 1;
    }
    return 32;
}

void HLL::process_element(string element,uint32_t reg_pos)
{
    uint32_t hashres_32 = str_hash32(element,HASH_SEED_3);
    uint8_t lz_num = get_leading_zeros(hashres_32) + 1;
    HLL_registers[reg_pos] = max(lz_num , HLL_registers[reg_pos]);
}

int HLL::get_spread(array<uint8_t,HLL::register_num> virtual_HLL)
{
    double inv_sum = 0;
    for(size_t i = 0;i < virtual_HLL.size();i++)
    {
        inv_sum += pow(2,0-virtual_HLL[i]);
    }
    double E = alpha_m * 128 * 128 / inv_sum;
    if(E <= 2.5 * 128)
    {
        uint32_t zeros_num = 0;
        for(size_t i = 0;i < virtual_HLL.size();i++)
        {
            if(virtual_HLL[i] == 0)
                zeros_num++;
        }
        E = 128 * log((double)128/zeros_num);
    }
    return (int)E;
}

void RerSkt::process_flow(string flowid,string element)
{
    uint32_t hashres_32 = str_hash32(flowid,HASH_SEED_1);
    uint32_t HLL_pos = hashres_32 % table_size;
    hashres_32 = str_hash32(element,HASH_SEED_2);
    uint32_t unit_index = hashres_32%HLL::register_num;
    array<uint64_t,2> hashres_128 = str_hash128(flowid,HASH_SEED_1);
    uint32_t table_num;
    if(unit_index < 64)
    {
        table_num = (hashres_128[0] & ( static_cast<uint64_t>(1) << (64 - unit_index - 1) ) ) == 0 ? 0 : 1; 
    }
    else
    {
        table_num = (hashres_128[1] & ( static_cast<uint64_t>(1) << (128 - unit_index - 1) ) ) == 0 ? 0 : 1; 
    }
    if(table_num == 0)
    {
        table1[HLL_pos].process_element(element,unit_index);
    }
    else
    {
        table2[HLL_pos].process_element(element,unit_index);
    }
    
    //detect superspreaders
    if(DETECT_SUPERSPREADER == false)
        return;
    uint32_t flowsrpead = get_flow_spread(flowid);
    FLOW tmpflow;
    tmpflow.flowid = flowid;  tmpflow.flow_spread = flowsrpead;
    if(heap.size() < heap_size)
    {
        heap.push_back(tmpflow);
        std::push_heap(heap.begin(), heap.end(), MinHeapCmp());
    }
    else
    {
        if(flowsrpead >= heap[0].flow_spread)
        {
            pop_heap(heap.begin(), heap.end(), MinHeapCmp());
            heap.pop_back();
            heap.push_back(tmpflow);
            std::push_heap(heap.begin(), heap.end(), MinHeapCmp());
        }
    }
}

int RerSkt::get_flow_spread(string flowid)
{
    uint32_t hashres_32 = str_hash32(flowid,HASH_SEED_1);
    uint32_t HLL_pos = hashres_32 % table_size;
    array<uint64_t,2> hashres_128 = str_hash128(flowid,HASH_SEED_1);
    array<uint8_t,128> primary_est;
    array<uint8_t,128> complement_set;
    for(size_t i = 0;i < primary_est.size();i++)
    {
        uint32_t table_num;
        if(i < 64)
        {
            table_num = (hashres_128[0] & ( static_cast<uint64_t>(1) << (64 - i - 1) ) ) == 0 ? 0 : 1; 
        }
        else
        {
            table_num = (hashres_128[1] & ( static_cast<uint64_t>(1) << (128 - i - 1) ) ) == 0 ? 0 : 1; 
        }
        if(table_num == 0)
        {
            primary_est[i] = table1[HLL_pos].HLL_registers[i];
            complement_set[i] = table2[HLL_pos].HLL_registers[i];
        }
        else
        {
            primary_est[i] = table2[HLL_pos].HLL_registers[i];
            complement_set[i] = table1[HLL_pos].HLL_registers[i];
        }
    }
    int pri_spread = HLL::get_spread(primary_est);
    int comp_spread = HLL::get_spread(complement_set);
    int flow_spread = pri_spread - comp_spread;
    return flow_spread;
}