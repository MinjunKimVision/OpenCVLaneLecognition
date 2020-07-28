
#include <cv.hpp>
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <iostream>

using namespace std;
using namespace cv;


int  main()
{
	// 입력 이미지
	Mat src = imread("C:/Users/B85MGB/Pictures/road.jpg", IMREAD_COLOR);

	// 실행 시간 측정 
	static int64 Gaussian_start, Gaussian_end;
	static float Gaussian_time;
	static int64 median_start, median_end;
	static float median_time;
	static int64 bilatreal_start, bilatreal_end;
	static float bilatreal_time;
	Mat Gaussian_gray, src_gray, median_gray, Bilatreal_gray;
	Mat Gaussian_canny, median_canny, Bilatreal_canny;
	//Gaussian
	cout << " Gaussian Test ";

	Gaussian_start = getTickCount();
	for (int i = 0; i < 4000; i++) {
		Mat Gaussian;
		cv::GaussianBlur(src, Gaussian, Size(3, 3), 5, 5, BORDER_DEFAULT);
		cv::cvtColor(Gaussian, Gaussian_gray, COLOR_RGB2GRAY);
		cv::Canny(Gaussian_gray, Gaussian_canny, 30, 150);
		vector<Vec4i> Gaussian_lines;
		HoughLinesP(Gaussian_canny, Gaussian_lines, 1, CV_PI / 180, 30, 30, 3);
		for (int i = 0; i < Gaussian_lines.size(); i++)
		{
			Vec4i gl = Gaussian_lines[i];
			line(Gaussian, Point(gl[0], gl[1]), Point(gl[2], gl[3]),
				Scalar(0, 0, 255), 1, LINE_AA);
		}
	}
	Gaussian_end = getTickCount();
	Gaussian_time = (Gaussian_end - Gaussian_start) / getTickFrequency();
	cout << " --- Done. Processing Time : " << Gaussian_time << " sec. " << endl;

	cout << " median Test ";
	//median
	median_start = getTickCount();
	for (int i = 0; i < 4000; i++) {
		Mat median;
		cv::medianBlur(src, median, 3);
		cv::cvtColor(median, median_gray, COLOR_RGB2GRAY);
		cv::Canny(median_gray, median_canny, 30, 150);
		vector<Vec4i> median_lines;
		HoughLinesP(median_canny, median_lines, 1, CV_PI / 180, 30, 30, 3);
		for (int i = 0; i < median_lines.size(); i++)
		{
			Vec4i gl = median_lines[i];
			line(median, Point(gl[0], gl[1]), Point(gl[2], gl[3]),
				Scalar(0, 0, 255), 1, LINE_AA);
		}
	}
	median_end = getTickCount();
	median_time = (median_end - median_start) / getTickFrequency();
	cout << " --- Done. Processing Time : " << median_time << " sec. " << endl;

	cout << " bilatreal Test ";
	//Bilatreal
	bilatreal_start = getTickCount();
	for (int i = 0; i < 4000; i++) {
		Mat bilatreal;
		cv::bilateralFilter(src, bilatreal, 3, 250, 10, BORDER_DEFAULT);
		cv::cvtColor(bilatreal, Bilatreal_gray, COLOR_RGB2GRAY);
		cv::Canny(Bilatreal_gray, Bilatreal_canny, 30, 150);
		vector<Vec4i> bilatreal_lines;
		HoughLinesP(Bilatreal_canny, bilatreal_lines, 1, CV_PI / 180, 30, 30, 3);
		for (int i = 0; i < bilatreal_lines.size(); i++)
		{
			Vec4i gl = bilatreal_lines[i];
			line(bilatreal, Point(gl[0], gl[1]), Point(gl[2], gl[3]),
				Scalar(0, 0, 255), 1, LINE_AA);
		}
	}
	bilatreal_end = getTickCount();
	bilatreal_time = (bilatreal_end - bilatreal_start) / getTickFrequency();
	cout << " --- Done. Processing Time : " << bilatreal_time << " sec. " << endl;

	return  0;
}
