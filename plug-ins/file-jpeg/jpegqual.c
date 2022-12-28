/* jpegqual.c - analyze quality settings of existing JPEG files
 *
 * Copyright (C) 2007 Raphaël Quinet <raphael@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * This program analyzes the quantization tables of the JPEG files
 * given on the command line and displays their quality settings.
 *
 * It is useful for developers or maintainers of the JPEG plug-in
 * because it can be used to validate the formula used in
 * jpeg_detect_quality(), by comparing the quality reported for
 * different JPEG files.
 *
 * It can also dump quantization tables so that they can be integrated
 * into this program and recognized later.  This can be used to identify
 * which device or which program saved a JPEG file.
 */

/*
 * TODO:
 * - rename this program!
 * - update quant_info[].
 * - reorganize the options: 2 groups for print options and for selection.
 * - re-add the code to compare different formulas for approx. IJG quality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <jpeglib.h>
#include <jerror.h>

#include "jpeg-quality.h"

/* command-line options */
static gboolean   option_summary      = FALSE;
static gboolean   option_ctable       = FALSE;
static gboolean   option_table_2cols  = FALSE;
static gboolean   option_unknown      = FALSE;
static gboolean   option_ignore_err   = FALSE;
static gchar    **filenames           = NULL;

static const GOptionEntry option_entries[] =
{
  {
    "ignore-errors", 'i', 0, G_OPTION_ARG_NONE, &option_ignore_err,
    "Continue processing other files after a JPEG error", NULL
  },
  {
    "summary",  's', 0, G_OPTION_ARG_NONE, &option_summary,
    "Print summary information and closest IJG quality", NULL
  },
  {
    "tables",   't', 0, G_OPTION_ARG_NONE, &option_table_2cols,
    "Dump quantization tables", NULL
  },
  {
    "c-tables", 'c', 0, G_OPTION_ARG_NONE, &option_ctable,
    "Dump quantization tables as C code", NULL
  },
  {
    "ctables",  0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &option_ctable,
    NULL, NULL
  },
  {
    "unknown",  'u', 0, G_OPTION_ARG_NONE, &option_unknown,
    "Only print information about files with unknown tables", NULL
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_FILENAME_ARRAY, &filenames,
    NULL, NULL
  },
  { NULL }
};

/* information about known JPEG quantization tables */
typedef struct
{
  const guint  hash;              /* hash of luminance/chrominance tables */
  const gint   lum_sum;           /* sum of luminance table divisors */
  const gint   chrom_sum;         /* sum of chrominance table divisors */
  const gint   subsmp_h;          /* horizontal subsampling (1st component) */
  const gint   subsmp_v;          /* vertical subsampling (1st component) */
  const gint   num_quant_tables;  /* number of tables (< 0 if no grayscale) */
  const gchar *source_name;       /* name of software of device */
  const gchar *setting_name;      /* name of quality setting */
  const gint   ijg_qual;          /* closest IJG quality setting */
} QuantInfo;

