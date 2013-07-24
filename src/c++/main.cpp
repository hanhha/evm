/**
 * Eulerian Video Magnification
 *
 * Copyright 2013 Chris Hiszpanski. All rights reserved.
 */

#include <deque>
#include <string>

#include <boost/array.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <opencv2/opencv.hpp>

/*!
 * Preview window. Creates and destroys window.
 */
struct Preview {
	/// Constructor
	Preview(const std::string& windowName) : windowName_(windowName) {
		cv::namedWindow(windowName_);
	}

	/// Display
	void operator()(const cv::Mat image) {
		cv::imshow(windowName_, image);
	}

	/// Destructor
	~Preview() {
		cv::destroyWindow(windowName_);
	}

	private:
		const std::string windowName_;
};

/*!
 * Filter. Temporally band-pass filters frames.
 */
template<typename T, std::size_t N>
struct Filter {

	/**
	 * Constructor
	 *
	 * @param  fc  Center frequency
	 * @param  bw  Bandwidth
	 * @param  sr  Sample rate
	 */
	Filter(T fc, T bw, T sr) : taps_(design(fc, bw, sr)) {
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

	private:
		/**
		 * Design the filter
		 *
		 * Difference of two low-pass filtered with Blackman window is used.
		 *
		 * @param  fc  Center frequency
		 * @param  bw  Bandwidth
		 * @param  sr  Sample rate
		 *
		 * @return  Impulse response
		 */
		boost::array<T, N> design(T fc, T bw, T sr) {
			boost::array<T, N> h;

			const T f1 = (fc - bw / 2) / sr;
			const T f2 = (fc + bw / 2) / sr;

			for (std::size_t n = 0; n < N; n++) {
				// Compute band-pass filter coefficient
				if (n == N / 2) {
					h[n] = 2 * (f2 - f1);
				} else {
					h[n] = (sin(2*M_PI * f2 * (n - N/2)) -
						    sin(2*M_PI * f1 * (n - N/2))) /
						   (M_PI * (n - (N/2)));
				}

				// Appy window (Blackman)
				h[n] *= 0.42 -  0.5 * cos(2*M_PI * n / N)
					         + 0.08 * cos(4*M_PI * n / N);
			}

			return h;
		}

		std::deque<cv::Mat> frames_;
		boost::array<T, N> taps_;

};

/*!
 * Prints help information
 */
void help() {
}

/*!
 * Prints version information and exists.
 */
void version() {
}

/*!
 * Main loop
 */
int main(int argc, char **argv)
{
//	Window window("demo");
	// TODO Parameterize filter length
	cv::VideoCapture videoInput;
	cv::VideoWriter videoOutput;

	boost::program_options::options_description desc("Options");
	desc.add_options()
		("help", "produce help message")
		("input-file",
		 boost::program_options::value<std::string>(),
		 "input video file")
		("output-file",
		 boost::program_options::value<std::string>(),
		 "output video file")
	;

	boost::program_options::positional_options_description pd;
	pd.add("input-file", 1).add("output-file", 2);

	boost::program_options::variables_map vm;
	boost::program_options::store(
			boost::program_options::command_line_parser(argc, argv).
				options(desc).
				positional(pd).
				run(),
			vm);
	boost::program_options::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		exit(0);
	}
	if (!vm.count("input-file")) {
		std::cout << "Please specify an input file" << std::endl;
		exit(-1);
	}
	if (!vm.count("output-file")) {
		std::cout << "Please specify an output file" << std::endl;
		exit(-1);
	}

	// Open video for reading
	videoInput.open(vm["input-file"].as<std::string>());
	assert(videoInput.isOpened());

	// Open video for writing
	// TODO Replace hard-coded fourcc with something better
	videoOutput.open(vm["output-file"].as<std::string>(),
			CV_FOURCC('M', 'J', 'P', 'G'),
			double(videoInput.get(CV_CAP_PROP_FPS)),
			cv::Size(int(videoInput.get(CV_CAP_PROP_FRAME_WIDTH)),
				     int(videoInput.get(CV_CAP_PROP_FRAME_HEIGHT))));
	assert(videoOutput.isOpened());

	Filter<double, 30> filter(1.5, 1., double(videoInput.get(CV_CAP_PROP_FPS)));

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
