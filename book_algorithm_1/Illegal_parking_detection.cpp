// bookmark_algorithm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstring>

#include <windows.h>
#include <direct.h>
#include <atltime.h>



#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <vector>
#include <list>

 
#include "opencv/highgui.h"
#include "opencv/cv.h"
#include <vector>
#include <opencv2\opencv.hpp>



using namespace cv;
using namespace std;

CvHaarClassifierCascade *cascade;
CvHaarClassifierCascade *cascade_side;
CvHaarClassifierCascade *cascade_front;
CvMemStorage            *storage;
CvMemStorage            *storage_side;
CvMemStorage            *storage_front;

//**********contant parameters can be tuned for proper adjustment**********//
const int shortupdate = 8; //update rate of short term background
const int longupdate = 160; //update rate of long term background
const int fps = 1; //processing speed in terms of frames per second
const int origin_fps = 25; //original frame rate the video is coded
const int thresholdHigh = 30000; //upper limit of a vehicle size
const int thresholdLow = 1000; //lower limit of a vehicle size
const int resolutionx = 320,resolutiony = 256; //resolution the video is going to be processed with

struct Location{
	int ystart; //bndRect.y
	int yend; //bndRect.y + bndRect.height
	int xstart; //bndRect.x
	int xend; //bndRect.x + bndRect.width
};

bool cardetection(IplImage  *image, CvSize size);
bool cardetection_flip(IplImage  *image, CvSize size);
void findStatic(Mat& forelong, Mat& fore, Mat& Static);
void trackList(Mat& currentFrame, list<Location>& coordinates, list<int>& parking_time, list<bool>& info_recorded, string strStrLog, FILE *output, int seconds);



