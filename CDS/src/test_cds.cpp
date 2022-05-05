#include "cds.h"
#include "mylibpcap.h"
#include <ctime>
#include <unordered_map>

#define DETECT_SUPERSPREADERS 1
// #define DETECT_SUPERCHANGES 1


void write_ss(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, double cmratio);
void write_sc(string dataset, vector<IdSpread>& superchanges, uint32_t tmpmem);

int main()
{
    string dataset = "MAWI";

#ifdef DETECT_SUPERSPREADERS
    uint32_t mem = 30000;
    vector<double> cm_ratios{0, 0.01, 0.02, 0.03, 0.04, 0.05}; //0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9
    for(auto cmratio : cm_ratios){
        cout << "CarMon ratio: " << cmratio << endl;
        uint32_t filenum = 1;
        for (size_t i = 0; i < filenum; i++){  //datasets[dataset].size()
            CDS* cds = new CDS(mem, cmratio);
            FILE_HANDLER filehandler(dataset, i);
            string flowID, elemID;
            clock_t startTime,endTime;
            startTime = clock();
            while(int status = filehandler.get_item(flowID, elemID)){
                cds->update(flowID, elemID);
                // if(filehandler.proc_num() == 1e6)
                //     break;
            }
            endTime = clock();
            cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;

            vector<IdSpread> superspreaders;
            startTime = clock();
            cds->DetectSuperSpreaders(superspreaders);
            endTime = clock();
            cout << "The resolution time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
            write_ss(dataset, filehandler.get_filename(), superspreaders, mem, cmratio);
        }
    }
    
    return 0;
#endif

#ifdef DETECT_SUPERCHANGES
    // vector<uint32_t> mems{500, 1000, 1500, 2000};
    vector<uint32_t> mems{2000};
    for(auto tmpmem : mems){
        cout << "memory: " << tmpmem << endl;

        CDS cds1(tmpmem);
        string filename = datasets[dataset][0];
        PCAP_SESSION session1(dataset,filename,PCAP_FILE);
        IP_PACKET cur_packet;
        string srcip,dstip;

        while(int status = session1.get_packet(cur_packet)){
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if(per_src_flow)
                cds1.update(srcip,dstip);
            else
                cds1.update(dstip,srcip);
            /**/
            if(session1.proc_num() == 1788648)
                break;
            /**/
        }

        CDS cds2(tmpmem);
        filename = datasets[dataset][0]; /*KAGGLE*/
        PCAP_SESSION session2(dataset,filename,PCAP_FILE);

        while(int status = session2.get_packet(cur_packet)){
            /**/
            if(session2.proc_num() <= 1788648)
                continue;
            /**/
            srcip = cur_packet.get_srcip();
            dstip = cur_packet.get_dstip();
            if(per_src_flow)
                cds2.update(srcip,dstip);
            else
                cds2.update(dstip,srcip);
        }

        vector<IdSpread> superchanges;
        cds2.DetectSuperChanges(cds1, superchanges);
        sort(superchanges.begin(), superchanges.end(), IdSpreadComp);
        write_sc(dataset, superchanges, tmpmem);
    
    }
    return 0;

#endif
}

void write_ss(string dataset, string filename, vector<IdSpread>& superspreaders, uint32_t tmpmem, double cmratio){
    //string ifile_path = "../../get_groundtruth/truth/" + dataset + "/";
    string ofile_path = "../../CDS/output/SuperSpreaders/" + dataset + "/";
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