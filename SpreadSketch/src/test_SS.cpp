#include "SS.h"
#include "mylibpcap.h"
#include <fstream>
#include <unordered_map>

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, double cmratio);

int main()
{
    string dataset = "MAWI"; 

    uint32_t mem = 1000;
    vector<double> cm_ratios{0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9}; //
    for(auto cmratio : cm_ratios){
        cout << "CarMon ratio: " << cmratio << endl;
        uint32_t filenum = 1;
        for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
            SpreadSketch ss(mem, cmratio);
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
            ss.output_superspreaders(superspreaders);
            endTime = clock();
            cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_res(dataset, filehandler.get_filename(), superspreaders, mem, cmratio);
        }
    }
    return 0;
}

// string trans2ipformat(uint32_t val){
//     string ret = "";
//     for(size_t i = 0;i < 4;i++){
//         val = val >> (8 * i);
//         string tmp = to_string(val & 255);
//         while(tmp.size() != 3){
//             tmp = "0" + tmp;
//         }
//         ret = tmp + ret; 
//     }
//     return ret;
// }

void write_res(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, double cmratio){
    string ofile_path = "../../SpreadSketch/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + to_string(cmratio).substr(0,4) + "_" + filename + ".txt");
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