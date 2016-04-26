#ifndef __FERNNCLASSIFIER_H
#define __FERNNCLASSIFIER_H
#include<iostream>
#include<fstream>
#include<cstdlib>
#include"tld_type.h"

using namespace std;
class FerNNClassifier{
private:
    float thr_fern;
    int structSize;
    int nstructs;
    float valid;
    float ncc_thesame;
    float thr_nn;
    int acum;



public:
    float thr_nn_valid;
    float thrN;
    float thrP;
    int* features;
    int* nCounter;
    int* pCounter;
    float* posteriors;

    int read(fstream& infile);
    void print();
    void prepare(Scale* scales, int scales_size);
    int getNumStructs(){return nstructs;}
    float getFernTh(){return thr_fern;}
    float getNNTh(){return thr_nn;}
    void getFeatures(const unsigned char* frame, int* patch, int* fern);
    float measure_forest(int* fern);
    void update(int* fern, int C, int N);
    void trainF(int* fern, int fern_size);

};

#endif
