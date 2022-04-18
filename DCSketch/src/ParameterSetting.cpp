#include "DCSketch.h"
#include "mylibpcap.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <set>
#include <memory>
#include <algorithm>
#include <unistd.h>
using std::unique_ptr;

bool per_src_flow = true;

double vecmean(vector<double> vec){
    double sum_ = 0;
    for(auto item : vec)
        sum_ += item;
    return sum_ / vec.size();
}

double vecmax(vector<double> vec){
    double max_ = -1;
    for(auto item : vec){
        if(item > max_)
            max_ = item;
    }
    return max_;
}

double vecmin(vector<double> vec){
    double min_ = 1e9;
    for(auto item : vec){
        if(item < min_)
            min_ = item;
    }
    return min_;
}

int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};

    string dataset = "MAWI";
    if(dataset == "CAIDA"){
        per_src_flow = false;
        cout<<"per_src_flow = false"<<endl;
    }

    // vector<double> Layer1Ratio{0.9, 0.8, 0.7, 0.6, 0.5};
    vector<double> Layer1Ratio{0.6};
    uint32_t tmpmem = 1000;
    for(auto tmpratio : Layer1Ratio){
        vector<double> Layer1factors;
        vector<double> Layer2factors;
        for (size_t i = 0; i < datasets[dataset].size(); i++){
            DCSketch dcsketch(tmpmem, tmpratio);
            string filename = datasets[dataset][i];
            PCAP_SESSION session(dataset,filename,PCAP_FILE);

            IP_PACKET cur_packet;
            string srcip,dstip;
            // clock_t startTime,endTime;
            // startTime = clock();
            while(int status = session.get_packet(cur_packet)){
                srcip = cur_packet.get_srcip();
                dstip = cur_packet.get_dstip();
                if (per_src_flow)
                    dcsketch.process_element(srcip,dstip);
                else
                    dcsketch.process_element(dstip,srcip);
            }
            // endTime = clock();
            // cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            dcsketch.get_global_info();

            array<double,2> tmpfactors = dcsketch.GetLoadFactor();
            Layer1factors.push_back(tmpfactors[0]);
            Layer2factors.push_back(tmpfactors[1]);
        }
        cout << "Layer1 Ratio: " << tmpratio << endl;
        cout << "Layer1factors: Max: " << vecmax(Layer1factors) << " Min: " << vecmin(Layer1factors) << " Meam: " << vecmean(Layer1factors) << endl;
        cout << "Layer2factors: Max: " << vecmax(Layer2factors) << " Min: " << vecmin(Layer2factors) << " Meam: " << vecmean(Layer2factors) << endl;
    }
    return 0;
}
