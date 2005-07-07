/*
 * The GIMP Foreground Extraction Utility 
 * segmentator.c - main algorithm.
 *
 * For algorithm documentation refer to:
 * G. Friedland, K. Jantz, L. Knipping, R. Rojas: 
 * "Image Segmentation by Uniform Color Clustering -- Approach and Benchmark Results", 
 * Technical Report B-05-07, Department of Computer Science, Freie Universitaet Berlin, June 2005.
 * http://www.inf.fu-berlin.de/inst/pubs/tr-b-05-07.pdf
 * 
 * Algorithm idea by Gerald Friedland. 
 * This implementation is Copyright (C) 2005 by Gerald Friedland <fland@inf.fu-berlin.de> 
 * and Kristian Jantz <jantz@inf.fu-berlin.de>. 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Revision History:
 * Version 0.1 - 2005-06-16 initial version
 * Version 0.2 - 2005-06-22 several bug fixes and optimized using MS Benchmark
 * Version 0.3 - 2005-06-23 tested it and fixed several bugs. Now same error rate like Java version.
 * Version 0.4 - 2005-06-27 tested it further and improved segmentation error to 4.25% (MS Benchmark)
 *
 * TODO:
 * - Improve speed (see paper)
 * - Find further bugs
 *
 * To compile use:
 * gcc -ansi -pedantic -Wall -fexpensive-optimizations -O5 -ffast-math -c segmentator.c -o segmentator.o
 * Need to be linked with: -lm (math library)
 * 
 * Instructions:
 * Call function segmentate as documented.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "segmentator.h"

/* Simulate a java.util.ArrayList */
/* These methods are NOT generic */

typedef struct {
	float l;
	float a;
	float b;
	int cardinality;
} lab;


typedef struct {
	lab *array;
	int arraylength;
	void *next;
} ArrayList;

void addtoList(ArrayList * list, lab * newarray, int newarraylength)
{
	ArrayList *cur = list;
	ArrayList *prev;
	do {
		prev = cur;
		cur = cur->next;
	} while (cur);
	prev->next = malloc(sizeof(ArrayList));
	((ArrayList *) (prev->next))->next = NULL;
	((ArrayList *) (prev->next))->array = NULL;
	((ArrayList *) (prev->next))->arraylength = 0;
	prev->array = newarray;
	prev->arraylength = newarraylength;
}

int listSize(ArrayList * list)
{
	int count = 0;
	ArrayList *cur = list;
	while (cur->array) {
		count++;
		cur = cur->next;
	}
	return count;
}

lab *listToArray(ArrayList * list, int *returnlength)
{
	int i = 0, len;
	ArrayList *cur = list;
	lab *arraytoreturn;
	len = listSize(list);
	arraytoreturn = malloc(len * sizeof(lab));
	returnlength[0] = len;
	while (cur->array) {
		arraytoreturn[i++] = cur->array[0];	/* Every array in the list node has only one point when we call this method */
		cur = cur->next;
	}
	return arraytoreturn;
}

void freelist(ArrayList * list)
{
	ArrayList *cur = list;
	ArrayList *prev;
	do {
		prev = cur;
		cur = cur->next;
		if (prev->array)
			free(prev->array);
		free(prev);
	} while (cur);
}

/* RGB -> CIELAB and other interesting methods... */

unsigned char getRed(unsigned int rgb)
{
	return (rgb) & 0xFF;
}

unsigned char getGreen(unsigned int rgb)
{
	return (rgb >> 8) & 0xFF;
}

unsigned char getBlue(unsigned int rgb)
{
	return (rgb >>16) & 0xFF;
}

unsigned char getAlpha(unsigned int rgb)
{
	return (rgb >> 24) & 0xFF;
} 


