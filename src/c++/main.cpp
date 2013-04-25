#include <vector>

#include <opencv2/opencv.hpp>

int main(int argc, char **argv)
{
	std::string windowName = "evm demo";

	// create window
	cv::namedWindow(windowName);

	// clean up
	cv::destroyWindow(windowName);

	return 0;
}
