#include"Header.h"
CvSize getFrameSize(CvCapture* capture){
	int width = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	int height = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
	CvSize size = cvSize(width, height);
	return size;
}