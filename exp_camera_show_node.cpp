#define ROS
//#define CAMERA_SHOW
//#define CAMERA_SHOW_MORE

#ifdef ROS
#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <sys/wait.h>
#include <termio.h>
#include <unistd.h>
#include "std_msgs/Int16.h"
#include <geometry_msgs/Twist.h>
#include "std_msgs/Int8.h"
#endif
#include <cv.hpp>
/*#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>//일반적으로 사용하는 OpenCV 헤더들
*/

#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>



#define PI 3.1415926
#define CAP_TIME 40
#define FRAME_DIFFER 25

#define ConnerHand 0.8
#define StraightHand 0

double error_differ = 0.0;
double error_sum = 0.0;
double P = 0.0;
double I = 0.0;
double D = 0.0;
float p_value = 0.8, i_value = 0.0, d_value = 0.2;
using namespace std;
using namespace cv;


const int Width = 320;
const int Height = 240;
const int ControlH = 60;
const int XHalf = (Width / 2);
const int YHalf = (Height / 2);
const int YPoint = 40;
const int YCarP = 0;

const int RoiWidth_left = 20;
const int RoiWidth_right = 280;
const int RoiXHalf = (RoiWidth_right / 2);

const float slope_threshold = 0.3;
const Scalar Red = Scalar(0, 0, 255);
const Scalar Blue = Scalar(255, 0, 0);
const Scalar Yellow = Scalar(50, 250, 250);
const Scalar Sky = Scalar(215, 200, 60);
const Scalar Pink = Scalar(220, 110, 230);

float ldistance, rdistance;
double integralation;
double past_value;
double derivative;

int flag_start_end = 0;
int lap_count = 0;
bool flag = true;
int check_point_detection(Mat& a) {
    int pixel_count = 0;
    int white_point = 0;
    Mat gray;

    cvtColor(a, gray, COLOR_BGR2GRAY);
    threshold(gray, gray, 200, 255, THRESH_BINARY);
    for (int i = 0; i < 240; i++) {
        white_point = gray.at<uchar>(30, i);
        if (white_point == 255) {
            pixel_count++;
            cout << "pixel:" << pixel_count << endl;
        }
    }
    if (pixel_count > 45 && flag) {
        flag = false;
        lap_count++;
    }
    if (pixel_count < 10) {
        flag = true;
    }
    return lap_count;
}

int pid(int value) {
	derivative = value - past_value;
	P = p_value*value;
	I = i_value*integralation;
	D = d_value*derivative;
	int pid_value = P+I+D;
	//std::cout<<pid_value;
	past_value = value;
	//integralation += derivative;
	// when sended stop signal
	if (value == 9999) {
		//spd = 0;
		pid_value = 0;
	}


	return pid_value;
}

void show_lines(Mat& img, vector<Vec4i>& lines, Scalar color = Scalar(0, 0, 0), int thickness = 1)
{
	bool color_gen = false;
	if (color == Scalar(0, 0, 0))
		color_gen = true;
	for (int i = 0; i < lines.size(); i++)
	{
		if (color_gen == true)
			color = Scalar(rand() % 256, rand() % 256, rand() % 256);
		line(img, Point(lines[i][0], lines[i][1]), Point(lines[i][2], lines[i][3]), color, thickness);
	}
}



void  split_left_right(vector<Vec4i> lines, vector<Vec4i>& left_lines, vector<Vec4i>& right_lines)
{
	vector<float> slopes;
	vector<Vec4i> new_lines;

	for (int i = 0; i < lines.size(); i++)
	{
		int x1 = lines[i][0];
		int y1 = lines[i][1];
		int x2 = lines[i][2];
		int y2 = lines[i][3];

		float slope;
		//Calculate slope
		if (x2 - x1 == 0) //corner case, avoiding division by 0
			slope = 999.0; //practically infinite slope
		else
			slope = (y2 - y1) / (float)(x2 - x1);

		//Filter lines based on slope
		if (abs(slope) > slope_threshold) {
			slopes.push_back(slope);
			new_lines.push_back(lines[i]);
		}
	}

	for (int i = 0; i < new_lines.size(); i++)
	{
		Vec4i line = new_lines[i];
		float slope = slopes[i];

		int x1 = line[0];
		int y1 = line[1];
		int x2 = line[2];
		int y2 = line[3];

		if (slope > 0 && x1 > RoiXHalf && x2 > RoiXHalf)//
			right_lines.push_back(line);
		else if (slope < 0 && x1 < RoiXHalf && x2 < RoiXHalf)//slope < 0 && x1 < cx && x2 < cx
			left_lines.push_back(line);
	}
}

