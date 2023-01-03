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
// #define TEST_CARMON_PLUS_PRIOR_TASK1
// #define TEST_SUPERSPREADER_ACC 1
// #define TEST_CARMON_PLUS_PRIOR_TASK2
// #define TEST_SUPERCHANGES_ACC 1

double get_precision(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set);
double get_recall(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set);
double get_error(vector<IdSpread>& esti_set, vector<IdSpread>& truth_set);

int main() {
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};
    datasets["FACEBOOK"] = {"page_page"};
    datasets["ZIPF"] = {"zipf_0.9", "zipf_1.1", "zipf_1.3", "zipf_1.0"};
    for (size_t i = 8;i < 19;i++){
        datasets["CAIDA_SUB"].push_back(to_string(i));
    }

#ifdef TEST_PERFLOW_ACC
    string dataset = "MAWI";
    string filepath = "../../Couper/output/PerFlowSpread/" + dataset + "/";
    // string filepath = "../../vHLL/output/" + dataset + "/";
    // string filepath = "../../rerskt/output/" + dataset + "/";
    // string filepath = "../../bSkt/output/" + dataset + "/";

    vector<uint32_t> mems{1000};  //500, 750, 1000, 1250, 1500, 1750, 2000
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    // vector<uint32_t> mems{1500};
    // vector<double> layer1_ratios{0.6};  //{0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75}; //
    // for (size_t i = 0;i < layer1_ratios.size();i++){
    //     mems.push_back(layer1_ratios[i] * 1000);
    // }
    vector<double> AREs;
    vector<double> AAEs;
    vector<double> ratio_errors;
    for(auto tmpmem : mems){
        double ARE_sum = 0;
        double AAE_sum = 0;
        uint32_t filenum = 1; //datasets[dataset].size();//
        for (size_t i = 0;i < 1;i++){
            string filename = to_string(tmpmem) + "_" + datasets[dataset][i] + ".txt";
            ifstream ifile(filepath + filename);
            if(!ifile){
                cout<<"unable to open file"<<endl;
                return 0;
            }
            double total_relat_error = 0;
            double total_abs_error = 0;
            double total_ratio_error = 0;
            uint32_t num = 0;
            while(!ifile.eof()){
                string flowid;
                int true_spread;
                int estimated_spread;
                ifile >> flowid >> true_spread >> estimated_spread;
                // cout << flowid << "  " << true_spread << "  " << estimated_spread << endl;
                if (estimated_spread > 10000000){
                    return 0;
                }
                    // cout << flowid << endl;
                if (true_spread == 0)
                    continue;
                double tmp_ratio_error = (double)true_spread / estimated_spread;
                tmp_ratio_error = tmp_ratio_error >= 1 ? tmp_ratio_error : 1 / tmp_ratio_error;
                total_relat_error += fabs((double)true_spread - estimated_spread)/true_spread;
                total_abs_error += fabs((double)true_spread - estimated_spread);
                total_ratio_error += tmp_ratio_error;
                num++;
            }
            double ARE = total_relat_error/num;
            double AAE = total_abs_error/num;
            double RATIO_ERROR = total_ratio_error / num;
            ARE_sum += ARE;
            AAE_sum += AAE;
            // cout<<datasets[dataset][i]<<endl;
            // cout << "ARE: " << ARE << " ";
            // cout << "AAE: " << AAE << " ";
            // cout << ARE << " ";
            // cout  << AAE << " ";
            AREs.push_back(ARE);
            AAEs.push_back(AAE);
            ratio_errors.push_back(RATIO_ERROR);
        }
        // cout << tmpmem << " : average_ARE: " << ARE_sum/filenum << endl;
        // cout << tmpmem << " : average_AAE: " << AAE_sum/filenum << endl;
    }
    // cout<<endl;
    for(auto x : AREs)
        cout << x <<" ";
    cout<<endl;
    for(auto x : AAEs)
        cout << x <<" ";
    cout<<endl;
    for(auto x : ratio_errors)
        cout << x <<" ";
    cout<<endl;

#endif

