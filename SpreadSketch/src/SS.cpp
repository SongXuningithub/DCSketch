#include "SS.h"

uint32_t get_leading_zeros(uint32_t bitstr)
{
    for(size_t i = 1;i <= 32;i++)
    {
        if( ((bitstr<<i)>>i) != bitstr )
            return i - 1;
    }
    return 32;
}

MultiResBitmap::MultiResBitmap()
{
    V.resize(c);
    uint32_t uint8num = ceil(b/8.0);
    for(size_t i = 0;i < c - 1;i++)
    {
        V[i].resize(uint8num);
    }
    V[c - 1].resize(uint8num * 2);
}

uint32_t MultiResBitmap::get_ones_num(uint32_t layer)
{
    auto tmpbitmap = V[layer];
    uint32_t tmp_b;
    if(layer < c - 1)
        tmp_b = b;
    else 
        tmp_b = b_hat;
    uint32_t setbit_num = 0;
    for(size_t i = 0;i < tmp_b;i++)
    {
        if( ( (tmpbitmap[i/8] >> (7 - (i%8)) ) && 1) == 1)
            setbit_num++;
    }
    return setbit_num;
}

void MultiResBitmap::update(uint32_t hashval)
{
    uint32_t l = get_leading_zeros(hashval);
    if(l < c - 1)
    {
        uint32_t setbit = hashval % b;
        V[l][setbit / 8] |= (128 >> (setbit % 8));
    }
    else
    {
        uint32_t setbit = hashval % b_hat;
        V[c - 1][setbit / 8] |= (128 >> (setbit % 8));
    }
}

uint32_t MultiResBitmap::get_cardinality()
{
    uint32_t base = c - 1;
    while(base >= 0)
    {
        uint32_t setmax;
        if(base == c - 1)
            setmax = b_hat * setmax_ratio;
        else
            setmax = b * setmax_ratio;
        if(get_ones_num(base) > setmax)
        {
            break;
        }
        base--;
    }
    base++;
    double m = 0;
    for(size_t i = base;i < c - 1;i++)
    {
        m += b * log( static_cast<double>(b) / (b - get_ones_num(i) ) );
    }
    m += b_hat * log( static_cast<double>(b_hat) / (b_hat - get_ones_num(c - 1) ) );
    uint32_t factor = powf64(2,base);
    return static_cast<uint32_t>(factor * m);
}