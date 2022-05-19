#include "Vector_BF.h"
#include "mylibpcap.h"
#include <ctime>
#include <unordered_map>

void write_res(string dataset, string filename, vector<IdSpread>* superspreaders, uint32_t tmpmem, double cmratio);

int main()
{
    string dataset = "MAWI";
    // vector<uint32_t> mems{500, 750, 1000, 1250, 1500, 1750, 2000};
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    // vector<uint32_t> mems{2000};
    uint32_t mem = 30000;
    vector<double> cm_ratios{0.1}; //0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9   0, 0.01, 0.02, 0.03,
    for(auto cmratio : cm_ratios){
        cout << "CarMon ratio: " << cmratio << endl;
        uint32_t filenum = 1;
        for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
            Vector_Bloom_Filter* vbf = new Vector_Bloom_Filter(mem, cmratio);
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                array<uint8_t,4> srcip_tuple;
                for(size_t i = 0;i < 4;i++){
                    srcip_tuple[i] = atoi(flowID.substr(i*3,3).c_str());
                }            
                vbf->process_packet(flowID,srcip_tuple,elemID);

                // if (filehandler.proc_num() == 10000000)
                //     break;
            }
            endTime = clock();
            cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

            uint32_t threshold = 1300;
            vbf->calc_Z(threshold);
            vector<IdSpread>* superspreaders = new vector<IdSpread>;
            
            startTime = clock();
            vbf->Detect_Superpoint(superspreaders);
            endTime = clock();
            cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_res(dataset, filehandler.get_filename(), superspreaders, mem, cmratio);
        }
    }
    return 0;
}

void write_res(string dataset, string filename, vector<IdSpread>* superspreaders, uint32_t tmpmem, double cmratio){
    string ofile_path = "../../Vector_BF/output/" + dataset + "/";
    ifstream ifile_hand;
    ofstream ofile_hand;
    ofile_hand = ofstream(ofile_path + to_string(tmpmem)+ "_" + to_string(cmratio).substr(0,4) + "_" + filename + ".txt");
    if(!ofile_hand){
        cout<<"fail to open files."<<endl;
        return;
    }
    bool first_line = true;
    for(auto item : *superspreaders){
        if(first_line)
            first_line = false;
        else
            ofile_hand << endl;
        ofile_hand << item.flowID << " " << item.spread;
    }
    ofile_hand.close();
}