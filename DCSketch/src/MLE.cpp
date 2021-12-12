#include "MLE.h"

void MLE::MLE_Init(double l1_f, double l2_f, double l1_e, double l2_e, double l1_bkts, double l2_bkts)
{
    lamda1 = 2 * l1_f / l1_bkts;
    lamda2 = 2 * l2_f / l2_bkts;
    l1_unit_err = l1_e / (2 * l1_f);
    l2_unit_err = l2_e / (2 * l2_f);
    cout<<"lamda1: "<<lamda1 <<"  lamda2: "<<lamda2<<endl;
    cout<<"l1_unit_err: "<<l1_unit_err <<"  l2_unit_err: "<<l2_unit_err<<endl;
    for(size_t i = 0;i < lamda1 * 10;i++)
    {
        layer1_poiss_pdf.push_back( pow(lamda1,i)/factorial(i)*exp(-lamda1) );
    }
    for(size_t i = 0;i < lamda2 * 10;i++)
    {
        layer2_poiss_pdf.push_back( pow(lamda2,i)/factorial(i)*exp(-lamda2) );
    }
}

double MLE::prop_bimtap_val(double zero_num, double s) //the probability that a bitmap has zero_num zeros after recording a set with cardinality s. 
{
    static const double bitmap_size = 6;
    double prob_zero_bit = pow((1 - 1/bitmap_size), s); 
    double prob_one_bit = 1 - prob_zero_bit;
    double prob = pow(prob_zero_bit, zero_num) * pow(prob_one_bit, bitmap_size - zero_num);
    return prob;
}

double MLE::prop_hll_val(double hll_val, double s) //the probability that a HyperLogLog has value hll_val after recording a set with cardinality s. 
{
#define PI 3.1415926
    static double sigma = 1.05/sqrt(32); 
    double prob = 1/( sqrt(2*PI) * sigma ) * exp( -(hll_val-s)*(hll_val-s) / (2*sigma*sigma) );
    return prob;
}

double MLE::prob_cond_bm(double bm_zero_num, double s)
{
    double prob_bm = 0;
    for(size_t conf_flows = 0;conf_flows <= layer1_poiss_pdf.size() ;conf_flows++)
    {
        double record_sum = l1_unit_err * conf_flows + s;
        double cond_prob = prop_bimtap_val(bm_zero_num, record_sum);
        double err_prob = layer1_poiss_pdf[conf_flows];  //prob_error_val_bm(err_val, lamda);
        double prob = cond_prob * err_prob;
        prob_bm += prob;
    }
    return prob_bm;
}

double MLE::prob_cond_hll(double hll_val, double s)
{
    double prob_hll = 0;
    for(size_t conf_flows = 0;conf_flows < layer2_poiss_pdf.size();conf_flows++)
    {
        double record_sum = l2_unit_err * conf_flows + s;
        double cond_prob = prop_hll_val(hll_val,record_sum);
        double err_prob = layer2_poiss_pdf[conf_flows]; //prob_error_val_hll(err_val,lamda);
        double prob = cond_prob * err_prob;
        prob_hll += prob;
    }
    return prob_hll;
}

double MLE::factorial(double n)
{
    if (n <= 1)
        return 1;
    double res = 1;
    for(size_t i = 1; i <= n; i++)
    {
        res *= i;
    }
    return res;
}

double MLE::prob_s1_val(double i, double total_c)
{
    if(total_c <= 15)
    {
        double comb = factorial(total_c) / ( factorial(i) * factorial(total_c - i) );
        return comb * pow(0.5,total_c);
    }
    double mu = total_c * 0.5;
    double sigma = sqrt( total_c * 0.5 * (1 - 0.5) );
    double prob = 1/(sqrt(2*PI)*sigma) * exp( -(i-mu)*(i-mu)/(2*sigma*sigma) );
    return prob;
}

double MLE::spread_prob(array<uint32_t,2> hll_vals, double spread)
{
    double prob_spread = 0;
    for (size_t hll1_s = 0.25 * spread; hll1_s <= 0.75*spread;hll1_s++)
    {
        double cond_p1 = prob_cond_hll(hll_vals[0], hll1_s);
        double cond_p2 = prob_cond_hll(hll_vals[1], spread - hll1_s);
        double hll1_s_prob = prob_s1_val(hll1_s,spread);
        prob_spread += cond_p1 * cond_p2 * hll1_s_prob;
    }
    return prob_spread;
}

double MLE::find_max(array<uint32_t,2> hll_vals, double low_bound, double up_bound)
{
    double left_spread = low_bound;
    double right_spread = up_bound;
    double left_prob = spread_prob(hll_vals,left_spread);
    double right_prob = spread_prob(hll_vals,right_spread);
    while(right_spread - left_spread > 3){
        double mid1_spread = floor( left_spread + (right_spread - left_spread)/3 );
        double mid1_prob = spread_prob(hll_vals, mid1_spread);
        double mid2_spread = floor( left_spread + (right_spread - left_spread)*2/3 );
        double mid2_prob = spread_prob(hll_vals, mid2_spread);
        if(mid1_prob == 0 && mid2_prob == 0)
        {
            left_spread += (right_spread - left_spread)*0.1;
            right_spread -= (right_spread - left_spread)*0.1;
            continue;
        }
        if(mid1_prob <= mid2_prob)
        {
            left_spread = mid1_spread;
            left_prob = mid1_prob;
        }
        else
        {
            right_spread = mid2_spread;
            right_prob = mid2_prob;
        }
    }
    double ret_spread = left_prob > right_spread ? left_prob : right_spread;
    return static_cast<uint32_t>(ret_spread);
}

// double MLE::prob_error_val_hll(double err_val, double lamda)
// {
//     double prob = 1; //= pow(lamda,err_val) / factorial(err_val) * exp(-lamda);
//     for(size_t i = 1;i <= err_val;i++)
//     {
//         prob *= (lamda / i);
//     }
//     return prob;
// }

// double MLE::prob_error_val_bm(double err_val, double lamda)
// {
//     double prob = 1; //= pow(lamda,err_val) / factorial(err_val) * exp(-lamda);
//     for(size_t i = 1;i <= err_val;i++)
//     {
//         prob *= (lamda / i);
//     }
//     return prob * exp(-lamda);;
// }