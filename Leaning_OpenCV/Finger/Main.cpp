#include"Header.h"
int main(){
	CvCapture* capture = cvCreateCameraCapture(0);
	if(!capture){
		printf("Failure\n");
		return -1;
	}
	runningCapture(capture);
	return 0;
}