/*
 * "$Id$"
 *
 *   Softweave calculator for gimp-print.
 *
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "print.h"

#if 0
#define TEST_RAW
#endif
#if 0
#define TEST_COOKED
#endif
#if 0
#define ACCUMULATE
#endif
#if 1
#define ASSERTIONS
#endif

#if defined(TEST_RAW) || defined(TEST_COOKED)
#define TEST
#endif

#ifdef ASSERTIONS
#define assert(x) if (!(x)) { fprintf(stderr, "ASSERTION FAILURE!  \"%s\", line %d.\n", __FILE__, __LINE__); exit(1); }
#else
#define assert(x) /* nothing */
#endif

#ifdef TEST
#define MAXCOLLECT (1000)
#endif

static int
gcd(int x, int y)
{
	assert (x >= 0);
	assert (y >= 0);
	if (y == 0)
		return x;
	while (x != 0) {
		if (y > x) {
			int t = x;
			x = y;
			y = t;
		}
		x %= y;
	}
	return y;
}

/* RAW WEAVE */

typedef struct raw {
	int separation;
	int jets;
	int oversampling;
	int advancebasis;
	int subblocksperpassblock;
	int passespersubblock;
	int strategy;
} raw_t;

/*
 * Strategy types currently defined:
 *
 *  0: microweave (intercepted at the escp2 driver level so we never
 *     see it here)
 *  1: zig-zag type pass block filling
 *  2: ascending pass block filling
 *  3: descending pass block filling
 *  4: ascending fill with 2x expansion
 *  5: ascending fill with 3x expansion
 *  6: staggered zig-zag neighbour-avoidance fill
 *
 * In theory, strategy 1 should be optimal; in practice, it can lead
 * to visible areas of banding.  If it's necessary to avoid filling
 * neighbouring rows in neighbouring passes, strategy 6 should be optimal,
 * at least for some weaves.
 */

static void
initialize_raw_weave(raw_t *w,	/* I - weave struct to be filled in */
                     int S,	/* I - jet separation */
                     int J,	/* I - number of jets */
                     int H,	/* I - oversampling factor */
                     int strat)	/* I - weave pattern variation to use */
{
	w->separation = S;
	w->jets = J;
	w->oversampling = H;
	w->advancebasis = J / H;
	w->subblocksperpassblock = gcd(S, w->advancebasis);
	w->passespersubblock = S / w->subblocksperpassblock;
	w->strategy = strat;
}

static void
calculate_raw_pass_parameters(raw_t *w,		/* I - weave parameters */
                              int pass,		/* I - pass number ( >= 0) */
                              int *startrow,	/* O - print head position */
                              int *subpass)	/* O - subpass number */
{
	int band, passinband, subpassblock, subpassoffset;

	band = pass / (w->separation * w->oversampling);
	passinband = pass % (w->separation * w->oversampling);
	subpassblock = pass % w->separation
	                 * w->subblocksperpassblock / w->separation;

	switch (w->strategy) {
	case 1:
		if (subpassblock * 2 < w->subblocksperpassblock)
			subpassoffset = 2 * subpassblock;
		else
			subpassoffset = 2 * (w->subblocksperpassblock
			                      - subpassblock) - 1;
		break;
	case 2:
		subpassoffset = subpassblock;
		break;
	case 3:
		subpassoffset = w->subblocksperpassblock - 1 - subpassblock;
		break;
	case 4:
		if (subpassblock * 2 < w->subblocksperpassblock)
			subpassoffset = 2 * subpassblock;
		else
			subpassoffset = 1 + 2 * (subpassblock
			                          - (w->subblocksperpassblock
			                              + 1) / 2);
		break;
	case 5:
		if (subpassblock * 3 < w->subblocksperpassblock)
			subpassoffset = 3 * subpassblock;
		else if (3 * (subpassblock - (w->subblocksperpassblock + 2) / 3)
		          < w->subblocksperpassblock - 2)
			subpassoffset = 2 + 3 * (subpassblock
			                          - (w->subblocksperpassblock
			                              + 2) / 3);
		else
			subpassoffset = 1 + 3 * (subpassblock
			                          - (w->subblocksperpassblock
			                              + 2) / 3
						  - w->subblocksperpassblock
						      / 3);
		break;
	case 6:
		if (subpassblock * 2 < w->subblocksperpassblock)
			subpassoffset = 2 * subpassblock;
		else if (subpassblock * 2 < w->subblocksperpassblock + 2)
			subpassoffset = 1;
		else
			subpassoffset = 2 * (w->subblocksperpassblock
			                      - subpassblock) + 1;
		break;
	default:
		subpassoffset = subpassblock;
		break;
	}

	*startrow = w->separation * w->jets * band
	              + w->advancebasis * passinband + subpassoffset;
	*subpass = passinband / w->separation;
}

