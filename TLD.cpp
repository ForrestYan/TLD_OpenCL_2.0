#include"TLD.h"

TLD::TLD(){}
TLD::TLD(char* filename){
    read(filename);
}

int TLD::read(char* filename){
    fstream infile(filename, ios::in);
    if(!infile){
	cerr<<"TLD.read error: failed to open file"<<endl;
	return -1;
    }

    infile>>min_win;
    infile>>patch_size;
    infile>>num_closest_init;
    infile>>num_warps_init;
    infile>>num_closest_update;
    infile>>num_warps_update;
    infile>>bad_overlap;
    infile>>bad_patches;
    classifier.read(infile);

    infile.close();
    return 0;
}

int TLD::init(const unsigned char* frame1, const BoundingBox* box){
    image_rows = IMAGE_ROWS;
    image_cols = IMAGE_COLS;
    buildGrid(box);
    getOverlappingBox(box, num_closest_init);
    cout<<"good boxes: "<<good_box_size<<endl;
    cout<<"bad boxes: "<<bad_box_size<<endl;
    cout<<"best box: "<<best_box.x<<" "<<best_box.y<<" ";
    cout<<best_box.width<<" "<<best_box.height<<" ";
    cout<<best_box.overlap<<" "<<best_box.sidx<<endl;
    cout<<"Bounding Box hull: "<<bbhull.x<<" ";
    cout<<bbhull.y<<" "<<bbhull.width<<" ";
    cout<<bbhull.height<<endl;

    lastbox.x = best_box.x;
    lastbox.y = best_box.y;
    lastbox.width = best_box.width;
    lastbox.height = best_box.height;
    lastbox.sidx = best_box.sidx;
    lastbox.overlap = best_box.overlap;
    lastconf = 1;
    lastvalid = true;
    classifier.prepare(scales, scales_size);
    generatePositiveData(frame1, num_warps_init);

    float mean;
    float stdev;
    meanStdDev(frame1, &best_box, &mean, &stdev);
    cout<<mean<<" "<<stdev<<endl;
    integral(frame1, iisum, iisqsum);
    var = pow(stdev, 2)*0.5;
    cout<<var<<endl;
    generateNegativeData(frame1);

}

void TLD::print(){
    
    cout<<"min_win: "<<min_win<<endl;
    cout<<"patch_size: "<<patch_size<<endl;
    cout<<"num_close_init: "<<num_closest_init<<endl;
    cout<<"num_warps: "<<num_warps_init<<endl;
    cout<<"num_closest: "<<num_closest_update<<endl;
    cout<<"num_warps_update: "<<num_warps_update<<endl;
    cout<<"bad_overlap: "<<bad_overlap<<endl;
    cout<<"bad_patches: "<<bad_patches<<endl;
    classifier.print();
}

int TLD::buildGrid(const BoundingBox* box){
    const float SHIFT = 0.1;
    const float SCALES[] = {0.16151, 0.19381, 0.23257, 0.27908, 0.33490, 
			  0.40188, 0.48225, 0.57870, 0.69444, 0.83333,
			  1.00000, 1.20000, 1.44000, 1.72800, 2.07360,
			  2.48832, 2.98598, 3.58318, 4.29982, 5.15978,
			  6.19174};
    int width;
    int height;
    int min_bb_size;

    scales_size = 0;
    grid_size = 0;
    for(int n=0; n<21; n++){
	width = round(box->width*SCALES[n]);
	height = round(box->height*SCALES[n]);
	min_bb_size = min(height, width);
	if(min_bb_size<min_win || width>image_cols || height>image_rows){
	    continue;
	}
	scales_size++;
	for(int y=1; y<image_rows-height; y+=round(SHIFT*min_bb_size)){
	    for(int x=1; x<image_cols-width; x+=round(SHIFT*min_bb_size)){
		grid_size++;
	    }
	}
    }

    scales = new Scale[scales_size];
    grid = new int[grid_size*5]();
    grid_overlap = new float[grid_size];

    int scales_point = 0;
    int grid_point = 0;
    int sc = 0;
    BoundingBox bbox;
    for(int n=0; n<21; n++){
	width = round(box->width*SCALES[n]);
	height = round(box->height*SCALES[n]);
	min_bb_size = min(height, width);
	if(min_bb_size<min_win || width>image_cols || height>image_rows){
	    continue;
	}
	scales[scales_point].width = width;
	scales[scales_point].height = height;
	scales_point++;
	for(int y=1; y<image_rows-height; y+=round(SHIFT*min_bb_size)){
	    for(int x=1; x<image_cols-width; x+=round(SHIFT*min_bb_size)){
		bbox.x = x;
		bbox.y = y;
		bbox.width = width;
		bbox.height = height;
		grid[grid_point*5+0] = x;
		grid[grid_point*5+1] = y;
		grid[grid_point*5+2] = width;
		grid[grid_point*5+3]  = height;
		grid_overlap[grid_point]  = bbOverlap(&bbox, box);
		grid[grid_point*5+4] = sc;
		grid_point++;
	    }
	}

	sc++;
    }
    for(int i=0; i<grid_size; i++){
//	cout<<grid_overlap[i]<<endl;
    }
    return 0;
}


