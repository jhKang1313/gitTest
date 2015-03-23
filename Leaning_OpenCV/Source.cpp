#include<opencv/cv.h>
#include<opencv/highgui.h>
#include<stdio.h>

CvRect selection;			//사용자가 선택한 영역.
bool bLButtonDown = false;	//마우스 왼쪽 버튼의 상태. 누르면 true;
typedef enum {INIT, CALC_HIST, TRACKING} STATUS;
STATUS trackingMode = INIT;		//
//callback function
void on_mouse(int event, int x, int y, int flag, void* param){
	static CvPoint origin;	// 처음 마우스 버튼이 눌린 지점.
	IplImage *image = (IplImage*)param;
	if(!image){
		printf("Error Occur\n");
		return;
	}
	if(bLButtonDown){
		selection.x = MIN(x, origin.x);
		selection.y = MIN(y, origin.y);
		selection.width = selection.x + CV_IABS(x - origin.x); // ABS는 절대값
		selection.height = selection.y + CV_IABS(y - origin.y);

		selection.x = MAX(selection.x,0);
		selection.y = MAX(selection.y, 0);
		selection.width = MIN(selection.width, image->width);
		selection.height = MIN(selection.height, image->height);
		selection.width -= selection.x;
		selection.height -= selection.y;
	}
	//이벤트 발생에 대한 처리를 해주지
	switch(event){
	case CV_EVENT_LBUTTONDOWN:		// 버튼이 눌렸다면
		origin = cvPoint(x, y);		//눌린 버튼 자리를 지정하고
		selection = cvRect(x,y, 0 , 0);		//눌린 자리에 rect을 생성하고
		bLButtonDown =  true;			//버튼을 눌린 상태로 만들고
		break;
	case CV_EVENT_LBUTTONUP:				//버튼을 떼면
		bLButtonDown = false;				// 뗀 상태로 만들고
		if(selection.width > 0 && selection.height > 0)		//만약에 선택된 영역이 0 이상이면
			trackingMode = CALC_HIST;				//선택된 영역에 대한 histgram을 계산.
		break;
	}
}
int main(){
	CvCapture* capture = cvCaptureFromFile("ball.wmv");		// 비디오 파일을 읽어오고
	if(!capture){
		printf("the video file was not found.");			//만약 못가져오면 에러메세지 출력후 종료
		return -1;
	}
	int width = (int) cvGetCaptureProperty(capture, 3);		//영상에 대한 width, height를 구한다.
	int height = (int) cvGetCaptureProperty(capture, 4);
	CvSize size = cvSize(width, height);				//그것으로 size를 구성

	IplImage* frame = NULL;								//capture로부터 가져올 frame 
	IplImage* hsvImage = cvCreateImage(size, IPL_DEPTH_8U, 3);	//3채널 정수형 8비트
	IplImage* hImage = cvCreateImage(size, IPL_DEPTH_8U, 1);		//다 모두 정수형 8비트
	IplImage* mask = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* backProject = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* dstImage = cvCreateImage(size, IPL_DEPTH_8U, 3);

	int histSize = 8; //히스토그램의 사이즈. 8개의 값이 존재하는...
	float valueRange[] = {0, 180};	//한 히스토그램의 x값은 0~180 까지 갖는다.
	float* ranges[] = {valueRange};		//범위

	CvHistogram *pHist = NULL;	// histgram을 저장할 pHist를 만든다.
	pHist = cvCreateHist(1, &histSize, CV_HIST_ARRAY, ranges, 1);	//차원은 1차원, 크기는 histSize, 타입은 array, 범위는 ranges 등으로 histogram 생성

	CvRect track_window; // 사용자가 선택하는 사각형 영역
	CvConnectedComp track_comp;		//track_window가 이동한거 저장
	CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_EPS |  CV_TERMCRIT_ITER, 10, 5);	//MeanShift함수의 종료 조건을 정의

	cvNamedWindow("dstImage");	//window창하나를 이름 짓고
	cvSetMouseCallback("dstImage", on_mouse, dstImage);	//dstImage 창에 on_mouse callback 함수를 지정.

	float max_val;
	CvPoint pt1, pt2;
	int k, hValue;
	char chKey;
	CvBox2D track_Box;
	for(int t = 0 ;; t++){
		frame = cvQueryFrame(capture);	//capture로 부터 frame 하나를 가져온다.

		if(!frame)
			break;
		cvCvtColor(frame, hsvImage, CV_BGR2HLS);		//frame을 hsv이미지로 색상변환
		cvCopy(frame, dstImage);						//dstImage에 frame 이미지를 저장한다. 
		if(bLButtonDown && selection.width > 0 && selection.height > 0){
			//버튼이 눌렸고, width랑 height가 늘어나고 있다면	--> 영역을 선택하고 있다면
			cvSetImageROI(dstImage, selection);			//
			cvXorS(dstImage, cvScalarAll(255), dstImage, 0);
			cvResetImageROI(dstImage);
		}
		if(trackingMode){
			// CALC_HIST 나 TRACKING 모드 일때
			int vmin = 50, vmax = 256, smin = 50;
			cvInRangeS(hsvImage, cvScalar(0, smin, MIN(vmin, vmax)), cvScalar(180, 256, MAX(vmin, vmax)), mask); 
			// 추적하려는 컬러의 범위를 cvScalar(0, smin, MIN(vmin, vmax)) ~ cvScalar(180, 256, MAX(vmin, vmax))사이로 지정하여
			// 범위 내의 화소는 255, 나머지는 0으로 하여 마스킹한다.
			// 즉 사용자가 지정한 영역의 생상은 255, 나머지는 0으로 masking 되어야 한ㄷ.
			cvSplit(hsvImage, hImage,0,0,0); // hsv의 H채널 이미지를 himage에 저장.
			if(trackingMode == CALC_HIST){
				//이제 사용자가 마우스를 때서 histogram 계산하는 작업.
				// 마스크 이미지를 만든다.
				cvSetImageROI(hImage, selection);	//선택한 영역에 대해 ROI를 붙임.
				cvSetImageROI(mask, selection);		//masking 된 이미지에 ROI를 붙임
				cvCalcHist(&hImage, pHist, 0 , mask); //pHist에는 추적하고자 하는 물체의 H체널의 히스토그램이 계싼. 
				cvGetMinMaxHistValue(pHist,0,&max_val, 0, 0);								//pHist에서 최대값을 구하고,
				cvScale(pHist->bins , pHist->bins, max_val ? 255. / max_val : 0.0);			//0,255의 범위로 스케일링

				cvResetImageROI(hImage);			//ROI를 재조정하고
				cvResetImageROI(mask);
				track_window = selection;			//track_window를 선택한 범위로
				trackingMode = TRACKING;

			}
			cvCalcBackProject(&hImage, backProject, pHist);
			cvAnd(backProject, mask, backProject);				//backProject와 mask를 비트 연산하여 추적하려는 색상의 위치에서만 값을 갖게하고 나머지는 0이 되게

			cvCamShift(backProject, track_window, criteria, &track_comp,&track_Box);
			
			track_window = track_comp.rect;						//
			pt1 = cvPoint(track_window.x, track_window.y);
			pt2 = cvPoint(pt1.x + track_window.width, pt1.y + track_window.height);

			//cvRectangle(dstImage, pt1, pt2, CV_RGB(0,0,255), 3);

			
			cvEllipseBox(dstImage, track_Box, CV_RGB(100,100,100), 3, CV_AA, 0);
		}
		cvShowImage("backProject", backProject);
		cvShowImage("hsvImage", hsvImage);
		cvShowImage("hImage", hImage);
		cvShowImage("mask", mask);
		cvShowImage("dstImage", dstImage);
		char chkey = cvWaitKey(50);
		if(chkey == 27)
			break;
	}

	cvDestroyAllWindows();
	cvReleaseCapture(&capture);
	cvReleaseHist(&pHist);
	cvReleaseImage(&hsvImage);
	cvReleaseImage(&hImage);
	cvReleaseImage(&backProject);
	cvReleaseImage(&dstImage);
	cvReleaseImage(&mask);
	return 0;
}