/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,   */
/* USA.                                                                    */
/***************************************************************************/

/*************************************************************/
/* This file contains routines for creating and setting up   */
/* visuals. There's also routines for converting RGB(A) data */
/* to whatever format the current visual is.                 */
/*************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <gck/gck.h>
#include <gck/gcktypes.h>
#include <gck/gckcolor.h>

#define RESERVED_COLORS 2

#define ROUND_TO_INT(val) ((val) + 0.5)

typedef struct {
  guchar ready;
  GckRGB color;
} _GckSampleType;

const GckRGB gck_rgb_white = {1.0, 1.0, 1.0, 1.0};
const GckRGB gck_rgb_black = {0.0, 0.0, 0.0, 1.0};
const GckRGB gck_rgb_zero = {0.0, 0.0, 0.0, 0.0};
const GckRGB gck_rgb_full = {1.0, 1.0, 1.0, 1.0};

GckNamedRGB gck_named_colors[] = {
    {255,250,250, "snow" },
    {248,248,255, "ghost white" },
    {248,248,255, "GhostWhite" },
    {245,245,245, "white smoke" },
    {245,245,245, "WhiteSmoke" },
    {220,220,220, "gainsboro" },
    {255,250,240, "floral white" },
    {255,250,240, "FloralWhite" },
    {253,245,230, "old lace" },
    {253,245,230, "OldLace" },
    {250,240,230, "linen" },
    {250,235,215, "antique white" },
    {250,235,215, "AntiqueWhite" },
    {255,239,213, "papaya whip" },
    {255,239,213, "PapayaWhip" },
    {255,235,205, "blanched almond" },
    {255,235,205, "BlanchedAlmond" },
    {255,228,196, "bisque" },
    {255,218,185, "peach puff" },
    {255,218,185, "PeachPuff" },
    {255,222,173, "navajo white" },
    {255,222,173, "NavajoWhite" },
    {255,228,181, "moccasin" },
    {255,248,220, "cornsilk" },
    {255,255,240, "ivory" },
    {255,250,205, "lemon chiffon" },
    {255,250,205, "LemonChiffon" },
    {255,245,238, "seashell" },
    {240,255,240, "honeydew" },
    {245,255,250, "mint cream" },
    {245,255,250, "MintCream" },
    {240,255,255, "azure" },
    {240,248,255, "alice blue" },
    {240,248,255, "AliceBlue" },
    {230,230,250, "lavender" },
    {255,240,245, "lavender blush" },
    {255,240,245, "LavenderBlush" },
    {255,228,225, "misty rose" },
    {255,228,225, "MistyRose" },
    {255,255,255, "white" },
    {  0,  0,  0, "black" },
    { 47, 79, 79, "dark slate gray" },
    { 47, 79, 79, "DarkSlateGray" },
    { 47, 79, 79, "dark slate grey" },
    { 47, 79, 79, "DarkSlateGrey" },
    {105,105,105, "dim gray" },
    {105,105,105, "DimGray" },
    {105,105,105, "dim grey" },
    {105,105,105, "DimGrey" },
    {112,128,144, "slate gray" },
    {112,128,144, "SlateGray" },
    {112,128,144, "slate grey" },
    {112,128,144, "SlateGrey" },
    {119,136,153, "light slate gray" },
    {119,136,153, "LightSlateGray" },
    {119,136,153, "light slate grey" },
    {119,136,153, "LightSlateGrey" },
    {190,190,190, "gray" },
    {190,190,190, "grey" },
    {211,211,211, "light grey" },
    {211,211,211, "LightGrey" },
    {211,211,211, "light gray" },
    {211,211,211, "LightGray" },
    { 25, 25,112, "midnight blue" },
    { 25, 25,112, "MidnightBlue" },
    {  0,  0,128, "navy" },
    {  0,  0,128, "navy blue" },
    {  0,  0,128, "NavyBlue" },
    {100,149,237, "cornflower blue" },
    {100,149,237, "CornflowerBlue" },
    { 72, 61,139, "dark slate blue" },
    { 72, 61,139, "DarkSlateBlue" },
    {106, 90,205, "slate blue" },
    {106, 90,205, "SlateBlue" },
    {123,104,238, "medium slate blue" },
    {123,104,238, "MediumSlateBlue" },
    {132,112,255, "light slate blue" },
    {132,112,255, "LightSlateBlue" },
    {  0,  0,205, "medium blue" },
    {  0,  0,205, "MediumBlue" },
    { 65,105,225, "royal blue" },
    { 65,105,225, "RoyalBlue" },
    {  0,  0,255, "blue" },
    { 30,144,255, "dodger blue" },
    { 30,144,255, "DodgerBlue" },
    {  0,191,255, "deep sky blue" },
    {  0,191,255, "DeepSkyBlue" },
    {135,206,235, "sky blue" },
    {135,206,235, "SkyBlue" },
    {135,206,250, "light sky blue" },
    {135,206,250, "LightSkyBlue" },
    { 70,130,180, "steel blue" },
    { 70,130,180, "SteelBlue" },
    {176,196,222, "light steel blue" },
    {176,196,222, "LightSteelBlue" },
    {173,216,230, "light blue" },
    {173,216,230, "LightBlue" },
    {176,224,230, "powder blue" },
    {176,224,230, "PowderBlue" },
    {175,238,238, "pale turquoise" },
    {175,238,238, "PaleTurquoise" },
    {  0,206,209, "dark turquoise" },
    {  0,206,209, "DarkTurquoise" },
    { 72,209,204, "medium turquoise" },
    { 72,209,204, "MediumTurquoise" },
    { 64,224,208, "turquoise" },
    {  0,255,255, "cyan" },
    {224,255,255, "light cyan" },
    {224,255,255, "LightCyan" },
    { 95,158,160, "cadet blue" },
    { 95,158,160, "CadetBlue" },
    {102,205,170, "medium aquamarine" },
    {102,205,170, "MediumAquamarine" },
    {127,255,212, "aquamarine" },
    {  0,100,  0, "dark green" },
    {  0,100,  0, "DarkGreen" },
    { 85,107, 47, "dark olive green" },
    { 85,107, 47, "DarkOliveGreen" },
    {143,188,143, "dark sea green" },
    {143,188,143, "DarkSeaGreen" },
    { 46,139, 87, "sea green" },
    { 46,139, 87, "SeaGreen" },
    { 60,179,113, "medium sea green" },
    { 60,179,113, "MediumSeaGreen" },
    { 32,178,170, "light sea green" },
    { 32,178,170, "LightSeaGreen" },
    {152,251,152, "pale green" },
    {152,251,152, "PaleGreen" },
    {  0,255,127, "spring green" },
    {  0,255,127, "SpringGreen" },
    {124,252,  0, "lawn green" },
    {124,252,  0, "LawnGreen" },
    {  0,255,  0, "green" },
    {127,255,  0, "chartreuse" },
    {  0,250,154, "medium spring green" },
    {  0,250,154, "MediumSpringGreen" },
    {173,255, 47, "green yellow" },
    {173,255, 47, "GreenYellow" },
    { 50,205, 50, "lime green" },
    { 50,205, 50, "LimeGreen" },
    {154,205, 50, "yellow green" },
    {154,205, 50, "YellowGreen" },
    { 34,139, 34, "forest green" },
    { 34,139, 34, "ForestGreen" },
    {107,142, 35, "olive drab" },
    {107,142, 35, "OliveDrab" },
    {189,183,107, "dark khaki" },
    {189,183,107, "DarkKhaki" },
    {240,230,140, "khaki" },
    {238,232,170, "pale goldenrod" },
    {238,232,170, "PaleGoldenrod" },
    {250,250,210, "light goldenrod yellow" },
    {250,250,210, "LightGoldenrodYellow" },
    {255,255,224, "light yellow" },
    {255,255,224, "LightYellow" },
    {255,255,  0, "yellow" },
    {255,215,  0, "gold" },
    {238,221,130, "light goldenrod" },
    {238,221,130, "LightGoldenrod" },
    {218,165, 32, "goldenrod" },
    {184,134, 11, "dark goldenrod" },
    {184,134, 11, "DarkGoldenrod" },
    {188,143,143, "rosy brown" },
    {188,143,143, "RosyBrown" },
    {205, 92, 92, "indian red" },
    {205, 92, 92, "IndianRed" },
    {139, 69, 19, "saddle brown" },
    {139, 69, 19, "SaddleBrown" },
    {160, 82, 45, "sienna" },
    {205,133, 63, "peru" },
    {222,184,135, "burlywood" },
    {245,245,220, "beige" },
    {245,222,179, "wheat" },
    {244,164, 96, "sandy brown" },
    {244,164, 96, "SandyBrown" },
    {210,180,140, "tan" },
    {210,105, 30, "chocolate" },
    {178, 34, 34, "firebrick" },
    {165, 42, 42, "brown" },
    {233,150,122, "dark salmon" },
    {233,150,122, "DarkSalmon" },
    {250,128,114, "salmon" },
    {255,160,122, "light salmon" },
    {255,160,122, "LightSalmon" },
    {255,165,  0, "orange" },
    {255,140,  0, "dark orange" },
    {255,140,  0, "DarkOrange" },
    {255,127, 80, "coral" },
    {240,128,128, "light coral" },
    {240,128,128, "LightCoral" },
    {255, 99, 71, "tomato" },
    {255, 69,  0, "orange red" },
    {255, 69,  0, "OrangeRed" },
    {255,  0,  0, "red" },
    {255,105,180, "hot pink" },
    {255,105,180, "HotPink" },
    {255, 20,147, "deep pink" },
    {255, 20,147, "DeepPink" },
    {255,192,203, "pink" },
    {255,182,193, "light pink" },
    {255,182,193, "LightPink" },
    {219,112,147, "pale violet red" },
    {219,112,147, "PaleVioletRed" },
    {176, 48, 96, "maroon" },
    {199, 21,133, "medium violet red" },
    {199, 21,133, "MediumVioletRed" },
    {208, 32,144, "violet red" },
    {208, 32,144, "VioletRed" },
    {255,  0,255, "magenta" },
    {238,130,238, "violet" },
    {221,160,221, "plum" },
    {218,112,214, "orchid" },
    {186, 85,211, "medium orchid" },
    {186, 85,211, "MediumOrchid" },
    {153, 50,204, "dark orchid" },
    {153, 50,204, "DarkOrchid" },
    {148,  0,211, "dark violet" },
    {148,  0,211, "DarkViolet" },
    {138, 43,226, "blue violet" },
    {138, 43,226, "BlueViolet" },
    {160, 32,240, "purple" },
    {147,112,219, "medium purple" },
    {147,112,219, "MediumPurple" },
    {216,191,216, "thistle" },
    {255,250,250, "snow1" },
    {238,233,233, "snow2" },
    {205,201,201, "snow3" },
    {139,137,137, "snow4" },
    {255,245,238, "seashell1" },
    {238,229,222, "seashell2" },
    {205,197,191, "seashell3" },
    {139,134,130, "seashell4" },
    {255,239,219, "AntiqueWhite1" },
    {238,223,204, "AntiqueWhite2" },
    {205,192,176, "AntiqueWhite3" },
    {139,131,120, "AntiqueWhite4" },
    {255,228,196, "bisque1" },
    {238,213,183, "bisque2" },
    {205,183,158, "bisque3" },
    {139,125,107, "bisque4" },
    {255,218,185, "PeachPuff1" },
    {238,203,173, "PeachPuff2" },
    {205,175,149, "PeachPuff3" },
    {139,119,101, "PeachPuff4" },
    {255,222,173, "NavajoWhite1" },
    {238,207,161, "NavajoWhite2" },
    {205,179,139, "NavajoWhite3" },
    {139,121, 94, "NavajoWhite4" },
    {255,250,205, "LemonChiffon1" },
    {238,233,191, "LemonChiffon2" },
    {205,201,165, "LemonChiffon3" },
    {139,137,112, "LemonChiffon4" },
    {255,248,220, "cornsilk1" },
    {238,232,205, "cornsilk2" },
    {205,200,177, "cornsilk3" },
    {139,136,120, "cornsilk4" },
    {255,255,240, "ivory1" },
    {238,238,224, "ivory2" },
    {205,205,193, "ivory3" },
    {139,139,131, "ivory4" },
    {240,255,240, "honeydew1" },
    {224,238,224, "honeydew2" },
    {193,205,193, "honeydew3" },
    {131,139,131, "honeydew4" },
    {255,240,245, "LavenderBlush1" },
    {238,224,229, "LavenderBlush2" },
    {205,193,197, "LavenderBlush3" },
    {139,131,134, "LavenderBlush4" },
    {255,228,225, "MistyRose1" },
    {238,213,210, "MistyRose2" },
    {205,183,181, "MistyRose3" },
    {139,125,123, "MistyRose4" },
    {240,255,255, "azure1" },
    {224,238,238, "azure2" },
    {193,205,205, "azure3" },
    {131,139,139, "azure4" },
    {131,111,255, "SlateBlue1" },
    {122,103,238, "SlateBlue2" },
    {105, 89,205, "SlateBlue3" },
    { 71, 60,139, "SlateBlue4" },
    { 72,118,255, "RoyalBlue1" },
    { 67,110,238, "RoyalBlue2" },
    { 58, 95,205, "RoyalBlue3" },
    { 39, 64,139, "RoyalBlue4" },
    {  0,  0,255, "blue1" },
    {  0,  0,238, "blue2" },
    {  0,  0,205, "blue3" },
    {  0,  0,139, "blue4" },
    { 30,144,255, "DodgerBlue1" },
    { 28,134,238, "DodgerBlue2" },
    { 24,116,205, "DodgerBlue3" },
    { 16, 78,139, "DodgerBlue4" },
    { 99,184,255, "SteelBlue1" },
    { 92,172,238, "SteelBlue2" },
    { 79,148,205, "SteelBlue3" },
    { 54,100,139, "SteelBlue4" },
    {  0,191,255, "DeepSkyBlue1" },
    {  0,178,238, "DeepSkyBlue2" },
    {  0,154,205, "DeepSkyBlue3" },
    {  0,104,139, "DeepSkyBlue4" },
    {135,206,255, "SkyBlue1" },
    {126,192,238, "SkyBlue2" },
    {108,166,205, "SkyBlue3" },
    { 74,112,139, "SkyBlue4" },
    {176,226,255, "LightSkyBlue1" },
    {164,211,238, "LightSkyBlue2" },
    {141,182,205, "LightSkyBlue3" },
    { 96,123,139, "LightSkyBlue4" },
    {198,226,255, "SlateGray1" },
    {185,211,238, "SlateGray2" },
    {159,182,205, "SlateGray3" },
    {108,123,139, "SlateGray4" },
    {202,225,255, "LightSteelBlue1" },
    {188,210,238, "LightSteelBlue2" },
    {162,181,205, "LightSteelBlue3" },
    {110,123,139, "LightSteelBlue4" },
    {191,239,255, "LightBlue1" },
    {178,223,238, "LightBlue2" },
    {154,192,205, "LightBlue3" },
    {104,131,139, "LightBlue4" },
    {224,255,255, "LightCyan1" },
    {209,238,238, "LightCyan2" },
    {180,205,205, "LightCyan3" },
    {122,139,139, "LightCyan4" },
    {187,255,255, "PaleTurquoise1" },
    {174,238,238, "PaleTurquoise2" },
    {150,205,205, "PaleTurquoise3" },
    {102,139,139, "PaleTurquoise4" },
    {152,245,255, "CadetBlue1" },
    {142,229,238, "CadetBlue2" },
    {122,197,205, "CadetBlue3" },
    { 83,134,139, "CadetBlue4" },
    {  0,245,255, "turquoise1" },
    {  0,229,238, "turquoise2" },
    {  0,197,205, "turquoise3" },
    {  0,134,139, "turquoise4" },
    {  0,255,255, "cyan1" },
    {  0,238,238, "cyan2" },
    {  0,205,205, "cyan3" },
    {  0,139,139, "cyan4" },
    {151,255,255, "DarkSlateGray1" },
    {141,238,238, "DarkSlateGray2" },
    {121,205,205, "DarkSlateGray3" },
    { 82,139,139, "DarkSlateGray4" },
    {127,255,212, "aquamarine1" },
    {118,238,198, "aquamarine2" },
    {102,205,170, "aquamarine3" },
    { 69,139,116, "aquamarine4" },
    {193,255,193, "DarkSeaGreen1" },
    {180,238,180, "DarkSeaGreen2" },
    {155,205,155, "DarkSeaGreen3" },
    {105,139,105, "DarkSeaGreen4" },
    { 84,255,159, "SeaGreen1" },
    { 78,238,148, "SeaGreen2" },
    { 67,205,128, "SeaGreen3" },
    { 46,139, 87, "SeaGreen4" },
    {154,255,154, "PaleGreen1" },
    {144,238,144, "PaleGreen2" },
    {124,205,124, "PaleGreen3" },
    { 84,139, 84, "PaleGreen4" },
    {  0,255,127, "SpringGreen1" },
    {  0,238,118, "SpringGreen2" },
    {  0,205,102, "SpringGreen3" },
    {  0,139, 69, "SpringGreen4" },
    {  0,255,  0, "green1" },
    {  0,238,  0, "green2" },
    {  0,205,  0, "green3" },
    {  0,139,  0, "green4" },
    {127,255,  0, "chartreuse1" },
    {118,238,  0, "chartreuse2" },
    {102,205,  0, "chartreuse3" },
    { 69,139,  0, "chartreuse4" },
    {192,255, 62, "OliveDrab1" },
    {179,238, 58, "OliveDrab2" },
    {154,205, 50, "OliveDrab3" },
    {105,139, 34, "OliveDrab4" },
    {202,255,112, "DarkOliveGreen1" },
    {188,238,104, "DarkOliveGreen2" },
    {162,205, 90, "DarkOliveGreen3" },
    {110,139, 61, "DarkOliveGreen4" },
    {255,246,143, "khaki1" },
    {238,230,133, "khaki2" },
    {205,198,115, "khaki3" },
    {139,134, 78, "khaki4" },
    {255,236,139, "LightGoldenrod1" },
    {238,220,130, "LightGoldenrod2" },
    {205,190,112, "LightGoldenrod3" },
    {139,129, 76, "LightGoldenrod4" },
    {255,255,224, "LightYellow1" },
    {238,238,209, "LightYellow2" },
    {205,205,180, "LightYellow3" },
    {139,139,122, "LightYellow4" },
    {255,255,  0, "yellow1" },
    {238,238,  0, "yellow2" },
    {205,205,  0, "yellow3" },
    {139,139,  0, "yellow4" },
    {255,215,  0, "gold1" },
    {238,201,  0, "gold2" },
    {205,173,  0, "gold3" },
    {139,117,  0, "gold4" },
    {255,193, 37, "goldenrod1" },
    {238,180, 34, "goldenrod2" },
    {205,155, 29, "goldenrod3" },
    {139,105, 20, "goldenrod4" },
    {255,185, 15, "DarkGoldenrod1" },
    {238,173, 14, "DarkGoldenrod2" },
    {205,149, 12, "DarkGoldenrod3" },
    {139,101,  8, "DarkGoldenrod4" },
    {255,193,193, "RosyBrown1" },
    {238,180,180, "RosyBrown2" },
    {205,155,155, "RosyBrown3" },
    {139,105,105, "RosyBrown4" },
    {255,106,106, "IndianRed1" },
    {238, 99, 99, "IndianRed2" },
    {205, 85, 85, "IndianRed3" },
    {139, 58, 58, "IndianRed4" },
    {255,130, 71, "sienna1" },
    {238,121, 66, "sienna2" },
    {205,104, 57, "sienna3" },
    {139, 71, 38, "sienna4" },
    {255,211,155, "burlywood1" },
    {238,197,145, "burlywood2" },
    {205,170,125, "burlywood3" },
    {139,115, 85, "burlywood4" },
    {255,231,186, "wheat1" },
    {238,216,174, "wheat2" },
    {205,186,150, "wheat3" },
    {139,126,102, "wheat4" },
    {255,165, 79, "tan1" },
    {238,154, 73, "tan2" },
    {205,133, 63, "tan3" },
    {139, 90, 43, "tan4" },
    {255,127, 36, "chocolate1" },
    {238,118, 33, "chocolate2" },
    {205,102, 29, "chocolate3" },
    {139, 69, 19, "chocolate4" },
    {255, 48, 48, "firebrick1" },
    {238, 44, 44, "firebrick2" },
    {205, 38, 38, "firebrick3" },
    {139, 26, 26, "firebrick4" },
    {255, 64, 64, "brown1" },
    {238, 59, 59, "brown2" },
    {205, 51, 51, "brown3" },
    {139, 35, 35, "brown4" },
    {255,140,105, "salmon1" },
    {238,130, 98, "salmon2" },
    {205,112, 84, "salmon3" },
    {139, 76, 57, "salmon4" },
    {255,160,122, "LightSalmon1" },
    {238,149,114, "LightSalmon2" },
    {205,129, 98, "LightSalmon3" },
    {139, 87, 66, "LightSalmon4" },
    {255,165,  0, "orange1" },
    {238,154,  0, "orange2" },
    {205,133,  0, "orange3" },
    {139, 90,  0, "orange4" },
    {255,127,  0, "DarkOrange1" },
    {238,118,  0, "DarkOrange2" },
    {205,102,  0, "DarkOrange3" },
    {139, 69,  0, "DarkOrange4" },
    {255,114, 86, "coral1" },
    {238,106, 80, "coral2" },
    {205, 91, 69, "coral3" },
    {139, 62, 47, "coral4" },
    {255, 99, 71, "tomato1" },
    {238, 92, 66, "tomato2" },
    {205, 79, 57, "tomato3" },
    {139, 54, 38, "tomato4" },
    {255, 69,  0, "OrangeRed1" },
    {238, 64,  0, "OrangeRed2" },
    {205, 55,  0, "OrangeRed3" },
    {139, 37,  0, "OrangeRed4" },
    {255,  0,  0, "red1" },
    {238,  0,  0, "red2" },
    {205,  0,  0, "red3" },
    {139,  0,  0, "red4" },
    {255, 20,147, "DeepPink1" },
    {238, 18,137, "DeepPink2" },
    {205, 16,118, "DeepPink3" },
    {139, 10, 80, "DeepPink4" },
    {255,110,180, "HotPink1" },
    {238,106,167, "HotPink2" },
    {205, 96,144, "HotPink3" },
    {139, 58, 98, "HotPink4" },
    {255,181,197, "pink1" },
    {238,169,184, "pink2" },
    {205,145,158, "pink3" },
    {139, 99,108, "pink4" },
    {255,174,185, "LightPink1" },
    {238,162,173, "LightPink2" },
    {205,140,149, "LightPink3" },
    {139, 95,101, "LightPink4" },
    {255,130,171, "PaleVioletRed1" },
    {238,121,159, "PaleVioletRed2" },
    {205,104,137, "PaleVioletRed3" },
    {139, 71, 93, "PaleVioletRed4" },
    {255, 52,179, "maroon1" },
    {238, 48,167, "maroon2" },
    {205, 41,144, "maroon3" },
    {139, 28, 98, "maroon4" },
    {255, 62,150, "VioletRed1" },
    {238, 58,140, "VioletRed2" },
    {205, 50,120, "VioletRed3" },
    {139, 34, 82, "VioletRed4" },
    {255,  0,255, "magenta1" },
    {238,  0,238, "magenta2" },
    {205,  0,205, "magenta3" },
    {139,  0,139, "magenta4" },
    {255,131,250, "orchid1" },
    {238,122,233, "orchid2" },
    {205,105,201, "orchid3" },
    {139, 71,137, "orchid4" },
    {255,187,255, "plum1" },
    {238,174,238, "plum2" },
    {205,150,205, "plum3" },
    {139,102,139, "plum4" },
    {224,102,255, "MediumOrchid1" },
    {209, 95,238, "MediumOrchid2" },
    {180, 82,205, "MediumOrchid3" },
    {122, 55,139, "MediumOrchid4" },
    {191, 62,255, "DarkOrchid1" },
    {178, 58,238, "DarkOrchid2" },
    {154, 50,205, "DarkOrchid3" },
    {104, 34,139, "DarkOrchid4" },
    {155, 48,255, "purple1" },
    {145, 44,238, "purple2" },
    {125, 38,205, "purple3" },
    { 85, 26,139, "purple4" },
    {171,130,255, "MediumPurple1" },
    {159,121,238, "MediumPurple2" },
    {137,104,205, "MediumPurple3" },
    { 93, 71,139, "MediumPurple4" },
    {255,225,255, "thistle1" },
    {238,210,238, "thistle2" },
    {205,181,205, "thistle3" },
    {139,123,139, "thistle4" },
    {  0,  0,  0, "gray0" },
    {  0,  0,  0, "grey0" },
    {  3,  3,  3, "gray1" },
    {  3,  3,  3, "grey1" },
    {  5,  5,  5, "gray2" },
    {  5,  5,  5, "grey2" },
    {  8,  8,  8, "gray3" },
    {  8,  8,  8, "grey3" },
    { 10, 10, 10, "gray4" },
    { 10, 10, 10, "grey4" },
    { 13, 13, 13, "gray5" },
    { 13, 13, 13, "grey5" },
    { 15, 15, 15, "gray6" },
    { 15, 15, 15, "grey6" },
    { 18, 18, 18, "gray7" },
    { 18, 18, 18, "grey7" },
    { 20, 20, 20, "gray8" },
    { 20, 20, 20, "grey8" },
    { 23, 23, 23, "gray9" },
    { 23, 23, 23, "grey9" },
    { 26, 26, 26, "gray10" },
    { 26, 26, 26, "grey10" },
    { 28, 28, 28, "gray11" },
    { 28, 28, 28, "grey11" },
    { 31, 31, 31, "gray12" },
    { 31, 31, 31, "grey12" },
    { 33, 33, 33, "gray13" },
    { 33, 33, 33, "grey13" },
    { 36, 36, 36, "gray14" },
    { 36, 36, 36, "grey14" },
    { 38, 38, 38, "gray15" },
    { 38, 38, 38, "grey15" },
    { 41, 41, 41, "gray16" },
    { 41, 41, 41, "grey16" },
    { 43, 43, 43, "gray17" },
    { 43, 43, 43, "grey17" },
    { 46, 46, 46, "gray18" },
    { 46, 46, 46, "grey18" },
    { 48, 48, 48, "gray19" },
    { 48, 48, 48, "grey19" },
    { 51, 51, 51, "gray20" },
    { 51, 51, 51, "grey20" },
    { 54, 54, 54, "gray21" },
    { 54, 54, 54, "grey21" },
    { 56, 56, 56, "gray22" },
    { 56, 56, 56, "grey22" },
    { 59, 59, 59, "gray23" },
    { 59, 59, 59, "grey23" },
    { 61, 61, 61, "gray24" },
    { 61, 61, 61, "grey24" },
    { 64, 64, 64, "gray25" },
    { 64, 64, 64, "grey25" },
    { 66, 66, 66, "gray26" },
    { 66, 66, 66, "grey26" },
    { 69, 69, 69, "gray27" },
    { 69, 69, 69, "grey27" },
    { 71, 71, 71, "gray28" },
    { 71, 71, 71, "grey28" },
    { 74, 74, 74, "gray29" },
    { 74, 74, 74, "grey29" },
    { 77, 77, 77, "gray30" },
    { 77, 77, 77, "grey30" },
    { 79, 79, 79, "gray31" },
    { 79, 79, 79, "grey31" },
    { 82, 82, 82, "gray32" },
    { 82, 82, 82, "grey32" },
    { 84, 84, 84, "gray33" },
    { 84, 84, 84, "grey33" },
    { 87, 87, 87, "gray34" },
    { 87, 87, 87, "grey34" },
    { 89, 89, 89, "gray35" },
    { 89, 89, 89, "grey35" },
    { 92, 92, 92, "gray36" },
    { 92, 92, 92, "grey36" },
    { 94, 94, 94, "gray37" },
    { 94, 94, 94, "grey37" },
    { 97, 97, 97, "gray38" },
    { 97, 97, 97, "grey38" },
    { 99, 99, 99, "gray39" },
    { 99, 99, 99, "grey39" },
    {102,102,102, "gray40" },
    {102,102,102, "grey40" },
    {105,105,105, "gray41" },
    {105,105,105, "grey41" },
    {107,107,107, "gray42" },
    {107,107,107, "grey42" },
    {110,110,110, "gray43" },
    {110,110,110, "grey43" },
    {112,112,112, "gray44" },
    {112,112,112, "grey44" },
    {115,115,115, "gray45" },
    {115,115,115, "grey45" },
    {117,117,117, "gray46" },
    {117,117,117, "grey46" },
    {120,120,120, "gray47" },
    {120,120,120, "grey47" },
    {122,122,122, "gray48" },
    {122,122,122, "grey48" },
    {125,125,125, "gray49" },
    {125,125,125, "grey49" },
    {127,127,127, "gray50" },
    {127,127,127, "grey50" },
    {130,130,130, "gray51" },
    {130,130,130, "grey51" },
    {133,133,133, "gray52" },
    {133,133,133, "grey52" },
    {135,135,135, "gray53" },
    {135,135,135, "grey53" },
    {138,138,138, "gray54" },
    {138,138,138, "grey54" },
    {140,140,140, "gray55" },
    {140,140,140, "grey55" },
    {143,143,143, "gray56" },
    {143,143,143, "grey56" },
    {145,145,145, "gray57" },
    {145,145,145, "grey57" },
    {148,148,148, "gray58" },
    {148,148,148, "grey58" },
    {150,150,150, "gray59" },
    {150,150,150, "grey59" },
    {153,153,153, "gray60" },
    {153,153,153, "grey60" },
    {156,156,156, "gray61" },
    {156,156,156, "grey61" },
    {158,158,158, "gray62" },
    {158,158,158, "grey62" },
    {161,161,161, "gray63" },
    {161,161,161, "grey63" },
    {163,163,163, "gray64" },
    {163,163,163, "grey64" },
    {166,166,166, "gray65" },
    {166,166,166, "grey65" },
    {168,168,168, "gray66" },
    {168,168,168, "grey66" },
    {171,171,171, "gray67" },
    {171,171,171, "grey67" },
    {173,173,173, "gray68" },
    {173,173,173, "grey68" },
    {176,176,176, "gray69" },
    {176,176,176, "grey69" },
    {179,179,179, "gray70" },
    {179,179,179, "grey70" },
    {181,181,181, "gray71" },
    {181,181,181, "grey71" },
    {184,184,184, "gray72" },
    {184,184,184, "grey72" },
    {186,186,186, "gray73" },
    {186,186,186, "grey73" },
    {189,189,189, "gray74" },
    {189,189,189, "grey74" },
    {191,191,191, "gray75" },
    {191,191,191, "grey75" },
    {194,194,194, "gray76" },
    {194,194,194, "grey76" },
    {196,196,196, "gray77" },
    {196,196,196, "grey77" },
    {199,199,199, "gray78" },
    {199,199,199, "grey78" },
    {201,201,201, "gray79" },
    {201,201,201, "grey79" },
    {204,204,204, "gray80" },
    {204,204,204, "grey80" },
    {207,207,207, "gray81" },
    {207,207,207, "grey81" },
    {209,209,209, "gray82" },
    {209,209,209, "grey82" },
    {212,212,212, "gray83" },
    {212,212,212, "grey83" },
    {214,214,214, "gray84" },
    {214,214,214, "grey84" },
    {217,217,217, "gray85" },
    {217,217,217, "grey85" },
    {219,219,219, "gray86" },
    {219,219,219, "grey86" },
    {222,222,222, "gray87" },
    {222,222,222, "grey87" },
    {224,224,224, "gray88" },
    {224,224,224, "grey88" },
    {227,227,227, "gray89" },
    {227,227,227, "grey89" },
    {229,229,229, "gray90" },
    {229,229,229, "grey90" },
    {232,232,232, "gray91" },
    {232,232,232, "grey91" },
    {235,235,235, "gray92" },
    {235,235,235, "grey92" },
    {237,237,237, "gray93" },
    {237,237,237, "grey93" },
    {240,240,240, "gray94" },
    {240,240,240, "grey94" },
    {242,242,242, "gray95" },
    {242,242,242, "grey95" },
    {245,245,245, "gray96" },
    {245,245,245, "grey96" },
    {247,247,247, "gray97" },
    {247,247,247, "grey97" },
    {250,250,250, "gray98" },
    {250,250,250, "grey98" },
    {252,252,252, "gray99" },
    {252,252,252, "grey99" },
    {255,255,255, "gray100" },
    {255,255,255, "grey100" },
    {169,169,169, "dark grey" },
    {169,169,169, "DarkGrey" },
    {169,169,169, "dark gray" },
    {169,169,169, "DarkGray" },
    {  0,  0,139, "dark blue" },
    {  0,  0,139, "DarkBlue" },
    {  0,139,139, "dark cyan" },
    {  0,139,139, "DarkCyan" },
    {139,  0,139, "dark magenta" },
    {139,  0,139, "DarkMagenta" },
    {139,  0,  0, "dark red" },
    {139,  0,  0, "DarkRed" },
    {144,238,144, "light green" },
    {144,238,144, "LightGreen" },
    {  0,  0,  0, NULL }
};

/******************/
/* Implementation */
/******************/