/* Gets an int containing rgb, and an lab struct */
lab *calcLAB(unsigned int rgb, lab * newpixel)
{
	float var_R = (getRed(rgb) / 255.0);
	float var_G = (getGreen(rgb) / 255.0);
	float var_B = (getBlue(rgb) / 255.0);
	float X, Y, Z, var_X, var_Y, var_Z;

	if (var_R > 0.04045)
		var_R = (float) pow((var_R + 0.055) / 1.055, 2.4);
	else
		var_R = var_R / 12.92;
	if (var_G > 0.04045)
		var_G = (float) pow((var_G + 0.055) / 1.055, 2.4);
	else
		var_G = var_G / 12.92;

	if (var_B > 0.04045)
		var_B = (float) pow((var_B + 0.055) / 1.055, 2.4);
	else
		var_B = var_B / 12.92;

	var_R = var_R * 100.0;
	var_G = var_G * 100.0;
	var_B = var_B * 100.0;

	/* Observer. = 2°, Illuminant = D65 */
	X = (float) (var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805);
	Y = (float) (var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722);
	Z = (float) (var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505);

	var_X = X / 95.047;	/* Observer = 2, Illuminant = D65 */
	var_Y = Y / 100.0;
	var_Z = Z / 108.883;

	if (var_X > 0.008856)
		var_X = (float) pow(var_X, (1.0 / 3));
	else
		var_X = (7.787 * var_X) + (16.0 / 116);
	if (var_Y > 0.008856)
		var_Y = (float) pow(var_Y, (1.0 / 3));
	else
		var_Y = (7.787 * var_Y) + (16.0 / 116);
	if (var_Z > 0.008856)
		var_Z = (float) pow(var_Z, (1.0 / 3));
	else
		var_Z = (7.787 * var_Z) + (16.0 / 116);

	newpixel->l = (116 * var_Y) - 16;
	newpixel->a = 500 * (var_X - var_Y);
	newpixel->b = 200 * (var_Y - var_Z);
	return newpixel;
}

float cie_f(float t)
{
	return t > 0.008856 ? (1 / 3.0) : 7.787 * t + 16.0 / 116.0;
}


/* Stage one of modified KD-Tree algorithm */
void stageone(lab * points, int dims, int depth, ArrayList * clusters,
	      float limits[DIMS], int length)
{
	int curdim = depth % dims;
	float min, max;
	/* find maximum and minimum */
	int i, countsm, countgr, smallc, bigc;
	float pivotvalue, curval;
	lab *smallerpoints;
	lab *biggerpoints;


	if (length < 1)
		return;
	if (curdim == 0)
		curval = points[0].l;
	else if (curdim == 1)
		curval = points[0].a;
	else
		curval = points[0].b;
	min = curval;
	max = curval;

	for (i = 1; i < length; i++) {
		if (curdim == 0)
			curval = points[i].l;
		else if (curdim == 1)
			curval = points[i].a;
		else if (curdim == 2)
			curval = points[i].b;
		if (min > curval)
			min = curval;
		if (max < curval)
			max = curval;
	}
	if (max - min > limits[curdim]) {	/* Split according to Rubner-Rule */
		/* split */

		pivotvalue = ((max - min) / 2.0) + min;
		countsm = 0;
		countgr = 0;
		for (i = 0; i < length; i++) {	/* find out cluster sizes */
			if (curdim == 0)
				curval = points[i].l;
			else if (curdim == 1)
				curval = points[i].a;
			else if (curdim == 2)
				curval = points[i].b;
			if (curval <= pivotvalue) {
				countsm++;
			} else {
				countgr++;
			}
		}

		smallerpoints = malloc(countsm * sizeof(lab));	/* allocate mem */
		biggerpoints = malloc(countgr * sizeof(lab));
		smallc = 0;
		bigc = 0;
		for (i = 0; i < length; i++) {	/* do actual split */
			if (curdim == 0)
				curval = points[i].l;
			else if (curdim == 1)
				curval = points[i].a;
			else if (curdim == 2)
				curval = points[i].b;
			if (curval <= pivotvalue) {
				smallerpoints[smallc++] = points[i];
			} else {
				biggerpoints[bigc++] = points[i];
			}
		}

		/* create subtrees */
		stageone(smallerpoints, dims, depth + 1, clusters, limits,
			 countsm);
		stageone(biggerpoints, dims, depth + 1, clusters, limits,
			 countgr);

	} else {		/* create leave */
		addtoList(clusters, points, length);
	}

}