static const QuantInfo quant_info[] =
{
  { 0x0a82648b,    64,    64, 0, 0,  2, "IJG JPEG Library", "quality 100", 100 },
  { 0x4d981764,    86,   115, 0, 0,  2, "IJG JPEG Library", "quality 99", 99 },
  { 0x62b71702,   151,   224, 0, 0,  2, "IJG JPEG Library", "quality 98", 98 },
  { 0x29e095c5,   221,   333, 0, 0,  2, "IJG JPEG Library", "quality 97", 97 },
  { 0xb62c754a,   292,   443, 0, 0,  2, "IJG JPEG Library", "quality 96", 96 },
  { 0x8e55c78a,   369,   558, 0, 0,  2, "IJG JPEG Library", "quality 95", 95 },
  { 0x0664d770,   441,   668, 0, 0,  2, "IJG JPEG Library", "quality 94", 94 },
  { 0x59e5c5bc,   518,   779, 0, 0,  2, "IJG JPEG Library", "quality 93", 93 },
  { 0xd6f26606,   592,   891, 0, 0,  2, "IJG JPEG Library", "quality 92", 92 },
  { 0x8aa986ad,   667,   999, 0, 0,  2, "IJG JPEG Library", "quality 91", 91 },
  { 0x17816eb1,   736,  1110, 0, 0,  2, "IJG JPEG Library", "quality 90", 90 },
  { 0x75de9350,   814,  1223, 0, 0,  2, "IJG JPEG Library", "quality 89", 89 },
  { 0x88fdf223,   884,  1332, 0, 0,  2, "IJG JPEG Library", "quality 88", 88 },
  { 0xf40a6a50,   961,  1444, 0, 0,  2, "IJG JPEG Library", "quality 87", 87 },
  { 0xe9f2c235,  1031,  1555, 0, 0,  2, "IJG JPEG Library", "quality 86", 86 },
  { 0x82683892,  1109,  1666, 0, 0,  2, "IJG JPEG Library", "quality 85", 85 },
  { 0xb1aecce8,  1179,  1778, 0, 0,  2, "IJG JPEG Library", "quality 84", 84 },
  { 0x83375efe,  1251,  1888, 0, 0,  2, "IJG JPEG Library", "quality 83", 83 },
  { 0x1e99f479,  1326,  2000, 0, 0,  2, "IJG JPEG Library", "quality 82", 82 },
  { 0x1a02d360,  1398,  2111, 0, 0,  2, "IJG JPEG Library", "quality 81", 81 },
  { 0x96129a0d,  1477,  2221, 0, 0,  2, "IJG JPEG Library", "quality 80", 80 },
  { 0x64d4144b,  1552,  2336, 0, 0,  2, "IJG JPEG Library", "quality 79", 79 },
  { 0x48a344ac,  1620,  2445, 0, 0,  2, "IJG JPEG Library", "quality 78", 78 },
  { 0x16e820e3,  1692,  2556, 0, 0,  2, "IJG JPEG Library", "quality 77", 77 },
  { 0x246b2e95,  1773,  2669, 0, 0,  2, "IJG JPEG Library", "quality 76", 76 },
  { 0x10b035e9,  1858,  2780, 0, 0,  2, "IJG JPEG Library", "quality 75", 75 },
  { 0xd5c653da,  1915,  2836, 0, 0,  2, "IJG JPEG Library", "quality 74", 74 },
  { 0xe349618c,  1996,  2949, 0, 0,  2, "IJG JPEG Library", "quality 73", 73 },
  { 0xb18e3dc3,  2068,  3060, 0, 0,  2, "IJG JPEG Library", "quality 72", 72 },
  { 0x955d6e24,  2136,  3169, 0, 0,  2, "IJG JPEG Library", "quality 71", 71 },
  { 0x641ee862,  2211,  3284, 0, 0,  2, "IJG JPEG Library", "quality 70", 70 },
  { 0xe02eaf0f,  2290,  3394, 0, 0,  2, "IJG JPEG Library", "quality 69", 69 },
  { 0xdb978df6,  2362,  3505, 0, 0,  2, "IJG JPEG Library", "quality 68", 68 },
  { 0x76fa2371,  2437,  3617, 0, 0,  2, "IJG JPEG Library", "quality 67", 67 },
  { 0x4882b587,  2509,  3727, 0, 0,  2, "IJG JPEG Library", "quality 66", 66 },
  { 0x25556ae1,  2583,  3839, 0, 0,  2, "IJG JPEG Library", "quality 65", 65 },
  { 0x103ec03a,  2657,  3950, 0, 0,  2, "IJG JPEG Library", "quality 64", 64 },
  { 0x0627181f,  2727,  4061, 0, 0,  2, "IJG JPEG Library", "quality 63", 63 },
  { 0x7133904c,  2804,  4173, 0, 0,  2, "IJG JPEG Library", "quality 62", 62 },
  { 0x8452ef1f,  2874,  4282, 0, 0,  2, "IJG JPEG Library", "quality 61", 61 },
  { 0xe2b013be,  2952,  4395, 0, 0,  2, "IJG JPEG Library", "quality 60", 60 },
  { 0x6f87fbc2,  3021,  4506, 0, 0,  2, "IJG JPEG Library", "quality 59", 59 },
  { 0x233f1c69,  3096,  4614, 0, 0,  2, "IJG JPEG Library", "quality 58", 58 },
  { 0xa04bbcb3,  3170,  4726, 0, 0,  2, "IJG JPEG Library", "quality 57", 57 },
  { 0xf3ccaaff,  3247,  4837, 0, 0,  2, "IJG JPEG Library", "quality 56", 56 },
  { 0x1967dbe9,  3323,  4947, 0, 0,  2, "IJG JPEG Library", "quality 55", 55 },
  { 0x44050d25,  3396,  5062, 0, 0,  2, "IJG JPEG Library", "quality 54", 54 },
  { 0xd050ecaa,  3467,  5172, 0, 0,  2, "IJG JPEG Library", "quality 53", 53 },
  { 0x9e99f8f1,  3541,  5281, 0, 0,  2, "IJG JPEG Library", "quality 52", 52 },
  { 0xdf2423f4,  3621,  5396, 0, 0,  2, "IJG JPEG Library", "quality 51", 51 },
  { 0xe0f48a64,  3688,  5505, 0, 0,  2, "IJG JPEG Library", "quality 50", 50 },
  { 0xe2c4f0d4,  3755,  5614, 0, 0,  2, "IJG JPEG Library", "quality 49", 49 },
  { 0x234f1bd7,  3835,  5729, 0, 0,  2, "IJG JPEG Library", "quality 48", 48 },
  { 0xf198281e,  3909,  5838, 0, 0,  2, "IJG JPEG Library", "quality 47", 47 },
  { 0x7de407a3,  3980,  5948, 0, 0,  2, "IJG JPEG Library", "quality 46", 46 },
  { 0xb3aa597b,  4092,  6116, 0, 0,  2, "IJG JPEG Library", "quality 45", 45 },
  { 0x32b48093,  4166,  6226, 0, 0,  2, "IJG JPEG Library", "quality 44", 44 },
  { 0x9ea9f85f,  4280,  6396, 0, 0,  2, "IJG JPEG Library", "quality 43", 43 },
  { 0x335d6006,  4393,  6562, 0, 0,  2, "IJG JPEG Library", "quality 42", 42 },
  { 0xa727ea4a,  4463,  6672, 0, 0,  2, "IJG JPEG Library", "quality 41", 41 },
  { 0x1889cfc4,  4616,  6897, 0, 0,  2, "IJG JPEG Library", "quality 40", 40 },
  { 0xb1aa548e,  4719,  7060, 0, 0,  2, "IJG JPEG Library", "quality 39", 39 },
  { 0x99bebdd3,  4829,  7227, 0, 0,  2, "IJG JPEG Library", "quality 38", 38 },
  { 0xf728d062,  4976,  7447, 0, 0,  2, "IJG JPEG Library", "quality 37", 37 },
  { 0xe1ba65b9,  5086,  7616, 0, 0,  2, "IJG JPEG Library", "quality 36", 36 },
  { 0x2c8ba6a4,  5240,  7841, 0, 0,  2, "IJG JPEG Library", "quality 35", 35 },
  { 0x03f7963a,  5421,  8114, 0, 0,  2, "IJG JPEG Library", "quality 34", 34 },
  { 0xa19bed1e,  5571,  8288, 0, 0,  2, "IJG JPEG Library", "quality 33", 33 },
  { 0x7945d01c,  5756,  8565, 0, 0,  2, "IJG JPEG Library", "quality 32", 32 },
  { 0xcc36df1a,  5939,  8844, 0, 0,  2, "IJG JPEG Library", "quality 31", 31 },
  { 0x3eb1b5ca,  6125,  9122, 0, 0,  2, "IJG JPEG Library", "quality 30", 30 },
  { 0xd7f65293,  6345,  9455, 0, 0,  2, "IJG JPEG Library", "quality 29", 29 },
  { 0x4c0a8178,  6562,  9787, 0, 0,  2, "IJG JPEG Library", "quality 28", 28 },
  { 0x8281d1a1,  6823, 10175, 0, 0,  2, "IJG JPEG Library", "quality 27", 27 },
  { 0x0bbc9f7e,  7084, 10567, 0, 0,  2, "IJG JPEG Library", "quality 26", 26 },
  { 0xa8ac1cbd,  7376, 11010, 0, 0,  2, "IJG JPEG Library", "quality 25", 25 },
  { 0x459b99fc,  7668, 11453, 0, 0,  2, "IJG JPEG Library", "quality 24", 24 },
  { 0xda09c178,  7995, 11954, 0, 0,  2, "IJG JPEG Library", "quality 23", 23 },
  { 0x1c651f15,  8331, 12511, 0, 0,  2, "IJG JPEG Library", "quality 22", 22 },
  { 0x59025244,  8680, 13121, 0, 0,  2, "IJG JPEG Library", "quality 21", 21 },
  { 0xa130f919,  9056, 13790, 0, 0,  2, "IJG JPEG Library", "quality 20", 20 },
  { 0x109756cf,  9368, 14204, 0, 0,  2, "IJG JPEG Library", "quality 19", 19 },
  { 0xe929cab5,  9679, 14267, 0, 0,  2, "IJG JPEG Library", "quality 18", 18 },
  { 0xcddca370, 10027, 14346, 0, 0,  2, "IJG JPEG Library", "quality 17", 17 },
  { 0xd5fc76c0, 10360, 14429, 0, 0,  2, "IJG JPEG Library", "quality 16", 16 },
  { 0x533a1a03, 10714, 14526, 0, 0,  2, "IJG JPEG Library", "quality 15", 15 },
  { 0x0d8adaff, 11081, 14635, 0, 0,  2, "IJG JPEG Library", "quality 14", 14 },
  { 0x0d2ee95d, 11456, 14754, 0, 0,  2, "IJG JPEG Library", "quality 13", 13 },
  { 0x3a1d59a0, 11861, 14864, 0, 0,  2, "IJG JPEG Library", "quality 12", 12 },
  { 0x66555d04, 12240, 14985, 0, 0,  2, "IJG JPEG Library", "quality 11", 11 },
  { 0x7fa051b1, 12560, 15110, 0, 0,  2, "IJG JPEG Library", "quality 10", 10 },
  { 0x7b668ca3, 12859, 15245, 0, 0,  2, "IJG JPEG Library", "quality 9", 9 },
  { 0xb44d7082, 13230, 15369, 0, 0,  2, "IJG JPEG Library", "quality 8", 8 },
  { 0xe838d325, 13623, 15523, 0, 0,  2, "IJG JPEG Library", "quality 7", 7 },
  { 0xb6f58977, 14073, 15731, 0, 0,  2, "IJG JPEG Library", "quality 6", 6 },
  { 0xfd3e9fc4, 14655, 16010, 0, 0,  2, "IJG JPEG Library", "quality 5", 5 },
  { 0x7782b922, 15277, 16218, 0, 0,  2, "IJG JPEG Library", "quality 4", 4 },
  { 0x5a03ac45, 15946, 16320, 0, 0,  2, "IJG JPEG Library", "quality 3", 3 },
  { 0xe0afaa36, 16315, 16320, 0, 0,  2, "IJG JPEG Library", "quality 2", 2 },
  { 0x6d640b8b, 16320, 16320, 0, 0,  2, "IJG JPEG Library", "quality 1", 1 },
  { 0x6d640b8b, 16320, 16320, 0, 0,  2, "IJG JPEG Library", "quality 0", 1 },
  { 0x4b1d5895,  8008, 11954, 0, 0,  2, "IJG JPEG Library", "not baseline 23", -22 },
  { 0x36c32c2c,  8370, 12511, 0, 0,  2, "IJG JPEG Library", "not baseline 22", -21 },
  { 0xa971f812,  8774, 13121, 0, 0,  2, "IJG JPEG Library", "not baseline 21", -20 },
  { 0xa01f5a9b,  9234, 13790, 0, 0,  2, "IJG JPEG Library", "not baseline 20", -19 },
  { 0x0e45ab9a,  9700, 14459, 0, 0,  2, "IJG JPEG Library", "not baseline 19", -17 },
  { 0x5e654320, 10209, 15236, 0, 0,  2, "IJG JPEG Library", "not baseline 18", -14 },
  { 0x5fc0115c, 10843, 16182, 0, 0,  2, "IJG JPEG Library", "not baseline 17", -11 },
  { 0x5d8b8e7b, 11505, 17183, 0, 0,  2, "IJG JPEG Library", "not baseline 16", -7 },
  { 0x63f8b8c1, 12279, 18351, 0, 0,  2, "IJG JPEG Library", "not baseline 15", -5 },
  { 0x675ecd7a, 13166, 19633, 0, 0,  2, "IJG JPEG Library", "not baseline 14", 0 },
  { 0x7a65d374, 14160, 21129, 0, 0,  2, "IJG JPEG Library", "not baseline 13", 0 },
  { 0xf5d0af6a, 15344, 22911, 0, 0,  2, "IJG JPEG Library", "not baseline 12", 0 },
  { 0x0227aaf0, 16748, 24969, 0, 0,  2, "IJG JPEG Library", "not baseline 11", 0 },
  { 0xffd2d3c8, 18440, 27525, 0, 0,  2, "IJG JPEG Library", "not baseline 10", 0 },
  { 0x27f48623, 20471, 30529, 0, 0,  2, "IJG JPEG Library", "not baseline 9", 0 },
  { 0xff1fab81, 23056, 34422, 0, 0,  2, "IJG JPEG Library", "not baseline 8", 0 },
  { 0xcfeac62b, 26334, 39314, 0, 0,  2, "IJG JPEG Library", "not baseline 7", 0 },
  { 0x4a8e947e, 30719, 45876, 0, 0,  2, "IJG JPEG Library", "not baseline 6", 0 },
  { 0xe668af85, 36880, 55050, 0, 0,  2, "IJG JPEG Library", "not baseline 5", 0 },
  { 0x6d4b1215, 46114, 68840, 0, 0,  2, "IJG JPEG Library", "not baseline 4", 0 },
  { 0xf2734901, 61445, 91697, 0, 0,  2, "IJG JPEG Library", "not baseline 3", 0 },
  { 0x9a2a42bc, 92200, 137625, 0, 0,  2, "IJG JPEG Library", "not baseline 2", 0 },
  { 0x1b178d6d, 184400, 275250, 0, 0,  2, "IJG JPEG Library", "not baseline 1", 0 },
  { 0x1b178d6d, 184400, 275250, 0, 0,  2, "IJG JPEG Library", "not baseline 0", 0 },

  /* FIXME: the following entries are incomplete and need to be verified */

  { 0x31258383,   319,   665, 2, 1, -2, "ACD ?", "?", -94 },
  { 0x91d018a3,   436,   996, 2, 1, -2, "ACD ?", "?", -92 },
  { 0x954ee70e,   664,  1499, 2, 1, -2, "ACD ?", "?", -88 },
  { 0xe351bb55,  1590,  3556, 2, 2, -2, "ACD ?", "?", -71 },
  { 0x5a81e2c0,    95,   166, 1, 1,  2, "Adobe Photoshop CS2", "quality 12", -98 },
  { 0xcd0d41ae,   232,   443, 1, 1,  2, "Adobe Photoshop CS2", "quality 11", -96 },
  { 0x1b141cb3,   406,   722, 1, 1,  2, "Adobe Photoshop CS2", "quality 10", -93 },
  { 0xc84c0187,   539,   801, 1, 1,  2, "Adobe Photoshop CS2", "quality 9", -92 },
  { 0x1e822409,   649,   853, 1, 1,  2, "Adobe Photoshop CS2", "quality 8", -91 },
  { 0x3104202b,   786,   926, 1, 1,  2, "Adobe Photoshop CS2", "quality 7", -90 },
  { 0xcd21f666,   717,   782, 2, 2,  2, "Adobe Photoshop CS2", "quality 6", -91 },
  { 0x1b74e018,   844,   849, 2, 2,  2, "Adobe Photoshop CS2", "quality 5", -90 },
  { 0xde39ed89,   962,   892, 2, 2,  2, "Adobe Photoshop CS2", "quality 4", -89 },
  { 0xbdef8414,  1068,   941, 2, 2,  2, "Adobe Photoshop CS2", "quality 3", -89 },
  { 0xfedf6432,  1281,   998, 2, 2,  2, "Adobe Photoshop CS2", "quality 2", -87 },
  { 0x5d6afd92,  1484,  1083, 2, 2,  2, "Adobe Photoshop CS2", "quality 1", -86 },
  { 0x4c7d2f7d,  1582,  1108, 2, 2,  2, "Adobe Photoshop CS2", "quality 0", -85 },
  { 0x68e798b2,    95,   168, 1, 1,  2, "Adobe Photoshop CS2", "save for web 100", -98 },
  { 0x9f3456f2,   234,   445, 1, 1,  2, "Adobe Photoshop CS2", "save for web 90", -96 },
  { 0xda807dd5,   406,   724, 1, 1,  2, "Adobe Photoshop CS2", "save for web 80", -93 },
  { 0xf70a37ce,   646,  1149, 1, 1,  2, "Adobe Photoshop CS2", "save for web 70", -90 },
  { 0xf36979d2,   974,  1769, 1, 1,  2, "Adobe Photoshop CS2", "save for web 60", -85 },
  { 0x4966f484,  1221,  1348, 2, 2,  2, "Adobe Photoshop CS2", "save for web 50", -86 },
  { 0xaddf6d45,  1821,  1997, 2, 2,  2, "Adobe Photoshop CS2", "save for web 40", -79 },
  { 0xeffa362a,  2223,  2464, 2, 2,  2, "Adobe Photoshop CS2", "save for web 30", -74 },
  { 0x7aa980c1,  2575,  2903, 2, 2,  2, "Adobe Photoshop CS2", "save for web 20", -70 },
  { 0x489e344f,  3514,  3738, 2, 2,  2, "Adobe Photoshop CS2", "save for web 10", -60 },
  { 0x1a2cffe0,   535,   750, 1, 1,  2, "Adobe Photoshop 7.0", "quality 10", -93 },
  { 0x1e96d5d3,   109,   171, 1, 1,  2, "Adobe Photoshop CS", "quality 12", -98 },
  { 0x6771042c,   303,   466, 1, 1,  2, "Adobe Photoshop CS, Camera Raw 3", "quality 11", -95 },
  { 0xd4553f25,   668,   830, 1, 1,  2, "Adobe Photoshop 7.0, CS", "quality 9", -91 },
  { 0xd3b24cb4,   794,   895, 1, 1,  2, "Adobe Photoshop 7.0, CS", "quality 8", -90 },
  { 0x4ad5990c,   971,   950, 1, 1,  2, "Adobe Photoshop CS", "quality 7", -89 },
  { 0x4293dfde,   884,   831, 2, 2,  2, "Adobe Photoshop CS", "quality 6", -90 },
  { 0xba0212ec,  1032,   889, 2, 2,  2, "Adobe Photoshop CS", "quality 5", -89 },
  { 0x4b50947d,  1126,   940, 2, 2,  2, "Adobe Photoshop CS", "quality 4", -88 },
  { 0xad0f8e5c,  1216,   977, 2, 2,  2, "Adobe Photoshop CS", "quality 3", -88 },
  { 0x560b5f0c,   339,   670, 1, 1,  2, "Adobe Photoshop ?", "save for web 85", -94 },
  { 0x9539b14b,   427,   613, 2, 2,  2, "Adobe Photoshop ?", "?", -94 },
  { 0x841f2655,   525,   941, 1, 1,  2, "Adobe Photoshop ?", "save for web 75", -92 },
  { 0xaa2161e2,   803,  1428, 1, 1,  2, "Adobe Photoshop ?", "save for web 65", -87 },
  { 0x743feb84,  1085,  1996, 1, 1,  2, "Adobe Photoshop ?", "save for web 55", -83 },
  { 0xe9f14743,  1156,  2116, 1, 1,  2, "Adobe Photoshop ?", "save for web 52", -82 },
  { 0x1003c8fb,  1175,  2169, 1, 1,  2, "Adobe Photoshop ?", "save for web 51", -81 },
  { 0xd7804c45,  2272,  2522, 2, 2,  2, "Adobe Photoshop ?", "save for web 29", -73 },
  { 0xcb5aa8ad,  2515,  2831, 2, 2,  2, "Adobe ImageReady", "save for web 22", -70 },
  { 0x956d2a00,  3822,  3975, 2, 2,  2, "Adobe ImageReady", "save for web 6", -57 },
  { 0xba53e0c5,  4028,  4174, 2, 2,  2, "Adobe Photoshop ?", "save for web 3", -55 },
  { 0x13c0c8bc,   513,     0, 1, 1,  1, "Adobe Photoshop ?", "?", -93 },
  { 0x3fad5c43,   255,   393, 2, 1,  2, "Apple Quicktime 7.1 or 7.2", "?", -96 },
  { 0x6529bd03,   513,   775, 2, 2,  2, "Apple Quicktime 7.2", "?", -93 },
  { 0x354e610a,   543,   784, 2, 1, -2, "Apple Quicktime 7.1", "?", -92 },
  { 0xd596795e,   361,   506, 2, 1,  2, "Apple ?", "?", -95 },
  { 0x74da8ba7,  1511,  2229, 2, 2,  2, "Apple ?", "?", -79 },
  { 0x6391ca2b,   188,   276, 2, 1, -2, "Canon EOS 300D, 350D or 400D", "Fine", -97 },
  { 0x00474eb0,   708,  1057, 2, 1, -2, "Canon EOS 10D", "Normal", -90 },
  { 0x535174bd,   533,  1325, 2, 1, -2, "Canon Digital Ixus v2", "Fine", -92 },
  { 0x535174bd,   533,  1325, 2, 1, -2, "Canon PowerShot A95, S1, S2, SD400 or SD630", "Fine", -89 },
  { 0xb7be6b97,   192,   556, 2, 1, -2, "Canon PowerShot S5 IS, A300, A430, S200, SD500, SD700, Ixus 700 or 800", "Superfine", -95 },
  { 0xb5b5c61d,   533,  1325, 1, 2, -2, "Canon Digital Ixus 400", "Fine", -89 },
  { 0xa7a2c471,   288,   443, 2, 1, -3, "FujiFilm MX-2700", "?", -96 },
  { 0x8db061f0,   389,   560, 2, 1, -3, "FujiFilm FinePix S700", "Fine", -94 },
  { 0xbb7b97ba,   515,   774, 2, 1, -3, "FujiFilm FinePix 2600 Zoom", "Fine", -93 },
  { 0x71bcdf92,   167,   240, 2, 2, -3, "HP PhotoSmart C850, C935", "?", -97 },
  { 0x9542cc81,  1970,  1970, 2, 2, -3, "HP PhotoSmart C812", "?", -78 },
  { 0xdb7b71d8,   369,   558, 2, 1, -3, "Kodak P880", "?", 95 },
  { 0x82e461f8,   566,   583, 2, 2, -2, "Kodak V610", "Fine", -93 },
  { 0x17816eb1,   736,  1110, 2, 2, -2, "Kodak DC240", "?", 90 },
  { 0x17816eb1,   736,  1110, 1, 1, -2, "Kodak Imaging", "High (high res.)", 90 },
  { 0x17816eb1,   736,  1110, 2, 1, -2, "Kodak Imaging", "High (medium res.)", 90 },
  { 0x17816eb1,   736,  1110, 4, 1, -2, "Kodak Imaging", "High (low res.)", 90 },
  { 0x3841f91b,   736,     0, 1, 1,  1, "Kodak Imaging", "High (grayscale)", 90 },
  { 0xe0f48a64,  3688,  5505, 1, 1, -2, "Kodak Imaging", "Medium (high res.)", 50 },
  { 0xe0f48a64,  3688,  5505, 2, 1, -2, "Kodak Imaging", "Medium (medium res.)", 50 },
  { 0xe0f48a64,  3688,  5505, 4, 1, -2, "Kodak Imaging", "Medium (low res.)", 50 },
  { 0x9ebccf53,  3688,     0, 1, 1,  1, "Kodak Imaging", "Medium (grayscale)", 50 },
  { 0xa130f919,  9056, 13790, 1, 1, -2, "Kodak Imaging", "Low (high res.)", 20 },
  { 0xa130f919,  9056, 13790, 2, 1, -2, "Kodak Imaging", "Low (medium res.)", 20 },
  { 0xa130f919,  9056, 13790, 4, 1, -2, "Kodak Imaging", "Low (low res.)", 20 },
  { 0x34216b8b,  9056,     0, 1, 1,  1, "Kodak Imaging", "Low (grayscale)", 20 },
  { 0x403b528f,   161,   179, 1, 1, -2, "Lead ?", "?", -98 },
  { 0x8550a881,   711,  1055, 1, 1, -2, "Lead ?", "?", -90 },
  { 0x98fb09fc,  1079,  1610, 1, 1, -2, "Lead ?", "?", -85 },
  { 0xfbb88fb8,  2031,  3054, 1, 1, -2, "Lead ?", "?", -72 },
  { 0x5fa57f78,  4835,  7226, 1, 1, -2, "Lead ?", "?", -37 },
  { 0x85b97881,  8199, 12287, 1, 1, -2, "Lead ?", "?", -22 },
  { 0xd3cd4ad0,    96,   117, 2, 1, -2, "Leica Digilux 3", "?", -98 },
  { 0x29e095c5,   221,   333, 2, 1, -2, "Leica M8", "?", 97 },
  { 0xee344795,   582,   836, 2, 1, -2, "Medion ?", "?", -92 },
  { 0x991408d7,   433,   667, 2, 1, -2, "Medion ?", "?", -94 },
  { 0x10b035e9,  1858,  2780, 2, 2,  2, "Microsoft Office", "Default", 75 },
  { 0x20fcfcb8,   116,   169, 2, 1, -2, "Nikon D50, D70, D70s, D80", "Fine", -98 },
  { 0x2530fec2,   218,   333, 2, 1, -2, "Nikon D70 or D70s", "Normal", -97 },
  { 0xe5dbee70,   616,   941, 2, 1, -2, "Nikon D70 or D70s", "Basic", -91 },
  { 0x0e082d61,   671,   999, 2, 1, -2, "Nikon D70 or D70s", "Basic + raw", -90 },
  { 0xcc6c9703,   127,   169, 2, 1, -2, "Nikon D70 v1.0", "Fine", -98 },
  { 0x8cdfa365,   302,   444, 2, 1, -2, "Nikon D70 v1.0", "Fine", -95 },
  { 0x23246639,   315,   499, 2, 1, -2, "Nikon D70 v1.0", "Fine", -95 },
  { 0x978378a8,   329,   500, 2, 1, -2, "Nikon D70 v1.0", "Fine", -95 },
  { 0x748a8379,   346,   500, 2, 1, -2, "Nikon D70 v1.0", "Fine", -95 },
  { 0xa85255cd,   372,   558, 2, 1, -2, "Nikon D70 v1.0", "Fine", -94 },
  { 0x016406e0,   389,   560, 2, 1, -2, "Nikon D70 v1.0", "Fine", -94 },
  { 0xda3a50f1,   419,   611, 2, 1, -2, "Nikon D70 v1.0", "Fine", -94 },
  { 0xd8e45108,   449,   668, 2, 1, -2, "Nikon D70 v1.0", "Fine", -93 },
  { 0x8a62bf3c,   506,   775, 2, 1, -2, "Nikon D70 v1.0", "Fine", -93 },
  { 0xc3108c99,   529,   781, 2, 1, -2, "Nikon D70 v1.0", "Fine", -92 },
  { 0xeabc51a5,   261,   389, 2, 1, -2, "Nikon D50", "Normal", -96 },
  { 0x0cddf617,   345,   499, 2, 1, -2, "Nikon D50", "Normal", -95 },
  { 0x2b3b6401,   855,  1279, 2, 1, -2, "Nikon D40", "?", -88 },
  { 0x5d1ca944,   667,   999, 2, 1, -3, "Nikon E4300", "Normal", 91 },
  { 0xabcbdc47,   736,  1110, 2, 1, -3, "Nikon E4300", "Normal", 90 },
  { 0x10b2ad77,   884,  1332, 2, 1, -3, "Nikon E4300", "Normal", 88 },
  { 0x0a82648b,    64,    64, 1, 1, -2, "Nikon Browser 6", "High quality", 100 },
  { 0xb091eaf2,   779,  1164, 1, 1, -2, "Nikon Browser 6 or PictureProject 1.6", "Standard quality", -89 },
  { 0x1a856066,  1697,  2554, 2, 1, -2, "Nikon Browser 6", "Standard eq", -76 },
  { 0xdf0774bd,  2746,  5112, 2, 2, -2, "Nikon Browser 6", "Standard compression", -57 },
  { 0xe2fd6fb9,  8024, 12006, 2, 2, -2, "Nikon Browser 6", "Maximum compression", -22 },
  { 0x17816eb1,   736,  1110, 2, 2, -2, "Olympus Camedia Master", "High quality?", 90 },
  { 0x96129a0d,  1477,  2221, 2, 2, -2, "Olympus u710,S710", "Super high quality?", 80 },
  { 0x824f84b9,   437,   617, 2, 1, -2, "Olympus u30D,S410D,u410D", "High quality", -94 },
  { 0x1b050d58,   447,   670, 2, 1, -2, "Olympus u30D,S410D,u410D", "High quality", -93 },
  { 0x1b050d58,   447,   670, 2, 1, -2, "Olympus u30D,S410D,u410D", "High quality", -93 },
  { 0x68058c37,   814,  1223, 2, 1, -3, "Olympus C960Z,D460Z", "Standard quality", 89 },
  { 0x10b2ad77,   884,  1332, 2, 1, -3, "Olympus C211Z", "Standard quality", 88 },
  { 0x0f5fa4cb,  1552,  2336, 2, 1, -3, "Olympus C990Z,D490Z", "High quality", 79 },
  { 0xf51554a8,   261,   392, 2, 1, -2, "Panasonic DMC-FZ5", "High", -96 },
  { 0xf01efe6e,   251,   392, 2, 1, -2, "Panasonic DMC-FZ30", "High", -96 },
  { 0x08064360,   280,   445, 2, 1, -2, "Panasonic DMC-FZ30", "High", -96 },
  { 0x05831bbb,   304,   448, 2, 1, -2, "Panasonic DMC-FZ30", "High", -95 },
  { 0xe6c08bea,   316,   499, 2, 1, -2, "Panasonic DMC-FZ30", "High", -95 },
  { 0xcb5f5f7d,   332,   550, 2, 1, -2, "Panasonic DMC-FZ30", "High", -95 },
  { 0xb53cf359,   355,   555, 2, 1, -2, "Panasonic DMC-FZ30", "High", -95 },
  { 0xdbcd2690,   375,   606, 2, 1, -2, "Panasonic DMC-FZ30", "High", -94 },
  { 0x594a3212,   400,   615, 2, 1, -2, "Panasonic DMC-FZ30", "High", -94 },
  { 0xde23f16a,   420,   667, 2, 1, -2, "Panasonic DMC-FZ30", "High", -94 },
  { 0xc0a43b37,   501,   775, 2, 1, -2, "Panasonic DMC-FZ30", "High", -93 },
  { 0xc298e887,   577,   891, 2, 1, -2, "Panasonic DMC-FZ30", "High", -92 },
  { 0x039b6bc2,   324,   499, 2, 1,  2, "Ricoh Caplio R1V", "?", -95 },
  { 0xf60dc348,   274,   443, 2, 1,  2, "Roxio PhotoSuite", "?", -96 },
  { 0xc6f47fa4,   634,   943, 2, 1, -2, "Samsung Digimax V3", "?", -91 },
  { 0xb9284f39,  1313,  1997, 2, 1, -2, "Samsung Digimax V3", "?", -82 },
  { 0x5dedca50,   218,   331, 2, 1, -2, "Samsung Digimax S600", "?", -97 },
  { 0x095451e2,   258,   389, 2, 1, -2, "Sony Cybershot", "?", -96 },
  { 0x4d981764,    86,   115, 2, 1, -2, "Sony DSC-W55", "Fine", 99 },
  { 0x6d2b20ce,   122,   169, 2, 1, -2, "Sony DSC-F828, DSC-F88", "?", -98 },
  { 0x29e095c5,   221,   333, 2, 1, -2, "Sony DSC-W30, DSC-W50, DSC-H2, DSC-H5", "?", 97 },
  { 0x59e5c5bc,   518,   779, 2, 1, -2, "Sony DSC-W70", "?", 93 },
  { 0x96129a0d,  1477,  2221, 2, 2, -2, "Sony DSC-W30, DSC-P43, DSC-S600", "?", 80 },
  { 0xa4d9a6d9,   324,   682, 2, 1, -2, "Sony DSLR-A100", "?", -94 },
  { 0x17816eb1,   736,  1110, 2, 1, -2, "SonyEricsson K750i", "Fine", 90 },
  { 0x10b035e9,  1858,  2780, 2, 1, -2, "SonyEricsson K750i or W200i", "Normal", 75 },
  { 0x1b0ad9d5,   836,  1094, 2, 2, -2, "SonyEricsson K750i", "Panorama fine", -89 },
  { 0x1cd8bb9f,  1672,  2188, 2, 2, -2, "SonyEricsson K750i", "Panorama normal", -79 },
  { 0x81d174af,   361,   555, 2, 1, -2, "SonyEricsson K750i", "?", -95 },
  { 0x991408d7,   433,   667, 2, 1, -2, "SonyEricsson K750i", "?", -94 },
  { 0x00034978,   954,  1443, 2, 1, -2, "SonyEricsson K750i", "?", -87 },
  { 0xd27667ab,  1024,  1504, 2, 1, -2, "SonyEricsson K750i", "?", -86 },
  { 0x94e96153,  1097,  1615, 2, 1, -2, "SonyEricsson K750i", "?", -85 },
  { 0xf524688a,  1168,  1727, 2, 1, -2, "SonyEricsson K750i", "?", -84 },
  { 0x5e5e4237,  1324,  2000, 2, 1, -2, "SonyEricsson K750i", "?", -82 },
  { 0x2e94a836,  1473,  2170, 2, 1, -2, "SonyEricsson K750i", "?", -80 },
  { 0xdd957ed4,  1615,  2394, 2, 1, -2, "SonyEricsson K750i", "?", -78 },
  { 0x4147561e,  1759,  2612, 2, 1, -2, "SonyEricsson K750i", "?", -76 },
  { 0x6f5af2b1,  1491,  1491, 2, 1, -2, "SonyEricsson Z600", "Default", -83 },
  { 0x641ee862,  2211,  3284, 2, 1, -2, "Trust 760 Powerc@m", "?", 70 },
  { 0x0bd95282,  2211,  3284, 1, 2, -2, "Trust 760 Powerc@m", "?", 70 },
  { 0xe9814c86,  1830,  2725, 1, 1,  2, "Xing VT-Compress", "?", -75 },
};

