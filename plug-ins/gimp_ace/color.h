/* These are the weighing factors used to convert color images to 
   the one-dimensional channel (luminosity?) to operate on 
   in gimp_ace.c:SeperateChannels()
   They're recombined in glace.c:Glace_Output(), 
   which should undergo some scrutiny. */
/* For more on this subject, see the thread in gimp-developer
   From: Carey Bunks <cbunks@bbn.com>
   Subject: [gimp-devel] Conversion to Grayscale Bug
   and http://www.inforamp.net/~poynton/ */

#ifndef NTSC_RGB

/* These are used to compute CIE luminace from linear RGB for
   contemporary CRT phosphors, as specified in ITU-R Recommendation
   BT.709 (formerly CCIR Rec. 709) */

/* Matrix for RGB -> XYZ conversion. */

#define X_r  0.412453
#define X_g  0.357580
#define X_b  0.180423

#define Y_r  0.212671
#define Y_g  0.715160
#define Y_b  0.072169

#define Z_r  0.019334
#define Z_g  0.119193
#define Z_b  0.950227

/* Matrix for XYZ -> RGB version. */

#define R_x  3.240479
#define R_y -1.537150
#define R_z -0.498535

#define G_x -0.969256
#define G_y  1.876992
#define G_z  0.041556

#define B_x  0.055648
#define B_y -0.204043
#define B_z  1.057311

#else

/* This computes luma from RGB, according to NTSC RGB CRT phosphors of
   1953.  Standardized in ITU-R Recommendation BT.601-4. (formerly
   CCIR Rec. 601) */

#define Y_r  0.299
#define Y_g  0.587
#define Y_b  0.114

#endif
