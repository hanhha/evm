/*!
 * Eulerian Video Magnification (EVM)
 */

#ifndef FILTER_HPP
#define FILTER_HPP

#include <cmath>

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_floating_point.hpp>

#include <opencv2/opencv.hpp>

/**
 * Temporal band-pass video filter
 *
 * @param  T  sample type (must be either float or double)
 * @param  N  filter length
 */
template<typename T, std::size_t N>
struct Filter {

    // Convenience types
    typedef cv::Mat Frame;
    typedef boost::optional<Frame> OptionalFrame;

    // Verify that filter size is odd
    BOOST_STATIC_ASSERT(N % 2 == 1);

    // Verify that type is floating-point
    BOOST_STATIC_ASSERT(boost::is_floating_point<T>::value);

    /**
     * Constructor
     *
     * Designs filter upon initialization with member initializer
     *
     * @param  lo  Lower corner frequency of band-pass filter
     * @param  hi  Higher corner frequency of band-pass filter
     * @param  sr  Sample rate
     * @param  alpha  Magnification factor
     * @param  chromaAttenuation  Chromatic attenuation
     */
    Filter(T lo, T hi, T sr, T alpha = 50., T chromaAttenuation = 1.) :
        alpha_(alpha),
        chromaAttenuation_(chromaAttenuation),
        taps_(design(lo, hi, sr))
    {
        // Sanity check that filter was designed correctly. Must be symmetric.
        for (unsigned int i = 0; i < (N - 1) / 2; ++i) {
            BOOST_ASSERT_MSG(abs(taps_[i] - taps_[(N-1)-i]) < 1e-99,
                             "Filter not symmetric!");
        }
    }

    /**
     * Filtering operation
     *
     * @param  src  Input frame (32-bit floating-point) reference.
     * @param  dst  Output frame (32-bit floating-point) reference.
     *
     * @return  True if destination frame output, false otherwise.
     */
    bool filter(const Frame src, Frame dst) {
        // Enqueue copy (input reference will be overwritten next frame)
        frames_.push_front(src.clone());

        // Only output frame when filter queue is full
        if (frames_.size() >= taps_.max_size()) {
            Frame merged;

            // Filter
            std::vector<Frame> out_channels(3);
            out_channels[0] = Frame::zeros(src.size(), CV_32F);
            out_channels[1] = Frame::zeros(src.size(), CV_32F);
            out_channels[2] = Frame::zeros(src.size(), CV_32F);
 
            for (unsigned int n = 0; n < N; ++n) {
                std::vector<Frame> in_channels(3);
                cv::split(frames_[n], in_channels);

                out_channels[0] += in_channels[0] *
                                   static_cast<float>(taps_[n]);
                out_channels[1] += chromaAttenuation_ *
                                   in_channels[1] *
                                   static_cast<float>(taps_[n]);
                out_channels[2] += chromaAttenuation_ *
                                   in_channels[2] *
                                   static_cast<float>(taps_[n]);

            }

            // Merge Y, Cr, and Cb channels into single frame
            cv::merge(out_channels, dst);

            // Amplify
            dst *= alpha_;

            // Discard oldest frame
            frames_.pop_back();

            return true;
        }

        return false;
    }

    private:
        /**
         * Designs band-pass finite impulse response filter
         *
         * @param  lo  Low frequency
         * @param  hi  High frequency
         * @param  sr  Sample rate
         *
         * @return  Array of filter coefficients
         */
        static boost::array<T, N> design(T lo, T hi, T sr) {
            // Ensure that Nyquist is satisfied
            BOOST_ASSERT(lo <  sr / 2.);
            BOOST_ASSERT(hi <= sr / 2.);

            // Filter order, M, is one less than number of taps
            const std::size_t M = N - 1;

            // Container for impulse response
            boost::array<T, N> h;

            for (unsigned int n = 0; n < N; n++) {
                // Compute filter coefficient (i.e. difference of sincs)
                if (M / 2 == n) {
                    h[n] = 2. * (hi - lo) / sr;
                } else {
                    T t = M_PI * (static_cast<T>(n) - M / 2);
                    h[n] = sin(2. * hi / sr * t) / t -
                           sin(2. * lo / sr * t) / t;
                }

                // Apply blackman window coefficient
                h[n] *= 0.42 - 0.5  * cos(2.* M_PI * static_cast<T>(n) / M)
                             + 0.08 * cos(4.* M_PI * static_cast<T>(n) / M);
            }

            return h;
        }

        // History of frames
        std::deque<cv::Mat> frames_;

        // Filter coefficients
        boost::array<T, N> taps_;

        // Magnification factor
        T alpha_;

        // Chromatic attenuation
        T chromaAttenuation_;
};

#endif