typedef struct
{
  guint32  hashval;
  gint     subsmp_h;
  gint     subsmp_v;
  gint     num_quant_tables;
  gint     ijg_qual;
  GSList  *files;
  guint16  luminance[DCTSIZE2];
  guint16  chrominance[DCTSIZE2];
} QuantTableData;

static GSList *found_tables = NULL;

#if 0 /* FIXME ---v-v-v---------------------------------------------v-v-v--- */

static guint16 **ijg_luminance       = NULL; /* luminance, baseline */
static guint16 **ijg_chrominance     = NULL; /* chrominance, baseline */
static guint16 **ijg_luminance_nb    = NULL; /* luminance, not baseline */
static guint16 **ijg_chrominance_nb  = NULL; /* chrominance, not baseline */

/*
 * Initialize the IJG quantization tables for each quality setting.
 */
static void
init_ijg_tables (void)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  gint                        q, i;

  ijg_luminance = g_new (guint16 *, 101);
  ijg_chrominance = g_new (guint16 *, 101);
  for (q = 0; q <= 100; q++)
    {
      ijg_luminance[q] = g_new (guint16, DCTSIZE2);
      ijg_chrominance[q] = g_new (guint16, DCTSIZE2);
    }

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  for (q = 0; q <= 100; q++)
    {
      jpeg_set_quality (&cinfo, q, TRUE);
      for (i = 0; i < DCTSIZE2; i++)
        ijg_luminance[q][i] = cinfo.quant_tbl_ptrs[0]->quantval[i];
      for (i = 0; i < DCTSIZE2; i++)
        ijg_chrominance[q][i] = cinfo.quant_tbl_ptrs[1]->quantval[i];
    }
  for (q = 0; q <= 100; q++)
    {
      jpeg_set_quality (&cinfo, q, FALSE);
      for (i = 0; i < DCTSIZE2; i++)
        ijg_luminance_nb[q][i] = cinfo.quant_tbl_ptrs[0]->quantval[i];
      for (i = 0; i < DCTSIZE2; i++)
        ijg_chrominance_nb[q][i] = cinfo.quant_tbl_ptrs[1]->quantval[i];
    }
  jpeg_destroy_compress (&cinfo);
}

