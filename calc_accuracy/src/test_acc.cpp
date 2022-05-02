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

#define TEST_PERFLOW_ACC 1
// #define TEST_SUPERSPREADER_ACC 1
// #define TEST_SUPERCHANGES_ACC 1

double get_precision(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set);
double get_recall(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set);
double get_error(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set);

int main()
{
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};

#ifdef TEST_PERFLOW_ACC
    string dataset = "MAWI";
    string filepath = "../../DCSketch/output/PerFlowSpread/" + dataset + "/";
    // string filepath = "../../vHLL/output/" + dataset + "/";
    // string filepath = "../../rerskt/output/" + dataset + "/";
    // string filepath = "../../bSkt/output/" + dataset + "/";

    // vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000};
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    vector<uint32_t> mems{1000};
    for(auto tmpmem : mems){
        double ARE_sum = 0;
        double AAE_sum = 0;
        uint32_t filenum = datasets[dataset].size();
        for (size_t i = 0;i < filenum;i++){
            string filename = to_string(tmpmem) + "_" + datasets[dataset][i] + ".txt";
            ifstream ifile(filepath + filename);
            if(!ifile){
                cout<<"unable to open file"<<endl;
                return 0;
            }
            double relat_error = 0;
            double abs_error = 0;
            uint32_t num = 0;
            while(!ifile.eof()){
                string flowid;
                int true_spread;
                int estimated_spread;
                ifile >> flowid >> true_spread >> estimated_spread;
                relat_error += fabs((double)true_spread - estimated_spread)/true_spread;
                abs_error += fabs((double)true_spread - estimated_spread);
                num++;
            }
            double ARE = relat_error/num;
            double AAE = abs_error/num;
            ARE_sum += ARE;
            AAE_sum += AAE;
            // cout<<"ARE: "<<ARE<<endl;
            // cout<<"AAE: "<<AAE<<endl;
        }
        cout << tmpmem << " : average_ARE: " << ARE_sum/filenum << endl;
        cout << tmpmem << " : average_AAE: " << AAE_sum/filenum << endl;
    }
#endif

#ifdef TEST_SUPERSPREADER_ACC
    unordered_map<string,vector<uint32_t>> thresholds;
    thresholds["MAWI"] = {20000, 3000, 1300};
    thresholds["CAIDA"] = {8000, 3400, 1800};
    thresholds["KAGGLE"] = {1000};

    string dataset = "CAIDA";

    vector<uint32_t> mems{500, 1000, 1500, 2000};
    for(uint32_t threshold : thresholds[dataset]){
        cout << "threshold: " << threshold << endl;
        for(auto tmpmem : mems){
            cout << "memory: " << tmpmem << "   ";
        
            string filename = datasets[dataset][0];
            ifstream ifile_truth("../../get_groundtruth/SuperSpreaders/"+dataset+"/"+ filename + ".txt");
            
            filename = to_string(tmpmem) + "_" + filename;
            ifstream ifile_esti("../../DCSketch/output/SuperSpreaders/"+dataset+ "/" + filename + ".txt");
            // ifstream ifile_esti("../../SpreadSketch/output/"+dataset+"/" + filename + ".txt");  
            // ifstream ifile_esti("../../Vector_BF/output/"+dataset+"/" + filename + ".txt"); 
            // ifstream ifile_esti("../../DCS/output/"+dataset+"/" + filename + ".txt"); 
            // ifstream ifile_esti("../../CDS/output/SuperSpreaders/"+dataset+"/" + filename + ".txt");

            if(!ifile_truth || !ifile_esti){
                cout<<"unable to open file"<<endl;
                return 0;
            }
            vector<IdSpread> superspreaders;
            while(!ifile_truth.eof()){
                string flowid;
                uint32_t true_spread;
                ifile_truth >> flowid >> true_spread;
                if (true_spread >= threshold)
                    superspreaders.push_back(IdSpread(flowid,true_spread));
            }
            vector<IdSpread> superspreaders_esti;
            double tuned_threshold = threshold * (1 - 0.4 * 0.1856);
            while(!ifile_esti.eof()){
                string flowid;
                uint32_t esti_spread;
                ifile_esti >> flowid >> esti_spread;
                if(esti_spread >= tuned_threshold)
                    superspreaders_esti.push_back(IdSpread(flowid,esti_spread));
                else
                    break;
            }
            ifile_esti.close();
            ifile_truth.close();
            double prec = get_precision(superspreaders_esti, superspreaders);
            double recall = get_recall(superspreaders_esti, superspreaders);
            double F1_Score;
            if(recall == 0 && prec == 0)
                F1_Score = -1;
            else
                F1_Score = 2 * prec * recall / (prec + recall);
            double rel_error = get_error(superspreaders_esti, superspreaders);
            cout<<"Precision: "<<prec<<"   Recall: "<<recall <<"   F1_Score: "<<F1_Score << "   Error: " << rel_error << endl;
        }
    }