/* Stage two of modified KD-Tree algorithm */
/* This is very similar to stageone... but in future there will bemore differences => not integrated into method stageone() */
void stagetwo(lab * points, int dims, int depth, ArrayList * clusters,
	      float limits[DIMS], int length, int total, float threshold)
{
	int curdim = depth % dims;
	float min, max;
	/* find maximum and minimum */
	int i, countsm, countgr, smallc, bigc;
	float pivotvalue, curval;
	int sum;
	lab *point;
	lab *smallerpoints;
	lab *biggerpoints;
	if (length < 1)
		return;
	if (curdim == 0)
		curval = points[0].l;
	else if (curdim == 1)
		curval = points[0].a;
	else
		curval = points[0].b;
	min = curval;
	max = curval;
	for (i = 1; i < length; i++) {
		if (curdim == 0)
			curval = points[i].l;
		else if (curdim == 1)
			curval = points[i].a;
		else if (curdim == 2)
			curval = points[i].b;
		if (min > curval)
			min = curval;
		if (max < curval)
			max = curval;
	}
	if (max - min > limits[curdim]) {	/* Split according to Rubner-Rule */
		/* split */
		pivotvalue = ((max - min) / 2.0) + min;
/*		printf("max=%f min=%f pivot=%f\n",max,min,pivotvalue); */
		countsm = 0;
		countgr = 0;
		for (i = 0; i < length; i++) {	/* find out cluster sizes */
			if (curdim == 0)
				curval = points[i].l;
			else if (curdim == 1)
				curval = points[i].a;
			else if (curdim == 2)
				curval = points[i].b;
			if (curval <= pivotvalue) {
				countsm++;
			} else {
				countgr++;
			}
		}
		smallerpoints = malloc(countsm * sizeof(lab));	/* allocate mem */
		biggerpoints = malloc(countgr * sizeof(lab));
		smallc = 0;
		bigc = 0;
		for (i = 0; i < length; i++) {	/* do actual split */
			if (curdim == 0)
				curval = points[i].l;
			else if (curdim == 1)
				curval = points[i].a;
			else if (curdim == 2)
				curval = points[i].b;
			if (curval <= pivotvalue) {
				smallerpoints[smallc++] = points[i];
			} else {
				biggerpoints[bigc++] = points[i];
			}
		}
		/* create subtrees */
		stagetwo(smallerpoints, dims, depth + 1, clusters, limits,
			 countsm, total, threshold);
		stagetwo(biggerpoints, dims, depth + 1, clusters, limits,
			 countgr, total, threshold);
/*		free(smallerpoints);
		free(biggerpoints); */

	} else {		/* create leave */
		sum = 0;
		for (i = 0; i < length; i++) {
			sum += points[i].cardinality;
		}
		if (((sum * 100.0) / total) >= threshold) {
			point = malloc(sizeof(lab));
			point->l = 0;
			point->a = 0;
			point->b = 0;
			for (i = 0; i < length; i++) {
				point->l += points[i].l;
				point->a += points[i].a;
				point->b += points[i].b;
			}
			point->l /= (length * 1.0);
			point->a /= (length * 1.0);
			point->b /= (length * 1.0);
/*			printf("cluster=%f, %f, %f sum=%d\n",point->l,point->a,point->b,sum); */
			addtoList(clusters, point, 1);
		}
		free(points);
	}
}

/* squared euclidean distance */
float euklid(lab p, lab q)
{
	float sum = (p.l - q.l) * (p.l - q.l);
	sum += (p.a - q.a) * (p.a - q.a);
	sum += (p.b - q.b) * (p.b - q.b);
	return sum;
}

/* Creates a color signature for a given set of pixels */
lab *createSignature(lab * input, int length, float limits[DIMS],
		     int *returnlength)
{
	ArrayList *clusters1;
	ArrayList *clusters2;
	ArrayList *curelem;
	lab *centroids;
	lab *cluster;
	lab centroid;
	lab *rval;
	int k, i;
	int clusters1size;
	clusters1 = malloc(sizeof(ArrayList));
	clusters1->next = NULL;

	stageone(input, DIMS, 0, clusters1, limits, length);
	clusters1size = listSize(clusters1);
	centroids = (lab *) malloc(clusters1size * sizeof(lab));	/* allocate mem */
	curelem = clusters1;

	i = 0;
	while (curelem->array) {
		centroid.l = 0;
		centroid.a = 0;
		centroid.b = 0;
		cluster = curelem->array;
		for (k = 0; k < curelem->arraylength; k++) {
			centroid.l += cluster[k].l;
			centroid.a += cluster[k].a;
			centroid.b += cluster[k].b;
		}
		centroids[i].l = centroid.l / (curelem->arraylength * 1.0);
		centroids[i].a = centroid.a / (curelem->arraylength * 1.0);
		centroids[i].b = centroid.b / (curelem->arraylength * 1.0);
		centroids[i].cardinality = curelem->arraylength;
		i++;
		curelem = curelem->next;
	}
/*	printf("step #1 -> %d clusters\n", clusters1size); */
	clusters2 = malloc(sizeof(ArrayList));
	clusters2->next = NULL;
	stagetwo(centroids, DIMS, 0, clusters2, limits, clusters1size, length, 0.1);	/* see paper by tomasi */
	rval = listToArray(clusters2, returnlength);
	freelist(clusters1);
	freelist(clusters2);
	free(centroids);
/*	printf("step #2 -> %d clusters\n", returnlength[0]); */
	return rval;
}