float TLD::bbOverlap(const BoundingBox* box1, const BoundingBox* box2){
    if(box1->x > box2->x+box2->width){
	return 0.0;
    }
    if(box1->y > box2->y+box2->height){
	return 0.0;
    }
    if(box1->x+box1->width < box2->x){
	return 0.0;
    }
    if(box1->y+box1->height < box2->y){
	return 0.0;
    }

    float colInt;
    float rowInt;
    colInt = min(box1->x+box1->width,box2->x+box2->width)-max(box1->x,box2->x);
    rowInt = min(box1->y+box1->height,box2->y+box2->height)-max(box1->y,box2->y);

    float intersection = colInt*rowInt;
    float area1 = box1->width*box1->height;
    float area2 = box2->width*box2->height;

    //cout<<intersection/(area1+area2 - intersection)<<endl;
    return intersection/(area1+area2 - intersection);
}


int TLD::getOverlappingBox(const BoundingBox* box, int num_closest){
    float max_overlap = 0;
    int max_index = 0;
    good_box_size = 0;
    bad_box_size = 0;
    int good_box_point = 0;
    int bad_box_point = 0;
    for(int i=0; i<grid_size; i++){
	if(grid_overlap[i] > max_overlap){
	    max_overlap = grid_overlap[i];
	    max_index = i; 
	}
	if(grid_overlap[i] > 0.6){
	    good_box_size++;	    
	}	
	if(grid_overlap[i] < bad_overlap){
	    bad_box_size++;	    
	}
    }


    best_box.x = grid[max_index*5+0];
    best_box.y = grid[max_index*5+1];
    best_box.width = grid[max_index*5+2];
    best_box.height = grid[max_index*5+3];
    best_box.sidx = grid[max_index*5+4];
    best_box.overlap = max_overlap;

    int* good_box_temp = new int[good_box_size];
    bad_box = new int[bad_box_size];
    for(int i=0; i<grid_size; i++){
	if(grid_overlap[i] > 0.6){
	    good_box_temp[good_box_point] = i;
	    good_box_point++;
	}	
	if(grid_overlap[i] < bad_overlap){
	    bad_box[bad_box_point] = i;
	    bad_box_point++;
	}
    }

    if(good_box_size <= num_closest){
	for(int i=0; i<good_box_size; i++){
	    good_box[i] = good_box_temp[i];
	}
    }
    else{
	for(int i=0; i<num_closest; i++){
	    for(int j=i+1; j<good_box_size; j++){
		if(grid_overlap[good_box_temp[i]]<grid_overlap[good_box_temp[j]]){
		    int temp;
		    temp = good_box_temp[i];
		    good_box_temp[i] = good_box_temp[j];
		    good_box_temp[j] = temp;
		}
	    }
	}

	for(int i=0; i<num_closest; i++){
	    good_box[i] = good_box_temp[i];
	}
	good_box_size = num_closest;
    }

    getBBHull();
    delete[] good_box_temp;

    return 0;
}


void TLD::getBBHull(){
    int x1 = INT_MAX;
    int x2 = 0;
    int y1 = INT_MAX;
    int y2 = 0;
    int idx;
    for(int i=0; i<good_box_size; i++){
	idx = good_box[i];
	x1 = min(grid[idx*5+0], x1);
	y1 = min(grid[idx*5+1], y1);
	x2 = max(grid[idx*5+0]+grid[idx*5+2], x2);
	y2 = max(grid[idx*5+1]+grid[idx*5+3], y2);
    }

    bbhull.x = x1;
    bbhull.y = y1;
    bbhull.width = x2-x1;
    bbhull.height = y2-y1;
}

