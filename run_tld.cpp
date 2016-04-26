#include<iostream>
#include<fstream>
#include<cstdlib>
#include"TLD.h"

using namespace std;

int main(int argc, char* argv[])
{
    int ret = 0;
    TLD tld;
    ret = tld.read("parameters.dat");
    if(ret < 0){
	cerr<<"tld.read error"<<endl;
	exit(-1);
    }
    unsigned char* frame;
    frame = new unsigned char[IMAGE_ROWS*IMAGE_COLS];
    fstream videoIn("car.raw", ios::in|ios::binary);
    if(!videoIn){
	cerr<<"open video file error"<<endl;
	exit(-1);
    }
    videoIn.read((char*)frame, IMAGE_ROWS*IMAGE_COLS);
    BoundingBox box;
    fstream boxfile("box.dat", ios::in);
    if(!boxfile){
	cerr<<"open box file error"<<endl;
	exit(-1);
    }
    boxfile>>box.x;
    boxfile>>box.y;
    boxfile>>box.width;
    boxfile>>box.height;
    box.width -= box.x;
    box.height -= box.y;
    box.overlap =  0.0;
    box.sidx = -1;
    tld.init(frame, &box);



    return 0;
}
