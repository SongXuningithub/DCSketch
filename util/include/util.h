#ifndef _UTILITY_H_
#define _UTILITY_H_

#include<iostream>
#include<sstream>
#include<fstream>
#include<bitset>
#include<string>
#include<unordered_map>
#include<vector>
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
    this->dataset = dataset;

    for (size_t i = 8;i < 19;i++){
        datasets["CAIDA_SUB"].push_back(to_string(i));
    }

    if(datasets[dataset].size() <= file_idx){
        cout << "file doesn't exist." <<endl;
        return;
    }
    filename = datasets[dataset][file_idx];

    if (dataset == "FACEBOOK" || dataset == "CAIDA_SUB"){
        txt_handler = new TXT_Handler(data_path, dataset, datasets[dataset][file_idx]);
    } else {
        if (dataset == "KAGGLE")
            pcap_handler = new PCAP_SESSION(data_path, dataset, datasets[dataset][file_idx], CSV_FILE);
        else
            pcap_handler = new PCAP_SESSION(data_path, dataset, datasets[dataset][file_idx], PCAP_FILE);
    }

}

int FILE_HANDLER::get_item(string& flowID, string& elemID){
    int status;
    if (dataset == "FACEBOOK" || dataset == "CAIDA_SUB"){
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

#endif