/*
 * Check if two quantization tables are identical.
 */
static gboolean
compare_tables (const guint16 *quant_table1,
                const guint16 *quant_table2)
{
  gint i;

  g_return_val_if_fail (quant_table1 != NULL, FALSE);
  g_return_val_if_fail (quant_table2 != NULL, FALSE);

  for (i = 0; i < DCTSIZE2; i++)
    if (quant_table1[i] != quant_table2[i])
      return FALSE;
  return TRUE;
}

#endif /* FIXME ---^-^-^--------------------------------------------^-^-^--- */

/*
 * Trivial hash function (simple, but good enough for 1 to 4 * 64 short ints).
 */
static guint32
hash_quant_tables (struct jpeg_decompress_struct *cinfo)
{
  guint32 hashval;
  gint    t;
  gint    i;

  hashval = 11;
  for (t = 0; t < 4; t++)
    if (cinfo->quant_tbl_ptrs[t])
      for (i = 0; i < DCTSIZE2; i++)
        hashval = hashval * 4177 + cinfo->quant_tbl_ptrs[t]->quantval[i];
  return hashval;
}

static guint32
hash_transposed_quant_tables (struct jpeg_decompress_struct *cinfo)
{
  guint32 hashval;
  gint    t;
  gint    i;
  gint    j;

  hashval = 11;
  for (t = 0; t < 4; t++)
    if (cinfo->quant_tbl_ptrs[t])
      for (i = 0; i < DCTSIZE; i++)
        for (j = 0; j < DCTSIZE; j++)
          hashval = hashval * 4177 + cinfo->quant_tbl_ptrs[t]->quantval[j * 8
                                                                        + i];
  return hashval;
}