int main(int argc, char* argv[]) {
	CvCapture *capture;
	int frame_no = 0;
	int initialBack = 0;
	int roundshort=1; //number of short term background updates
	int frequency=1;//frequency to take frame to process
	int tot; // store the size of ROI
	int seconds=1; //count the video time, starts from 1 second to reserve time for background stability
	int time_frame=0;
    
	list <Location> coordinates; //storing the area of interest coordinates
	list <int> parking_time; //count for accumulated parking time
	list <bool> info_recorded; // store the information whether the illegal parking at current location has been recorded

	bool isvehicle;
	bool vehicletype = 0;// testing purpose only; 0 for corner view, 1 for side view or front view
	FILE *output; 

	Mat input;
	Mat fore, forelong;
	Mat interest1;
	Mat shortBack, longBack;
	Mat currentBack;
	//Mat pyr, grayFramelong, grayFrameshort;
	Mat tFrame; // store the thresholded foreground changes for both short and long term backgrounds, store the region of interest
	Mat flipFrame; //flip the region of interest for detection
	IplImage  *frame = NULL; //store the input video frame
	IplImage  *frame1= NULL; //store the resized input video frame
	IplImage  *result= NULL;
	IplImage  *flipresult= NULL;
	Rect bndRect;

	//BackgroundSubtractorMOG2 bg; //more sensative
	Ptr<BackgroundSubtractorMOG2> bg = createBackgroundSubtractorMOG2(500, 16, true); //more sensative

	vector<vector<Point> > contours;
	
	//Created to save the detected static region
	CString imageDirName;
	imageDirName = "Static";
	CT2A ascii(imageDirName);
	remove(ascii);
	CreateDirectory(imageDirName, NULL);
	CString imageName;
	CString display;
	

	// load the classifiers
	cascade = (CvHaarClassifierCascade*) cvLoad(argv[1], 0, 0, 0);
	cascade_side = (CvHaarClassifierCascade*) cvLoad(argv[2], 0, 0, 0);
	cascade_front = (CvHaarClassifierCascade*) cvLoad(argv[3], 0, 0, 0);

	//setup memory buffer; needed by the vehicle detector
	storage = cvCreateMemStorage(0);
	storage_side = cvCreateMemStorage(0);
	storage_front = cvCreateMemStorage(0);
	
	//initialize camera
	capture = cvCaptureFromAVI(argv[4]);
	//string videoname = argv[4];
	string strStrLog = "log2.txt";
	cout << strStrLog <<endl;
	output = _fsopen( strStrLog.c_str() , "w", _SH_DENYNO);
    fclose(output);
	output = _fsopen( strStrLog.c_str() , "a", _SH_DENYNO);
	fprintf(output,"current processing : %s\n",argv[4]);
    fclose(output);

	//always check
	assert(cascade && storage && capture);
	assert(cascade_side && storage_side && capture);
	assert(cascade_front && storage_front && capture);


	frame = cvQueryFrame(capture);
	frame1 = cvCreateImage(cvSize(resolutionx,resolutiony),frame->depth,frame->nChannels);
	for(;;){
		isvehicle = true;
		frame = cvQueryFrame(capture);
		if(!frame){
			output = _fsopen( strStrLog.c_str() , "a", _SH_DENYNO);
			fprintf(output,"no more frames from video file : %s\n",argv[4]);
			fclose(output);
			break;//can't load in video frames
		}
		
		
		//wait 1 second for background to be stable
		if(initialBack<origin_fps){
			cvResize(frame, frame1);// resize input frame into standard 400*300 size
			input = cvarrToMat(frame1);//convert image format to Mat for background modeling purpose
			cvReleaseImage(&result);
			bg->apply(input, fore);
			bg->getBackgroundImage(currentBack);
			//bg.operator() (input,fore); //fore is short term foreground
			//bg.getBackgroundImage(currentBack);
			longBack = currentBack.clone();
			shortBack = currentBack.clone();
			initialBack++;
		}
		
		//==============process video at 5 fps speed=======================//
		else{	
			time_frame++;
			frame_no++;
			if(time_frame%origin_fps==0) 
			{
				seconds++;
				cout<<"seconds: "<<seconds<<endl;
				time_frame = 0;//reset time_frame counter
			}
			if(frequency<(origin_fps/fps))
			{
				//do nothing... reducing frame rates
				frequency++;
				//cout<<"frequency: "<<frequency<<endl;
			}else{
				cvResize(frame, frame1);// resize input frame into standard 400*300 size
				input = cvarrToMat(frame1);//convert image format to Mat for background modeling purpose
				cvReleaseImage(&result);
				
				bg->apply(input, fore);
				bg->getBackgroundImage(currentBack);
				//bg.operator() (input,fore); //fore is short term foreground
				//bg.getBackgroundImage(currentBack); //keep track of background modeling at backstage

				//====================check and verify the Location list=============================== 
				if(!coordinates.empty())
				{
					trackList(input, coordinates, parking_time, info_recorded,strStrLog,output,seconds);
				}

				
				//======================After checking previously stored locations, proceed on processing with current frame========================//
				absdiff(input, longBack, forelong); //forelong is long term foreground
				absdiff(input, shortBack, fore);
				//imshow("shortForeground", fore);
				//imshow("longForeground", forelong);
				imshow("shortBackground", shortBack);
				imshow("longBackground", longBack);

				//Static Region will be stored in Mat interest1
				findStatic(forelong, fore, interest1);
				imshow("interest", interest1);

            
				findContours( interest1, contours, CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);

				//==============For each contour, check the size, and examine with the trained classifier================
				for ( int i=0; i<contours.size(); i++ ){   
					bndRect = boundingRect(contours[i]);
					tot = (bndRect.width) * (bndRect.height);
					//cout << "tot " << tot <<endl;
					if(tot > thresholdLow && tot < thresholdHigh){	
						//rectangle( input,bndRect, CV_RGB(0, 255, 255), 2); //corner view
						tFrame = input;
						tFrame = tFrame(Range(bndRect.y,bndRect.y+bndRect.height),Range(bndRect.x,bndRect.x+bndRect.width));
						flip(tFrame, flipFrame,1); 
						result = cvCloneImage(&(IplImage)tFrame);
						flipresult = cvCloneImage(&(IplImage)flipFrame);
						//imshow("staticRegion", result);
						display.Format(_T("%d_%d"), seconds,frame_no);
						imageName = imageDirName + "//static_" + display + ".jpg";
						CT2CA strd2(imageName);
						string str2(strd2);
						imwrite(str2,tFrame);
						
						CvSize img_size = cvSize(bndRect.width,bndRect.height);
						if(!cardetection(result,img_size)){
							if(cardetection_flip(result,img_size)||cardetection_flip(flipresult,img_size)){
								//isvehicle remains to be true
								vehicletype = 0;
							}else{
								isvehicle = false;
							}
						}else{
							vehicletype = 1;
						}
						if(isvehicle){
							if(vehicletype){
								rectangle( input,bndRect, CV_RGB(0, 0, 139), 2); //side or front view
							}
							else{
								rectangle( input,bndRect, CV_RGB(0, 255, 255), 2); //corner view
							}
						
							//store the coordinates before long background updates for further tracking and detection
							if(!(frame_no<longupdate*origin_fps)){
								Location locnew;
								locnew.ystart = bndRect.y;
								locnew.yend =bndRect.y+bndRect.height;
								locnew.xstart = bndRect.x;
								locnew.xend = bndRect.x+bndRect.width;
								coordinates.push_back(locnew);
								parking_time.push_back(0);//start counting the parking time
								info_recorded.push_back(false);
							}
						}
						cvReleaseImage(&result);
						cvReleaseImage(&flipresult);
					}
				}

				if(!(frame_no<(roundshort*shortupdate*origin_fps))){
					shortBack = currentBack.clone();
					roundshort++;
				}
				if(!(frame_no < longupdate*origin_fps)){
					longBack = currentBack.clone(); //update the long term background
					roundshort=1; 
					frame_no = 0; //restart frame count to avoid number overflow
				}	
				imshow("Frame", input);
				if(waitKey(20) == 27) break;
				frequency = 1;
			}
		}
	}
	frame = NULL;
	cvReleaseImage(&frame1);
	cvReleaseImage(&result);
	cvReleaseImage(&flipresult);
	cvReleaseCapture(&capture);
	//input.deallocate();
	cvDestroyAllWindows();
	cvReleaseHaarClassifierCascade(&cascade);
	cvReleaseMemStorage(&storage);
	cvReleaseHaarClassifierCascade(&cascade_side);
	cvReleaseMemStorage(&storage_side);
	cvReleaseHaarClassifierCascade(&cascade_front);
	cvReleaseMemStorage(&storage_front);
	return 0;	   
}