/********************************************************/
/* This routine tries to allocate a biggest possible    */
/* color cube (used only on 8 bit pseudo color visuals) */
/* Shamelessly ripped from colormaps.c by S&P.          */
/* I'm not sure if I'm going to use this or not.        */
/********************************************************/

int gck_allocate_color_cube(GckVisualInfo * visinfo, int red, int green, int blue)
{
  int init_r, init_g, init_b;
  int total;
  int success;

  g_function_enter("gck_allocate_color_cube");
  g_assert(visinfo!=NULL);

  init_r = red;
  init_g = green;
  init_b = blue;

  /* First, reduce number of total colors to fit a 8 bit LUT */
  /* ======================================================= */

  total = red * green * blue + RESERVED_COLORS;
  while (total > 256)
    {
      if (blue >= red && blue >= green)
	blue--;
      else if (red >= green && red >= blue)
	red--;
      else
	green--;

      total = red * green * blue + RESERVED_COLORS;
    }

  /* Now, attempt to allocate the colors. If no success, reduce the */
  /* color cube resolution and try again.                           */
  /* ============================================================== */

  success = gdk_colors_alloc(visinfo->colormap, 0, NULL, 0, visinfo->allocedpixels, total);
  while (!success)
    {
      if (blue >= red && blue >= green)
	blue--;
      else if (red >= green && red >= blue)
	red--;
      else
	green--;

      total = red * green * blue + RESERVED_COLORS;
      if (red <= 2 || green <= 2 || blue <= 2)
	success = 1;
      else
	success = gdk_colors_alloc(visinfo->colormap, 0, NULL, 0, visinfo->allocedpixels, total);
    }

  /* If any shades value has been reduced to nothing, return error flag */
  /* ================================================================== */

  if (red > 1 && green > 1 && blue > 1)
    {
      success=TRUE;
      visinfo->shades_r = red;
      visinfo->shades_g = green;
      visinfo->shades_b = blue;
      visinfo->numcolors = total;
    }
  else success=FALSE;

  g_function_leave("gck_allocate_color_cube");
  return(success);
}