/* Smoothes the confidence matrix */
void smoothcm(float *cm, int xres, int yres, float f1, float f2, float f3)
{
	/* Smoothright */
	int y, x, idx;
	for (y = 0; y < yres; y++) {
		for (x = 0; x < xres - 2; x++) {
			idx = (y * xres) + x;
			cm[idx] =
			    f1 * cm[idx] + f2 * cm[idx + 1] + f3 * cm[idx +
								      2];
		}
	}
	/* Smoothleft */
	for (y = 0; y < yres; y++) {
		for (x = xres - 1; x >= 2; x--) {
			idx = (y * xres) + x;
			cm[idx] =
			    f3 * cm[idx - 2] + f2 * cm[idx - 1] +
			    f1 * cm[idx];
		}
	}
	/* Smoothdown */
	for (y = 0; y < yres - 2; y++) {
		for (x = 0; x < xres; x++) {
			idx = (y * xres) + x;
			cm[idx] =
			    f1 * cm[idx] + f2 * cm[((y + 1) * xres) + x] +
			    f3 * cm[((y + 2) * xres) + x];
		}
	}
	/* Smoothup */
	for (y = yres - 1; y >= 2; y--) {
		for (x = 0; x < xres; x++) {
			idx = (y * xres) + x;
			cm[idx] =
			    f3 * cm[((y - 2) * xres) + x] +
			    f2 * cm[((y - 1) * xres) + x] + f1 * cm[idx];
		}
	}
}


/* Simulate a queue needed for region growing */
/* The methods are NOT generic */
typedef struct {
	int val;
	int invalid;
	void *next;
} queue;


queue *createqueue()
{
	queue *q = malloc(sizeof(queue));
	q->next = NULL;
	q->val = 0;
	q->invalid = 1;
	return q;
}

void addtoqueue(queue * q, int val)
{
	queue *cur = q;
	while (cur->invalid == 0) {
		cur = cur->next;
	};
	cur->val = val;
	cur->invalid = 0;
	cur->next = createqueue();
}

int isempty(queue * q)
{
	if (q == NULL)
		return 1;
	return q->invalid;
}

int headval(queue * q)
{
	return q->val;
}

queue *removehead(queue * q)
{
	queue *n;
	q->val = 0;
	q->invalid = 1;
	if (q->next != NULL) {
		n = q->next;
		free(q);
	} else {
		n = q;
	}
	return n;
}

/* Region growing */
void findmaxblob(float *cm, unsigned int *image, int xres, int yres)
{
	int i;
	int curlabel = 1;
	int maxregion = 0;
	int maxblob = 0;
	int length = xres * yres;
	int regioncount = 0;
	int pos = 0;
	int *labelfield = malloc(xres * yres * sizeof(int));
	queue *q;
	memset(labelfield, 0, length * sizeof(int));
	q = createqueue();
	for (i = 0; i < length; i++) {
		regioncount = 0;
		if (labelfield[i] == 0 && cm[i] >= 0.5) {
			addtoqueue(q, i);
		}
		while (!isempty(q)) {
			pos = headval(q);
			q = removehead(q);
			if (pos < 0 || pos >= length) {
				continue;
			}
			if (labelfield[pos] == 0 && cm[pos] >= 0.5f) {
				labelfield[pos] = curlabel;
				regioncount++;
				addtoqueue(q, pos + 1);
				addtoqueue(q, pos - 1);
				addtoqueue(q, pos + xres);
				addtoqueue(q, pos - xres);
			}
		}
		if (regioncount > maxregion) {
			maxregion = regioncount;
			maxblob = curlabel;
		}
		curlabel++;
	}
	for (i = 0; i < length; i++) {	/* Kill everything that is not biggest blob! */
		if (labelfield[i] != 0) {
			if (labelfield[i] != maxblob) {
				cm[i] = 0.0;
			}
		}
	}
	free(q);
	free(labelfield);
}


