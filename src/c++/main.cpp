/**
 * Eulerian Video Magnification
 *
 * Copyright 2013 Vecture Labs LLC. All rights reserved.
 */

#include <string>

#include <opencv2/opencv.hpp>

/*!
 * Window manager. Creates and destroys window.
 */
struct Window {
	/// Constructor
	Window(const std::string& windowName) : windowName_(windowName) {
		cv::namedWindow(windowName_);
	}

	/// Display
	void operator()(const cv::Mat image) {
		cv::imshow(windowName_, image);
	}

	/// Destructor
	~Window() {
		cv::destroyWindow(windowName_);
	}

	private:
		const std::string windowName_;
};

/*!
 * Filter. Temporally band-pass filters frames.
 */
#include <array>
#include <deque>
#include <boost/optional.hpp>
template<unsigned int N>
struct Filter {
	/**
	 * Constructor
	 */
	Filter() : taps_() {
	}

	/**
	 * Filtering operation
	 *
	 * @param  input  Frame (Note that cv::Mat is already a reference)
	 *
	 * @return  Boost::optional encapsulated frame
	 */
	boost::optional<cv::Mat> operator()(const cv::Mat input) {
		boost::optional<cv::Mat> output;

		frames_.push_front(input.clone());

		if (frames_.size() >= N) {
			output = boost::optional<cv::Mat>(frames_.back());
			frames_.pop_back();
		}

		return output;
	}

	/**
	 * TODO Make into separate func called by constructor member initializer?
	 *
	 * Design the filter
	 *
	 * Kaiser window method is used to design two low-pass filters which are
	 * subtracted from one another to yield a band-pass filter.
	 *
	 * @param  w_lower  Lower freq of pass-band, as fraction of sample rate
	 * @param  w_upper  Upper freq of pass-band, as fraction of sample rate
	 *
	 * @return  None
	 */
	void design(double w_lower, double w_upper) {

	}

	private:
		std::deque<cv::Mat> frames_;
		std::array<double, N> taps_;
};

/*!
 * Main loop
 */
int main(int argc, char **argv)
{
//	Window window("demo");
	// TODO Parameterize filter length
	Filter<30> filter;
	cv::VideoCapture videoInput;
	cv::VideoWriter videoOutput;

	// Check usage
	// TODO Replace with boost::program_options
	if (3 != argc) {
		std::cerr << "usage: evm <input video> <output video>" << std::endl;
		return -1;
	}

	// Open video for reading
	videoInput.open(argv[1]);
	assert(videoInput.isOpened());

	// Open video for writing
	// TODO Replace hard-coded fourcc with something better
	videoOutput.open(argv[2],
			CV_FOURCC('M', 'J', 'P', 'G'),
			double(videoInput.get(CV_CAP_PROP_FPS)),
			cv::Size(int(videoInput.get(CV_CAP_PROP_FRAME_WIDTH)),
				     int(videoInput.get(CV_CAP_PROP_FRAME_HEIGHT))));
	assert(videoOutput.isOpened());

	// Work loop
	{
		cv::Mat frame;

		// TODO Terminate loop when both videoInput _and_ filter are empty
		while (videoInput >> frame, !frame.empty()) {

			// Band-pass filter
			boost::optional<cv::Mat> filteredFrame;
			if (filteredFrame = filter(frame)) {
				videoOutput << filteredFrame.get();
			}
		}
	}

	return 0;
}
