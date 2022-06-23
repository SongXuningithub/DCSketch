#ifndef _UTILITY_H_
#define _UTILITY_H_

#include<iostream>
#include<sstream>
#include<fstream>
#include<bitset>
#include<string>
#include<unordered_map>
#include<vector>
#include<cmath>
#include"mylibpcap.h"

using namespace std;

struct IdSpread{
public:
    string flowID;
    uint32_t spread;
    IdSpread(string str,uint32_t s){flowID = str; spread = s;}
};

bool IdSpreadComp(IdSpread& a, IdSpread& b){
    return a.spread > b.spread;
}

string Uint32toIPstr(uint32_t val){
    string ret = "";
    for(size_t i = 0;i < 4;i++){
        uint8_t tmpval = (val >> (i * 8)) & 255;
        string tmpstr = to_string(tmpval);
        ret = (string(3 - tmpstr.length(), '0') + tmpstr) + ret;
    }
    return ret;
}

uint32_t IPstrtoUint32(string IPstr){
    uint32_t ret = 0;
    for(size_t i = 0;i < 4;i++){
        uint32_t tmp = stoi(IPstr.substr(i*3,3));
        ret = (ret << 8) + tmp;
    }
    return ret;
}

inline uint32_t get_one_num(uint8_t val){
    bitset<8> tmp(val);
    return tmp.count();
}

uint32_t get_leading_zeros(uint32_t bitstr){
    uint32_t tmp = 2147483648;   //1<<31
    for(size_t i = 0;i < 32;i++){
        if((bitstr & tmp) != 0)
            return i;
        tmp >>= 1;
    }
    return 32;
}

class TXT_Handler{
private:
    ifstream txtfile;
    bool eof_flag = false;
    uint32_t pkt_num = 0;
public:
    TXT_Handler(string data_path, string dataset, string filename);
    int get_packet(string& flowID, string& elemID);
};

TXT_Handler::TXT_Handler(string data_path, string dataset, string filename){
    string txtfile_name = data_path + dataset + "/" + filename + ".txt";
    txtfile.open(txtfile_name);
}

int TXT_Handler::get_packet(string& flowID, string& elemID){
    string linedata;
    getline(txtfile, linedata);
    if(txtfile.eof() || linedata==""){
        printf("Pcap file parse over !\n");
        eof_flag = true;  
        cout<<" pkt num:"<<pkt_num<<endl;
        txtfile.close();
        return 0;
    }
    pkt_num++;
    flowID = linedata.substr(0, linedata.find(' '));
    elemID = linedata.substr(linedata.find(' ')+1);
    return 1;
}

class FILE_HANDLER{
private:
    string data_path = "/home/xun/dataset/";
    PCAP_SESSION* pcap_handler;
    TXT_Handler* txt_handler;
    string dataset;
    string filename;
    uint32_t item_num = 0;

public:
    FILE_HANDLER(string dataset, uint32_t filenum);
    int get_item(string& flowID, string& elemID);
    uint32_t proc_num();
    string get_filename();
};

FILE_HANDLER::FILE_HANDLER(string dataset, uint32_t file_idx){
    unordered_map<string,vector<string>> datasets;
    datasets["MAWI"] = {"pkts_frag_00001", "pkts_frag_00002"};
    datasets["CAIDA"] = {"5M_frag (1)", "5M_frag (2)", "5M_frag (3)", "5M_frag (4)", "5M_frag (5)"};
    datasets["KAGGLE"] = {"Unicauca"};
    datasets["FACEBOOK"] = {"page_page"};
    datasets["TWITTER"] = {"twitter_combined", "higgs-social_network"};
    datasets["ZIPF"] = {"zipf_0.9", "zipf_1.1", "zipf_1.3"};

    this->dataset = dataset;

    for (size_t i = 8;i < 19;i++){
        datasets["CAIDA_SUB"].push_back(to_string(i));
    }

    if(datasets[dataset].size() <= file_idx){
        cout << "file doesn't exist." <<endl;
        return;
    }
    filename = datasets[dataset][file_idx];

    if (dataset == "FACEBOOK" || dataset == "CAIDA_SUB" || dataset == "ZIPF"){
        txt_handler = new TXT_Handler(data_path, dataset, datasets[dataset][file_idx]);
    } else {
        if (dataset == "KAGGLE")
            pcap_handler = new PCAP_SESSION(data_path, dataset, datasets[dataset][file_idx], CSV_FILE);
        else
            pcap_handler = new PCAP_SESSION(data_path, dataset, datasets[dataset][file_idx], PCAP_FILE);
    }
    cout << this->get_filename() << endl;
}

int FILE_HANDLER::get_item(string& flowID, string& elemID){
    int status;
    if (dataset == "FACEBOOK" || dataset == "CAIDA_SUB" || dataset == "ZIPF"){
        status = txt_handler->get_packet(flowID, elemID);
    } else {
        IP_PACKET cur_packet;
        status = pcap_handler->get_packet(cur_packet);
        flowID = cur_packet.get_srcip();
        elemID = cur_packet.get_dstip();
        if(dataset == "CAIDA"){
            swap(flowID, elemID);
        }
    }
    if (status != 0)
        item_num++;
    return status;
}

uint32_t FILE_HANDLER::proc_num(){
    return item_num;
}

string FILE_HANDLER::get_filename(){
    return filename;
}

template <class Framework>
void write_perflow_spread(string dataset, string filename, string ofile_path, Framework& sketch, uint32_t tmpmem);

