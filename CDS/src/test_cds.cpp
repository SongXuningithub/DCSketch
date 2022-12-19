#include "cds.h"
#include "mylibpcap.h"
#include <ctime>
#include <unordered_map>

#define DETECT_SUPERSPREADERS 1
// #define DETECT_SUPERCHANGES 1


void write_ss(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, uint32_t cm_mem);
void write_sc(string dataset, vector<IdSpread>& superchanges, uint32_t tmpmem);

int main()
{
    string dataset = "MAWI";

#ifdef DETECT_SUPERSPREADERS
    uint32_t mem = 512*1000;  //256
    vector<double> cm_mems;
    for (size_t i = 1;i < 5;i++){
        cm_mems.push_back(i * 50);
    }
    
    for(auto cm_mem : cm_mems){
        cout << "Couper memory: " << cm_mem << endl;
        double cm_ratio = (double)cm_mem/mem;
        cout << cm_ratio <<endl;
        uint32_t filenum = 1;
        for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
            CDS* cds = new CDS(mem, cm_ratio);
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                cds->process_packet(flowID, elemID);
            }
            endTime = clock();
            cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

            vector<IdSpread> superspreaders;
            startTime = clock();
            cds->DetectSuperSpreaders(superspreaders);
            endTime = clock();
            cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_ss(dataset, filehandler.get_filename(), superspreaders, mem, cm_mem);
        }
    }
    
    return 0;
#endif

#ifdef DETECT_SUPERCHANGES
    vector<string> datasets{"MAWI"};
    for (string dataset:datasets){
        cout << dataset << endl;
        vector<uint32_t> mems{1024000};  //{75000, 100000, 125000, 150000}
        // cout << "memories: ";
        // for (size_t i = 0; i < 9;i++){
        //     mems.push_back(1000 * pow(2, i));
        //     cout << mems[i] << " ";
        // }
        // cout << endl;
        vector<double> times;
        for(auto tmpmem : mems){
            cout << "memory: " << tmpmem << endl;
            string flowID, elemID;

            CDS cds1(tmpmem, 0);
            FILE_HANDLER filehandler1(dataset, 0);
            while(int status = filehandler1.get_item(flowID, elemID)){
                cds1.process_packet(flowID, elemID);
            }

            CDS cds2(tmpmem, 0);
            FILE_HANDLER filehandler2(dataset, 1);
            while(int status = filehandler2.get_item(flowID, elemID)){
                cds2.process_packet(flowID, elemID);
            }

            clock_t startTime, endTime;
            startTime = clock();
            vector<IdSpread> superchanges;
            cds2.DetectSuperChanges(cds1, superchanges);
            endTime = clock();
            // cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            times.push_back((double)(endTime - startTime) / CLOCKS_PER_SEC);
            sort(superchanges.begin(), superchanges.end(), IdSpreadComp);
            write_sc(dataset, superchanges, tmpmem);
        }
        cout <<"Recovery times: ";
        for (double tmp : times){
            cout << tmp << "s ";
        }
        cout << endl;
    } 
    return 0;

#endif
}

void write_ss(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, uint32_t cm_mem){
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../CDS/output/SuperSpreaders/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + to_string(cm_mem).substr(0,4) + "_" + filename + ".txt");
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

void write_sc(string dataset, vector<IdSpread>& superchanges, uint32_t tmpmem){
    string ofile_path = "../../CDS/output/SuperChanges/" + dataset + "/";
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + dataset + ".txt");
    if(!ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    bool first_line = true;
    for(auto item : superchanges){
        if(first_line)
            first_line = false;
        else
            ofile_hand << endl;
        ofile_hand << item.flowID << " " << item.spread;
    }
    ofile_hand.close();
}