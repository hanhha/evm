/*!
 * filter.hpp
 */

#include <array>
#include <deque>

#include <opencv2/opencv.hpp>

/*!
 * Band-pass filter
 */

template<class T, size_t N>
struct BPF
{
	typedef cv::Mat Frame;

	/// Constructor
	BPF() {
	}

	/// Filter
	Frame operator()(const Frame& input) {
		Frame output;

		// allocates new array data if needed
		output.create(input.size(), input.type());

		// shift frames in delay line
		frames_.pop_back();
		frames_.push_front(input);
#if 0
		// filter
		for (int y; y < input.rows; y++) {
			for (int x; x < input.cols; x++) {
				output.at<T>(y,x) = 0;
				// TODO more efficient to use iterator?
				for (unsigned int z; z < taps_.size(); z++) {
					output.at<T>(y,x) += taps_[z] * frames_[z].at<T>(y,x);
				}
			}
		}
#endif
		return output;
	}

	private:
		std::array<T, N> taps_;
		std::deque<Frame> frames_;
};