bool find_line_params(vector<Vec4i>& left_lines, float* left_m, float* left_b)
{
	float  left_avg_x = 0, left_avg_y = 0, left_avg_slope = 0;
	if (left_lines.size() == 0)
		return false;
	for (int i = 0; i < left_lines.size(); i++)//calculate right avg of x and y and slope
	{
		//line(roi, Point(left_lines[i][0],left_lines[i][1]), Point(left_lines[i][2],left_lines[i][3]), color, 3);
		left_avg_x += left_lines[i][0];
		left_avg_x += left_lines[i][2];
		left_avg_y += left_lines[i][1];
		left_avg_y += left_lines[i][3];
		left_avg_slope += (left_lines[i][3] - left_lines[i][1]) / (float)(left_lines[i][2] - left_lines[i][0]);
	}
	left_avg_x = left_avg_x / (left_lines.size() * 2);
	left_avg_y = left_avg_y / (left_lines.size() * 2);
	left_avg_slope = left_avg_slope / left_lines.size();

	// return values
	*left_m = left_avg_slope;
	//b=y-mx //find Y intercet
	*left_b = left_avg_y - left_avg_slope * left_avg_x;

	return true;
}


void find_lines(Mat& img, vector<cv::Vec4i>& left_lines, vector<Vec4i>& right_lines, float* rdistance, float* ldistance)
{
	static float left_slope_mem = 1, right_slope_mem = 1, right_b_mem = 0, left_b_mem = 0;

	float left_b, right_b, left_m, right_m;

	bool draw_left = find_line_params(left_lines, &left_m, &left_b);
	if (draw_left) {
		float left_x0 = (-left_b) / left_m;
		float left_x120 = ((YHalf - ControlH) - left_b) / left_m;
		left_slope_mem = left_m;
		left_b_mem = left_b;
#ifdef CAMERA_SHOW
		line(img, Point(left_x0, 0), Point(left_x120, YHalf), Blue, 5);
		//cout << left_lines.size() << " left lines,";
#endif 
	}
	else {
		//cout << "\tNo Left Line,";
	}

	bool draw_right = find_line_params(right_lines, &right_m, &right_b);
	if (draw_right) {
		float right_x0 = (-right_b) / right_m;
		float right_x120 = ((YHalf - ControlH) - right_b) / right_m;
		right_slope_mem = right_m;
		right_b_mem = right_b;
#ifdef CAMERA_SHOW
		line(img, Point(right_x0, 0), Point(right_x120, YHalf), Red, 5);
#endif
		//cout << right_lines.size() << " right lines" << endl;
	}
	else {
		//cout << "\tNo RIght Line" << endl;
	}
	// y = mx + b ==> x0 = (y0-b)/m
	float left_xPt_Y60 = ((-left_b_mem) / left_slope_mem);
	float right_xPt_Y60 = ((-right_b_mem) / right_slope_mem);

	float left_xPt_Y0 = ((YCarP - left_b_mem) / left_slope_mem);
	float right_xPt_Y0 = ((YCarP - right_b_mem) / right_slope_mem);

	float l1distance = RoiXHalf - left_xPt_Y60;
	float r1distance = right_xPt_Y60 - RoiXHalf;
	float lr1differ = l1distance - r1distance;

	float l2distance = RoiXHalf - left_xPt_Y0;
	float r2distance = right_xPt_Y0 - RoiXHalf;

	float lr2differ = l2distance - r2distance;
	if (abs(lr1differ) < StraightHand && abs(lr2differ) < StraightHand) {
		*ldistance = l1distance / 2;
		*rdistance = r1distance / 2;
		cout << "--Straight--" << endl;
	}
	else if (lr1differ <= 0 && lr2differ <= 0) {
		//return lr2differ;
		*ldistance = l1distance;
		*rdistance = r1distance;
	}
	else if (lr1differ <= 0 && lr2differ > 0) {
		//return lr1differ;
		*ldistance = l1distance;
		*rdistance = r1distance;
	}
	else if (lr1differ > 0 && lr2differ <= 0) {
		//return lr1differ;
		*ldistance = l1distance;
		*rdistance = r1distance;
	}
	else if (lr1differ > 0 && lr2differ > 0) {
		*ldistance = l1distance;
		*rdistance = r1distance;
	}
	//return (lr1differ+lr2differ)/2;
}