void TLD::getPattern(const unsigned char* frame, const BoundingBox* box, 
		     float* pattern, float* mean, float* stdev){

    float scale_x = (float)box->width/15.0;
    float scale_y = (float)box->height/15.0;
    *mean = 0.f;
    for(int j=0; j<15; j++){
	float fy = (float)((j+0.5)*scale_y-0.5);
	int sy = floor(fy);
	fy = fy - sy;
	sy = min(sy, box->height-2);
	sy = max(0, sy);

	float cbufy_f[2];
	cbufy_f[0] = 1.f - fy;
	cbufy_f[1] = fy;

	for(int i=0; i<15; i++){
	    float fx = (float)((i+0.5)*scale_x-0.5);
	    int sx = floor(fx);
	    fx = fx - sx;

	    if(sx < 0){
		fx = 0;
		sx = 0;
	    }
	    if(sx >= box->width-1){
		fx = 0;
		sx = 13;
	    }
	    
	    float cbufx_f[2];
	    cbufx_f[0] = 1.f - fx;
	    cbufx_f[1] = fx;
	    
	    *(pattern+15*j+i)=
		frame[(sy+box->y)*IMAGE_COLS+(sx+box->x)]*cbufx_f[0]*cbufy_f[0]+
		frame[(sy+box->y+1)*IMAGE_COLS+(sx+box->x)]*cbufx_f[0]*cbufy_f[1]+
		frame[(sy+box->y)*IMAGE_COLS+(sx+box->x+1)]*cbufx_f[1]*cbufy_f[0]+
		frame[(sy+box->y+1)*IMAGE_COLS+(sx+box->x+1)]*cbufx_f[1]*cbufy_f[1];
	    *mean += pattern[i+15*j];
	}
    }
    *mean = *mean/225;
    *stdev = 0.f;
    //ofstream outfile("pattern.dat");

    for(int i=0; i<225; i++){
	*stdev += pow((pattern[i]- *mean), 2);
	//outfile<<pattern[i]<<endl;
	pattern[i] -= *mean; 
    }
    *stdev = sqrt(*stdev/225);
}

void TLD::gaussianBlur(const unsigned char* frame, unsigned char* img, int kWidth, int kHeight, float sigma){    
    int nCenter = (kWidth)/2;
    double* pdKernel = new double[kWidth*kHeight];
    double dSum = 0.0;
    for(int i=0; i<kHeight; i++){
	for(int j=0; j<kWidth; j++){
	    int nDis_x = j - nCenter;
	    int nDis_y = i - nCenter;
	    pdKernel[i*kWidth+j] = exp(-0.5*(pow(nDis_x,2)+pow(nDis_y,2))/pow(sigma, 2))/
	    (2*3.1415926*pow(sigma, 2));
	    dSum += pdKernel[i*kWidth+j];
	}
    }

    for(int i=0; i<kHeight; i++){
	for(int j=0; j<kWidth; j++){
	    pdKernel[i*kWidth+j] /= dSum;
	}
    }
    for(int i=0; i<image_rows; i++){
	for(int j=0; j<image_cols; j++){
	    float dFilter = 0.0;
	    float dSum = 0.0;
	    for(int x=(-nCenter); x<=nCenter; x++){
		for(int y=(-nCenter); y<=nCenter; y++){
		    if((i+x)>=0 && (i+x)<image_rows && 
		       (j+y)>=0 && (j+y)<image_cols){
			float pixel = frame[(i+x)*image_cols+(j+y)];
			dFilter += pixel*pdKernel[(x+nCenter)*kWidth+(y+nCenter)];
			dSum += pdKernel[(x+nCenter)*kWidth+(y+nCenter)];
		    }
		}
	    }

	    img[i*image_cols+j] = dFilter/dSum;
	}
    }

//    outfile.write((const char*)img, IMAGE_ROWS*IMAGE_COLS);
    delete[] pdKernel;
}


void TLD::generatePositiveData(const unsigned char* frame, int num_warps){
    float mean;
    float stdev;
    getPattern(frame, &best_box, pEx, &mean, &stdev);
    unsigned char img[image_rows*image_cols];
    gaussianBlur(frame, img, 9, 9, 1.5);

    pX = new int[good_box_size*11];
    pX_size = 0;    
    int* fern = new int[classifier.getNumStructs()]();
    int idx;
    int patch[5];
    for(int j=0; j<good_box_size; j++){
	idx = good_box[j];
	for(int t=0; t<5; t++){
	    patch[t] = grid[idx*5+t];
	}
	classifier.getFeatures(frame, patch, fern);
	pX[j*11+0] = 1;
	for(int t=0; t<10; t++){
	   pX[j*11+t+1] = fern[t];
        }
	pX_size++;
    }

}