/* Returns squared clustersize */
float getclustersize(float limits[DIMS])
{
	float sum =
	    (limits[0] - (-limits[0])) * (limits[0] - (-limits[0]));
	sum += (limits[1] - (-limits[1])) * (limits[1] - (-limits[1]));
	sum += (limits[2] - (-limits[2])) * (limits[2] - (-limits[2]));
	return sum;
}

/* calculates alpha\timesConfidencematrix */
void premultiplyMatrix(float alpha, float *cm, int length)
{
	int i;
	for (i = 0; i < length; i++) {
		cm[i] = alpha * cm[i];
	}
}


/* Normalizes a confidencematrix */
void normalizeMatrix(float *cm, int length)
{
	float max = 0.0;
	int i;
	float alpha = 0.0;
	for (i = 0; i < length; i++) {
		if (max < cm[i])
			max = cm[i];
	}
	if (max <= 0.0)
		return;
	if (max == 1.00)
		return;
	alpha = 1.00f / max;
	premultiplyMatrix(alpha, cm, length);
}

float min(float a, float b)
{
	return (a < b ? a : b);
}

float max(float a, float b)
{
	return (a > b ? a : b);
}

/* A confidence matrix eroder */
void erode2(float *cm, int xres, int yres)
{
	int idx, x, y;
	/* From right */
	for (y = 0; y < yres; y++) {
		for (x = 0; x < xres - 1; x++) {
			idx = (y * xres) + x;
			cm[idx] = min(cm[idx], cm[idx + 1]);
		}
	}
	/* From left */
	for (y = 0; y < yres; y++) {
		for (x = xres - 1; x >= 1; x--) {
			idx = (y * xres) + x;
			cm[idx] = min(cm[idx - 1], cm[idx]);
		}
	}
	/* From down */
	for (y = 0; y < yres - 1; y++) {
		for (x = 0; x < xres; x++) {
			idx = (y * xres) + x;
			cm[idx] = min(cm[idx], cm[((y + 1) * xres) + x]);
		}
	}
	/* From up */
	for (y = yres - 1; y >= 1; y--) {
		for (x = 0; x < xres; x++) {
			idx = (y * xres) + x;
			cm[idx] = min(cm[((y - 1) * xres) + x], cm[idx]);
		}
	}
}

/* A confidence matrix dilater */
/* A confidence matrix dilater */
void dilate2(float *cm, int xres, int yres)
{
	int x,y,idx;
	/* From right */
    	for (y = 0; y < yres; y++) {
      		for (x = 0; x < xres - 1; x++) {
			idx = (y * xres) + x;
			cm[idx] = max(cm[idx],cm[idx + 1]);
		}
	}
    /* From left */
    for (y = 0; y < yres; y++) {
    		for (x = xres - 1; x >= 1; x--) {
			idx = (y * xres) + x;
			cm[idx] = max(cm[idx - 1],cm[idx]);
      		}
    }
    /* From down */
    for (y = 0; y < yres - 1; y++) {
    		for (x = 0; x < xres; x++) {
			idx = (y * xres) + x;
			cm[idx] = max(cm[idx],cm[((y+1)*xres)+x]);
      		}
    }
    /* From up */
   for (y = yres - 1; y >= 1; y--) {
   		for (x = 0; x < xres; x++) {
			idx = (y * xres) + x;
			cm[idx] = max(cm[((y-1)*xres)+x],cm[idx]);
      		}
   }
}

/*
 * Call this method:
 * rgbs - the picture
 * confidencematrix - a confidencematrix with values <=0.1 is sure background, >=0.9 is sure foreground, rest unknown
 * xres, yres - the dimensions of the picture and the confidencematrix
 * limits - a three dimensional float array specifing the accuracy - a good value is: {0.66,1.25,2.5}
 * int smoothness - specifies how smooth the boundaries of a picture should be made (value greater or equal to 0). More smooth = fault tolerant, less smooth = exact boundaries - try 3 for a first guess.
 * returns and writes into the confidencematrix the resulting segmentation
 */
