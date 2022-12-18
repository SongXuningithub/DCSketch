#include <iostream>
#include <set>
#include <memory>
#include <algorithm>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cmath>
#include <string>
#include "util.h"
using namespace std;

double get_FNR(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set);

int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001"};
    datasets["CAIDA"] = {"5M_frag (1)"};
    vector<uint32_t> k{50,200,500};

    string dataset = "CAIDA";
    uint32_t mem = 1000;

    for(uint32_t tmpk : k){
        auto tmpmem = mem;
        string filename = datasets[dataset][0];
        ifstream ifile_truth("../../get_groundtruth/SuperSpreaders/"+dataset+"/"+ filename + ".txt");
        
        filename = to_string(tmpmem) + "_" + filename;
        ifstream ifile_esti("../../Couper/output/SuperSpreaders/"+dataset+ "/" + filename + ".txt");

        if(!ifile_truth || !ifile_esti){
            cout<<"unable to open file"<<endl;
            return 0;
        }
        vector<IdSpread> superspreaders;
        uint32_t n = 0;
        while(!ifile_truth.eof()){
            string flowid;
            uint32_t true_spread;
            ifile_truth >> flowid >> true_spread;
            superspreaders.push_back(IdSpread(flowid,true_spread));
            
            if(++n == tmpk)
                break;
        }
        vector<IdSpread> superspreaders_esti;
        while(!ifile_esti.eof()){
            string flowid;
            uint32_t esti_spread;
            ifile_esti >> flowid >> esti_spread;
            superspreaders_esti.push_back(IdSpread(flowid,esti_spread));
        }
        ifile_esti.close();
        ifile_truth.close();
        double fnr = get_FNR(superspreaders_esti, superspreaders);
        cout << "k: " << tmpk << "  FNR: "<< fnr << endl;
    }
    return 0;
}

double get_FNR(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set){
    uint32_t hit_num = 0;
    for (auto true_item : truth_set){
        for(auto esti_item : esti_set){
            if(true_item.flowID == esti_item.flowID){
                hit_num++;
                break;
            }
        }  
    }
    cout<<"truthset: "<<truth_set.size()<<endl;
    return 1 - static_cast<double>(hit_num)/truth_set.size();
}
