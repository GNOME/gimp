/* jpegqual.c - analyze quality settings of existing JPEG files
 *
 * Copyright (C) 2007 Raphaël Quinet <raphael@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This program analyzes the quantization tables of the JPEG files
 * given on the command line and displays their quality settings.
 *
 * It can be used to validate the formula used in jpeg_detect_quality(),
 * by comparing the quality reported for different JPEG files.
 *
 * It can also dump quantization tables so that they can be integrated
 * into this program and recognized later.  This can be used to identify
 * which device or which program saved a JPEG file.
 */

/*
 * TODO:
 * - rename this program!
 * - update quant_info[].
 * - get rid of the command-line option '--name' (too cumbersome to use).
 * - rewrite the parser for command-line options and use GOption instead.
 * - reorganize the options: 2 groups for print options and for selection.
 * - re-add the code to compare different formulas for approx. IJG quality.
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <jpeglib.h>

#include "jpeg-quality.h"

/* command-line options */
typedef struct
{
  const gchar *option;
  const gchar *description;
} OptionDesc;

static const OptionDesc options_desc[] =
  {
    { "-h, --help",
      "Display this help message" },
    { "-s, --summary",
      "Print summary information and IJG quality (enabled by default)" },
    { "-t, --tables",
      "Dump quantization tables" },
    { "-c, --c-tables",
      "Dump quantization tables as C code" },
    { "-n, --name <name>",
      "Camera or software name (used when printing tables)" },
    { "-u, --unknown",
      "Only print information about files with unknown tables" },
    { NULL, NULL }
  };

static gboolean  option_summary      = TRUE;
static gboolean  option_ctable       = FALSE;
static gboolean  option_table_2cols  = FALSE;
static gchar    *option_name         = NULL;
static gboolean  option_unknown      = FALSE;
#if 0 /* FIXME */
static guint16 **ijg_luminance       = NULL; /* luminance, baseline */
static guint16 **ijg_chrominance     = NULL; /* chrominance, baseline */
static guint16 **ijg_luminance_nb    = NULL; /* luminance, not baseline */
static guint16 **ijg_chrominance_nb  = NULL; /* chrominance, not baseline */
#endif

typedef struct
{
  const gint   lum_sum;
  const gint   chrom_sum;
  const gint   subsmp_h;
  const gint   subsmp_v;
  const gchar *source_name;
  const gchar *setting_name;
  const gint   ijg_qual;
} QuantInfo;

