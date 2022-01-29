#include"dcs.h"

DCS_Synopsis::DCS_Synopsis()
{
    cout<<"r: "<<r<<"  s: "<<s<<endl;
    for (size_t i = 0; i < 32; i++)
    {
        patterns[i] = 1 << (31 - i);
    }
}

void DCS_Synopsis::update(string u, string v){
    array<uint64_t,2> hashres = str_hash128(u+v,HASH_SEED);
    uint32_t bucket_idx = (hashres[0] >> 48) % bucket_num;
    uint32_t base = bucket_idx * s * r;
    array<uint32_t,r> cnt_arr_idxs;
    for (size_t i = 0; i < 3; i++)
    {
        uint32_t g_i_uv = ((hashres[0] >> (32-16*i) ) & 65535) % s;
        cnt_arr_idxs[i] = base + i * s + g_i_uv;
    }
    for (size_t i = 3; i < r; i++)
    {
        uint32_t g_i_uv = ((hashres[0] >> ( 48-16*(i-3) ) ) & 65535) % s;
        cnt_arr_idxs[i] = base + i * s + g_i_uv;
    }
    uint32_t u_32 = stoul(u);
    uint32_t v_32 = stoul(v);
    for(size_t i = 0; i < 32;i++)
    {
        if(u_32 & patterns[i] != 0)
        {
            for (size_t j = 0; j < r; j++)
            {
                counter_arrays[cnt_arr_idxs[j]][i+1]++;
            }
        }
        if(v_32 & patterns[i] != 0)
        {
            for (size_t j = 0; j < r; j++)
            {
                counter_arrays[cnt_arr_idxs[j]][i+33]++;
            }
        }
    }
    for (size_t j = 0; j < r; j++)
    {
        counter_arrays[cnt_arr_idxs[j]][0]++;
    }
}

bool compfun(pair<uint32_t,uint32_t>& a, pair<uint32_t,uint32_t>& b)
{
    return a.second >= b.second;
}

vector<pair<uint32_t, uint32_t>> DCS_Synopsis::BaseTopk(uint32_t k)
{
    // __uint128_t t = 1;
    uint32_t b = bucket_num - 1;
    set<pair<uint32_t,uint32_t>> dSample;
    while (b >= 0 && dSample.size() < (1+epsi) * s / 16)
    {
        set<pair<uint32_t,uint32_t>> ds;
        GetdSample(b, ds);
        for(auto iter : ds)
        {
            dSample.insert(iter);
        }
        b -= 1;
    }
    unordered_map<uint32_t,set<uint32_t>> res;
    for(auto iter : dSample)
    {
        res[iter.first].insert(iter.second);
    }
    vector<pair<uint32_t, uint32_t>> freqs;
    for(auto iter : res)
    {
        freqs.push_back(pair<uint32_t, uint32_t>(iter.first, iter.second.size())); 
    }
    sort(freqs.begin(), freqs.end(), compfun);
    while (freqs.size() > k)
    {
        freqs.pop_back();
    }
    for(size_t i = 0;i < freqs.size();i++)
    {
        freqs[i].second *= (1 << b);
    }
    return freqs;
}

void DCS_Synopsis::GetdSample(uint32_t b, set<pair<uint32_t,uint32_t>>& ds)
{
    ds.clear();
    pair<uint32_t,uint32_t> uv_pair;
    for(size_t j = 0;j < r;j++)
    {
        for (size_t k = 0; k < s; k++)
        {
            ReturnSingleton(b, j, k, uv_pair);
            if(uv_pair.first != 0)
            {
                ds.insert(uv_pair);
            }
        }
    }
}

void DCS_Synopsis::ReturnSingleton(uint32_t b, uint32_t j, uint32_t k, pair<uint32_t,uint32_t>& uv_pair)
{
    uint32_t idx = b * s * r + j * s + k;
    auto tmp_arr = counter_arrays[idx];
    uv_pair.first == 0;
    if(tmp_arr[0] == 0)
        return;
    uint32_t u=0,v=0;
    for(size_t i = 1;i <= log_m;i++)
    {
        if(tmp_arr[i] == tmp_arr[0])
            u |= patterns[i-1];
        else if(tmp_arr[i] != 0 && tmp_arr[i] != tmp_arr[0])
            return;
    }
    for(size_t i = log_m+1;i <= 2*log_m;i++)
    {
        if(tmp_arr[i] == tmp_arr[0])
            v |= patterns[i-33];
        else if(tmp_arr[i] != 0 && tmp_arr[i] != tmp_arr[0])
            return;
    }
    uv_pair.first = u;
    uv_pair.second = v;
}