static void
add_unknown_table (struct jpeg_decompress_struct *cinfo,
                   gchar                         *filename)
{
  guint32         hashval;
  GSList         *list;
  QuantTableData *table_data;
  gint            num_quant_tables;
  gint            t;
  gint            i;

  hashval = hash_quant_tables (cinfo);

  /* linear search - the number of unknown tables is usually very small */
  for (list = found_tables; list; list = list->next)
    {
      table_data = list->data;
      if (table_data->hashval == hashval
          && table_data->subsmp_h == cinfo->comp_info[0].h_samp_factor
          && table_data->subsmp_v == cinfo->comp_info[0].v_samp_factor)
        {
          /* this unknown table has already been seen in previous files */
          table_data->files = g_slist_prepend (table_data->files, filename);
          return;
        }
    }

  /* not found => new table */
  table_data = g_new (QuantTableData, 1);
  table_data->hashval = hashval;
  table_data->subsmp_h = cinfo->comp_info[0].h_samp_factor;
  table_data->subsmp_v = cinfo->comp_info[0].v_samp_factor;

  num_quant_tables = 0;
  for (t = 0; t < 4; t++)
    if (cinfo->quant_tbl_ptrs[t])
      num_quant_tables++;

  table_data->num_quant_tables = num_quant_tables;
  table_data->ijg_qual = jpeg_detect_quality (cinfo);
  table_data->files = g_slist_prepend (NULL, filename);

  if (cinfo->quant_tbl_ptrs[0])
    {
      for (i = 0; i < DCTSIZE2; i++)
        table_data->luminance[i] = cinfo->quant_tbl_ptrs[0]->quantval[i];
    }
  else
    {
      for (i = 0; i < DCTSIZE2; i++)
        table_data->luminance[i] = 0;
    }

  if (cinfo->quant_tbl_ptrs[1])
    {
      for (i = 0; i < DCTSIZE2; i++)
        table_data->chrominance[i] = cinfo->quant_tbl_ptrs[1]->quantval[i];
    }
  else
    {
      for (i = 0; i < DCTSIZE2; i++)
        table_data->chrominance[i] = 0;
    }

  found_tables = g_slist_prepend (found_tables, table_data);
}

