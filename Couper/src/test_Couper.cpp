#include "Couper.h"
#include "mylibpcap.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <set>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unistd.h>
using std::unique_ptr;

#define OUTPUT_PERFLOW_SPREAD 1
// #define OUTPUT_SUPER_SPREADERS 1

int main(){
    Couper couper_notuse(200, 0.6);
    string dataset = "FACEBOOK";
    uint32_t init_mem = 500;
    Get_Mem(couper_notuse, dataset, init_mem);

    // Couper not_used(1000, 0.5);
    
    // string ofile_path = "../../Couper/output/PerFlowSpread";
    // vector<double> layer1_ratios{0.5}; //{0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75};  //0.5, 0.55, 0.6, 0.65, 0.7, 0.75
    // for (size_t i = 0;i < layer1_ratios.size();i++){
    //     Test_task1(not_used, ofile_path, layer1_ratios[i]);
    // }

    // string ofile_path = "../../Couper/output/SuperSpreaders";   // SS_P4
    // Test_task2(not_used, ofile_path, 0.6);

    return 0;
}