#include"FerNNClassifier.h"

int FerNNClassifier::read(fstream& infile){
    infile>>valid;
    infile>>ncc_thesame;
    infile>>nstructs;
    infile>>structSize;
    infile>>thr_fern;
    infile>>thr_nn;
    infile>>thr_nn_valid;

    return 0;
}

void FerNNClassifier::print(){
    cout<<"valid: "<<valid<<endl;
    cout<<"ncc_thesame: "<<ncc_thesame<<endl;
    cout<<"nstruct: "<<nstructs<<endl;
    cout<<"structSize: "<<structSize<<endl;
    cout<<"thr_fern: "<<thr_fern<<endl;
    cout<<"thr_nn: "<<thr_nn<<endl;
    cout<<"thr_nn_valid"<<thr_nn_valid<<endl;
}

void FerNNClassifier::prepare(Scale* scales, int scales_size){
    acum = 0;
    int totalFertures = nstructs*structSize;
    features = new int[scales_size*totalFertures*4]();

    float x1f;
    float x2f;
    float y1f;
    float y2f;

    for(int i=0; i<totalFertures; i++){
	x1f = (float)rand()/RAND_MAX;
	x2f = (float)rand()/RAND_MAX;
	y1f = (float)rand()/RAND_MAX;
	y2f = (float)rand()/RAND_MAX;
	for(int s=0; s<scales_size; s++){
	    features[s*totalFertures*4+i*4+0] = x1f*scales[s].width;
	    features[s*totalFertures*4+i*4+1] = y1f*scales[s].height;
	    features[s*totalFertures*4+i*4+2] = x2f*scales[s].width;
	    features[s*totalFertures*4+i*4+3] = y2f*scales[s].height;
	}
    }

    for(int i=0; i<scales_size; i++){
	for(int j=0; j<totalFertures; j++){
	    for(int t=0; t<4; t++){
	//	cout<<features[i*4*totalFertures+j*4+t]<<" ";
	    }
	 //   cout<<endl;
	}
    }
    
    thrN = 0.5*nstructs;
    posteriors = new float[10*8192]();
    pCounter = new int[10*8192]();
    nCounter = new int[10*8192]();
}


void FerNNClassifier::getFeatures(const unsigned char* frame, int* patch, int* fern){
    int leaf;
    int x1;
    int y1;
    int x2;
    int y2;
    unsigned char pixel1;
    unsigned char pixel2;
    for(int i=0; i<nstructs; i++){
	leaf = 0;
	for(int j=0; j<structSize; j++){
	    x1=features[patch[4]*nstructs*structSize*4+i*structSize*4+j*4+0]; 
	    y1=features[patch[4]*nstructs*structSize*4+i*structSize*4+j*4+1]; 
	    x2=features[patch[4]*nstructs*structSize*4+i*structSize*4+j*4+2]; 
	    y2=features[patch[4]*nstructs*structSize*4+i*structSize*4+j*4+3]; 
	    pixel1 = frame[(x1+patch[0])*IMAGE_COLS+(y1+patch[1])];
	    pixel2 = frame[(x2+patch[0])*IMAGE_COLS+(y2+patch[1])];
	    leaf = (leaf<<1) + (pixel1>pixel2);
	}
	fern[i] = leaf;
    }
}


float FerNNClassifier::measure_forest(int* fern){
    float votes = 0;
    for(int i=0; i<nstructs; i++){
	votes += posteriors[i*8192+fern[i]];
    }

    return votes;
}


void FerNNClassifier::update(int* fern, int C, int N){
    int idx;
    for(int i=0; i<nstructs; i++){
	idx = fern[i];
	if(C == 1){
	    pCounter[i*8192+idx] += N;
	}
	else{
	    nCounter[i*8192+idx] += N;
	}

	if(pCounter[i*8192+idx] == 0){
	    posteriors[i*8192+idx] = 0;
	}
	else{
	    posteriors[i*8192+idx] = ((float)pCounter[i*8192+idx])/(pCounter[i*8192+idx]+nCounter[i*8192+idx]);
	}
    }

}


void FerNNClassifier::trainF(int* fern, int fern_size){
    thrP = thr_fern*nstructs;
    for(int i=0; i<fern_size; i++){
	if(fern[i*11+0] == 1){
	    if(measure_forest(fern+i*11+1)<=thrP){
		update(fern+i*11+1, 1, 1);
	    }
	}
	else{
	    if(measure_forest(fern+i*11+1)>=thrN){
		update(fern+i*11+1, 0, 1);
	    }
	}
    }
}