int img_process(Mat& frame)
{
	Mat grayframe, edge_frame, roi_gray_ch3;
	Mat roi;
	Mat kernel = Mat::ones(7, 7, CV_8U);
	Mat processed;
	Mat close, open;
	//Mat Th;
	//cvtColor(frame, grayframe, COLOR_BGR2GRAY);
	Rect rect_roi(RoiWidth_left, YHalf, RoiWidth_right, YHalf - ControlH);
	roi = frame(rect_roi);

	/*Point2f srcVertices[4];
	srcVertices[0] = Point(35, 2);//왼쪽 위x,y
	srcVertices[1] = Point(301, 2);//오른쪽 위x,y
	srcVertices[2] = Point(306, 58);//오른쪽 아래x,y
	srcVertices[3] = Point(29, 58);//왼쪽 아래x,y
	Point2f dstVertices[4];
	dstVertices[0] = Point(0, 0);
	dstVertices[1] = Point(320, 0);
	dstVertices[2] = Point(320, 240);
	dstVertices[3] = Point(0, 240);
	Mat perspectiveMatrix = getPerspectiveTransform(srcVertices, dstVertices);
	Mat dst(240, 320, CV_8UC3); //Destination for warped image
	Mat invertedPerspectiveMatrix;
	invert(perspectiveMatrix, invertedPerspectiveMatrix);
	warpPerspective(roi, dst, perspectiveMatrix, dst.size(), INTER_LINEAR, BORDER_CONSTANT);*/
	//imshow("BirdEyeView", dst);
	//imshow("frame",frame);
	cvtColor(roi, grayframe, COLOR_BGR2GRAY);
	//bilatrealFilter(grayframe,grayframe,3,250,10,BORDER_DEFAULT);//bilateral
	cvtColor(grayframe, roi_gray_ch3, COLOR_GRAY2BGR);
	//dilate(grayframe, processed, kernel);
	//erode(grayframe, processed, kernel);
	/*morphologyEx(processed, open, MORPH_OPEN, kernel);
	adaptiveThreshold(open, open, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, -2);*/
	//imshow("adaptiveThreshold_Open", open);
	morphologyEx(grayframe, close, MORPH_CLOSE, kernel);
	adaptiveThreshold(close, close, 180, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, -2.3);
	//threshold(grayframe, Th, 140, 255, THRESH_BINARY);
	//imshow("Threshold", Th);
	//adaptiveThreshold(dst, dst, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 3, -1.5);
	//imshow("adaptiveThreshold_BirdEye", dst);
	//Canny(open, edge_frame, 80, 230, 7); //min_val, max val , filter size
	check_point_detection(roi);
	vector<cv::Vec4i> lines_set;

	cv::HoughLinesP(close, lines_set, 1, PI / 180, 30, 25, 5);
#ifdef CAMERA_SHOW_MORE
	show_lines(roi_gray_ch3, lines_set);
#endif
	vector<Vec4i> right_lines;
	vector<Vec4i> left_lines;
	split_left_right(lines_set, left_lines, right_lines);
#ifdef CAMERA_SHOW
	show_lines(roi, left_lines, Sky, 2);
	show_lines(roi, right_lines, Pink, 2);
#endif
	//float ldistance, rdistance;
	find_lines(roi, left_lines, right_lines, &rdistance, &ldistance);
	//differ = 4.5*differ;
		//int differ = ldistance - rdistance;
	int differ = (ldistance * ConnerHand) - (rdistance * ConnerHand);//differ to float`
#ifdef CAMERA_SHOW
	circle(roi, Point(RoiXHalf, YPoint), 5, Scalar(250, 250, 250), -1);
	circle(roi, Point(RoiXHalf + (int)differ / 4.5, YPoint), 5, Scalar(0, 0, 255), 2);
	putText(roi, format("%3d - %3d = %f", (int)rdistance, (int)ldistance, (int)differ / 4.5), Point(RoiXHalf - 100, YHalf / 2), FONT_HERSHEY_SIMPLEX, 0.5, Yellow, 2);
	imshow("roi", roi);
	//imshow("Open",open);
	imshow("Close",close);
	//imshow("edgeframe",edge_frame);
#endif
#ifdef CAMERA_SHOW_MORE
//	imshow("frame", frame);
//	imshow("roi_gray_ch3", roi_gray_ch3);
#endif
//	differ = pid(differ);
	return differ;
}
vector<int> differ_mean_v(FRAME_DIFFER);