/**************************************************/
/* Create 8 bit RGB color cube. Also more or less */
/* ripped from colormaps.c by S&P.                */
/**************************************************/

void gck_create_8bit_rgb(GckVisualInfo * visinfo)
{
  unsigned int r, g, b;
  unsigned int dr, dg, db;
  int i = RESERVED_COLORS;

  g_function_enter("gck_create_8bit_rgb");
  g_assert(visinfo!=NULL);

  dr = (visinfo->shades_r > 1) ? (visinfo->shades_r - 1) : (1);
  dg = (visinfo->shades_g > 1) ? (visinfo->shades_g - 1) : (1);
  db = (visinfo->shades_b > 1) ? (visinfo->shades_b - 1) : (1);

  for (r = 0; r < visinfo->shades_r; r++)
    for (g = 0; g < visinfo->shades_g; g++)
      for (b = 0; b < visinfo->shades_b; b++)
	{
	  visinfo->colorcube[i] = visinfo->allocedpixels[i];

	  visinfo->rgbpalette[i].red = (guint) ROUND_TO_INT(255.0 * (double)(r * visinfo->visual->colormap_size) / (double)dr);
	  visinfo->rgbpalette[i].green = (guint) ROUND_TO_INT(255.0 * (double)(g * visinfo->visual->colormap_size) / (double)dg);
	  visinfo->rgbpalette[i].blue = (guint) ROUND_TO_INT(255.0 * (double)(b * visinfo->visual->colormap_size) / (double)db);
	  visinfo->rgbpalette[i].pixel = visinfo->allocedpixels[i];
	  visinfo->indextab[r][g][b] = (guchar) visinfo->allocedpixels[i];
	  i++;
	}

  /* Set up mapping tables */
  /* ===================== */

  for (i = 0; i < 256; i++)
    {
      visinfo->map_r[i] = (int)ROUND_TO_INT(((double)(visinfo->shades_r - 1) * ((double)i / 255.0)));
      visinfo->map_g[i] = (int)ROUND_TO_INT(((double)(visinfo->shades_g - 1) * ((double)i / 255.0)));
      visinfo->map_b[i] = (int)ROUND_TO_INT(((double)(visinfo->shades_b - 1) * ((double)i / 255.0)));
      visinfo->invmap_r[i] = (double)visinfo->map_r[i]*(255.0/(double)(visinfo->shades_r - 1));
      visinfo->invmap_g[i] = (double)visinfo->map_g[i]*(255.0/(double)(visinfo->shades_g - 1));
      visinfo->invmap_b[i] = (double)visinfo->map_b[i]*(255.0/(double)(visinfo->shades_b - 1));
    }

  /* Create reserved colors */
  /* ====================== */

  visinfo->rgbpalette[0].red = 0;
  visinfo->rgbpalette[0].green = 0;
  visinfo->rgbpalette[0].blue = 0;
  visinfo->rgbpalette[0].pixel = visinfo->allocedpixels[0];

  visinfo->rgbpalette[1].red = 65535;
  visinfo->rgbpalette[1].green = 65535;
  visinfo->rgbpalette[1].blue = 65535;
  visinfo->rgbpalette[1].pixel = visinfo->allocedpixels[1];

  g_function_leave("gck_create_8bit_rgb");
}

