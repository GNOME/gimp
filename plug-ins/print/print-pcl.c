/*
 * "$Id$"
 *
 *   Print plug-in HP PCL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Dave Hill (dave@minnie.demon.co.uk)
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

#include "print.h"

/* #define DEBUG */
/* #define PCL_DEBUG_DISABLE_COMPRESSION */

/*
 * Local functions...
 */
static void	pcl_mode0(FILE *, unsigned char *, int, int);
static void	pcl_mode2(FILE *, unsigned char *, int, int);

/*
 * Generic define for a name/value set
 */

typedef struct
{
    char  *pcl_name;
    int   pcl_code;
} pcl_t;

/*
 * Media size to PCL media size code table
 *
 * Note, you can force the list of papersizes given in the GUI to be only those
 * supported by defining PCL_NO_CUSTOM_PAPERSIZES
 */

/* #define PCL_NO_CUSTOM_PAPERSIZES */

#define PCL_PAPERSIZE_EXECUTIVE		1
#define PCL_PAPERSIZE_LETTER		2
#define PCL_PAPERSIZE_LEGAL		3
#define PCL_PAPERSIZE_TABLOID		6	/* "Ledger" */
#define PCL_PAPERSIZE_STATEMENT		15	/* Called "Manual" in print-util */
#define PCL_PAPERSIZE_SUPER_B		16	/* Called "13x19" in print-util */
#define PCL_PAPERSIZE_A5		25
#define PCL_PAPERSIZE_A4		26
#define PCL_PAPERSIZE_A3		27
#define PCL_PAPERSIZE_JIS_B5		45
#define PCL_PAPERSIZE_JIS_B4		46
#define PCL_PAPERSIZE_HAGAKI_CARD	71
#define PCL_PAPERSIZE_OUFUKU_CARD	72
#define PCL_PAPERSIZE_A6_CARD		73
#define PCL_PAPERSIZE_4x6		74
#define PCL_PAPERSIZE_5x8		75
#define PCL_PAPERSIZE_3x5		78
#define PCL_PAPERSIZE_MONARCH_ENV	80
#define PCL_PAPERSIZE_COMMERCIAL10_ENV	81
#define PCL_PAPERSIZE_DL_ENV		90
#define PCL_PAPERSIZE_C5_ENV		91
#define PCL_PAPERSIZE_C6_ENV		92
#define PCL_PAPERSIZE_CUSTOM		101	/* Custom size */
#define PCL_PAPERSIZE_INVITATION_ENV	109
#define PCL_PAPERSIZE_JAPANESE_3_ENV	110
#define PCL_PAPERSIZE_JAPANESE_4_ENV	111
#define PCL_PAPERSIZE_KAKU_ENV		113
#define PCL_PAPERSIZE_HP_CARD		114	/* HP Greeting card!?? */

/*
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999". NOTE: The names *must* match
 * those in print-util.c for the lookups to work properly!
 */

const static pcl_t pcl_media_sizes[] =
{
    {"Executive", PCL_PAPERSIZE_EXECUTIVE},		/* US Exec (7.25 x 10.5 in) */
    {"Letter", PCL_PAPERSIZE_LETTER},			/* US Letter (8.5 x 11 in) */
    {"Legal", PCL_PAPERSIZE_LEGAL},			/* US Legal (8.5 x 14 in) */
    {"Tabloid", PCL_PAPERSIZE_TABLOID},			/* US Tabloid (11 x 17 in) */
    {"Manual", PCL_PAPERSIZE_STATEMENT},		/* US Manual/Statement (5.5 x 8.5 in) */
    {"13x19", PCL_PAPERSIZE_SUPER_B},			/* US 13x19/Super B (13 x 19 in) */
    {"A5", PCL_PAPERSIZE_A5},				/* ISO/JIS A5 (148 x 210 mm) */
    {"A4", PCL_PAPERSIZE_A4},				/* ISO/JIS A4 (210 x 297 mm) */
    {"A3", PCL_PAPERSIZE_A3},				/* ISO/JIS A3 (297 x 420 mm) */
    {"B5 JIS", PCL_PAPERSIZE_JIS_B5},			/* JIS B5 (182 x 257 mm). */
    {"B4 JIS", PCL_PAPERSIZE_JIS_B4},			/* JIS B4 (257 x 364 mm). */
    {"Hagaki Card", PCL_PAPERSIZE_HAGAKI_CARD},		/* Japanese Hagaki Card (100 x 148 mm) */
    {"Oufuku Card", PCL_PAPERSIZE_OUFUKU_CARD},		/* Japanese Oufuku Card (148 x 200 mm) */
    {"A6", PCL_PAPERSIZE_A6_CARD},			/* ISO/JIS A6 card */
    {"4x6", PCL_PAPERSIZE_4x6},				/* US Index card (4 x 6 in) */
    {"5x8", PCL_PAPERSIZE_5x8},				/* US Index card (5 x 8 in) */
    {"3x5", PCL_PAPERSIZE_3x5},				/* US Index card (3 x 5 in) */
    {"Monarch", PCL_PAPERSIZE_MONARCH_ENV},		/* Monarch Envelope (3 7/8 x 7 1/2 in) */
    {"Commercial 10", PCL_PAPERSIZE_COMMERCIAL10_ENV},	/* US Commercial 10 Envelope (4.125 x 9.5 in) Portrait */
    {"DL", PCL_PAPERSIZE_DL_ENV},			/* DL envelope (110 x 220 mm) Portrait */
    {"C5", PCL_PAPERSIZE_C5_ENV},			/* C5 envelope (162 x 229 mm) */
    {"C6", PCL_PAPERSIZE_C6_ENV},			/* C6 envelope (114 x 162 mm) */
    {"A2 Invitation", PCL_PAPERSIZE_INVITATION_ENV},	/* US A2 Invitation envelope (4 3/8 x 5 3/4 in) */
    {"Long 3", PCL_PAPERSIZE_JAPANESE_3_ENV},		/* Japanese Long Envelope #3 (120 x 235 mm) */
    {"Long 4", PCL_PAPERSIZE_JAPANESE_4_ENV},		/* Japanese Long Envelope #4 (90 x 205 mm) */
    {"Kaku", PCL_PAPERSIZE_KAKU_ENV},			/* Japanese Kaku Envelope (240 x 332.1 mm) */
    {"HP Greeting Card", PCL_PAPERSIZE_HP_CARD}, 	/* Hp greeting card (size?? */
};
#define NUM_PRINTER_PAPER_SIZES	(sizeof(pcl_media_sizes) / sizeof(pcl_t))