/*
 * Analyze the JPEG quantization tables and return a list of devices or
 * software that can generate the same tables and subsampling factors.
 */
static GSList *
detect_source (struct jpeg_decompress_struct *cinfo,
               gint                           num_quant_tables)
{
  guint    lum_sum;
  guint    chrom_sum;
  gint     i;
  GSList  *source_list;

  /* compute sum of luminance and chrominance quantization tables */
  lum_sum = 0;
  chrom_sum = 0;
  if (cinfo->quant_tbl_ptrs[0])
    {
      for (i = 0; i < DCTSIZE2; i++)
        lum_sum += cinfo->quant_tbl_ptrs[0]->quantval[i];
    }
  if (cinfo->quant_tbl_ptrs[1])
    {
      for (i = 0; i < DCTSIZE2; i++)
        chrom_sum += cinfo->quant_tbl_ptrs[1]->quantval[i];
    }

  /* there can be more than one match (if sampling factors are compatible) */
  source_list = NULL;
  if (chrom_sum == 0 && num_quant_tables == 1)
    {
      /* grayscale */
      for (i = 0; i < G_N_ELEMENTS (quant_info); i++)
        {
          if (quant_info[i].lum_sum == lum_sum
              && (quant_info[i].subsmp_h == 0
                  || quant_info[i].subsmp_h
                  == cinfo->comp_info[0].h_samp_factor)
              && (quant_info[i].subsmp_v == 0
                  || quant_info[i].subsmp_v
                  == cinfo->comp_info[0].v_samp_factor)
              && quant_info[i].num_quant_tables > 0)
            {
              source_list = g_slist_append (source_list,
                                            (gpointer) (quant_info + i));
            }
        }
    }
  else
    {
      /* RGB and other color spaces */
      for (i = 0; i < G_N_ELEMENTS (quant_info); i++)
        {
          if (quant_info[i].lum_sum == lum_sum
              && quant_info[i].chrom_sum == chrom_sum
              && (quant_info[i].subsmp_h == 0
                  || quant_info[i].subsmp_h
                  == cinfo->comp_info[0].h_samp_factor)
              && (quant_info[i].subsmp_v == 0
                  || quant_info[i].subsmp_v
                  == cinfo->comp_info[0].v_samp_factor)
              && (quant_info[i].num_quant_tables == num_quant_tables
                  || quant_info[i].num_quant_tables == -num_quant_tables))
            {
              source_list = g_slist_append (source_list,
                                            (gpointer) (quant_info + i));
            }
        }
    }

  return source_list;
}