/**********************************/
/* Get visual and create colormap */
/**********************************/

GckVisualInfo *gck_visualinfo_new(void)
{
  GckVisualInfo *visinfo;

  g_function_enter("gck_visualinfo_new");

  visinfo = (GckVisualInfo *) malloc(sizeof(GckVisualInfo));
  if (visinfo!=NULL)
    {
      visinfo->visual = gdk_visual_get_best();
      visinfo->colormap = gdk_colormap_new(visinfo->visual, FALSE);
      visinfo->dithermethod = DITHER_FLOYD_STEINBERG;
    
      if (visinfo->visual->type == GDK_VISUAL_PSEUDO_COLOR)
        {
          /* Allocate colormap and create RGB map */
          /* ==================================== */
    
          if (gck_allocate_color_cube(visinfo, 6, 6, 6) == TRUE)
	    {
	      gck_create_8bit_rgb(visinfo);
	      gdk_colors_store(visinfo->colormap, visinfo->rgbpalette, visinfo->numcolors);
	    }
          else
	    {
	      free(visinfo);
	      visinfo = NULL;
	    }
        }
    }

  g_function_leave("gck_visualinfo_new");
  return (visinfo);
}

/***********************************************************/
/* Free memory associated with the GckVisualInfo structure */
/***********************************************************/

void gck_visualinfo_destroy(GckVisualInfo * visinfo)
{
  g_function_enter("gck_visualinfo_destroy");
  g_assert(visinfo!=NULL);

  gdk_colormap_unref(visinfo->colormap);

  free(visinfo);

  g_function_leave("gck_visualinfo_destroy");
}

/*************************************************/
/* This converts from TrueColor RGB (24bpp) to   */
/* whatever format the visual of our image uses. */
/*************************************************/

GckDitherType gck_visualinfo_get_dither(GckVisualInfo * visinfo)
{
  g_function_enter("gck_visualinfo_get_dither");
  g_assert(visinfo!=NULL);  
  g_function_leave("gck_visualinfo_get_dither");
  return (visinfo->dithermethod);
}

void gck_visualinfo_set_dither(GckVisualInfo * visinfo, GckDitherType method)
{
  g_function_enter("gck_visualinfo_set_dither");
  g_assert(visinfo!=NULL);
  visinfo->dithermethod = method;
  g_function_leave("gck_visualinfo_set_dither");
}

/*******************/
/* GdkGC functions */
/*******************/

void gck_gc_set_foreground(GckVisualInfo *visinfo,GdkGC *gc,
                           guchar r, guchar g, guchar b)
{
  GdkColor *color;
  
  g_function_enter("gck_gc_set_foreground");
  g_assert(visinfo!=NULL);
  g_assert(gc!=NULL);

  color=gck_rgb_to_gdkcolor(visinfo,r,g,b);
  gdk_gc_set_foreground(gc,color);
  free(color);

  g_function_leave("gck_gc_set_foreground");
}

void gck_gc_set_background(GckVisualInfo *visinfo,GdkGC *gc,
                           guchar r, guchar g, guchar b)
{
  GdkColor *color;
  
  g_function_enter("gck_gc_set_background");
  g_assert(visinfo!=NULL);
  g_assert(gc!=NULL);

  color=gck_rgb_to_gdkcolor(visinfo,r,g,b);
  gdk_gc_set_background(gc,color);
  free(color);

  g_function_leave("gck_gc_set_background");
}

/*************************************************/
/* RGB to 8 bpp pseudocolor (indexed) functions. */
/*************************************************/

GdkColor *gck_rgb_to_color8(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  gint index;
  GdkColor *color;

  g_function_enter("gck_rgb_to_color8");
  g_assert(visinfo!=NULL);

  color=(GdkColor *)malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  r = visinfo->map_r[r];
  g = visinfo->map_g[g];
  b = visinfo->map_b[b];
  index = visinfo->indextab[r][g][b];
  *color=visinfo->rgbpalette[index];

  g_function_leave("gck_rgb_to_color8");
  return (color);
}

/***************************************************/
/* RGB to 8 bpp pseudocolor using error-diffusion  */
/* dithering using the weights proposed by Floyd   */
/* and Steinberg (aka "Floyd-Steinberg dithering") */
/***************************************************/