static void
calculate_raw_row_parameters(raw_t *w,		/* I - weave parameters */
                             int row,		/* I - row number */
                             int subpass,	/* I - subpass number */
                             int *pass,		/* O - pass number */
                             int *jet,		/* O - jet number in pass */
                             int *startrow)	/* O - starting row of pass */
{
	int subblockoffset, subpassblock, band, baserow, passinband, offset;

	subblockoffset = row % w->subblocksperpassblock;
	switch (w->strategy) {
	case 1:
		if (subblockoffset % 2 == 0)
			subpassblock = subblockoffset / 2;
		else
			subpassblock = w->subblocksperpassblock
			                 - (subblockoffset + 1) / 2;
		break;
	case 2:
		subpassblock = subblockoffset;
		break;
	case 3:
		subpassblock = w->subblocksperpassblock - 1 - subblockoffset;
		break;
	case 4:
		if (subblockoffset % 2 == 0)
			subpassblock = subblockoffset / 2;
		else
			subpassblock = (subblockoffset - 1) / 2
			               + (w->subblocksperpassblock + 1) / 2;
		break;
	case 5:
		if (subblockoffset % 3 == 0)
			subpassblock = subblockoffset / 3;
		else if (subblockoffset % 3 == 1)
			subpassblock = (subblockoffset - 1) / 3
			                 + (w->subblocksperpassblock + 2) / 3;
		else
			subpassblock = (subblockoffset - 2) / 3
			                 + (w->subblocksperpassblock + 2) / 3
			                 + (w->subblocksperpassblock + 1) / 3;
		break;
	case 6:
		if (subblockoffset % 2 == 0)
			subpassblock = subblockoffset / 2;
		else if (subblockoffset == 1)
			subpassblock = w->subblocksperpassblock / 2;
		else
			subpassblock = w->subblocksperpassblock
			                 - (subblockoffset - 1) / 2;
	default:
		subpassblock = subblockoffset;
		break;
	}

	band = row / (w->separation * w->jets);
	baserow = row - subblockoffset - band * w->separation * w->jets;
	passinband = baserow / w->advancebasis;
	offset = baserow % w->advancebasis;

	while (offset % w->separation != 0
	       || passinband / w->separation != subpass
	       || passinband % w->separation / w->passespersubblock
	            != subpassblock)
	{
		offset += w->advancebasis;
		passinband--;
		if (passinband < 0) {
			const int roundedjets = w->advancebasis
			                          * w->oversampling;
			band--;
			passinband += w->separation * w->oversampling;
			offset += w->separation * (w->jets - roundedjets);
		}
	}

	*pass = band * w->oversampling * w->separation + passinband;
	*jet = offset / w->separation;
	*startrow = row - (*jet * w->separation);
}

/* COOKED WEAVE */

typedef struct cooked {
	raw_t rw;
	int first_row_printed;
	int last_row_printed;

	int first_premapped_pass;	/* First raw pass used by this page */
	int first_normal_pass;
	int first_postmapped_pass;
	int first_unused_pass;

	int *pass_premap;
	int *stagger_premap;
	int *pass_postmap;
	int *stagger_postmap;
} cooked_t;

