#include"Header.h"
void runningCapture(CvCapture* capture){
	CvSize size = getFrameSize(capture);
	IplImage *frame = NULL;
	IplImage *img = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage *hsvImage = cvCreateImage(size, IPL_DEPTH_8U,3);
	IplImage *hImage = cvCreateImage(size, IPL_DEPTH_8U,1);
	IplImage *mask = cvCreateImage(size, IPL_DEPTH_8U,1);
	IplImage *backProject = cvCreateImage(size, IPL_DEPTH_8U,1);
	IplImage *dstImage = cvCreateImage(size, IPL_DEPTH_8U,3);
	CvRect keyboard = cvRect(20,size.height/2, size.width-40, (size.height-40)/2);
	CvRect testRect = cvRect(20,20, 20,20);
	int histSize = 8;
	float valueRange[] = {0,80};
	float* ranges[] = {valueRange};
	CvHistogram *pHist = cvCreateHist(1, &histSize, CV_HIST_ARRAY, ranges);

	CvConnectedComp keyTrackComp;
	CvBox2D keyTrackBox;
	//CvRect track_window;
	CvRect track_window;
	CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_EPS|CV_TERMCRIT_ITER, 10, 5);
	int t = 1;
	float max_val;
	CvPoint pt1,pt2;
	int k, hValue;
	while(1){
		frame = cvQueryFrame(capture);
		if(!frame)
			break;

		cvCopy(frame, dstImage);
		cvCvtColor(frame, hsvImage, CV_BGR2HSV);

		cvFlip(dstImage, dstImage, 1);
		cvFlip(hsvImage, hsvImage, 1);
		cvFlip(hImage, hImage, 1);


		cvSetImageROI(dstImage, keyboard);

		int vmin = 0, vmax = 120, smin = 0;
		
		
		cvInRangeS(hsvImage, cvScalar(0,smin, MIN(vmin, vmax)), cvScalar(200, 200, MAX(vmin, vmax)), mask);
		cvSplit(hsvImage, hImage, 0,0,0);

		if(t == 1){
			cvSetImageROI(hImage, keyboard);
			cvSetImageROI(mask, keyboard);
			cvCalcHist(&hImage, pHist, 0 , mask);
			
			printf("Before scaling pHist...\n");
			for(k=0; k<histSize; k++)
			{
				hValue = (int)cvQueryHistValue_1D(pHist, k );
				printf("h[%d] = %d\n", k, hValue);	
			}

			cvGetMinMaxHistValue( pHist, 0, &max_val, 0, 0 );
			cvScale( pHist->bins, pHist->bins, 
				max_val ? 255. / max_val : 0.0);//[0, 255]

			printf("After scaling pHist to [0.255]...\n");
			for(k=0; k<histSize; k++)
			{
				hValue = (int)cvQueryHistValue_1D(pHist, k );
				printf("h[%d] = %d\n", k, hValue);	
			}
			cvResetImageROI(mask);
			cvResetImageROI(hImage);
			track_window = keyboard;
			t++;
		}
		cvCalcBackProject(&hImage, backProject, pHist);
		cvAnd(backProject, mask, backProject);
		cvCamShift(backProject, track_window, criteria, &keyTrackComp, &keyTrackBox);

		track_window = keyTrackComp.rect;
	//	pt1 = cvPoint(track_window.x, track_window.y);
	//	pt2 = cvPoint(pt1.x + track_window.width, pt1.y + track_window.height);

		//cvRectangle(dstImage, pt1, pt2, CV_RGB(255,0,0),2);
		
		cvEllipseBox(dstImage, keyTrackBox, CV_RGB(255,0,0), 3, CV_AA, 0);
		cvResetImageROI(dstImage);
		char c = cvWaitKey(20);
		if(c == 27)
			break;

		//drawKeyboardROI(keyboard, dstImage);
		cvShowImage("test", dstImage);
		cvShowImage("hsvImage", hsvImage);
		cvShowImage("mask", mask);
		cvShowImage("back", backProject);
		cvShowImage("hImage", hImage);
		
	}



	cvDestroyAllWindows();
	cvReleaseCapture(&capture);
	cvReleaseImage(&img);
	cvReleaseImage(&hsvImage);
	cvReleaseImage(&hImage);
	cvReleaseImage(&dstImage);
	cvReleaseImage(&backProject);
	cvReleaseImage(&mask);
	cvReleaseHist(&pHist);
	//cvRelease
}