void gck_rgb_to_image8_fs_dither(GckVisualInfo * visinfo, guchar * RGB_data, GdkImage * image,
				 int width, int height)
{
  guchar *imagedata;
  gint or, og, ob, mr, mg, mb, dr, dg, db;
  gint *row1, *row2, *temp, rowcnt;
  int xcnt, ycnt, diffx;
  long count = 0, rowsize;

  g_function_enter("gck_rgb_to_image8_fs_dither");
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  /* Allocate memory for FS errors */
  /* ============================= */

  rowsize = 3 * width;
  row1 = (gint *) malloc(sizeof(gint) * (size_t) rowsize);
  row2 = (gint *) malloc(sizeof(gint) * (size_t) rowsize);

  /* Initialize to zero */
  /* ================== */

  memset(row1, 0, 3 * sizeof(gint) * width);
  memset(row2, 0, 3 * sizeof(gint) * width);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  if (image->width < width)
    width = image->width;
  if (image->height < height)
    height = image->height;

  imagedata = (guchar *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      /* It's rec. to move from left to right on even  */
      /* rows and right to left on odd rows, so we do. */
      /* ============================================= */

      if ((ycnt % 1) == 0)
	{
	  rowcnt = 0;
	  for (xcnt = 0; xcnt < width; xcnt++)
	    {
	      /* Get exact (original) value */
	      /* ========================== */

	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      /* Extract and add the accumulated error for this pixel */
	      /* ==================================================== */

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      /* Make sure we don't run into an under- or overflow */
	      /* ================================================= */

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      /* Compute difference */
	      /* ================== */

	      dr = or - (gint) visinfo->invmap_r[or];
	      dg = og - (gint) visinfo->invmap_g[og];
	      db = ob - (gint) visinfo->invmap_b[ob];

	      /* Spread the error to the neighboring pixels.    */
	      /* We use the weights proposed by Floyd-Steinberg */
	      /* for 3x3 neighborhoods (1*, 3*, 5* and 7*1/16). */
	      /* ============================================== */

	      if (xcnt < width - 1)
		{
		  row1[(rowcnt + 3)] += 7 * (gint) dr;
		  row1[(rowcnt + 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt + 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt + 3)] += (gint) dr;
		      row2[(rowcnt + 3) + 1] += (gint) dg;
		      row2[(rowcnt + 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt > 0 && ycnt < height - 1)
		{
		  row2[(rowcnt - 3)] += 3 * (gint) dr;
		  row2[(rowcnt - 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt - 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      /* Clear the errorvalues of the processed row */
	      /* ========================================== */

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      /* Map RGB values to color cube and write pixel */
	      /* ============================================ */

	      mr = visinfo->map_r[(guchar) or];
	      mg = visinfo->map_g[(guchar) og];
	      mb = visinfo->map_b[(guchar) ob];

	      imagedata[xcnt] = visinfo->indextab[mr][mg][mb];
	      rowcnt += 3;
	    }
	}
      else
	{
	  rowcnt = rowsize - 3;
	  for (xcnt = width - 1; xcnt >= 0; xcnt--)
	    {
	      /* Same as above but in the other direction */
	      /* ======================================== */

	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      dr = or - (gint) visinfo->invmap_r[or];
	      dg = og - (gint) visinfo->invmap_g[og];
	      db = ob - (gint) visinfo->invmap_b[ob];

	      if (xcnt > 0)
		{
		  row1[(rowcnt - 3)] += 7 * (gint) dr;
		  row1[(rowcnt - 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt - 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt - 3)] += (gint) dr;
		      row2[(rowcnt - 3) + 1] += (gint) dg;
		      row2[(rowcnt - 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt < width - 1 && ycnt < height - 1)
		{
		  row2[(rowcnt + 3)] += 3 * (gint) dr;
		  row2[(rowcnt + 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt + 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      mr = visinfo->map_r[(guchar) or];
	      mg = visinfo->map_g[(guchar) og];
	      mb = visinfo->map_b[(guchar) ob];

	      imagedata[xcnt] = visinfo->indextab[mr][mg][mb];
	      rowcnt -= 3;
	    }
	}

      /* We're finished with this row, swap row-pointers */
      /* =============================================== */

      temp = row1;
      row1 = row2;
      row2 = temp;

      imagedata += width + diffx;
      count += rowsize;
    }

  free(row1);
  free(row2);

  g_function_leave("gck_rgb_to_image8_fs_dither");
}

/***********************************************************/
/* Plain (no dithering) RGB to 8 bpp pseudocolor (indexed) */
/***********************************************************/

void gck_rgb_to_image8(GckVisualInfo * visinfo,
                       guchar * RGB_data,
                       GdkImage * image,
                       int width, int height)
{
  guchar *imagedata, r, g, b;
  int xcnt, ycnt, diffx;
  long count = 0;

  g_function_enter("gck_rgb_to_image8");
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  imagedata = (guchar *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < width; xcnt++)
	{
	  if (xcnt < image->width && ycnt < image->height)
	    {
	      r = RGB_data[count];
	      g = RGB_data[count + 1];
	      b = RGB_data[count + 2];

	      r = visinfo->map_r[r];
	      g = visinfo->map_g[g];
	      b = visinfo->map_b[b];

	      *imagedata = visinfo->indextab[r][g][b];
	      imagedata++;
	    }
	  count += 3;
	}
      imagedata += diffx;
    }

  g_function_leave("gck_rgb_to_image8");
}

/************************************/
/* RGB to 16/15 bpp RGB ("HiColor") */
/************************************/

GdkColor *gck_rgb_to_color16(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  guint32 red, green, blue;
  GdkColor *color;

  g_function_enter("gck_rgb_to_color16");
  g_assert(visinfo!=NULL);

  color=(GdkColor *)malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  color->red = ((guint16) r) << 8;
  color->green = ((guint16) g) << 8;
  color->blue = ((guint16) b) << 8;

  red = ((guint32) r) >> (8 - visinfo->visual->red_prec);
  green = ((guint32) g) >> (8 - visinfo->visual->green_prec);
  blue = ((guint32) b) >> (8 - visinfo->visual->blue_prec);

  red = red << visinfo->visual->red_shift;
  green = green << visinfo->visual->green_shift;
  blue = blue << visinfo->visual->blue_shift;

  color->pixel = red | green | blue;

  g_function_leave("gck_rgb_to_color16");
  return (color);
}

/***************************************************/
/* RGB to 16 bpp truecolor using error-diffusion   */
/* dithering using the weights proposed by Floyd   */
/* and Steinberg (aka "Floyd-Steinberg dithering") */
/***************************************************/

void gck_rgb_to_image16_fs_dither(GckVisualInfo * visinfo,
                                  guchar * RGB_data,
                                  GdkImage * image,
                                  int width, int height)
{
  guint16 *imagedata, pixel;
  gint16 or, og, ob, dr, dg, db;
  gint16 *row1, *row2, *temp, rowcnt, rmask, gmask, bmask;
  int xcnt, ycnt, diffx;
  long count = 0, rowsize;

  g_function_enter("gck_rgb_to_image16_fs_dither");
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  /* Allocate memory for FS errors */
  /* ============================= */

  rowsize = 3 * width;
  row1 = (gint16 *) malloc(sizeof(gint16) * (size_t) rowsize);
  row2 = (gint16 *) malloc(sizeof(gint16) * (size_t) rowsize);

  /* Initialize to zero */
  /* ================== */

  memset(row1, 0, 3 * sizeof(gint16) * width);
  memset(row2, 0, 3 * sizeof(gint16) * width);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  if (image->width < width)
    width = image->width;
  if (image->height < height)
    height = image->height;

  rmask = (0xff << (8 - visinfo->visual->red_prec)) & 0xff;
  gmask = (0xff << (8 - visinfo->visual->green_prec)) & 0xff;
  bmask = (0xff << (8 - visinfo->visual->blue_prec)) & 0xff;

  imagedata = (guint16 *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      /* It is rec. to move from left to right on even */
      /* rows and right to left on odd rows, so we do. */
      /* ============================================= */

      if ((ycnt % 1) == 0)
	{
	  rowcnt = 0;
	  for (xcnt = 0; xcnt < width; xcnt++)
	    {
	      /* Get exact (original) value */
	      /* ========================== */

	      pixel = 0;
	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      /* Extract and add the accumulated error for this pixel */
	      /* ==================================================== */

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      /* Make sure we don't run into an under- or overflow */
	      /* ================================================= */

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      /* Compute difference */
	      /* ================== */

	      dr = or - (or & rmask);
	      dg = og - (og & gmask);
	      db = ob - (ob & bmask);

	      /* Spread the error to the neighboring pixels.    */
	      /* We use the weights proposed by Floyd-Steinberg */
	      /* for 3x3 neighborhoods (1*, 3*, 5* and 7*1/16). */
	      /* ============================================== */

	      if (xcnt < width - 1)
		{
		  row1[(rowcnt + 3)] += 7 * (gint) dr;
		  row1[(rowcnt + 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt + 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt + 3)] += (gint) dr;
		      row2[(rowcnt + 3) + 1] += (gint) dg;
		      row2[(rowcnt + 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt > 0 && ycnt < height - 1)
		{
		  row2[(rowcnt - 3)] += 3 * (gint) dr;
		  row2[(rowcnt - 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt - 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      /* Clear the errorvalues of the processed row */
	      /* ========================================== */

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      or = or >> (8 - visinfo->visual->red_prec);
	      og = og >> (8 - visinfo->visual->green_prec);
	      ob = ob >> (8 - visinfo->visual->blue_prec);

	      or = or << visinfo->visual->red_shift;
	      og = og << visinfo->visual->green_shift;
	      ob = ob << visinfo->visual->blue_shift;

	      pixel = pixel | or | og | ob;

	      imagedata[xcnt] = pixel;
	      rowcnt += 3;
	    }
	}
      else
	{
	  rowcnt = rowsize - 3;
	  for (xcnt = width - 1; xcnt >= 0; xcnt--)
	    {
	      /* Same as above but in the other direction */
	      /* ======================================== */

	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      dr = or - (or & rmask);
	      dg = og - (og & gmask);
	      db = ob - (ob & bmask);

	      if (xcnt > 0)
		{
		  row1[(rowcnt - 3)] += 7 * (gint) dr;
		  row1[(rowcnt - 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt - 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt - 3)] += (gint) dr;
		      row2[(rowcnt - 3) + 1] += (gint) dg;
		      row2[(rowcnt - 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt < width - 1 && ycnt < height - 1)
		{
		  row2[(rowcnt + 3)] += 3 * (gint) dr;
		  row2[(rowcnt + 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt + 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      or = or >> (8 - visinfo->visual->red_prec);
	      og = og >> (8 - visinfo->visual->green_prec);
	      ob = ob >> (8 - visinfo->visual->blue_prec);

	      or = or << visinfo->visual->red_shift;
	      og = og << visinfo->visual->green_shift;
	      ob = ob << visinfo->visual->blue_shift;

	      pixel = pixel | or | og | ob;

	      imagedata[xcnt] = pixel;
	      rowcnt -= 3;
	    }
	}

      /* We're finished with this row, swap row-pointers */
      /* =============================================== */

      temp = row1;
      row1 = row2;
      row2 = temp;

      imagedata += width + diffx;
      count += rowsize;
    }

  free(row1);
  free(row2);

  g_function_leave("gck_rgb_to_image16_fs_dither");
}

void gck_rgb_to_image16(GckVisualInfo * visinfo,
                        guchar * RGB_data,
                        GdkImage * image,
                        int width, int height)
{
  guint16 *imagedata, pixel, r, g, b;
  int xcnt, ycnt, diffx;
  long count = 0;

  g_function_enter("gck_rgb_to_image16");
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  imagedata = (guint16 *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < width; xcnt++)
	{
	  if (xcnt <= image->width && ycnt <= image->height)
	    {
	      pixel = 0;
	      r = ((guint16) RGB_data[count++]) >> (8 - visinfo->visual->red_prec);
	      g = ((guint16) RGB_data[count++]) >> (8 - visinfo->visual->green_prec);
	      b = ((guint16) RGB_data[count++]) >> (8 - visinfo->visual->blue_prec);

	      r = r << visinfo->visual->red_shift;
	      g = g << visinfo->visual->green_shift;
	      b = b << visinfo->visual->blue_shift;

	      pixel = pixel | r | g | b;

	      *imagedata = pixel;
	      imagedata++;
	    }
	}
      imagedata += diffx;
    }
  g_function_leave("gck_rgb_to_image16");
}

/************************/
/* RGB to RGB (sic!) :) */
/************************/

GdkColor *gck_rgb_to_color24(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  guint32 red, green, blue;
  GdkColor *color;

  g_function_enter("gck_rgb_to_color24");
  g_assert(visinfo!=NULL);

  color=(GdkColor *)malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  color->red = ((guint16) r) << 8;
  color->green = ((guint16) g) << 8;
  color->blue = ((guint16) b) << 8;

  red = ((guint32) r << 16);
  green = ((guint32) g) << 8;
  blue = ((guint32) b);

  color->pixel = red | green | blue;

  g_function_leave("gck_rgb_to_color24");
  return (color);
}

void gck_rgb_to_image24(GckVisualInfo * visinfo,
                        guchar * RGB_data,
                        GdkImage * image,
                        int width, int height)
{
  guchar *imagedata;
  int xcnt, ycnt, diffx;
  long count = 0, count2 = 0;

  g_function_enter("gck_rgb_to_image24");
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = 3 * (image->width - width);
  else
    diffx = 0;

  imagedata = (guchar *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < height; xcnt++)
	{
	  if (xcnt < image->width && ycnt < image->height)
	    {
	      imagedata[count2++] = RGB_data[count + 2];
	      imagedata[count2++] = RGB_data[count + 1];
	      imagedata[count2++] = RGB_data[count];
	    }
	  count += 3;
	}
      count2 += diffx;
    }
  g_function_leave("gck_rgb_to_image24");
}

/***************/
/* RGB to RGBX */
/***************/

GdkColor *gck_rgb_to_color32(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  guint32 red, green, blue;
  GdkColor *color;

  g_function_enter("gck_rgb_to_color32");
  g_assert(visinfo!=NULL);

  color=(GdkColor *)malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  color->red = ((guint16) r) << 8;
  color->green = ((guint16) g) << 8;
  color->blue = ((guint16) b) << 8;

  red = ((guint32) r) << 8;
  green = ((guint32) g) << 16;
  blue = ((guint32) b) << 24;

  color->pixel = red | green | blue;

  g_function_leave("gck_rgb_to_color32");
  return (color);
}

void gck_rgb_to_image32(GckVisualInfo * visinfo,
                        guchar * RGB_data,
                        GdkImage * image,
                        int width, int height)
{
  guint32 *imagedata, pixel, r, g, b;
  int xcnt, ycnt, diffx=0;
  long count = 0;

  g_function_enter("gck_rgb_to_image32");
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = image->width - width;

  imagedata = (guint32 *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < width; xcnt++)
	{
	  if (xcnt < image->width && ycnt < image->height)
	    {
	      r = (guint32) RGB_data[count++];
	      g = (guint32) RGB_data[count++];
	      b = (guint32) RGB_data[count++];

	      /* changed to work on 32 bit displays */
	      r = r << 16;
	      g = g << 8;
	      b = b;

	      pixel = r | g | b;
	      *imagedata = pixel;
	      imagedata++;
	    }
	}
      imagedata += diffx;
    }

  g_function_leave("gck_rgb_to_image32");
}

/**************************/
/* Conversion dispatchers */
/**************************/

void gck_rgb_to_gdkimage(GckVisualInfo * visinfo,
                         guchar * RGB_data,
                         GdkImage * image,
                         int width, int height)
{
  g_function_enter("gck_rgb_to_gdkimage");
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (visinfo->visual->type == GDK_VISUAL_PSEUDO_COLOR)
    {
      if (visinfo->visual->depth == 8)
	{
	  /* Standard 256 color display */
	  /* ========================== */

	  if (visinfo->dithermethod == DITHER_NONE)
	    gck_rgb_to_image8(visinfo, RGB_data, image, width, height);
	  else if (visinfo->dithermethod == DITHER_FLOYD_STEINBERG)
	    gck_rgb_to_image8_fs_dither(visinfo, RGB_data, image, width, height);
	}
    }
  else if (visinfo->visual->type == GDK_VISUAL_TRUE_COLOR ||
	 visinfo->visual->type == GDK_VISUAL_DIRECT_COLOR)
    {
      if (visinfo->visual->depth == 15 || visinfo->visual->depth == 16)
	{
	  /* HiColor modes */
	  /* ============= */

	  if (visinfo->dithermethod == DITHER_NONE)
	    gck_rgb_to_image16(visinfo, RGB_data, image, width, height);
	  else if (visinfo->dithermethod == DITHER_FLOYD_STEINBERG)
            gck_rgb_to_image16_fs_dither(visinfo, RGB_data, image, width, height);
	}
      else if (visinfo->visual->depth == 24 && image->bpp==3)
	{
	  /* Packed 24 bit mode */
	  /* ================== */

	  gck_rgb_to_image24(visinfo, RGB_data, image, width, height);
	}
      else if (visinfo->visual->depth == 32 || (visinfo->visual->depth == 24 && image->bpp==4))
	{
	  /* 32 bpp mode */
	  /* =========== */

	  gck_rgb_to_image32(visinfo, RGB_data, image, width, height);
	}
    }
  g_function_leave("gck_rgb_to_gdkimage");
}

GdkColor *gck_rgb_to_gdkcolor(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  GdkColor *color=NULL;

  g_function_enter("gck_rgb_to_gdkcolor");
  g_assert(visinfo!=NULL);

  if (visinfo->visual->type == GDK_VISUAL_PSEUDO_COLOR)
    {
      if (visinfo->visual->depth == 8)
	{
	  /* Standard 256 color display */
	  /* ========================== */

	  color=gck_rgb_to_color8(visinfo, r, g, b);
	}
    }
  else if (visinfo->visual->type == GDK_VISUAL_TRUE_COLOR ||
	 visinfo->visual->type == GDK_VISUAL_DIRECT_COLOR)
    {
      if (visinfo->visual->depth == 15 || visinfo->visual->depth == 16)
	{
	  /* HiColor modes */
	  /* ============= */

	  color=gck_rgb_to_color16(visinfo, r, g, b);
	}
      else if (visinfo->visual->depth == 24)
	{
	  /* Normal 24 bit mode */
	  /* ================== */

	  color=gck_rgb_to_color24(visinfo, r, g, b);
	}
      else if (visinfo->visual->depth == 32)
	{
	  /* 32 bpp mode */
	  /* =========== */

	  color=gck_rgb_to_color32(visinfo, r, g, b);
	}
    }

  g_function_leave("gck_rgb_to_gdkcolor");
  return (color);
}

/********************/
/* Color operations */
/********************/

/******************************************/
/* Bilinear interpolation stuff (Quartic) */
/******************************************/

double gck_bilinear(double x, double y, double *values)
{
  double xx, yy, m0, m1;

  g_function_enter("gck_bilinear");
  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  g_function_leave("gck_bilinear");
  return ((1.0 - yy) * m0 + yy * m1);
}

guchar gck_bilinear_8(double x, double y, guchar * values)
{
  double xx, yy, m0, m1;

  g_function_enter("gck_bilinear_8");
  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  g_function_leave("gck_bilinear_8");
  return ((guchar) ((1.0 - yy) * m0 + yy * m1));
}

guint16 gck_bilinear_16(double x, double y, guint16 * values)
{
  double xx, yy, m0, m1;

  g_function_leave("gck_bilinear_16");
  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  g_function_leave("gck_bilinear_16");
  return ((guint16) ((1.0 - yy) * m0 + yy * m1));
}

guint32 gck_bilinear_32(double x, double y, guint32 * values)
{
  double xx, yy, m0, m1;

  g_function_enter("gck_bilinear_32");
  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  g_function_leave("gck_bilinear_32");
  return ((guint32) ((1.0 - yy) * m0 + yy * m1));
}

GckRGB gck_bilinear_rgb(double x, double y, GckRGB *values)
{
  double m0, m1;
  double ix, iy;
  GckRGB v;

  g_function_enter("gck_bilinear_rgb");
  g_assert(values!=NULL);

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  /* Red */
  /* === */

  m0 = ix * values[0].r + x * values[1].r;
  m1 = ix * values[2].r + x * values[3].r;

  v.r = iy * m0 + y * m1;

  /* Green */
  /* ===== */

  m0 = ix * values[0].g + x * values[1].g;
  m1 = ix * values[2].g + x * values[3].g;

  v.g = iy * m0 + y * m1;

  /* Blue */
  /* ==== */

  m0 = ix * values[0].b + x * values[1].b;
  m1 = ix * values[2].b + x * values[3].b;

  v.b = iy * m0 + y * m1;

  g_function_leave("gck_bilinear_rgb");
  return (v);
}				/* bilinear */

GckRGB gck_bilinear_rgba(double x, double y, GckRGB *values)
{
  double m0, m1;
  double ix, iy;
  GckRGB v;

  g_function_enter("gck_bilinear_rgba");
  g_assert(values!=NULL);

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  /* Red */
  /* === */

  m0 = ix * values[0].r + x * values[1].r;
  m1 = ix * values[2].r + x * values[3].r;

  v.r = iy * m0 + y * m1;

  /* Green */
  /* ===== */

  m0 = ix * values[0].g + x * values[1].g;
  m1 = ix * values[2].g + x * values[3].g;

  v.g = iy * m0 + y * m1;

  /* Blue */
  /* ==== */

  m0 = ix * values[0].b + x * values[1].b;
  m1 = ix * values[2].b + x * values[3].b;

  v.b = iy * m0 + y * m1;

  /* Alpha */
  /* ===== */

  m0 = ix * values[0].a + x * values[1].a;
  m1 = ix * values[2].a + x * values[3].a;

  v.a = iy * m0 + y * m1;

  g_function_leave("gck_bilinear_rgba");
  return (v);
}				/* bilinear */

/********************************/
/* Multiple channels operations */
/********************************/

void gck_rgb_add(GckRGB * p, GckRGB * q)
{
  g_function_enter("gck_rgb_add");
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r + q->r;
  p->g = p->g + q->g;
  p->b = p->b + q->b;
  
  g_function_leave("gck_rgb_add");
}

void gck_rgb_sub(GckRGB * p, GckRGB * q)
{
  g_function_enter("gck_rgb_sub");
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r - q->r;
  p->g = p->g - q->g;
  p->b = p->b - q->b;
  
  g_function_leave("gck_rgb_sub");
}

void gck_rgb_mul(GckRGB * p, double b)
{
  g_function_enter("gck_rgb_mul");
  g_assert(p!=NULL);

  p->r = p->r * b;
  p->g = p->g * b;
  p->b = p->b * b;

  g_function_leave("gck_rgb_mul");
}

double gck_rgb_dist(GckRGB * p, GckRGB * q)
{
  g_function_enter("gck_rgb_dist");
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  g_function_leave("gck_rgb_dist");
  return (fabs(p->r - q->r) + fabs(p->g - q->g) + fabs(p->b - q->b));
}

double gck_rgb_max(GckRGB * p)
{
  double max;

  g_function_enter("gck_rgb_max");
  g_assert(p!=NULL);

  max = p->r;
  if (p->g > max)
    max = p->g;
  if (p->b > max)
    max = p->b;

  g_function_leave("gck_rgb_max");
  return (max);
}

double gck_rgb_min(GckRGB * p)
{
  double min;

  g_function_enter("gck_rgb_min");
  g_assert(p!=NULL);

  min=p->r;
  if (p->g < min)
    min = p->g;
  if (p->b < min)
    min = p->b;

  g_function_leave(" gck_rgb_min");
  return (min);
}

void gck_rgb_clamp(GckRGB * p)
{
  g_function_enter("gck_rgb_clamp");
  g_assert(p!=NULL);

  if (p->r > 1.0)
    p->r = 1.0;
  if (p->g > 1.0)
    p->g = 1.0;
  if (p->b > 1.0)
    p->b = 1.0;
  if (p->r < 0.0)
    p->r = 0.0;
  if (p->g < 0.0)
    p->g = 0.0;
  if (p->b < 0.0)
    p->b = 0.0;

  g_function_leave("gck_rgb_clamp");
}

void gck_rgb_set(GckRGB * p, double r, double g, double b)
{
  g_function_enter("gck_rgb_set");
  g_assert(p!=NULL);

  p->r = r;
  p->g = g;
  p->b = b;

  g_function_leave("gck_rgb_set");
}

void gck_rgb_gamma(GckRGB * p, double gamma)
{
  double ig;

  g_function_enter("gck_rgb_gamma");
  g_assert(p!=NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    (ig = 0.0);

  p->r = pow(p->r, ig);
  p->g = pow(p->g, ig);
  p->b = pow(p->b, ig);

  g_function_leave("gck_rgb_gamma");
}

void gck_rgba_add(GckRGB * p, GckRGB * q)
{
  g_function_enter("gck_rgba_add");
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r + q->r;
  p->g = p->g + q->g;
  p->b = p->b + q->b;
  p->a = p->a + q->a;

  g_function_leave("gck_rgba_add");
}

void gck_rgba_sub(GckRGB * p, GckRGB * q)
{
  g_function_enter("gck_rgba_sub");
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r - q->r;
  p->g = p->g - q->g;
  p->b = p->b - q->b;
  p->a = p->a - q->a;

  g_function_leave("gck_rgba_add");
}

void gck_rgba_mul(GckRGB * p, double b)
{
  g_function_enter("gck_rgba_mul");
  g_assert(p!=NULL);

  p->r = p->r * b;
  p->g = p->g * b;
  p->b = p->b * b;
  p->a = p->a * b;

  g_function_leave("gck_rgba_mul");
}

double gck_rgba_dist(GckRGB *p, GckRGB *q)
{
  g_function_enter("gck_rgba_dist");
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  g_function_leave("gck_rgba_dist");
  return (fabs(p->r - q->r) + fabs(p->g - q->g) +
	  fabs(p->b - q->b) + fabs(p->a - q->a));
}

/* These two are probably not needed */

double gck_rgba_max(GckRGB * p)
{
  double max;

  g_function_enter("gck_rgba_max");
  g_assert(p!=NULL);

  max = p->r;

  if (p->g > max)
    max = p->g;
  if (p->b > max)
    max = p->b;
  if (p->a > max)
    max = p->a;

  g_function_leave("gck_rgba_max");
  return (max);
}

double gck_rgba_min(GckRGB * p)
{
  double min;

  g_function_enter("gck_rgba_min");
  g_assert(p!=NULL);
  
  min = p->r;
  if (p->g < min)
    min = p->g;
  if (p->b < min)
    min = p->b;
  if (p->a < min)
    min = p->a;

  g_function_leave("gck_rgba_min");
  return (min);
}

void gck_rgba_clamp(GckRGB * p)
{
  g_function_enter("gck_rgba_clamp");
  g_assert(p!=NULL);

  if (p->r > 1.0)
    p->r = 1.0;
  if (p->g > 1.0)
    p->g = 1.0;
  if (p->b > 1.0)
    p->b = 1.0;
  if (p->a > 1.0)
    p->a = 1.0;
  if (p->r < 0.0)
    p->r = 0.0;
  if (p->g < 0.0)
    p->g = 0.0;
  if (p->b < 0.0)
    p->b = 0.0;
  if (p->a < 0.0)
    p->a = 0.0;

  g_function_leave("gck_rgba_clamp");
}

void gck_rgba_set(GckRGB * p, double r, double g, double b, double a)
{
  g_function_enter("gck_rgba_set");
  g_assert(p!=NULL);

  p->r = r;
  p->g = g;
  p->b = b;
  p->a = a;

  g_function_leave("gck_rgba_set");
}

/* This one is also not needed */

void gck_rgba_gamma(GckRGB * p, double gamma)
{
  double ig;

  g_function_enter("gck_rgba_gamma");
  g_assert(p!=NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    (ig = 0.0);

  p->r = pow(p->r, ig);
  p->g = pow(p->g, ig);
  p->b = pow(p->b, ig);
  p->a = pow(p->a, ig);

  g_function_leave("gck_rgba_gamma");
}

/**************************/
/* Colorspace conversions */
/**************************/

/***********************************************/
/* (Red,Green,Blue) <-> (Hue,Saturation,Value) */
/***********************************************/

void gck_rgb_to_hsv(GckRGB * p, double *h, double *s, double *v)
{
  double max,min,delta;

  g_function_enter("gck_rgb_to_hsv");
  g_assert(p!=NULL);
  g_assert(h!=NULL);
  g_assert(s!=NULL);
  g_assert(v!=NULL);

  max = gck_rgb_max(p);
  min = gck_rgb_min(p);

  *v = max;
  if (max != 0.0)
    {
      *s = (max - min) / max;
    }
  else
    *s = 0.0;
  if (*s == 0.0)
    *h = GCK_HSV_UNDEFINED;
  else
    {
      delta = max - min;
      if (p->r == max)
	{
	  *h = (p->g - p->b) / delta;
	}
      else if (p->g == max)
	{
	  *h = 2.0 + (p->b - p->r) / delta;
	}
      else if (p->b == max)
	{
	  *h = 4.0 + (p->r - p->g) / delta;
	}
      *h = *h * 60.0;
      if (*h < 0.0)
	*h = *h + 360;
    }
  g_function_leave("gck_rgb_to_hsv");
}

void gck_hsv_to_rgb(double h, double s, double v, GckRGB * p)
{
  int i;
  double f, w, q, t;

  g_function_enter("gck_hsv_to_rgb");
  g_assert(p!=NULL);

  if (s == 0.0)
    {
      if (h == GCK_HSV_UNDEFINED)
	{
	  p->r = v;
	  p->g = v;
	  p->b = v;
	}
    }
  else
    {
      if (h == 360.0)
	h = 0.0;
      h = h / 60.0;
      i = (int)h;
      f = h - i;
      w = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));
      switch (i)
	{
	case 0:
	  p->r = v;
	  p->g = t;
	  p->b = w;
	  break;
	case 1:
	  p->r = q;
	  p->g = v;
	  p->b = w;
	  break;
	case 2:
	  p->r = w;
	  p->g = v;
	  p->b = t;
	  break;
	case 3:
	  p->r = w;
	  p->g = q;
	  p->b = v;
	  break;
	case 4:
	  p->r = t;
	  p->g = w;
	  p->b = v;
	  break;
	case 5:
	  p->r = v;
	  p->g = w;
	  p->b = q;
	  break;
	}
    }

  g_function_leave("gck_hsv_to_rgb");
}

/***************************************************/
/* (Red,Green,Blue) <-> (Hue,Saturation,Lightness) */
/***************************************************/

void gck_rgb_to_hsl(GckRGB * p, double *h, double *s, double *l)
{
  double max,min,delta;

  g_function_enter("gck_rgb_to_hsl");
  g_assert(p!=NULL);
  g_assert(h!=NULL);
  g_assert(s!=NULL);
  g_assert(l!=NULL);

  max = gck_rgb_max(p);
  min = gck_rgb_min(p);

  *l = (max + min) / 2.0;

  if (max == min)
    {
      *s = 0.0;
      *h = GCK_HSL_UNDEFINED;
    }
  else
    {
      if (*l <= 0.5)
	*s = (max - min) / (max + min);
      else
	*s = (max - min) / (2.0 - max - min);

      delta = max - min;
      if (p->r == max)
	{
	  *h = (p->g - p->b) / delta;
	}
      else if (p->g == max)
	{
	  *h = 2.0 + (p->b - p->r) / delta;
	}
      else if (p->b == max)
	{
	  *h = 4.0 + (p->r - p->g) / delta;
	}
      *h = *h * 60.0;
      if (*h < 0.0)
	*h = *h + 360.0;
    }
  g_function_leave("gck_rgb_to_hsl");
}

double _gck_value(double n1, double n2, double hue)
{
  double val;

  if (hue > 360.0)
    hue = hue - 360.0;
  else if (hue < 0.0)
    hue = hue + 360.0;
  if (hue < 60.0)
    val = n1 + (n2 - n1) * hue / 60.0;
  else if (hue < 180.0)
    val = n2;
  else if (hue < 240.0)
    val = n1 + (n2 - n1) * (240.0 - hue) / 60.0;
  else
    val = n1;

  return (val);
}

void gck_hsl_to_rgb(double h, double s, double l, GckRGB * p)
{
  double m1, m2;

  g_function_enter("gck_hsl_to_rgb");
  g_assert(p!=NULL);

  if (l <= 0.5)
    m2 = l * (l + s);
  else
    m2 = l + s + l * s;
  m1 = 2.0 * l - m2;

  if (s == 0)
    {
      if (h == GCK_HSV_UNDEFINED)
	p->r = p->g = p->b = 1.0;
    }
  else
    {
      p->r = _gck_value(m1, m2, h + 120.0);
      p->g = _gck_value(m1, m2, h);
      p->b = _gck_value(m1, m2, h - 120.0);
    }

  g_function_leave("gck_hsl_to_rgb");
}

#define GCK_RETURN_RGB(x, y, z) {rgb->r = x; rgb->g = y; rgb->b = z; return; }

/***********************************************************************************/
/* Theoretically, hue 0 (pure red) is identical to hue 6 in these transforms. Pure */
/* red always maps to 6 in this implementation. Therefore UNDEFINED can be         */
/* defined as 0 in situations where only unsigned numbers are desired.             */
/***********************************************************************************/

void gck_rgb_to_hwb(GckRGB *rgb, gdouble *hue,gdouble *whiteness,gdouble *blackness)
{
  /* RGB are each on [0, 1]. W and B are returned on [0, 1] and H is        */
  /* returned on [0, 6]. Exception: H is returned UNDEFINED if W ==  1 - B. */
  /* ====================================================================== */

  gdouble R = rgb->r, G = rgb->g, B = rgb->b, w, v, b, f;
  gint i;

  w = gck_rgb_min(rgb);
  v = gck_rgb_max(rgb);
  b = 1.0 - v;
  
  if (v == w)
    {
      *hue=GCK_HSV_UNDEFINED;
      *whiteness=w;
      *blackness=b;
    }
  else
    {
      f = (R == w) ? G - B : ((G == w) ? B - R : R - G);
      i = (R == w) ? 3.0 : ((G == w) ? 5.0 : 1.0);
    
      *hue=(360.0/6.0)*(i - f /(v - w));
      *whiteness=w;
      *blackness=b;
    }
}

void gck_hwb_to_rgb(gdouble H,gdouble W, gdouble B, GckRGB *rgb)
{
  /* H is given on [0, 6] or UNDEFINED. W and B are given on [0, 1]. */
  /* RGB are each returned on [0, 1].                                */
  /* =============================================================== */
  
  gdouble h = H, w = W, b = B, v, n, f;
  gint i;

  h=6.0*h/360.0;
    
  v = 1.0 - b;
  if (h == GCK_HSV_UNDEFINED)
    {
      rgb->r=v;
      rgb->g=v;
      rgb->b=v;
    }
  else
    {
      i = floor(h);
      f = h - i;
      if (i & 1) f = 1.0 - f;  /* if i is odd */
      n = w + f * (v - w);     /* linear interpolation between w and v */
    
      switch (i)
        {
          case 6:
          case 0: GCK_RETURN_RGB(v, n, w);
            break;
          case 1: GCK_RETURN_RGB(n, v, w);
            break;
          case 2: GCK_RETURN_RGB(w, v, n);
            break;
          case 3: GCK_RETURN_RGB(w, n, v);
            break;
          case 4: GCK_RETURN_RGB(n, w, v);
            break;
          case 5: GCK_RETURN_RGB(v, w, n);
            break;
        }
    }

}

/*********************************************************************/
/* Sumpersampling code (Quartic)                                     */
/* This code is *largely* based on the sources for POV-Ray 3.0. I am */
/* grateful to the POV-Team for such a great program and for making  */
/* their sources available.  All comments / bug reports /            */
/* etc. regarding this code should be addressed to me, not to the    */
/* POV-Ray team.  Any bugs are my responsibility, not theirs.        */
/*********************************************************************/

gulong gck_render_sub_pixel(int max_depth, int depth, _GckSampleType ** block,
			    int x, int y, int x1, int y1, int x3, int y3, double threshold,
			    int sub_pixel_size, GckRenderFunction render_func, GckRGB * color)
{
  int x2, y2, cnt;		/* Coords of center sample */
  double dx1, dy1;		/* Delta to upper left sample */
  double dx3, dy3, weight;	/* Delta to lower right sample */
  GckRGB c[4],tmpcol;
  unsigned long num_samples = 0;

  /* Get offsets for corners */
  /* ======================= */

  dx1 = (double)(x1 - sub_pixel_size / 2) / sub_pixel_size;
  dx3 = (double)(x3 - sub_pixel_size / 2) / sub_pixel_size;

  dy1 = (double)(y1 - sub_pixel_size / 2) / sub_pixel_size;
  dy3 = (double)(y3 - sub_pixel_size / 2) / sub_pixel_size;

  /* Render upper left sample */
  /* ======================== */

  if (!block[y1][x1].ready)
    {
      num_samples++;
      (*render_func) (x + dx1, y + dy1, &c[0]);
      block[y1][x1].ready = 1;
      block[y1][x1].color = c[0];
    }
  else
    c[0] = block[y1][x1].color;

  /* Render upper right sample */
  /* ========================= */

  if (!block[y1][x3].ready)
    {
      num_samples++;
      (*render_func) (x + dx3, y + dy1, &c[1]);
      block[y1][x3].ready = 1;
      block[y1][x3].color = c[1];
    }
  else
    c[1] = block[y1][x3].color;

  /* Render lower left sample */
  /* ======================== */

  if (!block[y3][x1].ready)
    {
      num_samples++;
      (*render_func) (x + dx1, y + dy3, &c[2]);
      block[y3][x1].ready = 1;
      block[y3][x1].color = c[2];
    }
  else
    c[2] = block[y3][x1].color;

  /* Render lower right sample */
  /* ========================= */

  if (!block[y3][x3].ready)
    {
      num_samples++;
      (*render_func) (x + dx3, y + dy3, &c[3]);
      block[y3][x3].ready = 1;
      block[y3][x3].color = c[3];
    }
  else
    c[3] = block[y3][x3].color;

  /* Check for supersampling */
  /* ======================= */

  if (depth <= max_depth)
    {
      /* Check whether we have to supersample */
      /* ==================================== */

      if ((gck_rgba_dist(&c[0], &c[1]) >= threshold) ||
	  (gck_rgba_dist(&c[0], &c[2]) >= threshold) ||
	  (gck_rgba_dist(&c[0], &c[3]) >= threshold) ||
	  (gck_rgba_dist(&c[1], &c[2]) >= threshold) ||
	  (gck_rgba_dist(&c[1], &c[3]) >= threshold) ||
	  (gck_rgba_dist(&c[2], &c[3]) >= threshold))
	{
	  /* Calc coordinates of center subsample */
	  /* ==================================== */

	  x2 = (x1 + x3) / 2;
	  y2 = (y1 + y3) / 2;

	  /* Render sub-blocks */
	  /* ================= */

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x1, y1, x2, y2,
	     threshold, sub_pixel_size, render_func, &c[0]);

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x2, y1, x3, y2,
	     threshold, sub_pixel_size, render_func, &c[1]);

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x1, y2, x2, y3,
	     threshold, sub_pixel_size, render_func, &c[2]);

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x2, y2, x3, y3,
	     threshold, sub_pixel_size, render_func, &c[3]);
	}
    }

  if (c[0].a==0.0 || c[1].a==0.0 || c[2].a==0.0 || c[3].a==0.0)
    {
      tmpcol.r=0.0;
      tmpcol.g=0.0;
      tmpcol.b=0.0;
      weight=2.0;

      for (cnt=0;cnt<4;cnt++)
        {
          if (c[cnt].a!=0.0)
            {
              tmpcol.r+=c[cnt].r;
              tmpcol.g+=c[cnt].g;
              tmpcol.b+=c[cnt].b;
              weight/=2.0;
            }
        }

      color->r=weight*tmpcol.r;
      color->g=weight*tmpcol.g;
      color->b=weight*tmpcol.b;
    }
  else
    {
      color->r = 0.25 * (c[0].r + c[1].r + c[2].r + c[3].r);
      color->g = 0.25 * (c[0].g + c[1].g + c[2].g + c[3].g);
      color->b = 0.25 * (c[0].b + c[1].b + c[2].b + c[3].b);
    }

  color->a = 0.25 * (c[0].a + c[1].a + c[2].a + c[3].a);

  return (num_samples);
}				/* Render_Sub_Pixel */

gulong gck_adaptive_supersample_area(int x1, int y1, int x2, int y2, int max_depth,
				     double threshold,
			             GckRenderFunction render_func,
		                     GckPutPixelFunction put_pixel_func,
			             GckProgressFunction progress_func)
{
  int x, y, width;		/* Counters, width of region */
  int xt, xtt, yt;		/* Temporary counters */
  int sub_pixel_size;		/* Numbe of samples per pixel (1D) */
  size_t row_size;		/* Memory needed for one row */
  GckRGB color;			/* Rendered pixel's color */
  _GckSampleType tmp_sample;	/* For swapping samples */
  _GckSampleType *top_row, *bot_row, *tmp_row;	/* Sample rows */
  _GckSampleType **block;	/* Sample block matrix */
  unsigned long num_samples;

  g_function_enter("gck_adaptive_supersample_area");

  /* Initialize color */
  /* ================ */

  color.r = color.b = color.g = color.a = 0.0;

  /* Calculate sub-pixel size */
  /* ======================== */

  sub_pixel_size = 1 << max_depth;

  /* Create row arrays */
  /* ================= */

  width = x2 - x1 + 1;

  row_size = (sub_pixel_size * width + 1) * sizeof(_GckSampleType);

  top_row = (_GckSampleType *) malloc(row_size);
  bot_row = (_GckSampleType *) malloc(row_size);

  for (x = 0; x < (sub_pixel_size * width + 1); x++)
    {
      top_row[x].ready = 0;

      top_row[x].color.r = 0.0;
      top_row[x].color.g = 0.0;
      top_row[x].color.b = 0.0;
      top_row[x].color.a = 0.0;

      bot_row[x].ready = 0;

      bot_row[x].color.r = 0.0;
      bot_row[x].color.g = 0.0;
      bot_row[x].color.b = 0.0;
      bot_row[x].color.a = 0.0;
    }

  /* Allocate block matrix */
  /* ===================== */

  block = malloc((sub_pixel_size + 1) * sizeof(_GckSampleType *));	/* Rows */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    block[y] = malloc((sub_pixel_size + 1) * sizeof(_GckSampleType));

  for (y = 0; y < (sub_pixel_size + 1); y++)
    for (x = 0; x < (sub_pixel_size + 1); x++)
      {
	block[y][x].ready = 0;
	block[y][x].color.r = 0.0;
	block[y][x].color.g = 0.0;
	block[y][x].color.b = 0.0;
	block[y][x].color.a = 0.0;
      }

  /* Render region */
  /* ============= */

  num_samples = 0;

  for (y = y1; y <= y2; y++)
    {
      /* Clear the bottom row */
      /* ==================== */

      for (xt = 0; xt < (sub_pixel_size * width + 1); xt++)
	bot_row[xt].ready = 0;

      /* Clear first column */
      /* ================== */

      for (yt = 0; yt < (sub_pixel_size + 1); yt++)
	block[yt][0].ready = 0;

      /* Render row */
      /* ========== */

      for (x = x1; x <= x2; x++)
	{
	  /* Initialize block by clearing all but first row/column */
	  /* ===================================================== */

	  for (yt = 1; yt < (sub_pixel_size + 1); yt++)
	    for (xt = 1; xt < (sub_pixel_size + 1); xt++)
	      block[yt][xt].ready = 0;

	  /* Copy samples from top row to block */
	  /* ================================== */

	  for (xtt = 0, xt = x * sub_pixel_size; xtt < (sub_pixel_size + 1); xtt++, xt++)
	    block[0][xtt] = top_row[xt];

	  /* Render pixel on (x, y) */
	  /* ====================== */

	  num_samples += gck_render_sub_pixel(max_depth, 1, block, x, y, 0, 0, sub_pixel_size,
					      sub_pixel_size, threshold, sub_pixel_size, render_func, &color);

	  (*put_pixel_func) (x, y, &color);

	  /* Copy block information to rows */
	  /* ============================== */

	  top_row[(x + 1) * sub_pixel_size] = block[0][sub_pixel_size];

	  for (xtt = 0, xt = x * sub_pixel_size; xtt < (sub_pixel_size + 1); xtt++, xt++)
	    bot_row[xt] = block[sub_pixel_size][xtt];

	  /* Swap first and last columns */
	  /* =========================== */

	  for (yt = 0; yt < (sub_pixel_size + 1); yt++)
	    {
	      tmp_sample = block[yt][0];
	      block[yt][0] = block[yt][sub_pixel_size];
	      block[yt][sub_pixel_size] = tmp_sample;
	    }
	}

      /* Swap rows */
      /* ========= */

      tmp_row = top_row;
      top_row = bot_row;
      bot_row = tmp_row;

      /* Call progress display function (if any) */
      /* ======================================= */

      if (progress_func != NULL)
	(*progress_func) (y1, y2, y);
    }				/* for */

  /* Free memory */
  /* =========== */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    free(block[y]);

  free(block);
  free(top_row);
  free(bot_row);

  g_function_leave("gck_adaptive_supersample_area");
  return (num_samples);
} /* Adaptive_Supersample_Area */