static void
sort_by_start_row(int *map, int *startrows, int count)
{
	/*
	 * Yes, it's a bubble sort, but we do it no more than 4 times
	 * per page, and we are only sorting a small number of items.
	 */

	int dirty;

	do {
		int x;
		dirty = 0;
		for (x = 1; x < count; x++) {
			if (startrows[x - 1] > startrows[x]) {
				int temp = startrows[x - 1];
				startrows[x - 1] = startrows[x];
				startrows[x] = temp;
				temp = map[x - 1];
				map[x - 1] = map[x];
				map[x] = temp;
				dirty = 1;
			}
		}
	} while (dirty);
}

static void
calculate_stagger(raw_t *w, int *map, int *startrows_stagger, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		int startrow, subpass;
		calculate_raw_pass_parameters(w, map[i], &startrow, &subpass);
		startrow -= w->separation * w->jets;
		startrows_stagger[i] = (startrows_stagger[i] - startrow)
		                         / w->separation;
	}
}

static void
invert_map(int *map, int *stagger, int count, int oldfirstpass,
           int newfirstpass)
{
	int i;
	int *newmap, *newstagger;
	newmap = malloc(count * sizeof(int));
	newstagger = malloc(count * sizeof(int));

	for (i = 0; i < count; i++) {
		newmap[map[i] - oldfirstpass] = i + newfirstpass;
		newstagger[map[i] - oldfirstpass] = stagger[i];
	}

	memcpy(map, newmap, count * sizeof(int));
	memcpy(stagger, newstagger, count * sizeof(int));
	free(newstagger);
	free(newmap);
}

static void
make_passmap(raw_t *w, int **map, int **starts, int first_pass_number,
             int first_pass_to_map, int first_pass_after_map,
             int first_pass_to_stagger, int first_pass_after_stagger,
             int first_row_of_maximal_pass, int separations_to_distribute)
{
	int *passmap, *startrows;
	int passes_to_map = first_pass_after_map - first_pass_to_map;
	int i;

	assert(first_pass_to_map <= first_pass_after_map);
	assert(first_pass_to_stagger <= first_pass_after_stagger);

	*map = passmap = malloc(passes_to_map * sizeof(int));
	*starts = startrows = malloc(passes_to_map * sizeof(int));

	for (i = 0; i < passes_to_map; i++) {
		int startrow, subpass;
		int pass = i + first_pass_to_map;
		calculate_raw_pass_parameters(w, pass, &startrow, &subpass);
		passmap[i] = pass;
		if (first_row_of_maximal_pass >= 0)
			startrow = first_row_of_maximal_pass - startrow
			             + w->separation * w->jets;
		else
			startrow -= w->separation * w->jets;
		while (startrow < 0)
			startrow += w->separation;
		startrows[i] = startrow;
	}

	sort_by_start_row(passmap, startrows, passes_to_map);

	separations_to_distribute++;

	for (i = 0; i < first_pass_after_stagger - first_pass_to_stagger; i++) {
		int offset = first_pass_to_stagger - first_pass_to_map;
		if (startrows[i + offset] / w->separation
		      < i % separations_to_distribute)
		{
			startrows[i + offset]
			  = startrows[i + offset] % w->separation
			     + w->separation * (i % separations_to_distribute);
		}
	}

	if (first_row_of_maximal_pass >= 0) {
		for (i = 0; i < passes_to_map; i++) {
			startrows[i] = first_row_of_maximal_pass - startrows[i];
		}
	}

	sort_by_start_row(passmap, startrows, passes_to_map);
	calculate_stagger(w, passmap, startrows, passes_to_map);

	invert_map(passmap, startrows, passes_to_map, first_pass_to_map,
		   first_pass_to_map - first_pass_number);
}

