/* airbrush_blob.h: routines for manipulating scan converted convex
 *         polygons.
 *  
 * Copyright 1998, Owen Taylor <otaylor@gtk.org>
 *
 * > Please contact the above author before modifying the copy <
 * > of this file in the GIMP distribution. Thanks.            <
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
 *
*/

#ifndef __AIRBRUSHBLOB_H__
#define __AIRBRUSHBLOB_H__

typedef enum {
  CROSS = 0,
  CROSS_LEFT = 1,
  CROSS_RIGHT = 2,
  CROSS_WHOLE_LINE = 3,
  CROSS_NORMAL = 4
} CrossType;

typedef enum {
  RIGHT_LEFT = 0,
  LEFT_RIGHT = 1,
  TOP_BOT = 2,
  BOT_TOP = 3,
  NONE = 4
} MoveType;

/* The AirBlob, which is a abstract of a real AirBrushBlob */

typedef struct _AirBlob AirBlob;
typedef struct _AirPoint AirPoint;
typedef struct _SupportLine SupportLine;


struct _AirPoint {
  int x;
  int y;
};

struct _SupportLine {
  double size;
  double dist;
};

struct _AirBlob {
  double direction_abs;
  double direction;
  double ycenter;
  double xcenter;
  SupportLine main_line;
  SupportLine minor_line;
  SupportLine maincross_line;
  SupportLine minorcross_line;
}; 


/* The AirLine is a reslut of a AirBlob */
typedef struct _AirLine AirLine;

struct _AirLine {
  int xcenter;
  int ycenter;
  AirPoint line[16];
  int min_x, min_y;
  int max_x, max_y;
  int width, height;
  int nlines;
};


typedef struct _AirBrushBlobPoint AirBrushBlobPoint;
typedef struct _AirBrushBlobSpan AirBrushBlobSpan;
typedef struct _AirBrushBlob AirBrushBlob;

struct _AirBrushBlobPoint {
  int x;
  int y;
};

struct _AirBrushBlobSpan {
  int left;	
  double angle_left;  
  double angle_left_abs;
  double dist_left;
  int right;
  double angle_right; 
  double angle_right_abs;
  double dist_right;

  CrossType cross_type;
  int x_cross;

  int center;
  double dist;

};

struct _AirBrushBlob {
  int y;
  int height;
  int width;
  int min_x;
  int max_x;
  MoveType move;
  double direction_abs;
  double direction;
  CrossType cross;
  AirBrushBlobSpan data[1];
};


typedef struct _AirBrush AirBrush;

struct _AirBrush {
  AirBrushBlob airbrush_blob;
  AirBlob airblob;
};




AirBlob *create_air_blob (double xc, double yc, double xt, double yt, double xr, double yr, double xb, double yb, double xl, double yl, double direction_abs, double direction);
AirBlob *trans_air_blob(AirBlob *airblob_last, AirBlob *airblob_present, double dist, int xc, int yc);
AirLine *create_air_line(AirBlob *airblob);





AirBrushBlob *airbrush_blob_convex_union (AirBrushBlob *b1, AirBrushBlob *b2);
AirBrushBlob *airbrush_blob_ellipse (double xc, double yc, double xt, double yt, double xr, double yr, double xb, double yb, double xl, double yl);
void          airbrush_blob_bounds  (AirBrushBlob *b, int *x, int *y, int *width, int *height);




#endif /* __AIRBRUSHBLOB_H__ */