#endif

#ifdef TEST_SUPERCHANGES_ACC
    
    unordered_map<string,vector<uint32_t>> thresholds;
    thresholds["MAWI"] = {5000, 4000};
    thresholds["CAIDA"] = {5000, 4000};
    thresholds["KAGGLE"] = {100};

    string dataset = "CAIDA";

    vector<uint32_t> mems{500, 1000, 1500, 2000};
    for(uint32_t threshold : thresholds[dataset]){
        cout << "threshold: " << threshold << endl;
        for(auto tmpmem : mems){
            cout << "memory: " << tmpmem << "   ";
        
            ifstream ifile_truth("../../get_groundtruth/SuperChanges/" + dataset + ".txt");
            
            string filename = to_string(tmpmem) + "_" + dataset;
            ifstream ifile_esti("../../DCSketch/output/SuperChanges/"+ dataset+ "/" + filename + ".txt"); 
            // ifstream ifile_esti("../../CDS/output/SuperChanges/"+dataset+"/" + filename + ".txt");

            if(!ifile_truth || !ifile_esti){
                cout<<"unable to open file"<<endl;
                return 0;
            }
            vector<IdSpread> superchanges;
            while(!ifile_truth.eof()){
                string flowid;
                uint32_t true_spread;
                ifile_truth >> flowid >> true_spread;
                if (true_spread >= threshold)
                    superchanges.push_back(IdSpread(flowid,true_spread));
            }
            vector<IdSpread> superchanges_esti;
            double tuned_threshold = threshold * (1 - 0.4 * 0.1856);
            while(!ifile_esti.eof()){
                string flowid;
                uint32_t esti_spread;
                ifile_esti >> flowid >> esti_spread;
                if(esti_spread >= tuned_threshold)
                    superchanges_esti.push_back(IdSpread(flowid,esti_spread));
                else
                    break;
            }
            ifile_esti.close();
            ifile_truth.close();
            double prec = get_precision(superchanges_esti, superchanges);
            double recall = get_recall(superchanges_esti, superchanges);
            double F1_Score;
            if(recall == 0 && prec == 0)
                F1_Score = -1;
            else
                F1_Score = 2 * prec * recall / (prec + recall);
            double rel_error = get_error(superchanges_esti, superchanges);
            cout<<"Precision: "<<prec<<"   Recall: "<<recall <<"   F1_Score: "<<F1_Score << "   Error: " << rel_error << endl;
        }
    }
#endif
    return 0;
}




double get_error(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set){
    double total_error = 0;
    uint32_t hit_num = 0;
    for (auto true_item : truth_set){
        for(auto esti_item : esti_set){
            if(true_item.flowID == esti_item.flowID){
                hit_num++;
                total_error += abs((double)esti_item.spread - true_item.spread) / (double)true_item.spread;
                break;
            }
        }  
    }
    
    if(hit_num == 0)
        return -1;
    return total_error/hit_num;
}

double get_precision(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set){
    uint32_t hit_num = 0;
    for(auto esti_item : esti_set){
        for (auto true_item : truth_set){
            if(true_item.flowID == esti_item.flowID){
                hit_num++;
                break;
            }
        }  
    }
    if(esti_set.size() == 0)
        return -1;
    return static_cast<double>(hit_num)/esti_set.size();
}

double get_recall(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set){
    uint32_t hit_num = 0;
    for (auto true_item : truth_set){
        for(auto esti_item : esti_set){
            if(true_item.flowID == esti_item.flowID){
                hit_num++;
                break;
            }
        }  
    }
    return static_cast<double>(hit_num)/truth_set.size();
}