static void
calculate_pass_map(cooked_t *w,		/* I - weave parameters */
                   int pagelength,	/* I - number of rows on page */
                   int firstrow,	/* I - first printed row */
                   int lastrow)		/* I - last printed row */
{
	int startrow, subpass;
	int pass = -1;

	w->first_row_printed = firstrow;
	w->last_row_printed = lastrow;

	if (pagelength <= lastrow)
		pagelength = lastrow + 1;

	do {
		calculate_raw_pass_parameters(&w->rw, ++pass,
		                              &startrow, &subpass);
	} while (startrow - w->rw.separation < firstrow);

	w->first_premapped_pass = pass;

	while (startrow < w->rw.separation * w->rw.jets
	       && startrow - w->rw.separation < pagelength
	       && startrow <= lastrow + w->rw.separation * w->rw.jets)
	{
		calculate_raw_pass_parameters(&w->rw, ++pass,
		                              &startrow, &subpass);
	}
	w->first_normal_pass = pass;

	while (startrow - w->rw.separation < pagelength
	       && startrow <= lastrow + w->rw.separation * w->rw.jets)
	{
		calculate_raw_pass_parameters(&w->rw, ++pass,
		                              &startrow, &subpass);
	}
	w->first_postmapped_pass = pass;

	while (startrow <= lastrow + w->rw.separation * w->rw.jets) {
		calculate_raw_pass_parameters(&w->rw, ++pass,
		                              &startrow, &subpass);
	}
	w->first_unused_pass = pass;

	/*
	 * FIXME: make sure first_normal_pass doesn't advance beyond
	 * first_postmapped_pass, or first_postmapped_pass doesn't
	 * retreat before first_normal_pass.
	 */

	if (w->first_normal_pass > w->first_premapped_pass) {
		int spread, separations_to_distribute, normal_passes_mapped;
		separations_to_distribute = firstrow / w->rw.separation;
		spread = (separations_to_distribute + 1) * w->rw.separation;
		normal_passes_mapped = (spread + w->rw.advancebasis - 1)
		                            / w->rw.advancebasis;
		w->first_normal_pass += normal_passes_mapped;
		make_passmap(&w->rw, &w->pass_premap, &w->stagger_premap,
		             w->first_premapped_pass,
		             w->first_premapped_pass, w->first_normal_pass,
			     w->first_premapped_pass,
			     w->first_normal_pass - normal_passes_mapped,
		             -1, separations_to_distribute);
	} else {
		w->pass_premap = 0;
		w->stagger_premap = 0;
	}

	if (w->first_unused_pass > w->first_postmapped_pass) {
		int spread, separations_to_distribute, normal_passes_mapped;
		separations_to_distribute = (pagelength - lastrow - 1)
		                                     / w->rw.separation;
		spread = (separations_to_distribute + 1) * w->rw.separation;
		normal_passes_mapped = (spread + w->rw.advancebasis)
		                             / w->rw.advancebasis;
		w->first_postmapped_pass -= normal_passes_mapped;
		make_passmap(&w->rw, &w->pass_postmap, &w->stagger_postmap,
		             w->first_premapped_pass,
		             w->first_postmapped_pass, w->first_unused_pass,
			     w->first_postmapped_pass + normal_passes_mapped,
			     w->first_unused_pass,
		             pagelength - 1
		                 - w->rw.separation * (w->rw.jets - 1),
			     separations_to_distribute);
	} else {
		w->pass_postmap = 0;
		w->stagger_postmap = 0;
	}
}

void *					/* O - weave parameter block */
initialize_weave_params(int S,		/* I - jet separation */
                        int J,		/* I - number of jets */
                        int H,		/* I - oversampling factor */
                        int firstrow,	/* I - first row number to print */
                        int lastrow,	/* I - last row number to print */
                        int pagelength,	/* I - number of rows on the whole
                        		       page, without using any
                        		       expanded margin facilities */
                        int strategy)	/* I - weave pattern variant to use */
{
	cooked_t *w = malloc(sizeof(cooked_t));
	if (w) {
		initialize_raw_weave(&w->rw, S, J, H, strategy);
		calculate_pass_map(w, pagelength, firstrow, lastrow);
	}
	return w;
}

