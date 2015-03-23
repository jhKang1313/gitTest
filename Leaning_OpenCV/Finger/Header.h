#ifndef HEADER_H
#define HEADER_H
#include<opencv/cv.h>
#include<opencv/highgui.h>
#include<stdio.h>
CvSize getFrameSize(CvCapture* capture);
void drawKeyboardROI(CvRect rect, IplImage* dstImage );
void runningCapture(CvCapture* capture);
#endif