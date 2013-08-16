/*!
 * Eulerian Video Magnification (EVM)
 */

// Use instead of boost::program_options as it requires linking against boost
#include <getopt.h>

/**
 * Standard C Library
 */
#include <cassert>
#include <cstdio>
#include <deque>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/**
 * Boost C++ Library
 *
 * Only use header-only templates so that compilation of boost is not
 * necessary. if the user does not have boost installed, the bundled version
 * can be used.
 */
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

// OpenCV
#include <opencv2/opencv.hpp>

#include "filter.hpp"


/**
 * Print help message and exit successfully
 */
void help(std::ostream& os)
{
    os << "Usage: evm [options] <input-file> <output-file>" << std::endl
       << std::endl;

    os << "Options:" << std::endl
       << "  -a, --alpha=NUM     Magnification factor" << std::endl
       << "  -c, --chromatt=NUM  Color attenuation factor" << std::endl
       << "  -l, --lower=NUM     Lower bandpass frequency (in Hz)" << std::endl
       << "  -u, --upper=NUM     Upper bandpass frequency (in Hz)" << std::endl
       << "  -h, --help          Print this message and exit" << std::endl
       << "  -v, --version       Print version and exit" << std::endl
       << std::endl;

    exit(EXIT_SUCCESS);
}

/**
 * Print version and exit successfully
 */
void version(std::ostream& os)
{
    os << "Version 0.1.0" << std::endl;

    os << "Boost version: "
       << BOOST_VERSION / 100000     << '.'
       << BOOST_VERSION / 100 % 1000 << '.'
       << BOOST_VERSION % 100        << std::endl
       << std::endl;

    exit(EXIT_SUCCESS);
}

/**
 * Parse command-line arguments
 */
boost::unordered_map<std::string, std::string> argparse(int argc, char **argv)
{
    boost::unordered_map<std::string, std::string> m;

    // Default option values
    m["alpha"] = std::string("50");
    m["lower"] = std::string("0.8333");
    m["upper"] = std::string("1.0");
    m["chromatt"] = std::string("1.0");

    while (true) {
        int c;
        int option_index = 0;

        static struct option long_options[] = {
            { "alpha",      1, 0, 'a' },
            { "chromatt",   1, 0, 'c' },
            { "help",       0, 0, 'h' },
            { "lower",      1, 0, 'l' },
            { "upper",      1, 0, 'u' },
            { "version",    0, 0, 'v' },
            { 0,            0, 0,  0  }
        };

        c = getopt_long(argc, argv, "a:c:hl:u:v", long_options, &option_index);
        if (-1 == c)
            break;

        switch (c) {
            case 'a':
                m["alpha"] = std::string(optarg);
                break;
            case 'c':
                m["chromatt"] = std::string(optarg);
                break;
            case 'h':
                help(std::cout);
                break;
            case 'l':
                m["lower"] = std::string(optarg);
                break;
            case 'u':
                m["upper"] = std::string(optarg);
                break;
            case 'v':
                version(std::cout);
                break;
            case '?':
            default:
                break;
        }
    }

    // Input file
    if (optind < argc) {
        m["input-file"] = std::string(argv[optind]);
    } else {
        std::cerr << "Error: Missing input file" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Output file
    if (optind + 1 < argc) {
        m["output-file"] = std::string(argv[optind + 1]);
    } else {
        std::cerr << "Error: Missing output file" << std::endl;
        exit(EXIT_FAILURE);
    }

    return m;
}

/**
 * Main loop
 */
int main(int argc, char **argv)
{
    boost::unordered_map<std::string, std::string> argm = argparse(argc, argv);

    const std::string inputFile  = argm.at("input-file");
    const std::string outputFile = argm.at("output-file");
    const double alpha = boost::lexical_cast<double>(argm.at("alpha"));
    const double upper = boost::lexical_cast<double>(argm.at("upper"));
    const double lower = boost::lexical_cast<double>(argm.at("lower"));
    const double chromatt = boost::lexical_cast<double>(argm.at("chromatt"));
    const unsigned int num_levels = 4;

	cv::VideoCapture videoInput;
	cv::VideoWriter videoOutput;

	// Open video for reading
    videoInput.open(inputFile);
    assert(videoInput.isOpened());

	// Open video for writing
	videoOutput.open(outputFile,
		CV_FOURCC('P', 'I', 'M', '1'),
		static_cast<double>(videoInput.get(CV_CAP_PROP_FPS)),
		cv::Size(static_cast<int>(videoInput.get(CV_CAP_PROP_FRAME_WIDTH)),
			     static_cast<int>(videoInput.get(CV_CAP_PROP_FRAME_HEIGHT))));
	assert(videoOutput.isOpened());
    
    // Instantiate filter
	Filter<double, 119> filter(lower,
                              upper, 
                              float(videoInput.get(CV_CAP_PROP_FPS)),
                              alpha,
                              chromatt);

    // Get number of frames
    unsigned int frame_num = 0;
    const unsigned int num_frames =
        static_cast<unsigned int>(videoInput.get(CV_CAP_PROP_FRAME_COUNT));

	// Work loop
	{
		cv::Mat frame;

		while (videoInput >> frame, !frame.empty()) {
            cv::Mat proc;

            frame_num++;

            // Convert to 32-bit floating point image. This avoids information
            // loss later on.
            {
                cv::Mat tmp;
                frame.convertTo(tmp, CV_32F, 1./255);
                frame = tmp;
            }

            // Separate luminance and chrominance components
            cv::cvtColor(frame, frame, CV_BGR2YCrCb);

            // Down-scale via gaussian pyramid
            {
                cv::Mat in = frame;
                cv::Mat out;

                for (int i = 0; i < num_levels; i++) {
                    cv::pyrDown(in, out);
                    in = out;
                }

                proc = out;
            }

			// Band-pass filter and amplify
			if (!filter.filter(proc, proc)) {
                continue;
            }

            // Scale up to match original
            {
                cv::Mat tmp;
                cv::resize(proc, tmp, frame.size(), 0, 0, cv::INTER_CUBIC);
                proc = tmp;
            }

            // Recombine
            frame += proc;

            // Convert back into RGB colorspace
            cv::cvtColor(frame, frame, CV_YCrCb2BGR);

            // convert back to 8-bit
            {
                cv::Mat tmp;
                frame.convertTo(tmp, CV_8U, 255.);
                frame = tmp;
            }

            // Write processed frame to output file
            videoOutput << frame;

            // Update percentage complete message
            fprintf(stderr, "Complete: %02.1f %%\r",
                100.0 * static_cast<float>(frame_num) /
                        static_cast<float>(num_frames));
		}
	}

	return EXIT_SUCCESS;
}