void
destroy_weave_params(void *vw)
{
	cooked_t *w = (cooked_t *) vw;

	if (w->pass_premap) free(w->pass_premap);
	if (w->stagger_premap) free(w->stagger_premap);
	if (w->pass_postmap) free(w->pass_postmap);
	if (w->stagger_postmap) free(w->stagger_postmap);
	free(w);
}

void
calculate_row_parameters(void *vw,		/* I - weave parameters */
                         int row,		/* I - row number */
                         int subpass,		/* I - subpass */
                         int *pass,		/* O - pass containing row */
                         int *jetnum,		/* O - jet number of row */
                         int *startingrow,	/* O - phys start row of pass */
                         int *ophantomrows,	/* O - missing rows at start */
                         int *ojetsused)	/* O - jets used by pass */
{
	cooked_t *w = (cooked_t *) vw;
	int raw_pass, jet, startrow, phantomrows, jetsused;
	int stagger = 0;
	int extra;

	assert(row >= w->first_row_printed);
	assert(row <= w->last_row_printed);
	calculate_raw_row_parameters(&w->rw,
	                             row + w->rw.separation * w->rw.jets,
	                             subpass, &raw_pass, &jet, &startrow);
	startrow -= w->rw.separation * w->rw.jets;
	jetsused = w->rw.jets;
	phantomrows = 0;

	if (raw_pass < w->first_normal_pass) {
		*pass = w->pass_premap[raw_pass - w->first_premapped_pass];
		stagger = w->stagger_premap[raw_pass - w->first_premapped_pass];
	} else if (raw_pass >= w->first_postmapped_pass) {
		*pass = w->pass_postmap[raw_pass - w->first_postmapped_pass];
		stagger = w->stagger_postmap[raw_pass
		                             - w->first_postmapped_pass];
	} else {
		*pass = raw_pass - w->first_premapped_pass;
	}

	startrow += stagger * w->rw.separation;
	*jetnum = jet - stagger;
	if (stagger < 0) {
		stagger = -stagger;
		phantomrows += stagger;
	}
	jetsused -= stagger;

	extra = w->first_row_printed
	             - (startrow + w->rw.separation * phantomrows);
	if (extra > 0) {
		extra = (extra + w->rw.separation - 1) / w->rw.separation;
		jetsused -= extra;
		phantomrows += extra;
	}

	extra = startrow + w->rw.separation * (phantomrows + jetsused - 1)
	          - w->last_row_printed;
	if (extra > 0) {
		extra = (extra + w->rw.separation - 1) / w->rw.separation;
		jetsused -= extra;
	}

	*startingrow = startrow;
	*ophantomrows = phantomrows;
	*ojetsused = jetsused;
}

#ifdef TEST_COOKED
static void
calculate_pass_parameters(cooked_t *w,		/* I - weave parameters */
                          int pass,		/* I - pass number ( >= 0) */
                          int *startrow,	/* O - print head position */
                          int *subpass,		/* O - subpass number */
			  int *phantomrows,	/* O - missing rows */
			  int *jetsused)	/* O - jets used to print */
{
	int raw_pass = pass + w->first_premapped_pass;
	int stagger = 0;
	int extra;

	if (raw_pass < w->first_normal_pass) {
		int i = 0;
		while (i + w->first_premapped_pass < w->first_normal_pass) {
			if (w->pass_premap[i] == pass) {
				raw_pass = i + w->first_premapped_pass;
				stagger = w->stagger_premap[i];
				break;
			}
			i++;
		}
	} else if (raw_pass >= w->first_postmapped_pass) {
		int i = 0;
		while (i + w->first_postmapped_pass < w->first_unused_pass) {
			if (w->pass_postmap[i] == pass) {
				raw_pass = i + w->first_postmapped_pass;
				stagger = w->stagger_postmap[i];
				break;
			}
			i++;
		}
	}

	calculate_raw_pass_parameters(&w->rw, raw_pass, startrow, subpass);
	*startrow -= w->rw.separation * w->rw.jets;
	*jetsused = w->rw.jets;
	*phantomrows = 0;

	*startrow += stagger * w->rw.separation;
	if (stagger < 0) {
		stagger = -stagger;
		*phantomrows += stagger;
	}
	*jetsused -= stagger;

	extra = w->first_row_printed - (*startrow + w->rw.separation * *phantomrows);
	extra = (extra + w->rw.separation - 1) / w->rw.separation;
	if (extra > 0) {
		*jetsused -= extra;
		*phantomrows += extra;
	}

	extra = *startrow + w->rw.separation * (*phantomrows + *jetsused - 1)
	          - w->last_row_printed;
	extra = (extra + w->rw.separation - 1) / w->rw.separation;
	if (extra > 0) {
		*jetsused -= extra;
	}
}
#endif /* TEST_COOKED */