bool cardetection(IplImage  *image, CvSize size){
	int correct=0;
	CvSeq *object_side = cvHaarDetectObjects(
						image,
						cascade_side,
						storage_side,
						1.1, //1.1,//1.5, //-------------------SCALE FACTOR
						1, //2        //------------------MIN NEIGHBOURS
						0, //CV_HAAR_DO_CANNY_PRUNING
						cvSize(0,0),//cvSize( 30,30), // ------MINSIZE
						size //cvSize(70,70)//cvSize(640,480)  //---------MAXSIZE
					);
	CvSeq *object_front = cvHaarDetectObjects(
						image,
						cascade_front,
						storage_front,
						1.1, //1.1,//1.5, //-------------------SCALE FACTOR
						1, //2        //------------------MIN NEIGHBOURS
						0, //CV_HAAR_DO_CANNY_PRUNING
						cvSize(0,0),//cvSize( 30,30), // ------MINSIZE
						size //cvSize(70,70)//cvSize(640,480)  //---------MAXSIZE
					);
	
	//===============Proves to be a vehicle==================//
	if((object_side->total)!=0){
		correct++;
	}
	if((object_front->total)!=0){
		correct++;
	}
	
	if(correct>0){
		return true;
	}else{
		return false;
	}
}

bool cardetection_flip(IplImage  *image, CvSize size){
	CvSeq *object = cvHaarDetectObjects(
						image,
						cascade,
						storage,
						1.1, //1.1,//1.5, //-------------------SCALE FACTOR
						1, //2        //------------------MIN NEIGHBOURS
						0, //CV_HAAR_DO_CANNY_PRUNING
						cvSize(0,0),//cvSize( 30,30), // ------MINSIZE
						size //cvSize(70,70)//cvSize(640,480)  //---------MAXSIZE
					);
	//Proves to be a vehicle
	if((object->total)!=0){
		return true;
	}else{
		return false;
	}
}