/*
 * ... FIXME: docs
 */
static void
print_summary (struct jpeg_decompress_struct *cinfo,
               gint                           num_quant_tables)
{
  gint    quality;
  gint    i;
  GSList *source_list;

  /* detect JPEG quality - test the formula used in the jpeg plug-in */
  quality = jpeg_detect_quality (cinfo);
  if (quality > 0)
    g_print ("\tQuality:  %02d (exact)\n", quality);
  else if (quality < 0)
    g_print ("\tQuality:  %02d (approx)\n", -quality);
  else
    g_print ("\tQuality:  unknown\n");

  /* JPEG sampling factors */
  g_print ("\tSampling: %dx%d",
           cinfo->comp_info[0].h_samp_factor,
           cinfo->comp_info[0].v_samp_factor);
  if ((cinfo->num_components > 1 && cinfo->num_components != 3)
      || cinfo->comp_info[1].h_samp_factor != 1
      || cinfo->comp_info[1].v_samp_factor != 1
      || cinfo->comp_info[2].h_samp_factor != 1
      || cinfo->comp_info[2].v_samp_factor != 1)
    {
      for (i = 1; i < cinfo->num_components; i++)
        g_print (",%dx%d",
                 cinfo->comp_info[i].h_samp_factor,
                 cinfo->comp_info[i].v_samp_factor);
    }
  g_print ("\n");

  /* Number of quantization tables */
  g_print ("\tQ.tables: %d\n", num_quant_tables);

  source_list = detect_source (cinfo, num_quant_tables);
  if (source_list)
    {
      GSList  *l;
      guint32  hash;
      guint32  hash_t;

      hash = hash_quant_tables (cinfo);
      hash_t = hash_transposed_quant_tables (cinfo);

      for (l = source_list; l; l = l->next)
        {
          QuantInfo   *source_info = l->data;
          const gchar *comment = "";

          if (source_info->hash == hash)
            comment = "";
          else if (source_info->hash == hash_t)
            comment = " (rotated)";
          else if (num_quant_tables == 1)
            comment = " (grayscale)";
          else
            comment = " (FALSE MATCH)";

          g_print ("\tSource:   %s - %s%s\n",
                   source_info->source_name,
                   source_info->setting_name,
                   comment);
        }
      g_slist_free (source_list);
    }
  else
    g_print ("\tSource:   unknown\n");
}

/*
 * Print a quantization table as a C array.
 */
static void
print_ctable (gint           table_id,
              const guint16 *quant_table,
              gboolean       more)
{
  gint i;

  g_return_if_fail (quant_table != NULL);
  if (table_id >= 0)
    g_print ("    {  /* table %d */\n      ", table_id);
  else
    g_print ("    {\n      ");
  for (i = 0; i < DCTSIZE2; i++)
    {
      if (i == DCTSIZE2 - 1)
        g_print ("%3d\n", quant_table[i]);
      else if ((i + 1) % DCTSIZE == 0)
        g_print ("%3d,\n      ", quant_table[i]);
      else
        g_print ("%3d, ", quant_table[i]);
    }
  if (more)
    g_print ("    },\n");
  else
    g_print ("    }\n");
}

/*
 * Print one or two quantization tables, two columns.
 */
static void
print_table_2cols (gint           table1_id,
                   const guint16 *quant_table1,
                   gint           table2_id,
                   const guint16 *quant_table2)
{
  gint i;
  gint j;

  if (quant_table2)
    g_print ("\tQuantization table %d:              Quantization table %d:\n\t",
             table1_id, table2_id);
  else
    g_print ("\tQuantization table %d:\n\t", table1_id);
  for (i = 0; i < DCTSIZE; i++)
    {
      if (quant_table1)
        {
          for (j = 0; j < DCTSIZE; j++)
            {
              if (j != DCTSIZE - 1)
                g_print ("%3d ", quant_table1[i * DCTSIZE + j]);
              else
                {
                  if (quant_table2)
                    g_print ("%3d  | ", quant_table1[i * DCTSIZE + j]);
                  else if (i != DCTSIZE - 1)
                    g_print ("%3d\n\t", quant_table1[i * DCTSIZE + j]);
                  else
                    g_print ("%3d\n", quant_table1[i * DCTSIZE + j]);
                }
            }
        }
      else
        {
          g_print ("                                 | ");
        }
      if (quant_table2)
        {
          for (j = 0; j < DCTSIZE; j++)
            {
              if (j != DCTSIZE - 1)
                g_print ("%3d ", quant_table2[i * DCTSIZE + j]);
              else if (i != DCTSIZE - 1)
                g_print ("%3d\n\t", quant_table2[i * DCTSIZE + j]);
              else
                g_print ("%3d\n", quant_table2[i * DCTSIZE + j]);
            }
        }
    }
}

