#include"Header.h"
void drawKeyboardROI(CvRect rect, IplImage* dstImage ){
	CvPoint pt1 = cvPoint(rect.x, rect.y);
	CvPoint pt2 = cvPoint(rect.x + rect.width, rect.y + rect.height);
	cvRectangle(dstImage, pt1,pt2, CV_RGB(0,255, 0), 5);
}