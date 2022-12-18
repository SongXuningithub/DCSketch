#include "bSkt.h"

uint8_t HLL::get_leading_zeros(uint32_t bitstr){
    for(size_t i = 1;i <= 32;i++){
        if( ((bitstr<<i)>>i) != bitstr )
            return i - 1;
    }
    return 32;
}

void HLL::record_element(uint32_t hashres){
    uint8_t lz_num = get_leading_zeros(hashres) + 1;
    uint32_t reg_pos = hashres & (register_num - 1);
    HLL_registers[reg_pos] = max(lz_num , HLL_registers[reg_pos]);
}

int HLL::get_spread(){
    double inv_sum = 0;
    for(size_t i = 0;i < HLL_registers.size();i++)
        inv_sum += pow(2,0-HLL_registers[i]);
    double E = alpha_m * register_num * register_num / inv_sum;
    if(E <= 2.5 * register_num){
        uint32_t zeros_num = 0;
        for(size_t i = 0;i < HLL_registers.size();i++){
            if(HLL_registers[i] == 0)
                zeros_num++;
        }
        if(zeros_num == 0)
            zeros_num = 1;
        E = register_num * log((double)register_num/zeros_num);
    }
    return (int)E;
}

double HLL::memory_utilization(){
    double used = 0;
    for (auto reg : HLL_registers){
        if (reg > 0)
            used++;
    }
    return used / register_num;
}

void Bitmap::reset(){
    for(size_t i = 0;i < raw.size();i++)
        raw[i] = 0;
}

void Bitmap::record_element(uint32_t hashres){
    uint32_t unit_pos = hashres % bitnum;
    uint32_t bitmap_pos = unit_pos / 8;
    raw[bitmap_pos] |= 1 << (7 - (unit_pos % 8));
}

uint32_t Bitmap::get_unitval(uint32_t bitpos){
    uint32_t bitmap_pos = bitpos / 8;
    uint32_t res =  1 & (raw[bitmap_pos] >> (7 - (bitpos % 8)));
    return res;
}

int Bitmap::get_spread(){
    uint32_t empty_bits = 0;
    for(size_t i = 0;i < bitnum;i++){
        uint32_t tmp = get_unitval(i);
        if(tmp == 0)
            empty_bits++;
    }
    empty_bits = empty_bits > 0 ? empty_bits : 1;
    double empty_frac = static_cast<double>(empty_bits) / bitnum;
    double card = bitnum * log(1 / empty_frac);
    return static_cast<int>(card);
}

double Bitmap::memory_utilization(){
    double used = 0;
    for (auto tmp : raw){
        bitset<8> tmpbits(tmp);
        used += tmpbits.count();
    }
    return used / bitnum;
}


void MultiResBitmap::shared_param_Init(){
    c = 2 + ceil(log2(C / (2.6744 * b)));
    mrbitmap_size = b * (c - 1) + b_hat;
    cout << "MultiResBitmap shared params initialized." << endl;
}

MultiResBitmap::MultiResBitmap(){
    if(init_flag == false) {
        shared_param_Init();
        init_flag = true;
    }
    bitmaps.resize(c);
    uint32_t uint8num = ceil(b/8.0);
    for(size_t i = 0;i < c - 1;i++)
        bitmaps[i].resize(uint8num);
    uint8num = ceil(b_hat/8.0);
    bitmaps[c - 1].resize(uint8num);
}

uint32_t MultiResBitmap::get_ones_num(uint32_t layer){
    auto tmpbitmap = bitmaps[layer];
    uint32_t setbit_num = 0;
    for(size_t i = 0;i < tmpbitmap.size();i++){
        setbit_num += get_one_num(tmpbitmap[i]);
    }
    return setbit_num;
}

void MultiResBitmap::record_element(uint32_t hashres){
    uint32_t l = get_leading_zeros(hashres);
    uint32_t setbit;
    hashres <<= 14;
    hashres >>= 14;
    if(l < MultiResBitmap::c - 1)
        setbit = hashres % MultiResBitmap::b;
    else
        setbit = hashres % MultiResBitmap::b_hat;
    if(l < c - 1){
        bitmaps[l][setbit / 8] |= (128 >> (setbit % 8));
    } else {
        bitmaps[c - 1][setbit / 8] |= (128 >> (setbit % 8));
    }
}

