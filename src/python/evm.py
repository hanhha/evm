#!/usr/bin/env python

"""
Eulerian Video Magnification (EVM) Demo

Chris Hiszpanski <chiszp@gmail.com>
"""

import time
import sys

import cv2
import numpy as np


def band_pass_filter(input, wl, wh, sample_rate):
	"""Applies ideal band-pass filter to a given video

	Arguments:
		input : video to be filtered (as a 4-d numpy array (time, height,
		        width, channels))
		wl : lower cut-off frequency of band-pass filter
		wh : upper cut-off frequency of band-pass filter
		sample_rate : 

	Return:
		Temporally filtered video as 4-d array
	"""
	num_frames, height, width, num_channels = input.shape

	# construct 1-d frequency domain mask
	freq = range(num_frames)
	freq = map(lambda x: float(x) / num_frames * sample_rate, freq)
	mask = np.asarray(map(lambda x: x > wl and x < wh, freq))

	# copy mask such that it applies to each pixel and channel
	#
	# important: numpy prepends new axes by default whereas matlab postpends
	#            hence, we use np.newaxis to explicitly define postpending.
	mask = np.tile(mask[:,np.newaxis,np.newaxis,np.newaxis], \
	               (1, height, width, num_channels))

	# transform input frames into frequency domain
	F = np.fft.fft(input, axis=0)

	# suppress out-of-band frequencies
	F[np.invert(mask)] = 0

	# transform back into time domain
	return np.real(np.fft.ifft(F, axis=0))


class EVM():
	"""Eulerian Video Magnification"""

	def __init__(self, filename):
		"""Constructor"""
		from cv import CV_CAP_PROP_FPS
		from cv import CV_CAP_PROP_FRAME_WIDTH
		from cv import CV_CAP_PROP_FRAME_HEIGHT
		from cv import CV_CAP_PROP_FRAME_COUNT

		# create new handle to video
		self.video = cv2.VideoCapture(filename)

		self.frameWidth = int(self.video.get(CV_CAP_PROP_FRAME_WIDTH))
		self.frameHeight = int(self.video.get(CV_CAP_PROP_FRAME_HEIGHT))
		self.frameCount = int(self.video.get(CV_CAP_PROP_FRAME_COUNT))
		self.numChannels = 3

		# create windows for original and processed video
		cv2.namedWindow("original")
		cv2.namedWindow("processed")

		# position displayed windows
		cv2.moveWindow("original", 100, 100)
		cv2.moveWindow("processed", 100 + self.frameWidth + 50, 100)

		# allocate memory for input frames
		self.in_frames = np.ndarray(shape=(self.frameCount, \
		                                   self.frameHeight, \
		                                   self.frameWidth, \
		                                   self.numChannels), \
		                            dtype=np.float32)

	def process(self, numlevels=4, alpha=50., chromAttenuation=1.):
		"""Process video

		Arguments:
			numlevels : Number of pyramid levels to compute
		"""
		from cv import CV_32F
		from cv import CV_BGR2YCrCb, CV_YCrCb2BGR
		from pyramid import gaussian

		print 'reading in video...'
		for frameNumber in xrange(self.frameCount):

			# read frame of video
			(retval, frame) = self.video.read()
			if not retval:
				break

			# convert to floating-point (no double in opencv)
			frame = np.asarray(frame, dtype=np.float32) / 255.

			# separate luminance and chrominance
			frame = cv2.cvtColor(frame, CV_BGR2YCrCb)

			# store frame into memory
			self.in_frames[frameNumber] = frame

		print 'computing pyramid...'
		# compute pyramid on first frame
		pyramid = gaussian(self.in_frames[0], numlevels)
		height, width, _ = pyramid[-1].shape

		# allocate memory for downsampled frames
		self.ds_frames = np.ndarray(shape=(self.frameCount, \
		                                   height, \
		                                   width, \
		                                   self.numChannels), \
		                            dtype=np.float32)
		self.ds_frames[0] = pyramid[-1]
		                                      
		for frameNumber in xrange(1, self.frameCount):

			# spatial decomposition (specify laplacian or gaussian)
			pyramid = gaussian(self.in_frames[frameNumber], numlevels)

			# store downsampled frame into memory
			self.ds_frames[frameNumber] = pyramid[-1]

		print 'filtering...'
		output = band_pass_filter(self.ds_frames, 50./60., 60./60., 30)

		print 'amplifying...'
		output[:,:,:,0] *= alpha
		output[:,:,:,1] *= (alpha * chromAttenuation)
		output[:,:,:,2] *= (alpha * chromAttenuation)

		for i in xrange(self.frameCount):
			from cv import CV_INTER_CUBIC

			orig = self.in_frames[i]

			# enlarge to match size of original frame (keep as 32-bit float)
			filt = cv2.resize(output[i], (self.frameWidth, self.frameHeight), \
			                  interpolation=CV_INTER_CUBIC)
			filt = filt.astype(np.float32)

			filt = filt + orig

			filt = cv2.cvtColor(filt, CV_YCrCb2BGR)

			filt[filt > 1] = 1
			filt[filt < 0] = 0
#			filt = cv2.add(orig, filt, dtype=CV_32F)

			# preview
			cv2.imshow('original', cv2.cvtColor(orig, CV_YCrCb2BGR))
			cv2.imshow('processed', filt)
			cv2.waitKey(20)

if __name__ == '__main__':
	if (len(sys.argv) != 2):
		print 'usage: evm <file>'
		sys.exit(-1)

	evm = EVM(sys.argv[1])

	evm.process()
