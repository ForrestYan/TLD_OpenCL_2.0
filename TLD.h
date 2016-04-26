#ifndef __TLD_H
#define __TLD_H

#include<iostream>
#include<fstream>
#include<cmath>
#include<limits.h>
#include<algorithm>
#include"tld_type.h"
#include"FerNNClassifier.h"
using namespace std;

struct BoundingBox{
    int x;
    int y;
    int width;
    int height;
    float overlap;
    int sidx;
};


class TLD{
public:
    TLD();
    TLD(char* filename);
    int read(char* filename);
    void print();

    int init(const unsigned char* frame1, const BoundingBox* box);
    void processFrame();
    void track();
    void detect();
    void learn();


    int buildGrid(const BoundingBox* box);
    float bbOverlap(const BoundingBox* box1, const BoundingBox* box2);
    int getOverlappingBox(const BoundingBox* box, int num_closest);
    void getBBHull();
    void getPattern(const unsigned char* frame, const BoundingBox* box, 
	float* pattern, float* mean, float* stdev);
    void gaussianBlur(const unsigned char* frame, unsigned char* img, int kWidth, int kHeight, float sigma);
    void generatePositiveData(const unsigned char* frame, int num_warps);    
    void integral(const unsigned char* frame, float* iisum, float* iisqsum);
    void meanStdDev(const unsigned char* frame, BoundingBox* box, float* mean, float* stdev);
    float getVar(const BoundingBox* box, float* sum, float* sqsum, int sum_cols);
    void generateNegativeData(const unsigned char* frame);

   // Scale* scales;
   // int scales_size;

private:
    FerNNClassifier classifier;

    int image_rows;
    int image_cols;

    int min_win;
    int patch_size;
    int num_closest_init;
    int num_warps_init;
    int num_closest_update;
    int num_warps_update;
    float bad_overlap;
    int bad_patches;

    //Bounding Boxes
    int* grid;
    float* grid_overlap;
    int grid_size;
    Scale* scales;
    int scales_size;
    int good_box[10];
    int good_box_size;
    int* bad_box;
    int bad_box_size;
    BoundingBox best_box;
    BoundingBox bbhull;

    float pEx[225];
    int* pX;
    int pX_size;
    float* nEx;
    int* nX;
    int nX_size;

    float iisum[(IMAGE_ROWS+1)*(IMAGE_COLS+1)];
    float iisqsum[(IMAGE_ROWS+1)*(IMAGE_COLS+1)];
    float var;

    BoundingBox lastbox;
    bool lastvalid;
    float lastconf;
};



#endif
