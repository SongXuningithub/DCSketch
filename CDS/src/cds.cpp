#include"cds.h"

CDS::CDS(uint32_t mem) //kB
{
    v = mem * 1024 * 8 / (mi[0] + mi[1] + mi[2]);
    data.resize(mi[0] + mi[1] + mi[2]);
    for(size_t i = 0;i < data.size();i++)
    {
        data[i].resize(v/8 + 1);
    }
    cout<<"v: "<<v<<endl;
}

uint32_t CDS::hash_funs(uint32_t i, uint32_t x)
{
    uint64_t res = x % mi[i];
    return res;
}

void CDS::update(string flowid, string element)
{
    uint32_t st = stoul(flowid);
    uint32_t dt = stoul(element);
    uint32_t base = 0;
    uint32_t fsd = str_hash32(flowid + element,HASH_SEED);
    for(size_t i = 0;i < 3;i++)
    {
        uint32_t col = base + hash_funs(i, st);
        data[col][fsd/8] |= 128>>(fsd%8);
        base += mi[i];
    }
}

vector<uint8_t> get_intersection(vector<uint8_t>& vec1, vector<uint8_t>& vec2)
{
    vector<uint8_t> res(vec1.size());
    for(size_t i = 0;i < res.size();i++)
    {
        res[i] = vec1[i] & vec2[i];
    }
    return res;
}

uint32_t CDS::get_cardinality(uint32_t x)
{
    uint32_t base = 0;
    uint32_t col = base + hash_funs(0,x);
    auto vec1 = data[col];
    base += mi[0];
    
    col = base + hash_funs(1,x);
    auto vec2 = data[col];
    base += mi[1];
    
    col = base + hash_funs(2,x);
    auto vec3 = data[col];
    
    auto tmpvec = get_intersection(vec1, vec2);
    auto inter_vec = get_intersection(tmpvec , vec3);

    return get_cardinality(inter_vec);
}

uint32_t CDS::get_cardinality(vector<uint8_t> vec)
{
    double zero_num = 0;
    for(size_t i = 0;i < v;i++)
    {
        if(vec[i/8] & (128 >> (i%8)) == 0 )
            zero_num++;
    }
    if(zero_num == 0)
        zero_num = 1;
    uint32_t res = v * log(v / zero_num);
    return res;
}

void CDS::FindSuperCols()
{
    //get F1OD
    array<uint32_t,3> F1;
    uint32_t base = 0;
    for (size_t i = 0; i < 3; i++)
    {
        uint32_t table_sum = 0;
        for(size_t col = base;col < base + mi[i];col++)
        {
            table_sum += get_cardinality(data[col]);
        }
        F1[i] = table_sum;
        base += mi[i];
    }
    sort(F1.begin(), F1.end());
    uint32_t F1OD = F1[1];
    cout<<"F1OD = "<<F1OD<<endl;
    thresh = phi * F1OD;
    cout<<"thresh = "<<thresh<<endl;
    for(size_t i = 0;i < mi[0];i++)
    {
        uint32_t tmp = get_cardinality(data[i]);
        if(tmp > thresh)
            cols1.push_back(i);
    }
    for(size_t i = 0;i < mi[1];i++)
    {
        uint32_t tmp = get_cardinality(data[i]);
        if(tmp > thresh)
            cols2.push_back(i);
    } 

    // for(size_t i = 0;i < cols1.size();i++)
    // {
        
    // }
    // for(size_t i = 0;i < cols2.size();i++)
    // {
        
    // } 
}

void CDS::GetInverse()
{
    uint64_t tmp = 1;
    while (true)
    {
        if(tmp * Mi[0] % M == 1)
        {
            Mi_inverse[0] = tmp;
            break;
        }
        tmp++;
    }
    tmp = 1;
    while (true)
    {
        if(tmp * Mi[1] % M == 1)
        {
            Mi_inverse[1] = tmp;
            break;
        }
        tmp++;
    }
}

void CDS::DetectSuperSpreaders(set<uint32_t>& superspreaders)
{
    FindSuperCols();
    set<uint32_t> candidates;
    for(size_t i = 0;i < cols1.size();i++)
    {
        uint64_t c1 = cols1[i];
        for(size_t j = 0;j < cols2.size();j++)
        {
            uint64_t c2 = cols2[j];
            uint64_t y_hat = Mi[0] * Mi_inverse[0] * c1 + Mi[1] * Mi_inverse[1] * c2;
            while (y_hat >= M || y_hat >= (p - y_hat)/M)
            {
                y_hat -= M;
            }
            uint64_t Q = (p - y_hat)/M;
            for(uint64_t k = 0;k <= Q;k++)
            {
                uint64_t tmpx = (k*M + y_hat) % p;
                if(tmpx < n) 
                    candidates.insert(static_cast<uint32_t>(tmpx));
            }
        }
    }

    superspreaders.clear();
    for (auto iter : candidates)
    {
        if(get_cardinality(iter) >= thresh)
        {
            superspreaders.insert(iter);
        }
    }
    

}