void TLD::integral(const unsigned char* frame, float* iisum, float* iisqsum){
    int rows = image_rows+1;
    int cols = image_cols+1;
    float columnSum[cols];
    float columnsqSum[cols];
    for(int i=0; i<rows; i++){
	for(int j=0; j<cols; j++){	
	    if(i==0 | j==0){
		iisum[i*cols+j] = 0;
		iisqsum[i*cols+j] = 0;
		columnSum[j] = 0;
		columnsqSum[j] = 0;
	    }
	    else{
		columnSum[j] = columnSum[j]+(float)frame[(i-1)*image_cols+j-1];
		columnsqSum[j] = columnsqSum[j]+pow(frame[(i-1)*image_cols+j-1],2);
		iisum[i*cols+j] = iisum[i*cols+j-1]+columnSum[j];
		iisqsum[i*cols+j] = iisqsum[i*cols+j-1]+columnsqSum[j];
	    }
	}	
    }
}



void TLD::meanStdDev(const unsigned char* frame, BoundingBox* box, float* mean, float* stdev){
    
    *mean = 0.0;
    int n=0;
    for(int i=box->y; i<box->y+box->height; i++){
	for(int j=box->x; j<box->x+box->width; j++){
	    *mean += frame[i*image_cols+j]; 
	    n++;
	}
    }

    *mean /= box->width*box->height;
    
    for(int i=box->y; i<box->y+box->height; i++){
	for(int j=box->x; j<box->x+box->width; j++){
	    *stdev += pow((frame[i*image_cols+j]-(*mean)), 2); 
	}
    }
    *stdev /= box->width*box->height;
    *stdev = sqrt(*stdev);
}

float TLD::getVar(const BoundingBox* box, float* sum, float* sqsum, int sum_cols){
    float brs = sum[(box->y+box->height)*sum_cols+box->x+box->width];
    float bls = sum[(box->y+box->height)*sum_cols+box->x];
    float trs = sum[(box->y)*sum_cols+box->x+box->width];
    float tls = sum[(box->y)*sum_cols+box->x];
    float brsq = sqsum[(box->y+box->height)*sum_cols+box->x+box->width];
    float blsq = sqsum[(box->y+box->height)*sum_cols+box->x];
    float trsq = sqsum[(box->y)*sum_cols+box->x+box->width];
    float tlsq = sqsum[(box->y)*sum_cols+box->x];

    float mean = (brs+tls-trs-bls)/(((float)box->width)*box->height);
    float sqmean = (brsq+tlsq-trsq-blsq)/(((float)box->width)*box->height);

    return sqmean - pow(mean, 2);
}

void TLD::generateNegativeData(const unsigned char* frame){
    int idx;
    int* fern = new int[classifier.getNumStructs()];
    nX = new int[bad_box_size*11];
    nX_size = 0;
    int patch[5];

    random_shuffle(bad_box, bad_box+bad_box_size);
    for(int i=0; i<bad_box_size; i++){
	idx = bad_box[i];
	BoundingBox box;
	box.x = grid[idx*5+0];
	box.y = grid[idx*5+1];
	box.width = grid[idx*5+2];
	box.height = grid[idx*5+3];
		
	if(getVar(&box, iisum, iisqsum, image_cols+1)<var*0.5){
	    continue;
	}
	for(int j=0; j<5; j++){
	    patch[j] = grid[idx*5+j];
	}
	classifier.getFeatures(frame, patch, fern);
	nX[11*nX_size+0] = 0;
	for(int j=0; j<10; j++){
	    nX[11*nX_size+j+1] = fern[j];
	}
	nX_size++;
    }

    float dum1, dum2;
    nEx = new float[225*bad_patches]();
    float nEx_buffer[225]; 

    for(int i=0; i<bad_patches; i++){
	idx = bad_box[i];
	BoundingBox box;
	box.x = grid[idx*5+0];
	box.y = grid[idx*5+1];
	box.width = grid[idx*5+2];
	box.height = grid[idx*5+3];
	box.sidx = grid[idx*5+4];
	getPattern(frame, &box, nEx+225*i, &dum1, &dum2);

    }
    delete[] fern;
}

