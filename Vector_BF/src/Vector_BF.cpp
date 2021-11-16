#include "Vector_BF.h"


void BF_Table::resize(uint32_t rows_, uint32_t m_)
{
    rows = rows_;
    m = m_;
    raw.resize(rows);
    for(size_t i = 0;i < rows;i++)
    {
        order_nums[i] = i;
        raw[i].resize( static_cast<size_t>(ceil(m/8.0)) );
    }
}

void BF_Table::update(uint32_t row,uint32_t col)
{
    uint32_t uint8pos = col / 8;
    uint32_t bitpos = col % 8;
    raw[row][uint8pos] |= (1 << (7 - bitpos)) ;
}

void BF_Table::append(uint32_t order_num,vector<uint8_t> rowdata)
{
    order_nums.push_back(order_num);
    raw.push_back(rowdata);
    rows++;
}

/****************************************************************/

Vector_Bloom_Filter::Vector_Bloom_Filter(uint32_t m_)
{
    m = m_;
    for(size_t k = 0;k < 5;k++)
    {
        raw[k].resize(4096,m);
    }
}

uint32_t Vector_Bloom_Filter::VBF_hash_1to5(uint32_t hash_num,array<uint8_t,4> srcip_tuple,string srcip_str)
{
    if(hash_num<=4)
    {
        uint32_t row = srcip_tuple[hash_num - 1];
        row <<= 4;
        row += (srcip_tuple[hash_num%4]>>4);
        return row;
    }
    else
    {
        uint32_t hashres32 = str_hash32(srcip_str,HASH_SEED);
        uint32_t row = hashres32 & 4095;
        return row;
    }
}

uint32_t Vector_Bloom_Filter::VBF_hash_f(string srcip,string dstip)
{
    uint32_t hashres32 = str_hash32(srcip+dstip,HASH_SEED);
    uint32_t col = hashres32 % m;
    return col;
}

void Vector_Bloom_Filter::process_packet(string srcip,array<uint8_t,4> srcip_tuple,string dstip)
{
    uint32_t col = VBF_hash_f(srcip,dstip);

    for(size_t k = 1;k <= 5;k++)
    {
        uint32_t row = VBF_hash_1to5(k,srcip_tuple,srcip);
        raw[k-1].update(row,col);
    }
}

uint32_t Vector_Bloom_Filter::compare_tailhead(uint32_t num1,uint32_t num2)
{
    if((num1 & 15) == (num2 >> 8))
        return 1;
    if((num1 & 15) == (num2 >> 16))
        return 2;
    if((num1 & 15) == (num2 >> 24))
        return 3;  
    return 0;
}

uint32_t Vector_Bloom_Filter::merge(uint32_t num1,uint32_t num2,uint32_t flag)
{
    uint32_t bias = flag * 8;
    uint32_t ret = (num1 << bias) | num2;
    return ret;
}

uint32_t Vector_Bloom_Filter::get_zero_num(vector<uint8_t> vec)
{
    uint32_t zero_num = 0;
    for(size_t i = 0;i < vec.size();i++)
    {
        uint8_t tmp = vec[i];
        for(size_t bit = 0;bit < 8;bit++)
        {
            if( ((tmp >> bit) && 1) == 0)
                zero_num++;
        }
    }
    return zero_num;
}

void Vector_Bloom_Filter::calc_Z(uint32_t threshold)
{
    double s = 0;
    for(size_t i = 0;i < 4096;i++)
    {
        double zeronum = get_zero_num(raw[4].raw[i]);
        zeronum = max(1.0,zeronum);
        s += m * log(m / zeronum);
    }
    double t = m * (1 - exp( -s/(4096*m) ) );
    uint32_t delta_b = s * t / (4096 * m);
    Z = m * exp(-static_cast<double>(threshold)/m);
}

void Vector_Bloom_Filter::Merge_String(BF_Table& input1,BF_Table& input2,BF_Table& output)
{
    for(size_t i = 0;i < input1.rows;i++)
    {
        for(size_t j = 0;j < input2.rows;j++)
        {
            uint32_t u = input1.order_nums[i];
            uint32_t w = input2.order_nums[j];
            uint32_t flag = compare_tailhead(u,w);
            if(flag > 0)
            {
                uint32_t v = merge(u,w,flag);
                vector<uint8_t> V_u = input1.raw[i];
                vector<uint8_t> V_w = input1.raw[i];
                vector<uint8_t> V_v(V_u.size());
                for(size_t t = 0;t < V_u.size();t++)
                {
                    V_v[t] = V_u[t] & V_w[t];
                }
                uint32_t zeronum = get_zero_num(V_v);
                if(zeronum <= Z)
                {
                    output.append(v,V_v);
                }
            }
        }
    }
}

void Generate_IP(BF_Table& input1,BF_Table& input2,BF_Table& output)
{
    
}