/*
 * Media type to code table
 */

#define PCL_PAPERTYPE_PLAIN	0
#define PCL_PAPERTYPE_BOND	1
#define PCL_PAPERTYPE_PREMIUM	2
#define PCL_PAPERTYPE_GLOSSY	3	/* or photo */
#define PCL_PAPERTYPE_TRANS	4
#define PCL_PAPERTYPE_QPHOTO	5	/* Quick dry photo (2000 only) */
#define PCL_PAPERTYPE_QTRANS	6	/* Quick dry transparency (2000 only) */

const static pcl_t pcl_media_types[] =
{
    {"Plain", PCL_PAPERTYPE_PLAIN},
    {"Bond", PCL_PAPERTYPE_BOND},
    {"Premium", PCL_PAPERTYPE_PREMIUM},
    {"Glossy/Photo", PCL_PAPERTYPE_GLOSSY},
    {"Transparency", PCL_PAPERTYPE_TRANS},
    {"Quick-dry Photo", PCL_PAPERTYPE_QPHOTO},
    {"Quick-dry Transparency", PCL_PAPERTYPE_QTRANS},
};
#define NUM_PRINTER_PAPER_TYPES	(sizeof(pcl_media_types) / sizeof(pcl_t))

/*
 * Media feed to code table. There are different names for the same code,
 * so we encode them by adding "lumps" of "PAPERSOURCE_MOD".
 * This is removed later to get back to the main codes.
 */

#define PAPERSOURCE_MOD			16

#define PCL_PAPERSOURCE_STANDARD	0	/* Don't output code */
#define PCL_PAPERSOURCE_MANUAL		2
#define PCL_PAPERSOURCE_ENVELOPE	3	/* Not used */

/* LaserJet types */
#define PCL_PAPERSOURCE_LJ_TRAY2	1
#define PCL_PAPERSOURCE_LJ_TRAY3	4
#define PCL_PAPERSOURCE_LJ_TRAY4	5
#define PCL_PAPERSOURCE_LJ_TRAY1	8

/* Deskjet 340 types */
#define PCL_PAPERSOURCE_340_PCSF	1 + PAPERSOURCE_MOD
						/* Portable sheet feeder for 340 */
#define PCL_PAPERSOURCE_340_DCSF	4 + PAPERSOURCE_MOD
						/* Desktop sheet feeder for 340 */

/* Other Deskjet types */
#define PCL_PAPERSOURCE_DJ_TRAY		1 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
#define PCL_PAPERSOURCE_DJ_TRAY2	4 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Tray 2 for 2500 */
#define PCL_PAPERSOURCE_DJ_OPTIONAL	5 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Optional source for 2500 */
#define PCL_PAPERSOURCE_DJ_AUTO		7 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Autoselect for 2500 */

const static pcl_t pcl_media_sources[] =
{
    {"Standard", PCL_PAPERSOURCE_STANDARD},
    {"Manual", PCL_PAPERSOURCE_MANUAL},
/*  {"Envelope", PCL_PAPERSOURCE_ENVELOPE}, */
    {"Tray 1", PCL_PAPERSOURCE_LJ_TRAY1},
    {"Tray 2", PCL_PAPERSOURCE_LJ_TRAY2},
    {"Tray 3", PCL_PAPERSOURCE_LJ_TRAY3},
    {"Tray 4", PCL_PAPERSOURCE_LJ_TRAY4},
    {"Portable Sheet Feeder", PCL_PAPERSOURCE_340_PCSF},
    {"Desktop Sheet Feeder", PCL_PAPERSOURCE_340_DCSF},
    {"Tray", PCL_PAPERSOURCE_DJ_TRAY},
    {"Tray 2", PCL_PAPERSOURCE_DJ_TRAY2},
    {"Optional Source", PCL_PAPERSOURCE_DJ_OPTIONAL},
    {"Autoselect", PCL_PAPERSOURCE_DJ_AUTO},
};
#define NUM_PRINTER_PAPER_SOURCES	(sizeof(pcl_media_sources) / sizeof(pcl_t))

#define PCL_RES_150_150		1
#define PCL_RES_300_300		2
#define PCL_RES_600_300		4	/* DJ 600 series */
#define PCL_RES_600_600_MONO	8	/* DJ 600/800/1100/2000 b/w only */
#define PCL_RES_600_600		16	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_1200_600	32	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_2400_600	64	/* DJ 9xx/1220C/PhotoSmart */

const static pcl_t pcl_resolutions[] =
{
    {"150x150 DPI", PCL_RES_150_150},
    {"300x300 DPI", PCL_RES_300_300},
    {"600x300 DPI", PCL_RES_600_300},
    {"600x600 DPI monochrome", PCL_RES_600_600_MONO},
    {"600x600 DPI", PCL_RES_600_600},
    {"1200x600 DPI", PCL_RES_1200_600},
    {"2400x600 DPI", PCL_RES_2400_600},
};
#define NUM_RESOLUTIONS		(sizeof(pcl_resolutions) / sizeof (pcl_t))

