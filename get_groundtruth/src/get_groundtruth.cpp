//#include "hash_table.h"
#include "mylibpcap.h"
#include "util.h"
#include <unordered_map>
#include <string>
#include <fstream>
#include <set>
#include <vector>
#include <random>
#include <algorithm>
using namespace std;

#define PER_FLOW_INFO 1
// #define SUPER_SPREADERS 1
// #define SUPER_CHANGES 1
// #define GEN_SUB_TRACES 1

bool per_src_flow = true;

int main()
{
#ifdef PER_FLOW_INFO
    unordered_map<string,set<string>> truth;
    string dataset = "CAIDA_SUB";
    uint32_t filenum = 11;
    for (size_t i = 0;i < filenum; i++){
        FILE_HANDLER filehandler(dataset, i);

        string flowid, elemID;
        while(int status = filehandler.get_item(flowid, elemID)){
            // cout << flowid << "  " << elemID << endl;
            truth[flowid].insert(elemID);
            if(filehandler.proc_num()%1000000 == 0){
                cout<<"have processed "<<filehandler.proc_num()<<" items."<<endl;
            }
        }

        ofstream truthfile;
        ofstream truthglobal_file;
        truthfile.open("../../get_groundtruth/truth/"+ dataset + "/" + filehandler.get_filename() + ".txt",ios::binary);
        truthglobal_file.open("../../get_groundtruth/truth/"+ dataset + "/" + filehandler.get_filename() + "_global.txt",ios::binary);
        
        if(!truthfile || !truthglobal_file){
            cout<<"fail to open"<<endl;
            return -1;
        }
        uint32_t bigger_16 = 0;
        uint32_t bigger_64 = 0;
        uint32_t bigger_200 = 0;
        uint32_t bigger_1000 = 0;
        uint32_t biggest_spread = 0;
        bool first_line = true;
        for(auto iter = truth.begin();iter!=truth.end();iter++){
            if(first_line)
                first_line = false;
            else 
                truthfile << endl;
            uint32_t cur_spread = iter->second.size();
            if(cur_spread>16){
                if(cur_spread>1000)
                    bigger_1000++;
                if(cur_spread>200)
                    bigger_200++;    
                if(cur_spread>64)
                    bigger_64++; 
                bigger_16++;
                truthglobal_file << iter->first<<"  "<<cur_spread<<endl;
                if(cur_spread > biggest_spread)
                    biggest_spread = cur_spread;
            }
            truthfile << iter->first<<"  "<<cur_spread;
        }
        truthglobal_file<<"total items: "<<filehandler.proc_num()<<endl;
        truthglobal_file<<"number of flows: "<<truth.size()<<endl;
        truthglobal_file<<"flows with spread bigger than 16: "<<bigger_16<<endl;
        truthglobal_file<<"flows with spread bigger than 64: "<<bigger_64<<endl;
        truthglobal_file<<"flows with spread bigger than 200: "<<bigger_200<<endl;
        truthglobal_file<<"flows with spread bigger than 1000: "<<bigger_1000<<endl;
        truthglobal_file<<"biggest spread: "<<biggest_spread<<endl;
        truthfile.close();
        truthglobal_file.close();
    }
    
#endif

#ifdef SUPER_SPREADERS
    string dataset = "FACEBOOK";
    // string filename = "pkts_frag_00002";
    // string filename = "5M_frag (2)";
    string filename = "page_page";
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    ifstream ifile_hand;
    ifile_hand = ifstream(ifile_path + filename + "_global.txt");
    if(!ifile_hand){
        cout<<"fail to open files."<<endl;
        return 0;
    }

    vector<IdSpread> ssCandidates;
    while(!ifile_hand.eof()){
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        if(flowid == "total")
            break;
        ifile_hand >> spread;
        if(spread > 200)
            ssCandidates.push_back(IdSpread(flowid,spread));
    }
    ifile_hand.close();
    
    sort(ssCandidates.begin(),ssCandidates.end(),IdSpreadComp); 

    ofstream truthfile;
    truthfile.open("../../get_groundtruth/SuperSpreaders/" + dataset + "/" + filename+".txt",ios::binary);
    if(!truthfile){
        cout<<"fail to open"<<endl;
        return -1;
    }
    bool first_line = true;
    size_t trav_num = ssCandidates.size() < 500 ? ssCandidates.size() : 500; 
    for(size_t i = 0;i < trav_num;i++) {
        if(first_line)
            first_line = false;
        else 
            truthfile << endl;
        truthfile << ssCandidates[i].flowID << " " << ssCandidates[i].spread;
    }
    truthfile.close();
#endif

#ifdef SUPER_CHANGES
    datasets["KAGGLE"] = {"Unicauca_1", "Unicauca_2"};
    string dataset = "KAGGLE";
    int scthresh = 100;
    
    unordered_map<string,uint32_t> kv1;
    unordered_map<string,uint32_t> kv2;
    
    string filename = datasets[dataset][0];
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    ifstream ifile_hand;
    ifile_hand = ifstream(ifile_path + filename + "_global.txt");
    if(!ifile_hand){
        cout<<"fail to open files."<<endl;
        return 0;
    }
    while(!ifile_hand.eof()){
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        if(flowid[0]>'9' || flowid[0]<'0')
            break;
        ifile_hand >> spread;
        kv1[flowid] = spread;
    }
    ifile_hand.close();

    filename = datasets[dataset][1];
    ifile_hand = ifstream(ifile_path + filename + "_global.txt");
    if(!ifile_hand){
        cout<<"fail to open files."<<endl;
        return 0;
    }
    while(!ifile_hand.eof()){
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        if(flowid[0]>'9' || flowid[0]<'0')
            break;
        ifile_hand >> spread;
        kv2[flowid] = spread;
    }
    ifile_hand.close();

    unordered_map<string,uint32_t> superchanges;
    for(auto iter : kv1){
        int epoch1_val = iter.second;
        if (epoch1_val >= scthresh){
            if (kv2.find(iter.first) != kv2.end()){
                int epoch2_val = kv2[iter.first];
                int change_val = abs(epoch1_val - epoch2_val);
                if (change_val >= scthresh){
                    superchanges[iter.first] = change_val;
                }
            } else {
                superchanges[iter.first] = epoch1_val;
            }
        }
    }

    for(auto iter : kv2){
        int epoch2_val = iter.second;
        if (epoch2_val >= scthresh){
            if (kv1.find(iter.first) != kv1.end()){
                int epoch1_val = kv1[iter.first];
                int change_val = abs(epoch1_val - epoch2_val);
                if (change_val >= scthresh){
                    superchanges[iter.first] = change_val;
                }
            } else {
                superchanges[iter.first] = epoch2_val;
            }
        }
    }

    vector<IdSpread> FinalOutputs;
    for(auto iter : superchanges)
        FinalOutputs.push_back(IdSpread(iter.first, iter.second)); 
    sort(FinalOutputs.begin(), FinalOutputs.end(), IdSpreadComp);

    ofstream truthfile;
    truthfile.open("../../get_groundtruth/SuperChanges/" + dataset + ".txt",ios::binary);
    if(!truthfile){
        cout<<"fail to open"<<endl;
        return -1;
    }
    bool first_line = true;
    for(auto iter : FinalOutputs){
        if(first_line)
            first_line = false;
        else 
            truthfile << endl;
        truthfile << iter.flowID << " " << iter.spread;
    }
    truthfile.close();
#endif

#ifdef GEN_SUB_TRACES
    string dataset = "CAIDA";
    unordered_map<string,set<string>> truth;
    FILE_HANDLER filehandler(dataset, 0);
    string flowID, elemID;
    while(int status = filehandler.get_item(flowID, elemID)){
        truth[flowID].insert(elemID);
        if(filehandler.proc_num()%1000000 == 0){
            cout<<"have processed "<<filehandler.proc_num()<<" items."<<endl;
        }
    }
    vector<string> keys;
    for(auto flow = truth.begin();flow != truth.end();flow++){
        keys.push_back(flow->first);
    }
    
    // vector<uint32_t> v(truth.size());
    // for (size_t i = 0;i < v.size();i++)
    //     v[i] = i;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    for (size_t i = 8;i < 19;i++){
        uint32_t flow_num = 1 << i;
        ofstream sub_trace;
        sub_trace.open("../../get_groundtruth/sub_traces/"+ dataset + "/" + to_string(i) + ".txt", ios::binary);
         if(!sub_trace){
            cout<<"fail to open"<<endl;
            return -1;
        }
        for(size_t j = 0;j < flow_num;j++){
            string key = keys[j];
            for (auto iter = truth[key].begin();iter != truth[key].end();iter++){
                string val = *iter;
                sub_trace << key <<"  "<< val << endl;
            }
        }
        sub_trace.close();
    }
#endif

    return 0;
}

