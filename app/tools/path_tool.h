/* The GIMP -- an image manipulation program
 * 
 * This file Copyright (C) 1999 Simon Budig
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

#ifndef __PATH_TOOL_H__
#define __PATH_TOOL_H__

/*
 * Every new curve-type has to have a parameter between 0 and 1, and
 * should go from a starting to a target point.
 */

/* Some defines... */

#define PATH_TOOL_WIDTH 8
#define PATH_TOOL_HALFWIDTH 4

/*  function prototypes  */


/* Small functions to determine coordinates, iterate over path/curve/segment */

void    path_segment_get_coordinates (PathSegment *,
				      gdouble,
				      gint *,
				      gint *);
void    path_traverse_path           (NPath *,
				      PathTraverseFunc,
				      CurveTraverseFunc,
				      SegmentTraverseFunc,
				      gpointer);
void    path_traverse_curve          (NPath *,
				      PathCurve *,
				      CurveTraverseFunc,
				      SegmentTraverseFunc,
				      gpointer);
void    path_traverse_segment        (NPath *,
				      PathCurve *,
				      PathSegment *,
				      SegmentTraverseFunc,
				      gpointer);
gdouble path_locate_point            (NPath *,
				      PathCurve **,
				      PathSegment **,
				      gint,
				      gint,
				      gint,
				      gint,
				      gint);

/* Tools to manipulate paths, curves, segments */

PathCurve   * path_add_curve       (NPath *,
				    gdouble,
				    gdouble);
PathSegment * path_append_segment  (NPath *,
				    PathCurve *,
				    SegmentType,
				    gdouble,
				    gdouble);
PathSegment * path_prepend_segment (NPath *,
				    PathCurve *,
				    SegmentType,
				    gdouble,
				    gdouble);
PathSegment * path_split_segment   (PathSegment *,
				    gdouble);
void          path_join_curves     (PathSegment *,
				    PathSegment *);
void          path_flip_curve      (PathCurve *);
void          path_free_path       (NPath *);
void          path_free_curve      (PathCurve *);
void          path_free_segment    (PathSegment *);
void          path_delete_segment  (PathSegment *);
void          path_print           (NPath *);
void          path_offset_active   (NPath *,
                                    gdouble,
                                    gdouble);
void          path_set_flags       (GimpPathTool *path_tool,
				    NPath *,
				    PathCurve *,
				    PathSegment *,
				    guint32,
				    guint32);

gdouble  gimp_path_tool_on_curve   (GimpPathTool *path_tool,
                                    gint,
                                    gint,
                                    gint,
		                    NPath**,
                                    PathCurve**,
                                    PathSegment**);
gboolean gimp_path_tool_on_anchors (GimpPathTool *path_tool,
                                    gint,
                                    gint,
                                    gint,
		                    NPath**,
                                    PathCurve**,
                                    PathSegment**);
gint     gimp_path_tool_on_handles (GimpPathTool *path_tool,
                                    gint,
                                    gint,
                                    gint,
		                    NPath **,
                                    PathCurve **,
                                    PathSegment **);

gint     path_tool_cursor_position (NPath         *path,
                                    gdouble        x,
                                    gdouble        y,
                                    gint           halfwidth,
                                    gint           halfheight,
                                    NPath        **pathP,
                                    PathCurve    **curveP,
                                    PathSegment  **segmentP,
                                    gdouble       *positionP,
                                    gint          *handle_idP);


/* High level image-manipulation functions */

void path_stroke                   (GimpPathTool *path_tool,
				    NPath *);
void path_to_selection             (GimpPathTool *path_tool,
				    NPath *);


#endif  /* __PATH_TOOL_H__ */