#ifdef TEST

#ifdef ACCUMULATE
#define PUTCHAR(x) /* nothing */
#else
#define PUTCHAR(x) putchar(x)
#endif

static void
plotpass(int startrow, int phantomjets, int jetsused, int totaljets,
         int separation, int subpass, int *collect, int *prints)
{
	int hpos, i;

	for (hpos = 0; hpos < startrow; hpos++)
		PUTCHAR(' ');

	for (i = 0; i < phantomjets; i++) {
		int j;
		PUTCHAR('X');
		hpos++;
		for (j = 1; j < separation; j++) {
			PUTCHAR(' ');
			hpos++;
		}
	}

	for (; i < phantomjets + jetsused; i++) {
		if (i > phantomjets) {
			int j;
			for (j = 1; j < separation; j++) {
				PUTCHAR('-');
				hpos++;
			}
		}
		if (hpos < MAXCOLLECT) {
			if (collect[hpos] & 1 << subpass)
				PUTCHAR('^');
			else if (subpass < 10)
				PUTCHAR('0' + subpass);
			else
				PUTCHAR('A' + subpass - 10);
			collect[hpos] |= 1 << subpass;
			prints[hpos]++;
		} else {
			PUTCHAR('0' + subpass);
		}
		hpos++;
	}

	while (i++ < totaljets) {
		int j;
		for (j = 1; j < separation; j++) {
			PUTCHAR(' ');
			hpos++;
		}
		PUTCHAR('X');
	}
#ifdef ACCUMULATE
	for (i=0; i<=MAXCOLLECT; i++) {
		if (collect[i] == 0)
			putchar(' ');
		else if (collect[i] < 10)
			putchar('0'+collect[i]);
		else
			putchar('A'+collect[i]-10);
	}
#endif
	putchar('\n');
}
#endif /* TEST */

#ifdef TEST_COOKED
int
main(int ac, char *av[])
{
	int S         =ac>1 ? atoi(av[1]) : 4;
	int J         =ac>2 ? atoi(av[2]) : 12;
	int H         =ac>3 ? atoi(av[3]) : 1;
	int firstrow  =ac>4 ? atoi(av[4]) : 1;
	int lastrow   =ac>5 ? atoi(av[5]) : 100;
	int pagelength=ac>6 ? atoi(av[6]) : 1000;
	int strategy  =ac>7 ? atoi(av[7]) : 1;
	cooked_t *weave;
	int passes;

	int pass, x;
	int collect[MAXCOLLECT];
	int prints[MAXCOLLECT];

	memset(collect, 0, MAXCOLLECT*sizeof(int));
	memset(prints, 0, MAXCOLLECT*sizeof(int));
	printf("S=%d  J=%d  H=%d  firstrow=%d  lastrow=%d  "
	       "pagelength=%d  strategy=%d\n",
	       S, J, H, firstrow, lastrow, pagelength, strategy);

	weave = initialize_weave_params(S, J, H, firstrow, lastrow,
	                                pagelength, strategy);
	passes = weave->first_unused_pass - weave->first_premapped_pass;

	for (pass = 0; pass < passes; pass++) {
		int startrow, subpass, phantomjets, jetsused;

		calculate_pass_parameters(weave, pass, &startrow, &subpass,
		                          &phantomjets, &jetsused);

		plotpass(startrow, phantomjets, jetsused, J, S, subpass,
		         collect, prints);
	}

	for (pass=MAXCOLLECT - 1; prints[pass] == 0; pass--)
		;

	for (x=0; x<=pass; x++) {
		if (collect[x] < 10)
			putchar('0'+collect[x]);
		else
			putchar('A'+collect[x]-10);
	}
	putchar('\n');

	for (x=0; x<=pass; x++) {
		if (prints[x] < 10)
			putchar('0'+prints[x]);
		else
			putchar('A'+prints[x]-10);
	}
	putchar('\n');

	return 0;
}
#endif /* TEST_COOKED */

