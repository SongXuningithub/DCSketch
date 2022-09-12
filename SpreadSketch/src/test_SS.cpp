#include "SS.h"
#include "mylibpcap.h"
#include <fstream>
#include <unordered_map>

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, uint32_t cm_mem);

int main()
{
    string dataset = "MAWI"; 

    uint32_t mem = 500;
    vector<double> cm_mems{100, 200}; // 50
    // for (size_t i = 0;i < 11;i++){
    //     cm_mems.push_back(100 + i * 50);
    // }

    for(auto cm_mem : cm_mems){
        cout << "CarMon memory: " << cm_mem << endl;
        uint32_t filenum = 1;
        for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
            double cm_ratio = cm_mem/mem;
            cout << cm_ratio <<endl;
            SpreadSketch ss(mem, cm_ratio);
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                ss.update(flowID, elemID);
            }
            endTime = clock();
            cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
        
            vector<IdSpread> superspreaders;
            startTime = clock();
            ss.report_superspreaders(superspreaders);
            endTime = clock();
            cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_res(dataset, filehandler.get_filename(), superspreaders, mem, cm_mem);
        }
    }
    return 0;
}

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, uint32_t cm_mem){
    string ofile_path = "../../SpreadSketch/output/" + dataset + "/";
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