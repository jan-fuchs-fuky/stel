#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import cv2
import pylab
import astropy.io.fits
import numpy as np

from scipy.optimize import leastsq

def fit_gauss_circular(xy, data):
	"""
	---------------------
	Purpose
	Fitting a star with a 2D circular gaussian PSF.
	---------------------
	Inputs
	* xy (list) = list with the form [x,y] where x and y are the integer positions in the complete image of the first pixel (the one with x=0 and y=0) of the small subimage that is used for fitting.
	* data (2D Numpy array) = small subimage, obtained from the full FITS image by slicing. It must contain a single object : the star to be fitted, placed approximately at the center.
	---------------------
	Output (list) = list with 6 elements, in the form [maxi, floor, height, mean_x, mean_y, fwhm]. The list elements are respectively:
	- maxi is the value of the star maximum signal,
	- floor is the level of the sky background (fit result),
	- height is the PSF amplitude (fit result),
	- mean_x and mean_y are the star centroid x and y positions, on the full image (fit results), 
	- fwhm is the gaussian PSF full width half maximum (fit result) in pixels
	---------------------
	"""
	
	#find starting values
	maxi = data.max()
	floor = np.ma.median(data.flatten())
	height = maxi - floor
	if height==0.0:				#if star is saturated it could be that median value is 32767 or 65535 --> height=0
		floor = np.mean(data.flatten())
		height = maxi - floor

	mean_x = (np.shape(data)[0]-1)/2
	mean_y = (np.shape(data)[1]-1)/2

	fwhm = np.sqrt(np.sum((data>floor+height/2.).flatten()))
	
	#---------------------------------------------------------------------------------
	sig = fwhm / (2.*np.sqrt(2.*np.log(2.)))
	width = 0.5/np.square(sig)
	
	p0 = floor, height, mean_x, mean_y, width

	#---------------------------------------------------------------------------------
	#fitting gaussian
	def gauss(floor, height, mean_x, mean_y, width):		
		return lambda x,y: floor + height*np.exp(-np.abs(width)*((x-mean_x)**2+(y-mean_y)**2))

	def err(p,data):
		return np.ravel(gauss(*p)(*np.indices(data.shape))-data)
	
	p = leastsq(err, p0, args=(data), maxfev=1000)
	p = p[0]
	
	#---------------------------------------------------------------------------------
	#formatting results
	floor = p[0]
	height = p[1]
	mean_x = p[2] + xy[0]
	mean_y = p[3] + xy[1]

	sig = np.sqrt(0.5/np.abs(p[4]))
	fwhm = sig * (2.*np.sqrt(2.*np.log(2.)))	
	
	output = [maxi, floor, height, mean_x, mean_y, fwhm]
	return output

class GetFWHM:

    def __init__(self):
        first_filename = sys.argv[1]

        self.stars = []
        self.focus = {}
        self.fwhm = {}

        self.load_fits(first_filename)
        self.get_stars()

        show_image = True
        for filename in sys.argv[1:]:
            self.load_fits(filename)
            print("=== %s ===" % filename)
            for star in self.stars:
                self.get_fwhm(star[0], star[1], show_image)
            show_image = False

        self.get_best_focus()

        print(self.focus)
        print(self.fwhm)

    def load_fits(self, filename):
        hdulist = astropy.io.fits.open(filename)

        prihdr = hdulist[0].header

        print("TELFOCUS = %s" % prihdr["TELFOCUS"])
        print("OBJECT = %s" % prihdr["OBJECT"])

        self.actual_focus = int(prihdr["TELFOCUS"])

        self.image = hdulist[0].data

        self.human_image = self.image
        #self.human_image = cv2.normalize(self.image, None, 0, 1, cv2.NORM_MINMAX, dtype=cv2.CV_32F)

        self.height, self.width = self.image.shape

    def get_stars(self):
        cv2.namedWindow("FITS")
        cv2.resizeWindow("FITS", self.width, self.height)
        cv2.setMouseCallback("FITS", self.mouse_callback)

        cv2.imshow("FITS", self.human_image)

        stars = []
        while (True):
            key = cv2.waitKey(1) & 0xFF

            if key == ord('p'):
                break

        cv2.destroyWindow("FITS")

    def get_fwhm(self, x, y, show_image):
        size = 20

        xy = [ x, y ]

        subimage = self.image[x-size:x+size+1, y-size:y+size+1]

        name = "%i_%i" % (x, y)

        if (show_image):
            cv2.imshow(name, subimage)
            cv2.waitKey(0)
            cv2.destroyWindow(name)

        maxi, floor, height, mean_x, mean_y, fwhm = fit_gauss_circular(xy, subimage)

        if name not in self.fwhm:
            self.focus[name] = []
            self.fwhm[name] = []

        self.focus[name].append(self.actual_focus)
        self.fwhm[name].append(fwhm)

        print("FWHM = %.1f (maxi = %.1f, floor = %.1f, height = %.1f, mean_x = %.1f, mean_y = %.1f)" %
            (fwhm, maxi, floor, height, mean_x, mean_y))

    def get_best_focus(self):

        for key in self.fwhm:
            begin = self.focus[key][0]
            end = self.focus[key][-1]

            if begin > end:
                tmp = begin
                begin = end
                end = tmp

            a, b, c = np.polyfit(self.focus[key], self.fwhm[key], 2)
            xp = np.arange(begin, end, 1)
            new_y = a * xp**2 + b * xp + c

            pylab.plot(self.focus[key], self.fwhm[key], "bo")
            pylab.plot(xp, new_y, "r-")
            pylab.show()

            min_idx = np.argmin(new_y)
            fwhm_best = new_y[min_idx]
            focus_best = xp[min_idx]

            print("fwhm_best = %.1f, focus_best = %i" % (fwhm_best, focus_best))

    def mouse_callback(self, event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONUP:
            self.stars.append([ y, x ])
            print(x, y)

def main():
    if (len(sys.argv) <= 1):
        print("Usage: %s FITS1 .. FITSN" % sys.argv[0])
        sys.exit()

    GetFWHM()

if __name__ == '__main__':
    main()
