#include "bSkt.h"

uint8_t HLL::get_leading_zeros(uint32_t bitstr)
{
    for(size_t i = 1;i <= 32;i++)
    {
        if( ((bitstr<<i)>>i) != bitstr )
            return i - 1;
    }
    return 32;
}

void HLL::process_element(uint32_t hashres)
{
    uint8_t lz_num = get_leading_zeros(hashres) + 1;
    uint32_t reg_pos = hashres & (128 - 1);
    HLL_registers[reg_pos] = max(lz_num , HLL_registers[reg_pos]);
}

int HLL::get_spread()
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
    return (int)E;
}

void bSkt::process_packet(string flowid,string element)
{
    array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    array<uint64_t,2> hash_element = str_hash128(flowid + element,HASH_SEED_2);
    for(size_t i = 0;i < 4;i++)
    {
        uint32_t tmp_flow_hash = static_cast<uint32_t>( hash_flowid[i/2] >> ( ((i+1) % 2) * 32 ) );
        uint32_t tmp_element_hash = static_cast<uint32_t>( hash_element[i/2] >> ( ((i+1) % 2) * 32 ) );
        uint32_t HLL_pos = tmp_flow_hash % table_size;
        tables[i][HLL_pos].process_element(tmp_element_hash);
    }
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

uint32_t bSkt::get_flow_spread(string flowid)
{
    array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    uint32_t spread = 1<<30;
    for(size_t i = 0;i < 4;i++)
    {
        uint32_t tmp_flow_hash = static_cast<uint32_t>( hash_flowid[i/2] >> ( ((i+1) % 2) * 32 ) );
        uint32_t HLL_pos = tmp_flow_hash % table_size;
        uint32_t tmp = tables[i][HLL_pos].get_spread();
        if(tmp < spread)
            spread = tmp;
    }
    return spread;
}