static const QuantInfo quant_info[] =
{
  {    64,    64, 0, 0, "IJG JPEG Library", "quality 100", 100 },
  {    86,   115, 0, 0, "IJG JPEG Library", "quality 99", 99 },
  {   151,   224, 0, 0, "IJG JPEG Library", "quality 98", 98 },
  {   221,   333, 0, 0, "IJG JPEG Library", "quality 97", 97 },
  {   292,   443, 0, 0, "IJG JPEG Library", "quality 96", 96 },
  {   369,   558, 0, 0, "IJG JPEG Library", "quality 95", 95 },
  {   441,   668, 0, 0, "IJG JPEG Library", "quality 94", 94 },
  {   518,   779, 0, 0, "IJG JPEG Library", "quality 93", 93 },
  {   592,   891, 0, 0, "IJG JPEG Library", "quality 92", 92 },
  {   667,   999, 0, 0, "IJG JPEG Library", "quality 91", 91 },
  {   736,  1110, 0, 0, "IJG JPEG Library", "quality 90", 90 },
  {   814,  1223, 0, 0, "IJG JPEG Library", "quality 89", 89 },
  {   884,  1332, 0, 0, "IJG JPEG Library", "quality 88", 88 },
  {   961,  1444, 0, 0, "IJG JPEG Library", "quality 87", 87 },
  {  1031,  1555, 0, 0, "IJG JPEG Library", "quality 86", 86 },
  {  1109,  1666, 0, 0, "IJG JPEG Library", "quality 85", 85 },
  {  1179,  1778, 0, 0, "IJG JPEG Library", "quality 84", 84 },
  {  1251,  1888, 0, 0, "IJG JPEG Library", "quality 83", 83 },
  {  1326,  2000, 0, 0, "IJG JPEG Library", "quality 82", 82 },
  {  1398,  2111, 0, 0, "IJG JPEG Library", "quality 81", 81 },
  {  1477,  2221, 0, 0, "IJG JPEG Library", "quality 80", 80 },
  {  1552,  2336, 0, 0, "IJG JPEG Library", "quality 79", 79 },
  {  1620,  2445, 0, 0, "IJG JPEG Library", "quality 78", 78 },
  {  1692,  2556, 0, 0, "IJG JPEG Library", "quality 77", 77 },
  {  1773,  2669, 0, 0, "IJG JPEG Library", "quality 76", 76 },
  {  1858,  2780, 0, 0, "IJG JPEG Library", "quality 75", 75 },
  {  1915,  2836, 0, 0, "IJG JPEG Library", "quality 74", 74 },
  {  1996,  2949, 0, 0, "IJG JPEG Library", "quality 73", 73 },
  {  2068,  3060, 0, 0, "IJG JPEG Library", "quality 72", 72 },
  {  2136,  3169, 0, 0, "IJG JPEG Library", "quality 71", 71 },
  {  2211,  3284, 0, 0, "IJG JPEG Library", "quality 70", 70 },
  {  2290,  3394, 0, 0, "IJG JPEG Library", "quality 69", 69 },
  {  2362,  3505, 0, 0, "IJG JPEG Library", "quality 68", 68 },
  {  2437,  3617, 0, 0, "IJG JPEG Library", "quality 67", 67 },
  {  2509,  3727, 0, 0, "IJG JPEG Library", "quality 66", 66 },
  {  2583,  3839, 0, 0, "IJG JPEG Library", "quality 65", 65 },
  {  2657,  3950, 0, 0, "IJG JPEG Library", "quality 64", 64 },
  {  2727,  4061, 0, 0, "IJG JPEG Library", "quality 63", 63 },
  {  2804,  4173, 0, 0, "IJG JPEG Library", "quality 62", 62 },
  {  2874,  4282, 0, 0, "IJG JPEG Library", "quality 61", 61 },
  {  2952,  4395, 0, 0, "IJG JPEG Library", "quality 60", 60 },
  {  3021,  4506, 0, 0, "IJG JPEG Library", "quality 59", 59 },
  {  3096,  4614, 0, 0, "IJG JPEG Library", "quality 58", 58 },
  {  3170,  4726, 0, 0, "IJG JPEG Library", "quality 57", 57 },
  {  3247,  4837, 0, 0, "IJG JPEG Library", "quality 56", 56 },
  {  3323,  4947, 0, 0, "IJG JPEG Library", "quality 55", 55 },
  {  3396,  5062, 0, 0, "IJG JPEG Library", "quality 54", 54 },
  {  3467,  5172, 0, 0, "IJG JPEG Library", "quality 53", 53 },
  {  3541,  5281, 0, 0, "IJG JPEG Library", "quality 52", 52 },
  {  3621,  5396, 0, 0, "IJG JPEG Library", "quality 51", 51 },
  {  3688,  5505, 0, 0, "IJG JPEG Library", "quality 50", 50 },
  {  3755,  5614, 0, 0, "IJG JPEG Library", "quality 49", 49 },
  {  3835,  5729, 0, 0, "IJG JPEG Library", "quality 48", 48 },
  {  3909,  5838, 0, 0, "IJG JPEG Library", "quality 47", 47 },
  {  3980,  5948, 0, 0, "IJG JPEG Library", "quality 46", 46 },
  {  4092,  6116, 0, 0, "IJG JPEG Library", "quality 45", 45 },
  {  4166,  6226, 0, 0, "IJG JPEG Library", "quality 44", 44 },
  {  4280,  6396, 0, 0, "IJG JPEG Library", "quality 43", 43 },
  {  4393,  6562, 0, 0, "IJG JPEG Library", "quality 42", 42 },
  {  4463,  6672, 0, 0, "IJG JPEG Library", "quality 41", 41 },
  {  4616,  6897, 0, 0, "IJG JPEG Library", "quality 40", 40 },
  {  4719,  7060, 0, 0, "IJG JPEG Library", "quality 39", 39 },
  {  4829,  7227, 0, 0, "IJG JPEG Library", "quality 38", 38 },
  {  4976,  7447, 0, 0, "IJG JPEG Library", "quality 37", 37 },
  {  5086,  7616, 0, 0, "IJG JPEG Library", "quality 36", 36 },
  {  5240,  7841, 0, 0, "IJG JPEG Library", "quality 35", 35 },
  {  5421,  8114, 0, 0, "IJG JPEG Library", "quality 34", 34 },
  {  5571,  8288, 0, 0, "IJG JPEG Library", "quality 33", 33 },
  {  5756,  8565, 0, 0, "IJG JPEG Library", "quality 32", 32 },
  {  5939,  8844, 0, 0, "IJG JPEG Library", "quality 31", 31 },
  {  6125,  9122, 0, 0, "IJG JPEG Library", "quality 30", 30 },
  {  6345,  9455, 0, 0, "IJG JPEG Library", "quality 29", 29 },
  {  6562,  9787, 0, 0, "IJG JPEG Library", "quality 28", 28 },
  {  6823, 10175, 0, 0, "IJG JPEG Library", "quality 27", 27 },
  {  7084, 10567, 0, 0, "IJG JPEG Library", "quality 26", 26 },
  {  7376, 11010, 0, 0, "IJG JPEG Library", "quality 25", 25 },
  {  7668, 11453, 0, 0, "IJG JPEG Library", "quality 24", 24 },
  {  7995, 11954, 0, 0, "IJG JPEG Library", "quality 23", 23 },
  {  8331, 12511, 0, 0, "IJG JPEG Library", "quality 22", 22 },
  {  8680, 13121, 0, 0, "IJG JPEG Library", "quality 21", 21 },
  {  9056, 13790, 0, 0, "IJG JPEG Library", "quality 20", 20 },
  {  9368, 14204, 0, 0, "IJG JPEG Library", "quality 19", 19 },
  {  9679, 14267, 0, 0, "IJG JPEG Library", "quality 18", 18 },
  { 10027, 14346, 0, 0, "IJG JPEG Library", "quality 17", 17 },
  { 10360, 14429, 0, 0, "IJG JPEG Library", "quality 16", 16 },
  { 10714, 14526, 0, 0, "IJG JPEG Library", "quality 15", 15 },
  { 11081, 14635, 0, 0, "IJG JPEG Library", "quality 14", 14 },
  { 11456, 14754, 0, 0, "IJG JPEG Library", "quality 13", 13 },
  { 11861, 14864, 0, 0, "IJG JPEG Library", "quality 12", 12 },
  { 12240, 14985, 0, 0, "IJG JPEG Library", "quality 11", 11 },
  { 12560, 15110, 0, 0, "IJG JPEG Library", "quality 10", 10 },
  { 12859, 15245, 0, 0, "IJG JPEG Library", "quality 9", 9 },
  { 13230, 15369, 0, 0, "IJG JPEG Library", "quality 8", 8 },
  { 13623, 15523, 0, 0, "IJG JPEG Library", "quality 7", 7 },
  { 14073, 15731, 0, 0, "IJG JPEG Library", "quality 6", 6 },
  { 14655, 16010, 0, 0, "IJG JPEG Library", "quality 5", 5 },
  { 15277, 16218, 0, 0, "IJG JPEG Library", "quality 4", 4 },
  { 15946, 16320, 0, 0, "IJG JPEG Library", "quality 3", 3 },
  { 16315, 16320, 0, 0, "IJG JPEG Library", "quality 2", 2 },
  { 16320, 16320, 0, 0, "IJG JPEG Library", "quality 1", 1 },
  { 16320, 16320, 0, 0, "IJG JPEG Library", "quality 0", 1 },
  {  8008, 11954, 0, 0, "IJG JPEG Library", "not baseline 23", -22 },
  {  8370, 12511, 0, 0, "IJG JPEG Library", "not baseline 22", -21 },
  {  8774, 13121, 0, 0, "IJG JPEG Library", "not baseline 21", -20 },
  {  9234, 13790, 0, 0, "IJG JPEG Library", "not baseline 20", -19 },
  {  9700, 14459, 0, 0, "IJG JPEG Library", "not baseline 19", -17 },
  { 10209, 15236, 0, 0, "IJG JPEG Library", "not baseline 18", -14 },
  { 10843, 16182, 0, 0, "IJG JPEG Library", "not baseline 17", -11 },
  { 11505, 17183, 0, 0, "IJG JPEG Library", "not baseline 16", -7 },
  { 12279, 18351, 0, 0, "IJG JPEG Library", "not baseline 15", -5 },
  { 13166, 19633, 0, 0, "IJG JPEG Library", "not baseline 14", 0 },
  { 14160, 21129, 0, 0, "IJG JPEG Library", "not baseline 13", 0 },
  { 15344, 22911, 0, 0, "IJG JPEG Library", "not baseline 12", 0 },
  { 16748, 24969, 0, 0, "IJG JPEG Library", "not baseline 11", 0 },
  { 18440, 27525, 0, 0, "IJG JPEG Library", "not baseline 10", 0 },
  { 20471, 30529, 0, 0, "IJG JPEG Library", "not baseline 9", 0 },
  { 23056, 34422, 0, 0, "IJG JPEG Library", "not baseline 8", 0 },
  { 26334, 39314, 0, 0, "IJG JPEG Library", "not baseline 7", 0 },
  { 30719, 45876, 0, 0, "IJG JPEG Library", "not baseline 6", 0 },
  { 36880, 55050, 0, 0, "IJG JPEG Library", "not baseline 5", 0 },
  { 46114, 68840, 0, 0, "IJG JPEG Library", "not baseline 4", 0 },
  { 61445, 91697, 0, 0, "IJG JPEG Library", "not baseline 3", 0 },
  { 92200, 137625, 0, 0, "IJG JPEG Library", "not baseline 2", 0 },
  { 184400, 275250, 0, 0, "IJG JPEG Library", "not baseline 1", 0 },
  { 184400, 275250, 0, 0, "IJG JPEG Library", "not baseline 0", 0 },
  {   319,   665, 2, 1, "ACD ?", "?", -94 },
  {   436,   996, 2, 1, "ACD ?", "?", -92 },
  {   664,  1499, 2, 1, "ACD ?", "?", -88 },
  {  1590,  3556, 2, 2, "ACD ?", "?", -71 },
  {   284,   443, 2, 2, "Adobe Photoshop CS3", "?", -96 },
  {   109,   171, 1, 1, "Adobe Photoshop CS or CS2", "quality 12", -98 },
  {   303,   466, 1, 1, "Adobe Photoshop CS2", "quality 11", -95 },
  {   535,   750, 1, 1, "Adobe Photoshop CS2", "quality 10", -93 },
  {   668,   830, 1, 1, "Adobe Photoshop CS2", "quality 9", -91 },
  {   794,   895, 1, 1, "Adobe Photoshop CS2", "quality 8", -90 },
  {   971,   950, 1, 1, "Adobe Photoshop CS2", "quality 7", -89 },
  {   884,   831, 2, 2, "Adobe Photoshop CS2", "quality 6", -90 },
  {  1032,   889, 2, 2, "Adobe Photoshop CS2", "quality 5", -89 },
  {  1126,   940, 2, 2, "Adobe Photoshop CS2", "quality 4", -88 },
  {  1216,   977, 2, 2, "Adobe Photoshop CS2", "quality 3", -88 },
  {  1281,   998, 2, 2, "Adobe Photoshop CS2", "quality 2", -87 },
  {  1484,  1083, 2, 2, "Adobe Photoshop CS2", "quality 1", -86 },
  {  1582,  1108, 2, 2, "Adobe Photoshop CS2", "quality 0", -85 },
  {    95,   168, 1, 1, "Adobe Photoshop CS2", "save for web 100", -98 },
  {   234,   445, 1, 1, "Adobe Photoshop CS2", "save for web 90", -96 },
  {   406,   724, 1, 1, "Adobe Photoshop CS2", "save for web 80", -93 },
  {   646,  1149, 1, 1, "Adobe Photoshop CS2", "save for web 70", -90 },
  {   974,  1769, 1, 1, "Adobe Photoshop CS2", "save for web 60", -85 },
  {  1221,  1348, 2, 2, "Adobe Photoshop CS2", "save for web 50", -86 },
  {  1821,  1997, 2, 2, "Adobe Photoshop CS2", "save for web 40", -79 },
  {  2223,  2464, 2, 2, "Adobe Photoshop CS2", "save for web 30", -74 },
  {  2575,  2903, 2, 2, "Adobe Photoshop CS2", "save for web 20", -70 },
  {  3514,  3738, 2, 2, "Adobe Photoshop CS2", "save for web 10", -60 },
  {    95,   166, 1, 1, "Adobe Photoshop CS or Lightroom", "save for web 100?", -98 },
  {   232,   443, 1, 1, "Adobe Photoshop CS", "?", -96 },
  {   649,   853, 1, 1, "Adobe Photoshop CS or Elements 4.0", "?", -91 },
  {   844,   849, 2, 2, "Adobe Photoshop CS", "?", -90 },
  {   962,   892, 2, 2, "Adobe Photoshop CS", "?", -89 },
  {   406,   722, 1, 1, "Adobe Photoshop 7.0", "?", -93 },
  {   539,   801, 1, 1, "Adobe Photoshop 7.0 CE", "?", -92 },
  {   786,   926, 1, 1, "Adobe Photoshop 7.0", "?", -90 },
  {   539,   801, 1, 1, "Adobe Photoshop 5.0", "?", -92 },
  {   717,   782, 2, 2, "Adobe Photoshop 5.0", "?", -91 },
  {  1068,   941, 2, 2, "Adobe Photoshop 4.0", "?", -89 },
  {   339,   670, 1, 1, "Adobe Photoshop ?", "?", -94 },
  {   406,   722, 1, 1, "Adobe Photoshop ?", "?", -93 },
  {   427,   613, 2, 2, "Adobe Photoshop ?", "?", -94 },
  {   525,   941, 1, 1, "Adobe Photoshop ?", "?", -92 },
  {   803,  1428, 1, 1, "Adobe Photoshop ?", "?", -87 },
  {  1085,  1996, 1, 1, "Adobe Photoshop ?", "?", -83 },
  {  1156,  2116, 1, 1, "Adobe Photoshop ?", "?", -82 },
  {  1175,  2169, 1, 1, "Adobe Photoshop ?", "?", -81 },
  {  2272,  2522, 2, 2, "Adobe Photoshop ?", "?", -73 },
  {   513,     0, 1, 1, "Adobe Photoshop ?", "?", -97 },
  {  2515,  2831, 2, 2, "Adobe ImageReady", "?", -70 },
  {  3822,  3975, 2, 2, "Adobe ImageReady", "?", -57 },
  {   255,   393, 2, 1, "Apple Quicktime 7", "?", -96 },
  {   513,   775, 2, 2, "Apple Quicktime 7", "?", -93 },
  {   543,   784, 2, 1, "Apple Quicktime 7", "?", -92 },
  {   361,   506, 2, 1, "Apple ?", "?", -95 },
  {  1511,  2229, 2, 2, "Apple ?", "?", -79 },
  {   188,   276, 2, 1, "Canon EOS 300D, 350D or 400D", "Fine", -97 },
  {   708,  1057, 2, 1, "Canon EOS 10D", "?", -90 },
  {   533,  1925, 2, 1, "Canon Digital Ixus", "Fine", -86 },
  {   533,  1325, 2, 1, "Canon PowerShot A95, S1, S2, SD400 or SD630", "Fine", -89 },
  {   192,   556, 1, 2, "Canon PowerShot A430", "Superfine", -95 },
  {   192,   556, 2, 1, "Canon PowerShot S200 or Ixus 800", "Superfine", -95 },
  {    64,    64, 2, 2, "Casio EX-Z600", "Fine", 100 },
  {   288,   443, 2, 1, "FujiFilm MX-2700", "?", -96 },
  {  2056,  3102, 2, 1, "FujiFilm DX-10", "?", -71 },
  {  1254,  1888, 2, 1, "FujiFilm MX-1700 Zoom", "?", -82 },
  {   515,   774, 2, 1, "FujiFilm FinePix 2600 Zoom", "Fine", -93 },
  {  1375,  1131, 2, 2, "Kodak DC210", "?", -86 },
  {   736,  1110, 2, 2, "Kodak DC240", "?", 90 },
  {   161,   179, 1, 1, "Lead ?", "?", -98 },
  {   711,  1055, 1, 1, "Lead ?", "?", -90 },
  {  1079,  1610, 1, 1, "Lead ?", "?", -85 },
  {  2031,  3054, 1, 1, "Lead ?", "?", -72 },
  {  4835,  7226, 1, 1, "Lead ?", "?", -37 },
  {  8199, 12287, 1, 1, "Lead ?", "?", -22 },
  {   582,   836, 2, 1, "Medion ?", "?", -92 },
  {   116,   169, 2, 1, "Nikon D50, D70, D70s, D80", "Fine", -98 },
  {   218,   333, 2, 1, "Nikon D70 or D70s", "Normal", -97 },
  {   616,   941, 2, 1, "Nikon D70 or D70s", "Basic", -91 },
  {   671,   999, 2, 1, "Nikon D70 or D70s", "Basic + raw", -90 },
  {   127,   169, 2, 1, "Nikon D70 v1.0", "Fine", -98 },
  {   302,   444, 2, 1, "Nikon D70 v1.0", "Fine", -95 },
  {   315,   499, 2, 1, "Nikon D70 v1.0", "Fine", -95 },
  {   329,   500, 2, 1, "Nikon D70 v1.0", "Fine", -95 },
  {   346,   500, 2, 1, "Nikon D70 v1.0", "Fine", -95 },
  {   372,   558, 2, 1, "Nikon D70 v1.0", "Fine", -94 },
  {   389,   560, 2, 1, "Nikon D70 v1.0", "Fine", -94 },
  {   419,   611, 2, 1, "Nikon D70 v1.0", "Fine", -94 },
  {   449,   668, 2, 1, "Nikon D70 v1.0", "Fine", -93 },
  {   506,   775, 2, 1, "Nikon D70 v1.0", "Fine", -93 },
  {   529,   781, 2, 1, "Nikon D70 v1.0", "Fine", -92 },
  {   855,  1279, 2, 1, "Nikon D40", "?", -88 },
  {    64,    64, 1, 1, "Nikon Browser 6", "High quality", 100 },
  {   779,  1164, 1, 1, "Nikon Browser 6", "Standard quality", -89 },
  {  1697,  2554, 2, 1, "Nikon Browser 6", "Standard eq", -76 },
  {  2746,  5112, 2, 2, "Nikon Browser 6", "Standard compression", -57 },
  {  8024, 12006, 2, 2, "Nikon Browser 6", "Maximum compression", -22 },
  {   736,  1110, 2, 2, "Olympus Camedia Master", "High quality?", 90 },
  {  1057,  1561, 2, 1, "Olympus u770SW,S770SW", "?", -85 },
  {   275,   395, 2, 1, "Olympus u750,S750", "Raw?", -96 },
  {  1477,  2221, 2, 2, "Olympus U710,S710", "Super high quality?", 80 },
  {   251,   392, 2, 1, "Panasonic DMC-FZ30", "High", -96 },
  {   280,   445, 2, 1, "Panasonic DMC-FZ30", "High", -96 },
  {   304,   448, 2, 1, "Panasonic DMC-FZ30", "High", -95 },
  {   316,   499, 2, 1, "Panasonic DMC-FZ30", "High", -95 },
  {   332,   550, 2, 1, "Panasonic DMC-FZ30", "High", -95 },
  {   355,   555, 2, 1, "Panasonic DMC-FZ30", "High", -95 },
  {   375,   606, 2, 1, "Panasonic DMC-FZ30", "High", -94 },
  {   400,   615, 2, 1, "Panasonic DMC-FZ30", "High", -94 },
  {   420,   667, 2, 1, "Panasonic DMC-FZ30", "High", -94 },
  {   501,   775, 2, 1, "Panasonic DMC-FZ30", "High", -93 },
  {   577,   891, 2, 1, "Panasonic DMC-FZ30", "High", -92 },
  {   324,   499, 2, 1, "Ricoh Caplio R1V", "?", -95 },
  {   634,   943, 2, 1, "Samsung Digimax V3", "?", -91 },
  {  1313,  1997, 2, 1, "Samsung Digimax V3", "?", -82 },
  {  1075,  1612, 2, 1, "Sanyo SR6", "?", -85 },
  {   785,  1168, 2, 1, "Sanyo SX113", "?", -89 },
  {   258,   389, 2, 1, "Sony Cybershot", "?", -96 },
  {  2782,  4259, 2, 1, "Sony Cybershot", "?", -61 },
  {    86,   115, 2, 1, "Sony DSC-W55", "?", 99 },
  {   221,   333, 2, 1, "Sony DSC-W50", "?", 97 },
  {  1477,  2221, 2, 2, "Sony DSC-W30, DSC-P43, DSC-S600", "?", 80 },
  {   736,  1110, 2, 1, "SonyEricsson K750i", "Fine", 90 },
  {  1858,  2780, 2, 1, "SonyEricsson K750i", "Normal", 75 },
  {   836,  1094, 2, 2, "SonyEricsson K750i", "Panorama fine", -89 },
  {  1672,  2188, 2, 2, "SonyEricsson K750i", "Panorama normal", -79 },
  {   361,   555, 2, 1, "SonyEricsson K750i", "?", -95 },
  {   433,   667, 2, 1, "SonyEricsson K750i", "?", -94 },
  {   954,  1443, 2, 1, "SonyEricsson K750i", "?", -87 },
  {  1024,  1504, 2, 1, "SonyEricsson K750i", "?", -86 },
  {  1097,  1615, 2, 1, "SonyEricsson K750i", "?", -85 },
  {  1168,  1727, 2, 1, "SonyEricsson K750i", "?", -84 },
  {  1324,  2000, 2, 1, "SonyEricsson K750i", "?", -82 },
  {  1473,  2170, 2, 1, "SonyEricsson K750i", "?", -80 },
  {  1615,  2394, 2, 1, "SonyEricsson K750i", "?", -78 },
  {  1759,  2612, 2, 1, "SonyEricsson K750i", "?", -76 },
  {  1491,  1491, 2, 1, "SonyEricsson Z600", "Default", -83 },
  {  1830,  2725, 1, 1, "Xing VT-Compress", "?", -75 }
};