#ifdef TEST_RAW
int
main(int ac, char *av[])
{
	int S     =ac>1 ? atoi(av[1]) : 4;
	int J     =ac>2 ? atoi(av[2]) : 12;
	int h_pos =ac>3 ? atoi(av[3]) : 1;
	int h_over=ac>4 ? atoi(av[4]) : 1;
	int v_over=ac>5 ? atoi(av[5]) : 1;
	int H = h_pos * h_over * v_over;

	int pass, passes, x;
	int collect[MAXCOLLECT];
	int prints[MAXCOLLECT];
	raw_t raw;

	memset(collect, 0, MAXCOLLECT*sizeof(int));
	memset(prints, 0, MAXCOLLECT*sizeof(int));
	printf("S=%d  J=%d  H=%d\n", S, J, H);

	if (H > J) {
		printf("H > J, so this weave will not work!\n");
	}
	passes = S * H * 3;

	initialize_raw_weave(&raw, S, J, H);

	for (pass=0; pass<passes + S * H; pass++) {
		int startrow, subpass, phantomjets, jetsused;

		calculate_raw_pass_parameters(&raw, pass, &startrow, &subpass);
		phantomjets = 0;
		jetsused = J;

		plotpass(startrow, phantomjets, jetsused, J, S, subpass,
		         collect, prints);
	}
	for (pass=MAXCOLLECT - 1; prints[pass] == 0; pass--)
		;

	for (x=0; x<=pass; x++) {
		if (collect[x] < 10)
			putchar('0'+collect[x]);
		else
			putchar('A'+collect[x]-10);
	}
	putchar('\n');

	for (x=0; x<=pass; x++) {
		if (prints[x] < 10)
			putchar('0'+prints[x]);
		else
			putchar('A'+prints[x]-10);
	}
	putchar('\n');

	printf("  A  first row given by pass lookup doesn't match row lookup\n"
	       "  B  jet out of range\n"
	       "  C  given jet number of pass doesn't print this row\n"
	       "  D  subpass given by reverse lookup doesn't match requested subpass\n");

	for (x = S * J; x < S * J * 20; x++) {
		int h;
		for (h = 0; h < H; h++) {
			int pass, jet, start, first, subpass, z;
			int a=0, b=0, c=0, d=0;
			calculate_raw_row_parameters(&raw, x, h, &pass, &jet, &start);
			for (z=0; z < pass; z++) {
				putchar(' ');
			}
			printf("%d", jet);
			calculate_raw_pass_parameters(&raw, pass, &first, &subpass);
			if (first != start)
				a=1;
			if (jet < 0 || jet >= J)
				b=1;
			if (x != first + jet * S)
				c=1;
			if (subpass != h)
				d=1;
			if (a || b || c || d) {
				printf("    (");
				if (a) putchar('A');
				if (b) putchar('B');
				if (c) putchar('C');
				if (d) putchar('D');
				putchar(')');
				printf("\npass=%d first=%d start=%d jet=%d subpass=%d", pass, first, start, jet, subpass);
			}
			putchar('\n');
		}
		/* putchar('\n'); */
	}

	return 0;
}

#endif /* TEST_RAW */
