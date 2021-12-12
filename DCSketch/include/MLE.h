#ifndef _MLE_H
#define _MLE_H
#include <iostream>
#include <math.h>
#include <vector>
#include <array>
using namespace std;

class MLE{
public:
    double lamda1;
    double lamda2;
    double l1_unit_err;
    double l2_unit_err;
    vector<double> layer1_poiss_pdf;
    vector<double> layer2_poiss_pdf;

void MLE_Init(double l1_f, double l2_f, double l1_e, double l2_e, double l1_bkts, double l2_bkts);
double factorial(double n);
double prop_bimtap_val(double zero_num, double s); //the probability that a bitmap has zero_num zeros after recording a set with cardinality s. 
double prop_hll_val(double hll_val, double s); //the probability that a HyperLogLog has value hll_val after recording a set with cardinality s. 
// double prob_error_val_bm(double err_val, double lamda);
// double prob_error_val_hll(double err_val, double lamda);
double prob_s1_val(double i, double total_c);
double prob_cond_bm(double bm_zero_num, double s);
double prob_cond_hll(double hll_val, double s);
double spread_prob(array<uint32_t,2> hll_vals, double spread);
double find_max(array<uint32_t,2> hll_vals, double low_bound, double up_bound);
};


#endif 