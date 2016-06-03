// WatershedTest.cpp : Defines the entry point for the console application.
//

#include <opencv2/core/utility.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include <iostream>

using namespace cv;
using namespace std;

Mat markerMask, img;
Point prevPt(-1, -1);

static void onMouse(int event, int x, int y, int flags, void*) {
	if (x < 0 || x >= img.cols || y < 0 || y >= img.rows)
		return;
	if (event == EVENT_LBUTTONUP || !(flags & EVENT_FLAG_LBUTTON))
		prevPt = Point(-1, -1);
	else if (event == EVENT_LBUTTONDOWN)
		prevPt = Point(x, y);
	else if (event == EVENT_MOUSEMOVE && (flags & EVENT_FLAG_LBUTTON))
	{
		Point pt(x, y);
		if (prevPt.x < 0)
			prevPt = pt;
		line(markerMask, prevPt, pt, Scalar(255, 255, 255), 5, 8, 0);
		line(img, prevPt, pt, Scalar(255, 0, 255), 5, 8, 0);
		prevPt = pt;
		imshow("image", img);
	}
}

Mat segmentImage(Mat img0, Mat localMarkerMask)
{
	int i, j, compCount = 0;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(localMarkerMask, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

	Mat markers(localMarkerMask.size(), CV_32S);
	markers = Scalar::all(0);
	int idx = 0;
	for (; idx >= 0; idx = hierarchy[idx][0], compCount++)
		drawContours(markers, contours, idx, Scalar::all(compCount + 1), -1, 8, hierarchy, INT_MAX);

	//generate random colors for segments
	vector<Vec3b> colorTab;
	for (i = 0; i < compCount; i++)
	{
		int b = theRNG().uniform(0, 255);
		int g = theRNG().uniform(0, 255);
		int r = theRNG().uniform(0, 255);
		colorTab.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
	}

	watershed(img0, markers);

	//create output image and fill in based on indices from watershed call
	Mat wshed(markers.size(), CV_8UC3);
	for (i = 0; i < markers.rows; i++)
		for (j = 0; j < markers.cols; j++)
		{
			int index = markers.at<int>(i, j);
			if (index == -1)
			{
				//if on border, set default color to white
				//then check 8 adjacent pixels, setting this pixel to
				//the color of the first adjacent pixel which is not white
				Vec3b color = Vec3b(255, 255, 255);
				for (int y = i - 1; y <= i + 1; y++)
				{
					for (int x = j - 1; x <= j + 1; x++)
					{
						if ((y > 0 && x > 0) && (y < markers.rows && x < markers.cols))
						{
							int index2 = markers.at<int>(y, x);
							if (index2 >= 0 && index2 <= compCount)
							{
								color = colorTab[index2 - 1];
							}
						}
					}
				}
				wshed.at<Vec3b>(i, j) = color;
			}
			else if (index <= 0 || index > compCount)
				wshed.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
			else
				wshed.at<Vec3b>(i, j) = colorTab[index - 1];
		}
	return wshed;
}

int main()
{
	String filename = "C:/image.jpg";
	Mat img0 = imread(filename, 1), imgGray;
	namedWindow("image", 1);

	//copy original image (img0) to working image (img)
	//create grey masks
	img0.copyTo(img);
	cvtColor(img, markerMask, COLOR_BGR2GRAY);
	cvtColor(markerMask, imgGray, COLOR_GRAY2BGR);
	markerMask = Scalar::all(0);
	imshow("image", img);
	setMouseCallback("image", onMouse, 0);

	Mat outputWshed;
	bool watershedRun = false;

	for (;;)
	{
		int c = waitKey(0);
		if ((char)c == 27) break;
		if ((char)c == 'r')
		{
			//reset image
			markerMask = Scalar::all(0);
			img0.copyTo(img);
			imshow("image", img);
		}
		if ((char)c == 'w' || (char)c == ' ')
		{
			//run watershed
			outputWshed = segmentImage(img0, markerMask);
			namedWindow("wsresult", 1);
			imshow("wsresult", outputWshed);
			watershedRun = true;
		}
	}
}