void findStatic(Mat& forelong, Mat& fore, Mat& Static){
	Mat tFrame;
	Mat grayFramelong, grayFrameshort;
	Mat pyr;
	threshold(forelong, tFrame, 120, 255, CV_THRESH_BINARY);
	pyrDown(tFrame, pyr);
	dilate(pyr, pyr, Mat(), Point(-1, -1), 2);
	erode(pyr, pyr, Mat(), Point(-1, -1), 2);
	pyrUp(pyr, tFrame);
	cvtColor( tFrame, grayFramelong, CV_BGR2GRAY );
	
	threshold(fore, tFrame, 120, 255, CV_THRESH_BINARY);
	pyrDown(tFrame, pyr);
	dilate(pyr, pyr, Mat(), Point(-1, -1), 2);
	erode(pyr, pyr, Mat(), Point(-1, -1), 2);
	pyrUp(pyr, tFrame);
	cvtColor(tFrame, grayFrameshort, CV_BGR2GRAY);
	
	absdiff(grayFramelong, grayFrameshort,Static);
}

void trackList(Mat& currentFrame, list <Location>& coordinates, list <int>& parking_time, 
	list <bool>& info_recorded, string strStrLog, FILE *output, int seconds){
	bool erase=false, isvehicle=true, vehicletype=0;
	list <Location>::iterator iter;
	list <int>::iterator parkingi;
	list <bool>:: iterator recordedi;
	parkingi = parking_time.begin();
	recordedi = info_recorded.begin();
	Mat tFrame, flipFrame;
	IplImage  *result = NULL;
	IplImage  *flipresult = NULL;

	for(iter=coordinates.begin();iter!=coordinates.end();iter++){
		if(erase){
			iter--;
			parkingi--;
			recordedi--;
		}
		tFrame = currentFrame;
		tFrame = tFrame(Range(iter->ystart,iter->yend),Range(iter->xstart,iter->xend));
		flip(tFrame, flipFrame,1);
		result = cvCloneImage(&(IplImage)tFrame);
		flipresult = cvCloneImage(&(IplImage)flipFrame);
		CvSize img_size = cvSize(iter->xend - iter->xstart,iter->yend - iter->ystart);

		if(!cardetection(result,img_size)){
			if(cardetection_flip(result,img_size)||cardetection_flip(flipresult,img_size)){
					//isvehicle remains to be true
					vehicletype = 0;
			}else{
					isvehicle = false;
			}
		}else{
			vehicletype = 1;
		}
		
		//a vehicle is detected at the marked location, draw rectangle					
		if(isvehicle) {
			CvPoint p1 = cvPoint(iter->xstart,iter->ystart);
			CvPoint p2 = cvPoint(iter->xend,iter->yend);
			if(*parkingi>30*fps) {
				// car stopped for 30 seconds
				if(!(*recordedi)) //*recordedi is false
				{
					output = _fsopen( strStrLog.c_str() , "a", _SH_DENYNO);
					fprintf(output, "illegal parking at %d:%d\n", (seconds-seconds%60)/60, (seconds%60));
					fclose(output);
					*recordedi = true;
				}
				rectangle( currentFrame, p1, p2, CV_RGB(255, 0, 0), 2);
			}else{
				if(vehicletype){
					rectangle( currentFrame, p1, p2, CV_RGB(0, 0, 139), 2); //side or front view
				}else{
					rectangle( currentFrame, p1, p2, CV_RGB(0, 255, 255), 2); //corner view
				}
				(*parkingi)++;
			}
		}
		else {
			//vehicle can't be detected at marked location
			(*parkingi)--;
		}

		//*=========check if current location has time count smaller than -2 seconds============*//
		if((*parkingi)<-2*fps) {
			//vehicle has left the marked location
			//TO DO: release a message: the parked vehicle has left the position
			if(*recordedi){
				// if it is true: previously a car has parked here
				output = _fsopen( strStrLog.c_str() , "a", _SH_DENYNO);
				fprintf(output, "parked car has left at %d:%d\n", (seconds-seconds%60)/60, (seconds%60));
				fclose(output);
			}
			parkingi = parking_time.erase(parkingi);
			iter = coordinates.erase(iter);
			recordedi = info_recorded.erase(recordedi);
			if(parkingi==parking_time.end()){
				break;
			}else{
				erase=true;
			}
		}else{
			erase=false;
		}
		cvReleaseImage(&result);
		cvReleaseImage(&flipresult);
		parkingi++;
		recordedi++;
	}				
}



