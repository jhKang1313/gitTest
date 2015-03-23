#include<opencv/cv.h>
#include<opencv/highgui.h>
#include<stdio.h>

CvRect selection;			//����ڰ� ������ ����.
bool bLButtonDown = false;	//���콺 ���� ��ư�� ����. ������ true;
typedef enum {INIT, CALC_HIST, TRACKING} STATUS;
STATUS trackingMode = INIT;		//
//callback function
void on_mouse(int event, int x, int y, int flag, void* param){
	static CvPoint origin;	// ó�� ���콺 ��ư�� ���� ����.
	IplImage *image = (IplImage*)param;
	if(!image){
		printf("Error Occur\n");
		return;
	}
	if(bLButtonDown){
		selection.x = MIN(x, origin.x);
		selection.y = MIN(y, origin.y);
		selection.width = selection.x + CV_IABS(x - origin.x); // ABS�� ���밪
		selection.height = selection.y + CV_IABS(y - origin.y);

		selection.x = MAX(selection.x,0);
		selection.y = MAX(selection.y, 0);
		selection.width = MIN(selection.width, image->width);
		selection.height = MIN(selection.height, image->height);
		selection.width -= selection.x;
		selection.height -= selection.y;
	}
	//�̺�Ʈ �߻��� ���� ó���� ������
	switch(event){
	case CV_EVENT_LBUTTONDOWN:		// ��ư�� ���ȴٸ�
		origin = cvPoint(x, y);		//���� ��ư �ڸ��� �����ϰ�
		selection = cvRect(x,y, 0 , 0);		//���� �ڸ��� rect�� �����ϰ�
		bLButtonDown =  true;			//��ư�� ���� ���·� �����
		break;
	case CV_EVENT_LBUTTONUP:				//��ư�� ����
		bLButtonDown = false;				// �� ���·� �����
		if(selection.width > 0 && selection.height > 0)		//���࿡ ���õ� ������ 0 �̻��̸�
			trackingMode = CALC_HIST;				//���õ� ������ ���� histgram�� ���.
		break;
	}
}
int main(){
	CvCapture* capture = cvCaptureFromFile("ball.wmv");		// ���� ������ �о����
	if(!capture){
		printf("the video file was not found.");			//���� ���������� �����޼��� ����� ����
		return -1;
	}
	int width = (int) cvGetCaptureProperty(capture, 3);		//���� ���� width, height�� ���Ѵ�.
	int height = (int) cvGetCaptureProperty(capture, 4);
	CvSize size = cvSize(width, height);				//�װ����� size�� ����

	IplImage* frame = NULL;								//capture�κ��� ������ frame 
	IplImage* hsvImage = cvCreateImage(size, IPL_DEPTH_8U, 3);	//3ä�� ������ 8��Ʈ
	IplImage* hImage = cvCreateImage(size, IPL_DEPTH_8U, 1);		//�� ��� ������ 8��Ʈ
	IplImage* mask = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* backProject = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* dstImage = cvCreateImage(size, IPL_DEPTH_8U, 3);

	int histSize = 8; //������׷��� ������. 8���� ���� �����ϴ�...
	float valueRange[] = {0, 180};	//�� ������׷��� x���� 0~180 ���� ���´�.
	float* ranges[] = {valueRange};		//����

	CvHistogram *pHist = NULL;	// histgram�� ������ pHist�� �����.
	pHist = cvCreateHist(1, &histSize, CV_HIST_ARRAY, ranges, 1);	//������ 1����, ũ��� histSize, Ÿ���� array, ������ ranges ������ histogram ����

	CvRect track_window; // ����ڰ� �����ϴ� �簢�� ����
	CvConnectedComp track_comp;		//track_window�� �̵��Ѱ� ����
	CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_EPS |  CV_TERMCRIT_ITER, 10, 5);	//MeanShift�Լ��� ���� ������ ����

	cvNamedWindow("dstImage");	//windowâ�ϳ��� �̸� ����
	cvSetMouseCallback("dstImage", on_mouse, dstImage);	//dstImage â�� on_mouse callback �Լ��� ����.

	float max_val;
	CvPoint pt1, pt2;
	int k, hValue;
	char chKey;
	CvBox2D track_Box;
	for(int t = 0 ;; t++){
		frame = cvQueryFrame(capture);	//capture�� ���� frame �ϳ��� �����´�.

		if(!frame)
			break;
		cvCvtColor(frame, hsvImage, CV_BGR2HLS);		//frame�� hsv�̹����� ����ȯ
		cvCopy(frame, dstImage);						//dstImage�� frame �̹����� �����Ѵ�. 
		if(bLButtonDown && selection.width > 0 && selection.height > 0){
			//��ư�� ���Ȱ�, width�� height�� �þ�� �ִٸ�	--> ������ �����ϰ� �ִٸ�
			cvSetImageROI(dstImage, selection);			//
			cvXorS(dstImage, cvScalarAll(255), dstImage, 0);
			cvResetImageROI(dstImage);
		}
		if(trackingMode){
			// CALC_HIST �� TRACKING ��� �϶�
			int vmin = 50, vmax = 256, smin = 50;
			cvInRangeS(hsvImage, cvScalar(0, smin, MIN(vmin, vmax)), cvScalar(180, 256, MAX(vmin, vmax)), mask); 
			// �����Ϸ��� �÷��� ������ cvScalar(0, smin, MIN(vmin, vmax)) ~ cvScalar(180, 256, MAX(vmin, vmax))���̷� �����Ͽ�
			// ���� ���� ȭ�Ҵ� 255, �������� 0���� �Ͽ� ����ŷ�Ѵ�.
			// �� ����ڰ� ������ ������ ������ 255, �������� 0���� masking �Ǿ�� �Ѥ�.
			cvSplit(hsvImage, hImage,0,0,0); // hsv�� Hä�� �̹����� himage�� ����.
			if(trackingMode == CALC_HIST){
				//���� ����ڰ� ���콺�� ���� histogram ����ϴ� �۾�.
				// ����ũ �̹����� �����.
				cvSetImageROI(hImage, selection);	//������ ������ ���� ROI�� ����.
				cvSetImageROI(mask, selection);		//masking �� �̹����� ROI�� ����
				cvCalcHist(&hImage, pHist, 0 , mask); //pHist���� �����ϰ��� �ϴ� ��ü�� Hü���� ������׷��� ���. 
				cvGetMinMaxHistValue(pHist,0,&max_val, 0, 0);								//pHist���� �ִ밪�� ���ϰ�,
				cvScale(pHist->bins , pHist->bins, max_val ? 255. / max_val : 0.0);			//0,255�� ������ �����ϸ�

				cvResetImageROI(hImage);			//ROI�� �������ϰ�
				cvResetImageROI(mask);
				track_window = selection;			//track_window�� ������ ������
				trackingMode = TRACKING;

			}
			cvCalcBackProject(&hImage, backProject, pHist);
			cvAnd(backProject, mask, backProject);				//backProject�� mask�� ��Ʈ �����Ͽ� �����Ϸ��� ������ ��ġ������ ���� �����ϰ� �������� 0�� �ǰ�

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