template <class Framework>
void Test_task1(Framework not_used, string ofile_path, double CarMon_Layer1_ratio){
    string dataset = "MAWI";
    vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000}; //500, 750, 1000, 1250, 1500, 1750, 2000
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;
        uint32_t filenum = 2;
        for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
            Framework sketch(tmpmem, CarMon_Layer1_ratio);
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            clock_t startTime, endTime;

            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                sketch.process_packet(flowID, elemID);
                // if(filehandler.proc_num()%1000000 == 0)
                //     cout<<"process packet "<<filehandler.proc_num()<<endl;
            }
            endTime = clock();
            cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_perflow_spread(dataset, filehandler.get_filename(), ofile_path, sketch, tmpmem);
        }   
    }
}

template <class Framework>
void write_perflow_spread(string dataset, string filename, string ofile_path, Framework& sketch, uint32_t tmpmem){
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ifile_hand = ifstream(ifile_path + filename + ".txt");
    ofile_hand = ofstream(ofile_path + "/" + dataset + "/" + to_string(tmpmem) + "_" + filename + ".txt");
       
    if(!ifile_hand || !ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    // clock_t startTime,endTime;
    // startTime = clock();
    bool first_line = true;
    while(!ifile_hand.eof()){
        if(first_line)
            first_line = false;
        else 
            ofile_hand << endl;
        string flowid;
        uint32_t spread;
        ifile_hand >> flowid;
        ifile_hand >> spread;
        uint32_t estimated_spread = sketch.get_flow_spread(flowid);
        ofile_hand << flowid <<" "<<spread<<" "<<estimated_spread;
    }
    // endTime = clock();
    // cout << "The query time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
    ifile_hand.close();
    ofile_hand.close();
}

void write_superspreaders(string dataset, string ofile_path, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem);

template <class Framework>
void Test_task2(Framework not_used, string ofile_path, double CarMon_Layer1_ratio){
    vector<string> datasets{"MAWI", "CAIDA"};
    vector<uint32_t> mems{500,  1000, 1500, 2000}; //500, 750, 1000, 1250, 1500, 1750, 2000
    for (string dataset : datasets){
        for(auto tmpmem : mems){
            cout << "memory: " << tmpmem << endl;
            uint32_t filenum = 1;
            for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
                Framework sketch(tmpmem, CarMon_Layer1_ratio);
                FILE_HANDLER filehandler(dataset, i);
                string flowID, elemID;
                clock_t startTime, endTime;

                startTime = clock();
                while(int status = filehandler.get_item(flowID, elemID)){
                    sketch.process_packet(flowID, elemID);
                    // if(filehandler.proc_num()%1000000 == 0)
                    //     cout<<"process packet "<<filehandler.proc_num()<<endl;
                }
                endTime = clock();
                cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

                vector<IdSpread> superspreaders;
                startTime = clock();
                sketch.report_superspreaders(superspreaders);
                endTime = clock();
                cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
                write_superspreaders(dataset, ofile_path, filehandler.get_filename(), superspreaders, tmpmem);
            }   
        }
    }
}

void write_superspreaders(string dataset, string ofile_path, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem){
    ofstream ofile_hand;
    ofile_path = ofile_path + "/" + dataset + "/";
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" +  filename + ".txt");
    if(!ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    bool first_line = true;
    for(auto item : superspreaders){
        if(first_line)
            first_line = false;
        else
            ofile_hand << endl;
        ofile_hand << item.flowID << " " << item.spread;
    }
    ofile_hand.close();
}

template <class Framework>
void Get_Mem(Framework not_use){
    string dataset = "CAIDA_SUB";
    uint32_t mem_base = 1000;
    double expo = 2.0;

    uint32_t filenum = 11;
    vector<double> expos(filenum);
    for (size_t i = 8; i < filenum; i++){
        if(i>0 && expos[i-1] >= 2.05){
            expos[i] = expo;
            cout << i << " " << expo << endl;
            continue;
        }
        while(true){
            double cmratio = 0;
            // cout << "CarMon ratio: " << cmratio << endl;

            double tmp_mem = mem_base * pow(10.0, expo);
            Framework sketch(tmp_mem, cmratio);
            // Framework sketch(tmp_mem, 0.6);
            // cout << "expo: " << expo <<"  tmp_mem: " << tmp_mem << endl;
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                sketch.process_packet(flowID, elemID);
                // if(filehandler.proc_num()%10000000 == 0){
                //     cout<<"process packet "<<filehandler.proc_num()<<endl;
                // }
            }
            endTime = clock();
            // cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            bool achieve = Check_Acc(dataset, filehandler.get_filename(), sketch, tmp_mem, cmratio, 0.5);
            if (achieve){
                expos[i] = expo;
                cout << i << " " << expo << endl;
                break;
            } else {
                expo += 0.05;
            }
        }
    }
    for(size_t i = 0;i < filenum;i++){
        cout << to_string(expos[i]).substr(0, 5) << " ";
    }
    cout << endl;
    return;
}

template <class Framework>  
bool Check_Acc(string dataset, string filename, Framework& sketch, uint32_t tmpmem, double cmratio, double acc_requirement){
    string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    ifstream ifile_hand;

    ifile_hand = ifstream(ifile_path + filename + ".txt");
    if(!ifile_hand){ 
        cout<<"fail to open files."<<endl;
        return false;
    }
    clock_t startTime,endTime;
    startTime = clock();
    bool first_line = true;
    double relat_error = 0;
    double num = 0;

    while(!ifile_hand.eof()){
        string flowid;
        int spread;
        ifile_hand >> flowid;
        ifile_hand >> spread;
        int estimated_spread = sketch.get_flow_spread(flowid);
        relat_error += fabs((double)spread - estimated_spread)/spread;
        num++;
    }
    endTime = clock();
    ifile_hand.close();

    double ARE = relat_error/num;
    if(ARE <= acc_requirement)
        return true;
    else
        return false;
}


#endif