float *segmentate(unsigned int *rgbs, float *confidencematrix, int xres,
		  int yres, float limits[DIMS], int smoothness)
{
	float clustersize = getclustersize(limits);
	int length = xres * yres;
	int surebgcount = 0, surefgcount = 0;
	int i, k, j;
	int bgsiglen, fgsiglen;
	lab *surebg, *surefg = NULL, *bgsig, *fgsig = NULL;
	char background = 0;
	float min, d;
	lab labpixel;
	for (i = 0; i < length; i++) {	/* count given foreground and background pixels */
		if (confidencematrix[i] <= 0.10f) {
			surebgcount++;
		} else if (confidencematrix[i] >= 0.90f) {
			surefgcount++;
		}
	}
	surebg = malloc(sizeof(lab) * surebgcount);	/* alloc mem for them */
	if (surefgcount > 0)
		surefg = malloc(sizeof(lab) * surefgcount);
	k = 0;
	j = 0;
	for (i = 0; i < length; i++) {	/* create inputs for colorsignatures */
		if (confidencematrix[i] <= 0.10f) {
			calcLAB(rgbs[i], &surebg[k]);
			k++;
		} else if (confidencematrix[i] >= 0.90f) {
			calcLAB(rgbs[i], &surefg[j]);
			j++;
		}
	}
	bgsig = createSignature(surebg, surebgcount, limits, &bgsiglen);	/* Create color signature for bg */
	if (bgsiglen < 1)
		return confidencematrix;	/* No segmentation possible */
	if (surefgcount > 0)
		fgsig = createSignature(surefg, surefgcount, limits, &fgsiglen);	/* Create color signature for fg if possible */
	else
		fgsiglen = 0;
	for (i = 0; i < length; i++) {	/* Classify - the slow way....Better: Tree traversation */
		if (confidencematrix[i] >= 0.90) {
			confidencematrix[i] = 1.0f;
			continue;
		}
		if (confidencematrix[i] <= 0.10) {
			confidencematrix[i] = 0.0f;
			continue;
		}
		calcLAB(rgbs[i], &labpixel);
		background = 1;
		min = euklid(labpixel, bgsig[0]);
		for (j = 1; j < bgsiglen; j++) {
			d = euklid(labpixel, bgsig[j]);
			if (d < min) {
				min = d;
			}
		}
		if (fgsiglen == 0) {
			if (min < clustersize)
				background = 1;
			else
				background = 0;
		} else {
			for (j = 0; j < fgsiglen; j++) {
				d = euklid(labpixel, fgsig[j]);
				if (d < min) {
					min = d;
					background = 0;
					break;
				}
			}
		}
		if (background == 0) {
			confidencematrix[i] = 1.0f;
		} else {
			confidencematrix[i] = 0.0f;
		}
	}
	smoothcm(confidencematrix, xres, yres, 0.33, 0.33, 0.33);	/* Smooth a bit for error killing */
	normalizeMatrix(confidencematrix, length);
	erode2(confidencematrix, xres, yres);	/* Now erode, to make sure only "strongly connected components" keep being connected */
	findmaxblob(confidencematrix, rgbs, xres, yres);	/* search the biggest connected component */
	for (i = 0; i < smoothness; i++) {
		smoothcm(confidencematrix, xres, yres, 0.33, 0.33, 0.33);	/* smooth again - as user specified */
	}
	normalizeMatrix(confidencematrix, length);
	for (i = 0; i < length; i++) {	/* Threshold the values */
		if (confidencematrix[i] >= 0.5)
			confidencematrix[i] = 1.0;
		else
			confidencematrix[i] = 0.0;
	}
	findmaxblob(confidencematrix, rgbs, xres, yres);	/* search the biggest connected component again to make sure jitter is killed */
	dilate2(confidencematrix, xres, yres);	/* Now dilate, to fill up boundary pixels killed by erode */
	if (surefg != NULL)
		free(surefg);
	if (surebg != NULL)
		free(surebg);
	if (bgsig != NULL)
		free(bgsig);
	if (fgsig != NULL)
		free(fgsig);
	return confidencematrix;
}