int MultiResBitmap::get_spread(){
    int base = c - 2;
    while(base >= 0){
        uint32_t setmax;
        if(base == c - 1)
            setmax = b_hat * setmax_ratio;
        else
            setmax = b * setmax_ratio;
        if(get_ones_num(base) > setmax)
            break;
        base--;
    }
    base++;
    double m = 0;
    for(size_t i = base;i < c - 1;i++){
        m += b * log( static_cast<double>(b) / (b - get_ones_num(i) ) );
    }
    m += b_hat * log( static_cast<double>(b_hat) / (b_hat - get_ones_num(c - 1) ) );
    uint32_t factor = powf64(2,base);
    return static_cast<uint32_t>(factor * m);
}

// double MultiResBitmap::memory_utilization(){
//     double used = 0;
//     for (size_t i = 0;i < bitmaps.size() - 1;i++){
//         used += get_ones_num(i);
//     }
//     used += get_ones_num(bitmaps.size() - 1);
//     return used / MultiResBitmap::mrbitmap_size;
// }

double MultiResBitmap::memory_utilization(){
    double used = 0;
    for (size_t i = 0;i < bitmaps.size();i++){
        if (get_ones_num(i) > 0)
            used++;
    }
    return used / MultiResBitmap::c;
}

template<class Estimator>
void bSkt<Estimator>::process_packet(string flowid, string element){
    //Couper: filter
    array<uint64_t,2> hash_flowid = str_hash128(flowid, HASH_SEED_1);
    array<uint64_t,2> hash_element = str_hash128(flowid + element, HASH_SEED_2);
    //bSkt
    // array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    // array<uint64_t,2> hash_element = str_hash128(flowid + element,HASH_SEED_2);
    for(size_t i = 0;i < 4;i++){
        uint32_t tmp_flow_hash = static_cast<uint32_t>( hash_flowid[i/2] >> ( ((i+1) % 2) * 32 ) );
        uint32_t tmp_element_hash = static_cast<uint32_t>( hash_element[i/2] >> ( ((i+1) % 2) * 32 ) );
        uint32_t Estimator_pos = tmp_flow_hash % table_size;
        tables[i][Estimator_pos].record_element(tmp_element_hash);
    }
    if(DETECT_SUPERSPREADER == false)
        return;
    uint32_t flowsrpead = get_flow_cardinality(flowid);
    FLOW tmpflow;
    tmpflow.flowid = flowid;  tmpflow.flow_spread = flowsrpead;
    if(inserted.find(flowid) != inserted.end()){
        for(auto iter = heap.begin();iter != heap.end();iter++){
            if(iter->flowid == flowid){
                iter->flow_spread = flowsrpead;
                make_heap(iter, heap.end(), MinHeapCmp());
                break;
            }
        }
        return;
    }    
    if(heap.size() < heap_size){
        heap.push_back(tmpflow);
        inserted.insert(flowid);
        // std::push_heap(heap.begin(), heap.end(), MinHeapCmp());
        // for(size_t i = 1;i < heap.size();i++){
        //     if(heap[0].flow_spread > heap[i].flow_spread)
        //         cout<<"heap error"<<endl;
        // }
    } else {
        std::push_heap(heap.begin(), heap.end(), MinHeapCmp());
        if(flowsrpead >= heap[0].flow_spread){
            inserted.erase(heap[0].flowid);
            pop_heap(heap.begin(), heap.end(), MinHeapCmp());
            heap.pop_back();
            heap.push_back(tmpflow);
            std::push_heap(heap.begin(), heap.end(), MinHeapCmp());
            inserted.insert(flowid);
        }
    }
}

template<class Estimator>
uint32_t bSkt<Estimator>::get_flow_cardinality(string flowid){
    array<uint64_t,2> hash_flowid = str_hash128(flowid, HASH_SEED_1);
    //bSkt
    // array<uint64_t,2> hash_flowid = str_hash128(flowid,HASH_SEED_1);
    uint32_t spread = 1<<30;
    for(size_t i = 0;i < 4;i++){
        uint32_t tmp_flow_hash = static_cast<uint32_t>( hash_flowid[i/2] >> ( ((i+1) % 2) * 32 ) );
        uint32_t Estimator_pos = tmp_flow_hash % table_size;
        uint32_t tmp = tables[i][Estimator_pos].get_spread();
        if(tmp < spread)
            spread = tmp;
    }
    if (spread < 1)
        spread = 1;
    return spread;
}

template class bSkt<HLL>;
template class bSkt<Bitmap>;
template class bSkt<MultiResBitmap>;