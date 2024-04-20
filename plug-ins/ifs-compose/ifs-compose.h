/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * IfsCompose is a interface for creating IFS fractals by
 * direct manipulation.
 * Copyright (C) 1997 Owen Taylor
 *
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

typedef struct {
  gdouble a11,a12,a21,a22,b1,b2;
} Aff2;

typedef struct {
  gdouble vals[3][4];
} Aff3;

typedef struct {
  GdkPoint *points;
  gint      npoints;
} IPolygon;

typedef struct {
  gdouble    x, y;
  gdouble    theta;
  gdouble    scale;
  gdouble    asym;
  gdouble    shear;
  gint       flip;

  GeglColor *red_color;
  GeglColor *green_color;
  GeglColor *blue_color;
  GeglColor *black_color;

  GeglColor *target_color;
  gdouble    hue_scale;
  gdouble    value_scale;

  gint       simple_color;
  gdouble    prob;
} AffElementVals;

typedef struct
{
  gint num_elements;
  gint iterations;
  gint max_memory;
  gint subdivide;
  gdouble radius;
  gdouble aspect_ratio;
  gdouble center_x;
  gdouble center_y;
} IfsComposeVals;

typedef struct {
  AffElementVals v;

  Aff2 trans;
  Aff3 color_trans;

  gchar *name;

  IPolygon *click_boundary;
  IPolygon *draw_boundary;
} AffElement;


/* manipulation of affine transforms */
void aff2_translate       (Aff2    *naff,
                           gdouble  x,
                           gdouble  y);
void aff2_rotate          (Aff2    *naff,
                           gdouble  theta);
void aff2_scale           (Aff2    *naff,
                           gdouble  s,
                           gint     flip);
void aff2_distort         (Aff2    *naff,
                           gdouble  asym,
                           gdouble  shear);
void aff2_compute_stretch (Aff2    *naff,
			   gdouble  xo,
                           gdouble  yo,
			   gdouble  xn,
                           gdouble  yn);
void aff2_compute_distort (Aff2    *naff,
			   gdouble  xo,
                           gdouble  yo,
			   gdouble  xn,
                           gdouble  yn);
void aff2_compose         (Aff2    *naff,
                           Aff2    *aff1,
                           Aff2    *aff2);
void aff2_invert          (Aff2    *naff,
                           Aff2    *aff);
void aff2_apply           (Aff2    *aff,
                           gdouble  x,
                           gdouble  y,
			   gdouble *xf,
                           gdouble *yf);
void aff2_fixed_point     (Aff2    *aff,
                           gdouble *xf,
                           gdouble *yf);
void aff3_apply           (Aff3    *t,
                           gdouble  x,
                           gdouble  y,
                           gdouble  z,
			   gdouble *xf,
                           gdouble *yf,
                           gdouble *zf);


/* manipulation of polygons */
IPolygon *ipolygon_convex_hull (IPolygon *poly);
gint      ipolygon_contains    (IPolygon *poly,
                                gint      xt,
                                gint      yt);


/* manipulation of composite transforms */
AffElement *aff_element_new                  (gdouble      x,
                                              gdouble      y,
                                              GeglColor   *color,
                                              gint         count);
void        aff_element_free                 (AffElement  *elem);
void        aff_element_compute_trans        (AffElement  *elem,
                                              gdouble      width,
					      gdouble      height,
					      gdouble      center_x,
                                              gdouble      center_y);
void        aff_element_compute_color_trans  (AffElement  *elem);
void        aff_element_decompose_trans      (AffElement  *elem,
                                              Aff2        *aff,
					      gdouble      width,
                                              gdouble      height,
					      gdouble      center_x,
                                              gdouble      center_y);
void        aff_element_compute_boundary     (AffElement  *elem,
                                              gint         width,
					      gint         height,
					      AffElement **elements,
					      gint         num_elements);
void        aff_element_draw                 (AffElement  *elem,
                                              gint         selected,
                                              gint         width,
                                              gint         height,
                                              cairo_t     *cr,
                                              GdkRGBA     *color,
                                              PangoLayout *layout);


void       ifs_render (AffElement     **elements,
                       gint             num_elements,
                       gint             width,
                       gint             height,
                       gint             nsteps,
                       IfsComposeVals  *vals,
                       gint             band_y,
                       gint             band_height,
                       guchar          *data,
                       guchar          *mask,
                       guchar          *nhits,
                       gboolean         preview);

gchar    * ifsvals_stringify    (IfsComposeVals   *vals,
                                 AffElement      **elements);
gboolean   ifsvals_parse_string (const gchar      *str,
                                 IfsComposeVals   *vals,
                                 AffElement     ***elements);