int caculate_mean_differ(int value){
	double caculate_differ[FRAME_DIFFER] = {0,};
	int i;
	double return_differ=0;
		differ_mean_v.insert(differ_mean_v.begin(),value);
		differ_mean_v.erase(differ_mean_v.begin()+(int)FRAME_DIFFER);
		for(i = 0; i<FRAME_DIFFER; i++){
			/*if(i>=0 && i<5){
				caculate_differ[0] += differ_mean_v[i];
			}
			else if(i>=1 && i<2){
                        	caculate_differ[1] += differ_mean_v[i];
                	}
			else if(i>=2 && i<3){
                        	caculate_differ[2] += differ_mean_v[i];
                	}
			else if(i>=3 && i<4){
                        	caculate_differ[3] += differ_mean_v[i];
                	}
			else if(i>=4 && i<5){
                        	caculate_differ[4] += differ_mean_v[i];
        		}*/
			caculate_differ[i] = differ_mean_v[i];
		}
		/*for(i = 0 ;i<FRAME_DIFFER; i++){
			caculate_differ[i] = caculate_differ[i]/1.0;
		}*/
		return_differ += caculate_differ[0];
		for(i = 1; i<FRAME_DIFFER; i++){
			return_differ += (caculate_differ[i]*1.0*pow(0.165,i));
		}
		return_differ = return_differ/1.20; 
	return (int)return_differ;
}

struct timespec begin, end;
int main(int argc, char** argv)
{
	VideoCapture cap(0);//cameara_input
	//VideoCapture cap("C:/Users/GeeksMethod/source/repos/RegisTest/x64/Debug/clockwise.mp4");
  //VideoCapture cap("/home/cseecar/catkin_ws/video/clockwise.mp4");
	Mat frame;
	Mat img;
	if (!cap.isOpened()) {
		std::cout << "no camera!" << std::endl;
		return -1;
	}
#ifdef ROS
	ros::init(argc, argv, "cam_msg_publisher");
	ros::NodeHandle nh;
	std_msgs::Int16 cam_msg;
	ros::Publisher pub = nh.advertise<std_msgs::Int16>("cam_msg", 100);
        ros::init(argc, argv, "msg_key_publisher");
	 ros::NodeHandle nh3;

        //ros::Publisher pub2 = nh3.advertise<std_msgs::Int8>("data_msg", 100);
        ros::Publisher pub2 = nh3.advertise<geometry_msgs::Twist>("data_msg_key", 100);
        //ros::Rate loop_rate(1);
        geometry_msgs::Twist cmd;
    std_msgs::Int8 msg2;

        bool turn=true;

	int init_past = 1;
	//--------------------------------------------------
	ros::Rate loop_rate(50);
	cout << "start" << endl;
#endif
	int differ, key, fr_no = 0;
	bool capture = true;
	clock_gettime(CLOCK_MONOTONIC,&begin);
        int begin_t = (begin.tv_sec+begin.tv_nsec/1000000000.0)*1000.0;
	//clock_t tStart = clock();
	for (;;) {
		clock_gettime(CLOCK_MONOTONIC,&end);
                int end_t = (end.tv_sec + end.tv_nsec/1000000000.0)*1000.0;
                //cout << end_t-begin_t+CAP_TIME << endl;
		if(end_t<=begin_t+CAP_TIME){
                        continue;
                }
                else {
                        cout<<"delay :" << end_t-(begin_t+CAP_TIME) << endl;
                        begin_t = end_t;
                }
		if(capture) {
			cap >> frame;
			if (frame.empty())
				break;
		}

		if ((key = waitKey(30)) >= 0) {
			if (key == 27)
				break;
			else if (key = ' ') {
				capture = !capture;
			}
		}

		if (capture == false)
			continue;

		fr_no++;
		resize(frame, frame, Size(Width, Height));
		differ = img_process(frame);
		differ = caculate_mean_differ(differ);
		//differ = pid(differ);
#ifdef ROS
		cam_msg.data = differ+10;
		cout<<"differ : "<<cam_msg.data<<"lap_count : "<<lap_count<<endl;
		if(lap_count == 2){
			capture = !capture;
                	turn = !turn;
                	cmd.linear.y=turn;
                	pub2.publish(cmd);
		}
		pub.publish(cam_msg);
		loop_rate.sleep();
#else
		std::cout << fr_no << ":" << differ << endl;
#endif
	}

	std::cout << "Camera off" << endl;
	return 0;
}