const char *
pcl_default_resolution(const printer_t *printer)
{
  return pcl_resolutions[0].pcl_name;
}

/*
 * Printer capability data
 */

typedef struct {
  int model;
  int max_width;
  int max_height;
  int resolutions;
  int top_margin;
  int bottom_margin;
  int left_margin;
  int right_margin;
  int color_type;		/* 2 print head or one, 2 level or 4 */
  int printer_type;		/* Deskjet/Laserjet and quirks */
/* The paper size, paper type and paper source codes cannot be combined */
  int paper_sizes[NUM_PRINTER_PAPER_SIZES + 1];
				/* Paper sizes */
  int paper_types[NUM_PRINTER_PAPER_TYPES + 1];
				/* Paper types */
  int paper_sources[NUM_PRINTER_PAPER_SOURCES + 1];
				/* Paper sources */
  } pcl_cap_t;

#define PCL_COLOR_NONE		0
#define PCL_COLOR_CMY		1	/* One print head */
#define PCL_COLOR_CMYK		2	/* Two print heads */
#define PCL_COLOR_CMYK4		4	/* CRet printing */
#define PCL_COLOR_CMYKcm	8	/* CMY + Photo Cart */

#define PCL_PRINTER_LJ		1
#define PCL_PRINTER_DJ		2
#define PCL_PRINTER_NEW_ERG	4	/* use "\033*rC" to end raster graphics,
					   instead of "\033*rB" */
#define PCL_PRINTER_TIFF	8	/* Use TIFF compression */
#define PCL_PRINTER_MEDIATYPE	16	/* Use media type & print quality */
#define PCL_PRINTER_CUSTOM_SIZE	32	/* Custom sizes supported */

/*
 * FIXME - the 520 shouldn't be lumped in with the 500 as it supports
 * more paper sizes.
 *
 * The following models use depletion, raster quality and shingling:-
 * 500, 500c, 510, 520, 550c, 560c.
 * The rest use Media Type and Print Quality.
 *
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999". 
 */