#if 0 /* FIXME */

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

#endif /* FIXME */

static GSList *
detect_source (struct jpeg_decompress_struct *cinfo)
{
  guint   lum_sum;
  guint   chrom_sum;
  gint    i;
  GSList *source_list;

  /* compute sum of luminance and chrominance quantization tables */
  lum_sum = 0;
  chrom_sum = 0;
  for (i = 0; i < DCTSIZE2; i++)
    lum_sum += cinfo->quant_tbl_ptrs[0]->quantval[i];

  if (cinfo->quant_tbl_ptrs[1])
    {
      for (i = 0; i < DCTSIZE2; i++)
        chrom_sum += cinfo->quant_tbl_ptrs[1]->quantval[i];
    }

  /* there can be more than one match (if sampling factors are compatible) */
  source_list = NULL;
  if (chrom_sum == 0)
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
                  == cinfo->comp_info[0].v_samp_factor))
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
                  == cinfo->comp_info[0].v_samp_factor))
            {
              source_list = g_slist_append (source_list,
                                            (gpointer) (quant_info + i));
            }
        }
    }

  return source_list;
}

static void
print_summary (struct jpeg_decompress_struct *cinfo)
{
  gint       quality;
  gint       i;
  GSList    *source_list;
  QuantInfo *source_info;

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

  source_list = detect_source (cinfo);
  if (source_list)
    {
      GSList *l;

      for (l = source_list; l; l = l->next)
        {
          source_info = l->data;
          g_print ("\tSource:   %s - %s\n",
                   source_info->source_name,
                   source_info->setting_name);
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
  g_print ("    {  /* table %d */\n      ", table_id);
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
 * Analyze a JPEG file according to the command-line options.
 */
static gboolean
analyze_file (const gchar *filename)
{
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  FILE                          *f;
  gint                           i;
  GSList                        *source_list;

  if ((f = fopen (filename, "rb")) == NULL)
    {
      g_print ("Cannot open '%s'\n", filename);
      return FALSE;
    }

  if (option_summary)
    g_print ("%s:\n", filename);

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);

  jpeg_stdio_src (&cinfo, f);

  jpeg_read_header (&cinfo, TRUE);

  source_list = detect_source (&cinfo);
  if (!source_list || !option_unknown)
    {
      if (option_summary)
        print_summary (&cinfo);

      if (option_ctable)
        {
          g_print ("  {\n    /* %s */\n    \"%s\", \"?\",\n    %hd, %hd,\n",
                   filename,
                   (option_name ? option_name : filename),
                   cinfo.comp_info[0].h_samp_factor,
                   cinfo.comp_info[0].v_samp_factor);
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
 * Help?
 */
static void
usage (void)
{
  const OptionDesc *opt;

  g_print ("This program analyzes the quantization tables of the JPEG files given on the\n"
           "command line and displays their quality settings.\n"
           "\n");
  g_print ("Usage:\n"
           "  jpegqual [options] file [[options] file [...]]\n"
           "\n");
  g_print ("Options:\n");
  for (opt = options_desc; opt->option; opt++)
    g_print ("  %-20s %s.\n", opt->option, opt->description);
  g_print ("\n");
  g_print ("Options may be given before any file.  Long boolean options can be negated\n"
           "by prefixing them with --no.  For example: --no-summary.\n");
}

/*
 * Some compiler told me that it needed a function called main()...
 */
int
main (int   argc,
      char *argv[])
{
  gboolean parse_options = TRUE;

  g_set_prgname ("jpegqual");
  if (argc > 1)
    {
      for (argv++, argc--; argc; argv++, argc--)
        {
          if (parse_options && **argv == '-')
            {
              if (! strcmp (*argv, "--"))
                parse_options = FALSE;
              else if (! strcmp (*argv, "-s") || ! strcmp (*argv, "--summary"))
                option_summary = TRUE;
              else if (! strcmp (*argv, "--no-summary"))
                option_summary = FALSE;
              else if (! strcmp (*argv, "-c") || ! strcmp (*argv, "--c-tables"))
                option_ctable = TRUE;
              else if (! strcmp (*argv, "--no-c-tables"))
                option_ctable = FALSE;
              else if (! strcmp (*argv, "-t") || ! strcmp (*argv, "--tables"))
                option_table_2cols = TRUE;
              else if (! strcmp (*argv, "--no-tables"))
                option_table_2cols = FALSE;
              else if ((! strcmp (*argv, "-n") || ! strcmp (*argv, "--name"))
                        && argc > 1)
                {
                  argv++;
                  argc--;
                  option_name = *argv;
                }
              else if (! strcmp (*argv, "-u") || ! strcmp (*argv, "--unknown"))
                option_unknown = TRUE;
              else if (! strcmp (*argv, "--no-unkown"))
                option_unknown = FALSE;
              else if (! strcmp (*argv, "-h") || ! strcmp (*argv, "--help")
                       || ! strcmp (*argv, "-?"))
                {
                  usage ();
                  return 0;
                }
              else
                {
                  g_print ("Unknown option or missing argument: '%s'.  "
                           "Try '--help' for help.\n", *argv);
                  return 1;
                }
            }
          else
            {
              if (! analyze_file (*argv))
                return 1;
            }
        }
      return 0;
    }
  else
    {
      usage ();
      return 1;
    }
}
