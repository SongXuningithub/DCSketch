#include <iostream>
#include <set>
#include <memory>
#include <algorithm>
#include <unistd.h>
#include <fstream>
#include <cmath>
#include <string>
using namespace std;

#define TEST_PERFLOW_ACC 1
// #define TEST_SUPERSPREADER_ACC 1
// #define TEST_SUPERSPREADER_ERROR 1

double get_precision(set<string>& esti_set,set<string>& truth_set);
double get_recall(set<string>& esti_set,set<string>& truth_set);
double get_error(set<pair<string,uint32_t>> esti_set, set<pair<string,uint32_t>> truth_set);

int main()
{
#ifdef TEST_PERFLOW_ACC
    string dataset = "MAWI";
    string filepath = "../../DCSketch/output/PerFlowSpread/" + dataset + "/";
    // string filepath = "../../rerskt/output/" + dataset + "/";
    // string filepath = "../../bSkt/output/" + dataset + "/";
    double ARE_sum = 0;
    double AAE_sum = 0;
    uint32_t filenum = 2;
    for (size_t i = 1;i <= filenum;i++)
    {
        // string filename = "5M_frag (" + to_string(i) + ").txt";
        string filename = "pkts_frag_0000" + to_string(i) + ".txt";
        // string filename = "Unicauca.txt";
        ifstream ifile(filepath + filename);
        //ifstream ifile("../../Distribution_Estimation/files/"+dataset+"/0000MLEres.txt");
        if(!ifile)
        {
            cout<<"unable to open file"<<endl;
            return 0;
        }
        double relat_error = 0;
        double abs_error = 0;
        uint32_t num = 0;
        while(!ifile.eof())
        {
            string flowid;
            uint32_t true_spread;
            uint32_t estimated_spread;
            ifile >> flowid >> true_spread >> estimated_spread;
            relat_error += fabs((double)true_spread - estimated_spread)/true_spread;
            abs_error += fabs((double)true_spread - estimated_spread);
            num++;
            //cout<< flowid <<" "<< true_spread << " "<<estimated_spread<<endl; 

            // if(true_spread > 21)
            // {
            //     bigflow_num++;
            //     bigflow_error += fabs((double)true_spread - estimated_spread);
            // }
        }
        double ARE = relat_error/num;
        double AAE = abs_error/num;
        ARE_sum += ARE;
        AAE_sum += AAE;
        cout<<"ARE: "<<ARE<<endl;
        cout<<"AAE: "<<AAE<<endl;
    }
    cout << "average_ARE: " << ARE_sum/filenum << endl;
    cout << "average_AAE: " << AAE_sum/filenum << endl;
#endif

#ifdef TEST_SUPERSPREADER_ACC

    string dataset = "MAWI";
    // string filename = "5M_frag (1)";
    string filename = "pkts_frag_00001";
    // string filename = "Unicauca";
    // ifstream ifile_esti("../../DCSketch/output/SuperSpreaders/"+dataset+ "/" + filename + ".txt");
    // ifstream ifile_esti("../../SpreadSketch/output/"+dataset+"/" + filename + ".txt");  
    ifstream ifile_esti("../../Vector_BF/output/"+dataset+"/" + filename + ".txt"); 
    // ifstream ifile_esti("../../bSkt/SuperSpreader/"+dataset+"/" + filename + ".txt");  
    ifstream ifile_truth("../../get_groundtruth/SuperSpreaders/"+dataset+"/"+ filename + ".txt");
    uint32_t threshold = 3000;
    //ifstream ifile("../../Distribution_Estimation/files/"+dataset+"/0000MLEres.txt");
    if(!ifile_truth || !ifile_esti)
    {
        cout<<"unable to open file"<<endl;
        return 0;
    }
    set<string> superspreaders;
    while(!ifile_truth.eof())
    {
        string flowid;
        uint32_t true_spread;
        ifile_truth >> flowid >> true_spread;
        if (true_spread >= threshold)
        {
            superspreaders.insert(flowid);
        }
    }
    set<string> superspreaders_esti;
    double tuned_threshold = threshold;// * (1 - 0.5 * 0.1856);
    while(!ifile_esti.eof())
    {
        string flowid;
        uint32_t esti_spread;
        ifile_esti >> flowid >> esti_spread;
        if(esti_spread > tuned_threshold)
            superspreaders_esti.insert(flowid);
        else
            break;
    }
    ifile_esti.close();
    ifile_truth.close();
    double prec = get_precision(superspreaders_esti, superspreaders);
    double recall = get_recall(superspreaders_esti, superspreaders);
    double F1_Score = 2 * prec * recall / (prec + recall);
    cout<<"precision: "<<prec<<" ";
    cout<<"recall: "<<recall <<" F1_Score: "<<F1_Score<<endl;
    
#endif

#ifdef TEST_SUPERSPREADER_ERROR
    string dataset = "MAWI";
    // string filename = "5M_frag (1)";
    string filename = "pkts_frag_00001";
    // string filename = "Unicauca";
    array<uint32_t,3> CAIDA_threshs{8000,3400,1800};
    array<uint32_t,3> MAWI_threshs{20000,3000,1300};
    array<uint32_t,1> KAGGLE_threshs{1000};
    uint32_t threshold;

    for(size_t i = 0;i < 1;i++)
    {
        if(dataset == "CAIDA")
            threshold = CAIDA_threshs[i];
        else if(dataset == "MAWI")
            threshold = MAWI_threshs[i];
        // threshold = KAGGLE_threshs[0];
        // string ifilename = "../../DCSketch/output/SuperSpreaders/"+dataset+ "/" + filename + ".txt";
        // string ifilename = "../../SpreadSketch/output/"+dataset+"/" + filename + ".txt";
        string ifilename = "../../Vector_BF/output/"+dataset+"/" + filename + ".txt";
        // string ifilename = "../../bSkt/SuperSpreader/"+dataset+"/" + filename + ".txt";
        ifstream ifile_esti(ifilename);
         
        ifstream ifile_truth("../../get_groundtruth/SuperSpreaders/"+dataset+"/"+ filename + ".txt");
        if(!ifile_truth || !ifile_esti)
        {
            cout<<"unable to open file"<<endl;
            return 0;
        }
        set<pair<string,uint32_t>> superspreaders;
        while(!ifile_truth.eof())
        {
            string flowid;
            uint32_t true_spread;
            ifile_truth >> flowid >> true_spread;
            if (true_spread >= threshold)
            {
                superspreaders.insert( pair<string,uint32_t>(flowid, true_spread) );
            }
        }
        set<pair<string,uint32_t>> superspreaders_esti;
        double tuned_threshold;
        if (ifilename.find("DCSketch") != std::string::npos)
        {
            cout<<"tune threshold"<<endl;
            tuned_threshold = threshold * (1 - 0.5 * 0.1856);
        }
        else
            tuned_threshold = threshold;
        while(!ifile_esti.eof())
        {
            string flowid;
            uint32_t esti_spread;
            ifile_esti >> flowid >> esti_spread;
            if(esti_spread > tuned_threshold)
                superspreaders_esti.insert( pair<string,uint32_t>(flowid, esti_spread) );
            else
                break;
        }
        ifile_esti.close();
        ifile_truth.close();
        double error = get_error(superspreaders_esti, superspreaders);
        cout<<"threshold: "<<threshold<<" error: "<<error<<endl;
    }
    
#endif

    return 0;
}

double get_error(set<pair<string,uint32_t>> esti_set, set<pair<string,uint32_t>> truth_set)
{
    double total_error = 0;
    uint32_t hit_num = 0;
    for(auto iter : esti_set)
    {
        for(auto true_iter : truth_set)
        {
            if(iter.first == true_iter.first)
            {
                total_error += abs((double)iter.second - true_iter.second) / true_iter.second;
                hit_num++;
                break;
            }
        }
    }
    if(hit_num == 0)
        return -1;
    return total_error/hit_num;
}

double get_precision(set<string>& esti_set,set<string>& truth_set)
{
    uint32_t hit_num = 0;
    for(auto item : esti_set)
    {
        if(truth_set.find(item) != truth_set.end())
            hit_num++;
    }
    return static_cast<double>(hit_num)/esti_set.size();
}

double get_recall(set<string>& esti_set,set<string>& truth_set)
{
    uint32_t hit_num = 0;
    for(auto item : truth_set)
    {
        if(esti_set.find(item) != esti_set.end())
            hit_num++;
    }
    return static_cast<double>(hit_num)/truth_set.size();
}