#ifdef TEST_SUPERSPREADER_ACC
    unordered_map<string,vector<uint32_t>> thresholds;
    thresholds["MAWI"] = {20000, 3000, 1300};
    thresholds["CAIDA"] = {8000, 3400, 1800};
    thresholds["KAGGLE"] = {1000};

    string dataset = "MAWI";

    vector<uint32_t> mems{1000};  //500, 1000, 1500, 2000  512*1000
    // vector<uint32_t> mems{1000};
    for(uint32_t threshold : thresholds[dataset]){
        cout << "threshold: " << threshold << endl;
        for(auto tmpmem : mems){
            cout << "memory: " << tmpmem << "   ";
        
            string filename = datasets[dataset][0];
            ifstream ifile_truth("../../get_groundtruth/SuperSpreaders/"+dataset+"/"+ filename + ".txt");
            
            filename = to_string(tmpmem) + "_" + filename;  //+ "_0_" 
            ifstream ifile_esti("../../Couper/output/SuperSpreaders/"+dataset+ "/" + filename + ".txt");
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
            double tuned_threshold = threshold;
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

    vector<uint32_t> mems;//{2000, 4000, 8000, 16000};//{5000, 50000, 75000, 100000, 125000, 150000};
    for (int i = 0; i < 9;i++){
        mems.push_back(1000 * pow(2, i-1));
        cout << mems[i] <<" ";
    }
    // cout << endl;
    for(uint32_t threshold : thresholds[dataset]){
        cout << "threshold: " << threshold << endl;
        unordered_map<string, vector<double>> acc;

        for(auto tmpmem : mems){
            // cout << "memory: " << tmpmem << endl;
        
            ifstream ifile_truth("../../get_groundtruth/SuperChanges/" + dataset + ".txt");
            
            string filename = to_string(tmpmem) + "_" + dataset;
            ifstream ifile_esti("../../Couper/output/SuperChanges/"+ dataset+ "/" + filename + ".txt"); 
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
            double tuned_threshold = threshold;
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
            // cout<<"Precision: "<<prec<<"   Recall: "<<recall <<"   F1_Score: "<<F1_Score << "   Error: " << rel_error << endl;
            acc["precisions"].push_back(prec);
            acc["recalls"].push_back(recall);
            acc["f1scores"].push_back(F1_Score);
            acc["errors"].push_back(rel_error);
        }

        for (auto metric : acc){
            cout << metric.first << ": ";
            for (double val : metric.second){
                cout << val << " ";
            }
            cout << endl;
        }
    }
#endif

#ifdef TEST_CARMON_PLUS_PRIOR_TASK1
    string dataset = "FACEBOOK";

    uint32_t mem(1000);
    vector<double> cm_ratios{0.05, 0.1, 0.15, 0.2, 0.25, 0.30, 0.35, 0.4, 0.45, 0.5, 0.55, 0.60};  //0.05, 0.1, 0.15, 0.2, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9
    for(auto cm_ratio : cm_ratios){
        // cout << "cm_ratio: " << cm_ratio << "   ";
        string filename = datasets[dataset][0];
        filename = to_string(mem)+ "_" + to_string(cm_ratio).substr(0,4) + "_" + filename + ".txt";
        // cout<<filename<<endl;
        // string filepath = "../../Couper/output/PerFlowSpread/" + dataset + "/";
        string filepath = "../../vHLL/output/" + dataset + "/";
        // string filepath = "../../rerskt/output/" + dataset + "/";
        // string filepath = "../../bSkt/output/" + dataset + "/";
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
        // cout<<datasets[dataset][i]<<endl;
        cout << ARE <<" ";
    }
    cout << endl;
#endif

#ifdef TEST_CARMON_PLUS_PRIOR_TASK2
    unordered_map<string,vector<uint32_t>> thresholds;
    thresholds["MAWI"] = {20000, 3000, 1300};
    thresholds["CAIDA"] = {8000, 3400, 1800};
    thresholds["KAGGLE"] = {1000};

    string dataset = "CAIDA";

    uint32_t mem(30000);
    vector<uint32_t> cm_mems;
    for (size_t i = 1;i < 13;i++){
        cm_mems.push_back(i * 50);
    }

    for(uint32_t threshold : thresholds[dataset]){
        cout << "threshold: " << threshold << endl;
        unordered_map<string, vector<double>> acc;

        for(uint32_t cm_mem : cm_mems){
            cout << "Couper memory: " << cm_mem << endl;
            string filename = datasets[dataset][0];
            ifstream ifile_truth("../../get_groundtruth/SuperSpreaders/"+dataset+"/"+ filename + ".txt");

            string esti_filename = to_string(mem)+ "_" + to_string(cm_mem).substr(0,4) + "_" + filename;
            // ifstream ifile_esti("../../Couper/output/SuperSpreaders/"+dataset+ "/" + filename + ".txt");
            // ifstream ifile_esti("../../SpreadSketch/output/"+dataset+"/" + esti_filename + ".txt");  
            ifstream ifile_esti("../../Vector_BF/output/"+dataset+"/" + esti_filename + ".txt"); 
            // ifstream ifile_esti("../../DCS/output/"+dataset+"/" + filename + ".txt"); 
            // ifstream ifile_esti("../../CDS/output/SuperSpreaders/"+dataset+"/" + esti_filename + ".txt");

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
            double tuned_threshold = threshold;
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
            // cout<<"Precision: "<<prec<<"   Recall: "<<recall <<"   F1_Score: "<<F1_Score << "   Error: " << rel_error << endl;
            acc["precisions"].push_back(prec);
            acc["recalls"].push_back(recall);
            acc["f1scores"].push_back(F1_Score);
            acc["errors"].push_back(rel_error);
        }
        for (auto metric : acc){
            cout << metric.first << ": ";
            for (double val : metric.second){
                cout << val << " ";
            }
            cout << endl;
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