/*
 * Error handling as in the IJG libjpeg example.
 */
typedef struct my_error_mgr
{
  struct jpeg_error_mgr pub;            /* "public" fields */
#ifdef __ia64__
  long double           dummy;          /* bug #138357 */
#endif
  jmp_buf               setjmp_buffer;  /* for return to caller */
} *my_error_ptr;

static void
my_error_exit (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  (*cinfo->err->output_message) (cinfo);
  longjmp (myerr->setjmp_buffer, 1);
}

/*
 * Analyze a JPEG file according to the command-line options.
 */
static gboolean
analyze_file (gchar *filename)
{
  struct jpeg_decompress_struct  cinfo;
  struct my_error_mgr            jerr;
  FILE                          *f;
  gint                           i;
  gint                           num_quant_tables;
  GSList                        *source_list;

  if ((f = g_fopen (filename, "rb")) == NULL)
    {
      g_printerr ("Cannot open '%s'\n", filename);
      return FALSE;
    }

  if (option_summary)
    g_print ("%s:\n", filename);

  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp (jerr.setjmp_buffer))
    {
      /* if we get here, the JPEG code has signaled an error. */
      jpeg_destroy_decompress (&cinfo);
      fclose (f);
      return FALSE;
    }
  jpeg_create_decompress (&cinfo);

  jpeg_stdio_src (&cinfo, f);

  jpeg_read_header (&cinfo, TRUE);

  num_quant_tables = 0;
  for (i = 0; i < 4; i++)
    if (cinfo.quant_tbl_ptrs[i])
      num_quant_tables++;

  source_list = detect_source (&cinfo, num_quant_tables);
  if (! source_list)
    {
      add_unknown_table (&cinfo, filename);
    }

  if (! option_unknown)
    {
      if (option_summary)
        print_summary (&cinfo, num_quant_tables);

      if (option_ctable)
        {
          g_print ("  {\n    /* %s */\n    \"?\", \"?\",\n    %d, %d,\n    %d,\n",
                   filename,
                   cinfo.comp_info[0].h_samp_factor,
                   cinfo.comp_info[0].v_samp_factor,
                   num_quant_tables);
          for (i = 0; i < 4; i++)
            if (cinfo.quant_tbl_ptrs[i])
              print_ctable (i, cinfo.quant_tbl_ptrs[i]->quantval,
                            (i < 3) && cinfo.quant_tbl_ptrs[i + 1]);
          g_print ("  },\n");
        }

      if (option_table_2cols)
        {
          print_table_2cols (0, cinfo.quant_tbl_ptrs[0]->quantval,
                             1, cinfo.quant_tbl_ptrs[1]->quantval);
          if (cinfo.quant_tbl_ptrs[2] || cinfo.quant_tbl_ptrs[3])
            print_table_2cols (2, cinfo.quant_tbl_ptrs[2]->quantval,
                               3, cinfo.quant_tbl_ptrs[3]->quantval);
        }
    }

  if (source_list)
    g_slist_free (source_list);

  jpeg_destroy_decompress (&cinfo);
  fclose (f);

  return TRUE;
}

/*
 * ... FIXME: docs
 */
static void
print_unknown_tables (void)
{
  GSList         *list;
  GSList         *flist;
  QuantTableData *table_data;
  gint            num_files;
  gint            total_files = 0;

  for (list = found_tables; list; list = list->next)
    {
      table_data = list->data;

      if (option_ctable)
        {
          g_print ("  {\n");
          num_files = 0;
          for (flist = table_data->files; flist; flist = flist->next)
            {
              g_print("    /* %s */\n", (gchar *)(flist->data));
              num_files++;
            }

          { /* FIXME */
            guint    lum_sum;
            guint    chrom_sum;
            gint     i;

            total_files += num_files;
            lum_sum = 0;
            chrom_sum = 0;
            for (i = 0; i < DCTSIZE2; i++)
              lum_sum += table_data->luminance[i];
            for (i = 0; i < DCTSIZE2; i++)
              chrom_sum += table_data->chrominance[i];
            g_print ("    /* hash 0x%x, IJG %d, lum %d, chrom %d, files: %d */\n",
                     table_data->hashval,
                     table_data->ijg_qual,
                     lum_sum, chrom_sum,
                     num_files);

            if (chrom_sum == 0 && table_data->num_quant_tables == 1)
              {
                /* grayscale */
                for (i = 0; i < G_N_ELEMENTS (quant_info); i++)
                  {
                    if (quant_info[i].lum_sum == lum_sum
                        && (quant_info[i].subsmp_h == 0
                            || quant_info[i].subsmp_h
                            == table_data->subsmp_h)
                        && (quant_info[i].subsmp_v == 0
                            || quant_info[i].subsmp_v
                            == table_data->subsmp_v)
                        && quant_info[i].num_quant_tables > 0)
                      {
                        g_print("    XXX \"%s\", \"%s\",\n",
                                quant_info[i].source_name,
                                quant_info[i].setting_name);
                      }
                  }
              }
            else
              {
                /* RGB and other color spaces */
                for (i = 0; i < G_N_ELEMENTS (quant_info); i++)
                  {
                    if (quant_info[i].lum_sum == lum_sum
                        && quant_info[i].chrom_sum == chrom_sum
                        && (quant_info[i].subsmp_h == 0
                            || quant_info[i].subsmp_h
                            == table_data->subsmp_h)
                        && (quant_info[i].subsmp_v == 0
                            || quant_info[i].subsmp_v
                            == table_data->subsmp_v)
                        && (quant_info[i].num_quant_tables == table_data->num_quant_tables
                            || quant_info[i].num_quant_tables == -table_data->num_quant_tables))
                      {
                        g_print("    XXX \"%s\", \"%s\",\n",
                                quant_info[i].source_name,
                                quant_info[i].setting_name);
                      }
                  }
              }
          } /* FIXME */

          g_print ("    \"?\", \"? (hash %x)\",\n"
                   "    %d, %d,\n    %d,\n",
                   table_data->hashval,
                   table_data->subsmp_h,
                   table_data->subsmp_v,
                   -table_data->num_quant_tables);
          print_ctable (-1, table_data->luminance, TRUE);
          print_ctable (-1, table_data->chrominance, FALSE);
          g_print ("  },\n");
        }
    }
  g_print ("/* TOTAL FILES: %d */\n", total_files);
}

/*
 * Some compiler told me that it needed a function called main()...
 */
int
main (int   argc,
      char *argv[])
{
  GOptionContext *context;
  GError         *error = NULL;
  gint            i;

  g_set_prgname ("jpegqual");

  context =
    g_option_context_new ("FILE [...] - analyzes JPEG quantization tables");
  g_option_context_add_main_entries (context, option_entries,
                                     NULL /* skip i18n? */);

  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  if (! filenames)
    {
      g_printerr ("Missing file name.  Try the option --help for help\n");
      return EXIT_FAILURE;
    }

  if (!option_summary && !option_ctable && !option_table_2cols)
    {
      g_printerr ("Missing output option.  Assuming that you wanted --summary.\n");
      option_summary = TRUE;
    }

  for (i = 0; filenames[i]; i++)
    {
      if (! analyze_file (filenames[i]) && ! option_ignore_err)
        return EXIT_FAILURE;
    }

  if (option_unknown && found_tables)
    {
      print_unknown_tables ();
    }

  return EXIT_SUCCESS;
}