static pcl_cap_t pcl_model_capabilities[] =
{
  /* Default/unknown printer - assume laserjet */
  { 0,
    17 * 72 / 2, 14 * 72,		/* Max paper size */
    PCL_RES_150_150 | PCL_RES_300_300,	/* Resolutions */
    12, 12, 18, 18,			/* Margins */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      0,
    },
    { -1,			/* No selectable paper types */
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 340 */
  { 340,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 41, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_340_PCSF,
      PCL_PAPERSOURCE_340_DCSF,
      -1,
    },
  },
  /* Deskjet 400 */
  { 400,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 41, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_JIS_B5,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 500, 520 */
  { 500,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 41, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ,
    {
/*    PCL_PAPERSIZE_EXECUTIVE,	The 500 doesn't support this, the 520 does */
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
/*    PCL_PAPERSIZE_DL_ENV,	The 500 doesn't support this, the 520 does */
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 500C */
  { 501,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 33, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 540C */
  { 540,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 33, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 550C, 560C */
  { 550,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    3, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
/* The 550/560 support COM10 and DL envelope, but the control codes
   are negative, indicating landscape mode. This needs thinking about! */
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 600/600C */
  { 600,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    0, 33, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 6xx series, plus 810/812/840/842/895 */
  { 601,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    0, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 69x series (Photo Capable) */
  { 690,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600,
    0, 33, 18, 18,
    PCL_COLOR_CMYK | PCL_COLOR_CMYKcm,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 850/855/870/890 (C-RET) */
  { 800,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    3, 33, 18, 18,
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 900 series, 1220C, PhotoSmart P1000/P1100 */
  { 900,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    3, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 1220C (or other large format 900) */
  { 901,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    3, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_SUPER_B,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_OUFUKU_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_3x5,
      PCL_PAPERSIZE_HP_CARD,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      PCL_PAPERSIZE_JAPANESE_3_ENV,
      PCL_PAPERSIZE_JAPANESE_4_ENV,
      PCL_PAPERSIZE_KAKU_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 1100C, 1120C */
  { 1100,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    3, 33, 18, 18,
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_SUPER_B,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      PCL_PAPERSIZE_JAPANESE_3_ENV,
      PCL_PAPERSIZE_JAPANESE_4_ENV,
      PCL_PAPERSIZE_KAKU_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 1200C, 1600C */
  { 1200,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
/* This printer is not mentioned in the Comparison tables,
   so I'll just pick some likely sizes... */
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 2000 */
  { 2000,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_3x5,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      PCL_PAPERTYPE_QPHOTO,
      PCL_PAPERTYPE_QTRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 2500 */
  { 2500,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      PCL_PAPERTYPE_QPHOTO,
      PCL_PAPERTYPE_QTRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_AUTO,
      PCL_PAPERSOURCE_DJ_TRAY,
      PCL_PAPERSOURCE_DJ_TRAY2,
      PCL_PAPERSOURCE_DJ_OPTIONAL,
      -1,
    },
  },
  /* LaserJet II series */
  { 2,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet III series */
  { 3,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4L */
  { 4,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4V, 4Si, 5Si */
  { 5,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,		/* Guess */
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4 series (except as above), 5 series, 6 series */
  { 6,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 5Si */
  { 7,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,		/* Guess */
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
};

/*
 * Convert a name into it's option value
 */

static int pcl_string_to_val(const char *string,		/* I: String */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  int code = -1;

 /*
  * Look up the string in the table and convert to the code.
  */

  for (i=0; i<num_options; i++) {
    if (!strcmp(string, options[i].pcl_name)) {
       code=options[i].pcl_code;
       break;
       }
  }

#ifdef DEBUG
  fprintf(stderr, "String: %s, Code: %d\n", string, code);
#endif

  return(code);
}

/*
 * Convert a value into it's option name
 */

static char * pcl_val_to_string(int code,			/* I: Code */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  char *string = NULL;

 /*
  * Look up the code in the table and convert to the string.
  */

  for (i=0; i<num_options; i++) {
    if (code == options[i].pcl_code) {
       string=options[i].pcl_name;
       break;
       }
  }

#ifdef DEBUG
  fprintf(stderr, "Code: %d, String: %s\n", code, string);
#endif

  return(string);
}

static double dot_sizes[] = { 0.5, 0.832, 1.0 };
static simple_dither_range_t variable_dither_ranges[] =
{
  { 0.152, 0x1, 0 },
  { 0.255, 0x2, 0 },
  { 0.38,  0x3, 0 },
  { 0.5,   0x1, 1 },
  { 0.67,  0x2, 1 },
  { 1.0,   0x3, 1 }
};

/*
 * pcl_get_model_capabilities() - Return struct of model capabilities
 */

static pcl_cap_t				/* O: Capabilities */
pcl_get_model_capabilities(int model)	/* I: Model */
{
  int i;
  int models= sizeof(pcl_model_capabilities) / sizeof(pcl_cap_t);
  for (i=0; i<models; i++) {
    if (pcl_model_capabilities[i].model == model) {
      return pcl_model_capabilities[i];
    }
  }
  fprintf(stderr,"pcl: model %d not found in capabilities list.\n",model);
  return pcl_model_capabilities[0];
}

static char *
c_strdup(const char *s)
{
  char *ret = malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

/*
 * Convert Media size name into PCL media code for printer
 */

static int pcl_convert_media_size(const char *media_size,	/* I: Media size string */
				  int  model)		/* I: model number */
{

  int i;
  int media_code = 0;
  pcl_cap_t caps;

 /*
  * First look up the media size in the table and convert to the code.
  */

  media_code = pcl_string_to_val(media_size, pcl_media_sizes,
                                 sizeof(pcl_media_sizes) / sizeof(pcl_t));

#ifdef DEBUG
  fprintf(stderr, "Media Size: %s, Code: %d\n", media_size, media_code);
#endif

 /*
  * Now see if the printer supports the code found.
  */

  if (media_code != -1) {
    caps = pcl_get_model_capabilities(model);

    for (i=0; (i<NUM_PRINTER_PAPER_SIZES) && (caps.paper_sizes[i] != -1); i++) {
      if (media_code == caps.paper_sizes[i])
        return(media_code);		/* Is supported */
    }

#ifdef DEBUG
    fprintf(stderr, "Media Code %d not supported by printer model %d.\n",
      media_code, model);
#endif
    return(-1);				/* Not supported */
  }
  else
    return(-1);				/* Not supported */
}

/*
 * 'pcl_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
pcl_parameters(const printer_t *printer,/* I - Printer model */
               char *ppd_file,		/* I - PPD file (not used) */
               char *name,		/* I - Name of parameter */
               int  *count)		/* O - Number of values */
{
  int		i;
  char		**valptrs;
  pcl_cap_t caps;

  static char *ink_types[] =
  {
    "Color + Black Cartridges",
    "Color + Photo Cartridges"
  };

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  caps = pcl_get_model_capabilities(printer->model);

#ifdef DEBUG
  fprintf(stderr, "Printer model = %d\n", printer->model);
  fprintf(stderr, "PageWidth = %d, PageLength = %d\n", caps.max_width, caps.max_height);
  fprintf(stderr, "Margins: top = %d, bottom = %d, left = %d, right = %d\n",
    caps.top_margin, caps.bottom_margin, caps.left_margin, caps.right_margin);
  fprintf(stderr, "Resolutions: %d\n", caps.resolutions);
  fprintf(stderr, "ColorType = %d, PrinterType = %d\n", caps.color_type, caps.printer_type);
#endif

  if (strcmp(name, "PageSize") == 0)
    {
      const papersize_t *papersizes = get_papersizes();
#ifdef PCL_NO_CUSTOM_PAPERSIZES
      int use_custom = 0;
#else
      int use_custom = ((caps.printer_type & PCL_PRINTER_CUSTOM_SIZE)
                         == PCL_PRINTER_CUSTOM_SIZE);
#endif
      valptrs = malloc(sizeof(char *) * known_papersizes());
      *count = 0;
      for (i = 0; i < known_papersizes(); i++)
	{
	  if (strlen(papersizes[i].name) > 0 &&
	      papersizes[i].width <= caps.max_width &&
	      papersizes[i].length <= caps.max_height &&
              ((use_custom == 1) || ((use_custom == 0) &&
              (pcl_convert_media_size(papersizes[i].name, printer->model) != -1)))
             )
	    {
	      valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
	      strcpy(valptrs[*count], papersizes[i].name);
	      (*count)++;
	    }
	}
      return (valptrs);
    }
  else if (strcmp(name, "MediaType") == 0)
  {
    if (caps.paper_types[0] == -1)
    {
      *count = 0;
      return (NULL);
    }
    else
    {
      valptrs = malloc(sizeof(char *) * NUM_PRINTER_PAPER_TYPES);
      *count = 0;
      for (i=0; (i < NUM_PRINTER_PAPER_TYPES) && (caps.paper_types[i] != -1); i++) {
        valptrs[i] = c_strdup(pcl_val_to_string(caps.paper_types[i], pcl_media_types,
                                        NUM_PRINTER_PAPER_TYPES));
        (*count)++;
      }
      return(valptrs);
    }
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    if (caps.paper_sources[0] == -1)
    {
      *count = 0;
      return (NULL);
    }
    else
    {
      valptrs = malloc(sizeof(char *) * NUM_PRINTER_PAPER_SOURCES);
      *count = 0;
      for (i=0; (i < NUM_PRINTER_PAPER_SOURCES) && (caps.paper_sources[i] != -1); i++) {
        valptrs[i] = c_strdup(pcl_val_to_string(caps.paper_sources[i], pcl_media_sources,
                                        NUM_PRINTER_PAPER_SOURCES));
        (*count)++;
      }
      return(valptrs);
    }
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    *count = 0;
    valptrs = malloc(sizeof(char *) * NUM_RESOLUTIONS);
    for (i = 0; i < NUM_RESOLUTIONS; i++)
    {
      if (caps.resolutions & pcl_resolutions[i].pcl_code)
      {
         valptrs[*count] = c_strdup(pcl_val_to_string(pcl_resolutions[i].pcl_code,
					pcl_resolutions, NUM_RESOLUTIONS));
         (*count)++;
      }
    }
    return(valptrs);
  }
  else if (strcmp(name, "InkType") == 0)
  {
    if (caps.color_type & PCL_COLOR_CMYKcm)
    {
      valptrs = malloc(sizeof(char *) * 2);
      valptrs[0] = c_strdup(ink_types[0]);
      valptrs[1] = c_strdup(ink_types[1]);
      *count = 2;
      return(valptrs);
    }
    else
      return(NULL);
  }
  else
    return (NULL);
}


/*
 * 'pcl_imageable_area()' - Return the imageable area of the page.
 */

void
pcl_imageable_area(const printer_t *printer,	/* I - Printer model */
		   const vars_t *v,     /* I */
                   int  *left,		/* O - Left position in points */
                   int  *right,		/* O - Right position in points */
                   int  *bottom,	/* O - Bottom position in points */
                   int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */
  pcl_cap_t caps;			/* Printer caps */

  caps = pcl_get_model_capabilities(printer->model);

  default_media_size(printer, v, &width, &length);

/*
 * Note: The margins actually vary with paper size, but since you can
 * move the image around on the page anyway, it hardly matters.
 */

  *left   = caps.left_margin;
  *right  = width - caps.right_margin;
  *top    = length - caps.top_margin;
  *bottom = caps.bottom_margin;
}

void
pcl_limit(const printer_t *printer,	/* I - Printer model */
	  const vars_t *v,  		/* I */
	  int  *width,			/* O - Left position in points */
	  int  *length)			/* O - Top position in points */
{
  pcl_cap_t caps= pcl_get_model_capabilities(printer->model);
  *width =	caps.max_width;
  *length =	caps.max_height;
}

/*
 * 'pcl_print()' - Print an image to an HP printer.
 */

void
pcl_print(const printer_t *printer,		/* I - Model */
          int       copies,		/* I - Number of copies */
          FILE      *prn,		/* I - File to print to */
          Image     image,		/* I - Image to print */
	  const vars_t    *v)
{
  unsigned char *cmap = v->cmap;
  int		model = printer->model;
  const char	*resolution = v->resolution;
  const char	*media_size;
  const char	*media_type = v->media_type;
  const char	*media_source = v->media_source;
  const char	*ink_type = v->ink_type;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  double 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  unsigned short *out;
  unsigned char	*in,		/* Input pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light Cyan bitmap data */
		*lmagenta;	/* Light Magenta bitmap data */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		length,		/* Length of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc;	/* Color conversion function... */
  void		(*writefunc)(FILE *, unsigned char *, int, int);
				/* PCL output function */
  int           image_height,
                image_width,
                image_bpp;
  void *	dither;
  pcl_cap_t	caps;		/* Printer capabilities */
  int		do_cret,	/* 300 DPI CRet printing */
		do_6color,	/* CMY + cmK printing */
		planes;		/* # of output planes */
  int		pcl_media_size, /* PCL media size code */
		pcl_media_type, /* PCL media type code */
		pcl_media_source;	/* PCL media source code */
  vars_t	nv;
  const papersize_t *pp;

  memcpy(&nv, v, sizeof(vars_t));
  caps = pcl_get_model_capabilities(model);

 /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);

 /*
  * Figure out the output resolution...
  */

  sscanf(resolution,"%dx%d",&xdpi,&ydpi);

#ifdef DEBUG
  fprintf(stderr,"pcl: resolution=%dx%d\n",xdpi,ydpi);
#endif

 /*
  * Choose the correct color conversion function...
  */
  if (((caps.resolutions & PCL_RES_600_600_MONO) == PCL_RES_600_600_MONO) &&
      output_type != OUTPUT_GRAY && xdpi == 600 && ydpi == 600) {
      fprintf(stderr, "600x600 resolution only available in MONO\n");
      output_type = OUTPUT_GRAY;
  }

  if (nv.image_type == IMAGE_MONOCHROME)
    {
      output_type = OUTPUT_GRAY;
    }

  if (caps.color_type == PCL_COLOR_NONE)
    output_type = OUTPUT_GRAY;

  colorfunc = choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, &nv);

  do_cret = (xdpi >= 300 && ((caps.color_type & PCL_COLOR_CMYK4) == PCL_COLOR_CMYK4) &&
	     nv.image_type != IMAGE_MONOCHROME);

#ifdef DEBUG
  fprintf(stderr, "do_cret = %d\n", do_cret);
#endif

  do_6color = (strcmp(ink_type, "Color + Photo Cartridges") == 0);
#ifdef DEBUG
  fprintf(stderr, "do_6color = %d\n", do_6color);
#endif

 /*
  * Compute the output size...
  */

  pcl_imageable_area(printer, &nv, &page_left, &page_right,
                     &page_bottom, &page_top);
#ifdef DEBUG
  printf("Before compute_page_parameters()\n");
  printf("page_left = %d, page_right = %d, page_top = %d, page_bottom = %d\n",
    page_left, page_right, page_top, page_bottom);
  printf("top = %d, left = %d\n", top, left);
  printf("scaling = %f, image_width = %d, image_height = %d\n", scaling,
    image_width, image_height);
#endif

  compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_height, &left, &top);

  /*
   * Recompute the image height and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = Image_height(image);
  image_width = Image_width(image);

#ifdef DEBUG
  printf("After compute_page_parameters()\n");
  printf("page_width = %d, page_height = %d\n", page_width, page_height);
  printf("out_width = %d, out_height = %d\n", out_width, out_height);
  printf("top = %d, left = %d\n", top, left);
#endif /* DEBUG */

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);

 /*
  * Send PCL initialization commands...
  */

  fputs("\033E", prn); 				/* PCL reset */

 /*
  * Set media size
  */

  if (strlen(v->media_size) > 0)
    media_size = v->media_size;
  else if ((pp = get_papersize_by_size(v->page_height, v->page_width)) != NULL)
    media_size = pp->name;
  else
    media_size = "";

  pcl_media_size = pcl_convert_media_size(media_size, model);

#ifdef DEBUG
  printf("pcl_media_size = %d, media_size = %s\n", pcl_media_size, media_size);
#endif

 /*
  * If the media size requested is unknown, try it as a custom size.
  *
  * Warning: The margins may need to be fixed for this!
  */

  if (pcl_media_size == -1) {
    fprintf(stderr, "Paper size %s is not directly supported by printer.\n",
      media_size);
    fprintf(stderr, "Trying as custom pagesize (watch the margins!)\n");
    pcl_media_size = PCL_PAPERSIZE_CUSTOM;			/* Custom */
  }

  fprintf(prn, "\033&l%dA", pcl_media_size);

  fputs("\033&l0L", prn);			/* Turn off perforation skip */
  fputs("\033&l0E", prn);			/* Reset top margin to 0 */

 /*
  * Convert media source string to the code, if specified.
  */

  if (strlen(media_source) != 0) {
    pcl_media_source = pcl_string_to_val(media_source, pcl_media_sources,
                         sizeof(pcl_media_sources) / sizeof(pcl_t));

#ifdef DEBUG
    printf("pcl_media_source = %d, media_source = %s\n", pcl_media_source,
           media_source);
#endif

    if (pcl_media_source == -1)
      fprintf(stderr, "Unknown media source %s, ignored.\n", media_source);
    else if (pcl_media_source != PCL_PAPERSOURCE_STANDARD) {

/* Correct the value by taking the modulus */

      pcl_media_source = pcl_media_source % PAPERSOURCE_MOD;
      fprintf(prn, "\033&l%dH", pcl_media_source);
    }
  }

 /*
  * Convert media type string to the code, if specified.
  */

  if (strlen(media_type) != 0) {
    pcl_media_type = pcl_string_to_val(media_type, pcl_media_types,
                       sizeof(pcl_media_types) / sizeof(pcl_t));

#ifdef DEBUG
    printf("pcl_media_type = %d, media_type = %s\n", pcl_media_type,
           media_type);
#endif

    if (pcl_media_type == -1) {
      fprintf(stderr, "Unknown media type %s, set to PLAIN.\n", media_type);
      pcl_media_type = PCL_PAPERTYPE_PLAIN;
    }
  }
  else
    pcl_media_type = PCL_PAPERTYPE_PLAIN;

 /*
  * Set DJ print quality to "best" if resolution >= 300
  */

  if ((xdpi >= 300) && ((caps.printer_type & PCL_PRINTER_DJ) == PCL_PRINTER_DJ))
  {
    if ((caps.printer_type & PCL_PRINTER_MEDIATYPE) == PCL_PRINTER_MEDIATYPE)
    {
      fputs("\033*o1M", prn);			/* Quality = presentation */
      fprintf(prn, "\033&l%dM", pcl_media_type);
    }
    else
    {
      fputs("\033*r2Q", prn);			/* Quality (high) */
      fputs("\033*o2Q", prn);			/* Shingling (4 passes) */

 /* Depletion depends on media type and cart type. */

      if ((pcl_media_type == PCL_PAPERTYPE_PLAIN)
	|| (pcl_media_type == PCL_PAPERTYPE_BOND)) {
      if ((caps.color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
          fputs("\033*o2D", prn);			/* Depletion 25% */
        else
          fputs("\033*o5D", prn);			/* Depletion 50% with gamma correction */
      }

      else if ((pcl_media_type == PCL_PAPERTYPE_PREMIUM)
             || (pcl_media_type == PCL_PAPERTYPE_GLOSSY)
             || (pcl_media_type == PCL_PAPERTYPE_TRANS))
        fputs("\033*o1D", prn);			/* Depletion none */
    }
  }

  if ((xdpi != ydpi) || (do_cret) || (do_6color))
						/* Set resolution using CRD */
  {

   /*
    * Send configure image data command with horizontal and
    * vertical resolutions as well as a color count...
    */

    if (output_type != OUTPUT_GRAY)
      if ((caps.color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        planes = 3;
      else
        if (do_6color)
          planes = 6;
        else
          planes = 4;
    else
      planes = 1;

    fprintf(prn, "\033*g%dW", 2 + (planes * 6));
    putc(2, prn);				/* Format 2 (Complex Direct Planar) */
    putc(planes, prn);				/* # output planes */

    if (planes != 3) {
      putc(xdpi >> 8, prn);			/* Black resolution */
      putc(xdpi, prn);
      putc(ydpi >> 8, prn);
      putc(ydpi, prn);
      putc(0, prn);
      putc(do_cret ? 4 : 2, prn);
    }

    if (planes != 1) {
      putc(xdpi >> 8, prn);			/* Cyan resolution */
      putc(xdpi, prn);
      putc(ydpi >> 8, prn);
      putc(ydpi, prn);
      putc(0, prn);
      putc(do_cret ? 4 : 2, prn);

      putc(xdpi >> 8, prn);			/* Magenta resolution */
      putc(xdpi, prn);
      putc(ydpi >> 8, prn);
      putc(ydpi, prn);
      putc(0, prn);
      putc(do_cret ? 4 : 2, prn);

      putc(xdpi >> 8, prn);			/* Yellow resolution */
      putc(xdpi, prn);
      putc(ydpi >> 8, prn);
      putc(ydpi, prn);
      putc(0, prn);
      putc(do_cret ? 4 : 2, prn);
    }
    if (planes == 6)
    {
      putc(xdpi >> 8, prn);			/* Light Cyan resolution */
      putc(xdpi, prn);
      putc(ydpi >> 8, prn);
      putc(ydpi, prn);
      putc(0, prn);
      putc(do_cret ? 4 : 2, prn);

      putc(xdpi >> 8, prn);			/* Light Magenta resolution */
      putc(xdpi, prn);
      putc(ydpi >> 8, prn);
      putc(ydpi, prn);
      putc(0, prn);
      putc(do_cret ? 4 : 2, prn);
    }
  }
  else
  {
    fprintf(prn, "\033*t%dR", xdpi);		/* Simple resolution */
    if (output_type != OUTPUT_GRAY)
    {
      if ((caps.color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        fputs("\033*r-3U", prn);		/* Simple CMY color */
      else
        fputs("\033*r-4U", prn);		/* Simple KCMY color */
    }
  }

#ifndef PCL_DEBUG_DISABLE_COMPRESSION
  if ((caps.printer_type & PCL_PRINTER_TIFF) == PCL_PRINTER_TIFF)
  {
    fputs("\033*b2M", prn);			/* Mode 2 (TIFF) */
    writefunc = pcl_mode2;
  }
  else
#endif
  {
    fputs("\033*b0M", prn);			/* Mode 0 (no compression) */
    writefunc = pcl_mode0;
  }

 /*
  * Convert image size to printer resolution and setup the page for printing...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

#ifdef DEBUG
  fprintf(stderr, "left %d margin %d top %d margin %d width %d height %d\n",
	  left, caps.left_margin, top, caps.top_margin, out_width, out_height);
#endif

  fprintf(prn, "\033&a%dH", 10 * left);		/* Set left raster position */
  fprintf(prn, "\033&a%dV", 10 * (top + caps.top_margin));
				/* Set top raster position */
  fprintf(prn, "\033*r%dS", out_width);		/* Set raster width */
  fprintf(prn, "\033*r%dT", out_height);	/* Set raster height */

  fputs("\033*r1A", prn); 			/* Start GFX */

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;
  if (do_cret)
    length *= 2;

  if (output_type == OUTPUT_GRAY)
  {
    black   = malloc(length);
    cyan    = NULL;
    magenta = NULL;
    yellow  = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
  }
  else
  {
    cyan    = malloc(length);
    magenta = malloc(length);
    yellow  = malloc(length);

    if ((caps.color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
      black = NULL;
    else
      black = malloc(length);
    if (do_6color)
    {
      lcyan    = malloc(length);
      lmagenta = malloc(length);
    }
    else
    {
      lcyan    = NULL;
      lmagenta = NULL;
    }
  }

 /*
  * Output the page, rotating as necessary...
  */

  compute_lut(256, &nv);

  if (xdpi > ydpi)
    dither = init_dither(image_width, out_width, 1, xdpi / ydpi, &nv);
  else
    dither = init_dither(image_width, out_width, ydpi / xdpi, 1, &nv);

/* Set up dithering for special printers. */

#if 0		/* Leave alone for now */
  dither_set_black_levels(dither, 1.5, 1.5, 1.5);
  dither_set_black_lower(dither, .4);
  dither_set_black_upper(dither, .999);
#endif

  if (do_cret)				/* 4-level printing for 800/1120 */
    {
      dither_set_y_ranges_simple(dither, 3, dot_sizes, nv.density);
      dither_set_k_ranges_simple(dither, 3, dot_sizes, nv.density);

/* Note: no printer I know of does both CRet (4-level) and 6 colour, but
   what the heck. variable_dither_ranges copied from print-escp2.c */

      if (do_6color)			/* Photo for 69x */
	{
	  dither_set_c_ranges(dither, 6, variable_dither_ranges, nv.density);
	  dither_set_m_ranges(dither, 6, variable_dither_ranges, nv.density);
	}
      else
	{
	  dither_set_c_ranges_simple(dither, 3, dot_sizes, nv.density);
	  dither_set_m_ranges_simple(dither, 3, dot_sizes, nv.density);
	}
    }
  else

/* Set light inks for 6 colour printers. Numbers copied from print-escp2.c */

    if (do_6color)
      dither_set_light_inks(dither, .25, .25, 0.0, nv.density);

  switch (nv.image_type)
    {
    case IMAGE_LINE_ART:
      dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      dither_set_ink_spread(dither, 14);
      break;
    }
  dither_set_density(dither, nv.density);

  in  = malloc(image_width * image_bpp);
  out = malloc(image_width * out_bpp * 2);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  for (y = 0; y < out_height; y ++)
  {
    int duplicate_line = 1;
#ifdef DEBUG
    printf("pcl_print: y = %d, line = %d, val = %d, mod = %d, height = %d\n",
           y, errline, errval, errmod, out_height);
#endif /* DEBUG */
    if ((y & 63) == 0)
      Image_note_progress(image, y, out_height);

    if (errline != errlast)
    {
      errlast = errline;
      duplicate_line = 0;
      Image_get_row(image, in, errline);
      (*colorfunc)(in, out, image_width, image_bpp, cmap, &nv);
    }

    if (do_cret)
    {
     /*
      * 4-level (CRet) dithers...
      */

      if (output_type == OUTPUT_GRAY)
      {
	dither_black(out, y, dither, black, duplicate_line);
        (*writefunc)(prn, black + length / 2, length / 2, 0);
        (*writefunc)(prn, black, length / 2, 1);
      }
      else
      {
	dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
		    yellow, NULL, black, duplicate_line);

        (*writefunc)(prn, black + length / 2, length / 2, 0);
        (*writefunc)(prn, black, length / 2, 0);
        (*writefunc)(prn, cyan + length / 2, length / 2, 0);
        (*writefunc)(prn, cyan, length / 2, 0);
        (*writefunc)(prn, magenta + length / 2, length / 2, 0);
        (*writefunc)(prn, magenta, length / 2, 0);
        (*writefunc)(prn, yellow + length / 2, length / 2, 0);
        if (do_6color)
        {
          (*writefunc)(prn, yellow, length / 2, 0);
          (*writefunc)(prn, lcyan + length / 2, length / 2, 0);
          (*writefunc)(prn, lcyan, length / 2, 0);
          (*writefunc)(prn, lmagenta + length / 2, length / 2, 0);
          (*writefunc)(prn, lmagenta, length / 2, 1);		/* Last plane set on light magenta */
        }
        else
          (*writefunc)(prn, yellow, length / 2, 1);		/* Last plane set on yellow */
      }
    }
    else
    {
     /*
      * Standard 2-level dithers...
      */

      if (output_type == OUTPUT_GRAY)
      {
	if (nv.image_type == IMAGE_MONOCHROME)
	  dither_monochrome(out, y, dither, black, duplicate_line);
	else
	  dither_black(out, y, dither, black, duplicate_line);
        (*writefunc)(prn, black, length, 1);
      }
      else
      {
	dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
		    yellow, NULL, black, duplicate_line);

        if (black != NULL)
          (*writefunc)(prn, black, length, 0);
        (*writefunc)(prn, cyan, length, 0);
        (*writefunc)(prn, magenta, length, 0);
        if (do_6color)
        {
          (*writefunc)(prn, yellow, length, 0);
          (*writefunc)(prn, lcyan, length, 0);
          (*writefunc)(prn, lmagenta, length, 1);		/* Last plane set on light magenta */
        }
        else
          (*writefunc)(prn, yellow, length, 1);		/* Last plane set on yellow */
      }
    }

    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
    }
  }
  Image_progress_conclude(image);

  free_dither(dither);


 /*
  * Cleanup...
  */

  free_lut(&nv);
  free(in);
  free(out);

  if (black != NULL)
    free(black);
  if (cyan != NULL)
  {
    free(cyan);
    free(magenta);
    free(yellow);
  }
  if (lcyan != NULL)
  {
    free(lcyan);
    free(lmagenta);
  }

  if ((caps.printer_type & PCL_PRINTER_NEW_ERG) == PCL_PRINTER_NEW_ERG)
    fputs("\033*rC", prn);
  else
    fputs("\033*rB", prn);

  fputs("\033&l0H", prn);		/* Eject page */
  fputs("\033E", prn);			/* PCL reset */
}



/*
 * 'pcl_mode0()' - Send PCL graphics using mode 0 (no) compression.
 */

static void
pcl_mode0(FILE          *prn,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           length,		/* I - Length of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  fprintf(prn, "\033*b%d%c", length, last_plane ? 'W' : 'V');
  fwrite(line, length, 1, prn);
}


/*
 * 'pcl_mode2()' - Send PCL graphics using mode 2 (TIFF) compression.
 */

static void
pcl_mode2(FILE          *prn,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           length,		/* I - Length of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  unsigned char	comp_buf[1536],		/* Compression buffer */
		*comp_ptr,		/* Current slot in buffer */
		*start,			/* Start of compressed data */
		repeat;			/* Repeating char */
  int		count,			/* Count of compressed bytes */
		tcount;			/* Temporary count < 128 */


 /*
  * Compress using TIFF "packbits" run-length encoding...
  */

  comp_ptr = comp_buf;

  while (length > 0)
  {
   /*
    * Get a run of non-repeated chars...
    */

    start  = line;
    line   += 2;
    length -= 2;

    while (length > 0 && (line[-2] != line[-1] || line[-1] != line[0]))
    {
      line ++;
      length --;
    }

    line   -= 2;
    length += 2;

   /*
    * Output the non-repeated sequences (max 128 at a time).
    */

    count = line - start;
    while (count > 0)
    {
      tcount = count > 128 ? 128 : count;

      comp_ptr[0] = tcount - 1;
      memcpy(comp_ptr + 1, start, tcount);

      comp_ptr += tcount + 1;
      start    += tcount;
      count    -= tcount;
    }

    if (length <= 0)
      break;

   /*
    * Find the repeated sequences...
    */

    start  = line;
    repeat = line[0];

    line ++;
    length --;

    while (length > 0 && *line == repeat)
    {
      line ++;
      length --;
    }

   /*
    * Output the repeated sequences (max 128 at a time).
    */

    count = line - start;
    while (count > 0)
    {
      tcount = count > 128 ? 128 : count;

      comp_ptr[0] = 1 - tcount;
      comp_ptr[1] = repeat;

      comp_ptr += 2;
      count    -= tcount;
    }
  }

 /*
  * Send a line of raster graphics...
  */

  fprintf(prn, "\033*b%d%c", (int)(comp_ptr - comp_buf), last_plane ? 'W' : 'V');
  fwrite(comp_buf, comp_ptr - comp_buf, 1, prn);
}
