/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __GIMP_CONTEXT_H__
#define __GIMP_CONTEXT_H__

#include <glib.h>

#include "apptypes.h"
#include "gdisplay.h"
#include "gimpimageF.h"
#include "gradient_header.h"
#include "patterns.h"
#include "toolsF.h"

#include "gimpobjectP.h"

#define GIMP_TYPE_CONTEXT            (gimp_context_get_type ())
#define GIMP_CONTEXT(obj)            (GIMP_CHECK_CAST ((obj), GIMP_TYPE_CONTEXT, GimpContext))
#define GIMP_CONTEXT_CLASS(klass)    (GIMP_CHECK_CLASS_CAST (klass, GIMP_TYPE_CONTEXT, GimpContextClass))
#define GIMP_IS_CONTEXT(obj)         (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_CONTEXT))
#define GIMP_IS_CONTEXT_CLASS(klass) (GIMP_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTEXT))

typedef enum
{
  GIMP_CONTEXT_ARG_IMAGE      = 1 << 0,
  GIMP_CONTEXT_ARG_DISPLAY    = 1 << 1,
  GIMP_CONTEXT_ARG_TOOL       = 1 << 2,
  GIMP_CONTEXT_ARG_FOREGROUND = 1 << 3,
  GIMP_CONTEXT_ARG_BACKGROUND = 1 << 4,
  GIMP_CONTEXT_ARG_OPACITY    = 1 << 5,
  GIMP_CONTEXT_ARG_PAINT_MODE = 1 << 6,
  GIMP_CONTEXT_ARG_BRUSH      = 1 << 7,
  GIMP_CONTEXT_ARG_PATTERN    = 1 << 8,
  GIMP_CONTEXT_ARG_GRADIENT   = 1 << 9,
  
  /* aliases
   */
  GIMP_CONTEXT_ARG_PAINT      = GIMP_CONTEXT_ARG_FOREGROUND |
                                GIMP_CONTEXT_ARG_BACKGROUND |
                                GIMP_CONTEXT_ARG_OPACITY |
                                GIMP_CONTEXT_ARG_PAINT_MODE |
                                GIMP_CONTEXT_ARG_BRUSH |
                                GIMP_CONTEXT_ARG_PATTERN |
                                GIMP_CONTEXT_ARG_GRADIENT,
  GIMP_CONTEXT_ARG_ALL        = GIMP_CONTEXT_ARG_IMAGE |
                                GIMP_CONTEXT_ARG_DISPLAY |
                                GIMP_CONTEXT_ARG_TOOL
} GimpContextArgs;

typedef struct _GimpContext GimpContext;
typedef struct _GimpContextClass GimpContextClass;

struct _GimpContext
{
  GimpObject	    object;

  gchar		   *name;
  GimpContext	   *parent;

  /*  FIXME: the solution of having a boolean for each attribute and the
   *         name "defined" need some brainstorming
   */
  gboolean	    image_defined;
  GimpImage	   *image;

  gboolean	    display_defined;
  GDisplay	   *display;

  gboolean          tool_defined;
  ToolType          tool;

  gboolean          foreground_defined;
  guchar            foreground[3];

  gboolean          background_defined;
  guchar            background[3];

  gboolean	    opacity_defined;
  gdouble	    opacity;

  gboolean	    paint_mode_defined;
  LayerModeEffects  paint_mode;

  gboolean          brush_defined;
  GimpBrush        *brush;

  gboolean          pattern_defined;
  GPattern         *pattern;

  gboolean          gradient_defined;
  gradient_t       *gradient;
};

struct _GimpContextClass
{
  GimpObjectClass parent_class;

  void (* image_changed)      (GimpContext *context, gpointer image);
  void (* display_changed)    (GimpContext *context, gpointer display);

  void (* tool_changed)       (GimpContext *context, gint tool_type);

  void (* foreground_changed) (GimpContext *context, guchar foreground[3]);
  void (* background_changed) (GimpContext *context, guchar background[3]);
  void (* opacity_changed)    (GimpContext *context, gdouble opacity);
  void (* paint_mode_changed) (GimpContext *context, gint paint_mode);
  void (* brush_changed)      (GimpContext *context, gpointer brush);
  void (* pattern_changed)    (GimpContext *context, gpointer pattern);
  void (* gradient_changed)   (GimpContext *context, gpointer gradient);
};

GtkType       gimp_context_get_type       (void);
GimpContext * gimp_context_new            (gchar       *name,
					   GimpContext *template,
					   GimpContext *parent);

/*  TODO: - gimp_context_set_parent ()
 *        - gimp_context_get_parent ()
 *        - gimp_context_find ()
 *
 *        probably interacting with the context manager:
 *        - gimp_context_push () which will call gimp_context_set_parent()
 *        - gimp_context_push_new () to do a GL-style push
 *        - gimp_context_pop ()
 *
 *        a proper mechanism to prevent silly operations like pushing
 *        the user context to some client stack etc.
 */


/*  to be used by the context management system only
 *
 *  FIXME: move them to a private header
 */
void          gimp_context_set_current    (GimpContext *context);
void          gimp_context_set_user       (GimpContext *context);
void          gimp_context_set_default    (GimpContext *context);

/*  these are always available
 */
GimpContext * gimp_context_get_current    (void);
GimpContext * gimp_context_get_user       (void);
GimpContext * gimp_context_get_default    (void);
GimpContext * gimp_context_get_standard   (void);

/*  functions for manipulating a single context
 *
 *  FIXME: this interface may be ok but the implementation is
 *         ugly code duplication. There needs to be a generic way.
 */
gchar       * gimp_context_get_name           (GimpContext *context);

GimpContext * gimp_context_get_parent         (GimpContext *context);
void          gimp_context_set_parent         (GimpContext *context,
					       GimpContext *parent);

/*  image  */

GimpImage        * gimp_context_get_image          (GimpContext     *context);
void               gimp_context_set_image          (GimpContext     *context,
						    GimpImage       *image);
gboolean           gimp_context_image_defined      (GimpContext     *context);
void               gimp_context_define_image       (GimpContext     *context,
						    gboolean         defined);

/*  display  */

GDisplay         * gimp_context_get_display        (GimpContext     *context);
void               gimp_context_set_display        (GimpContext     *context,
						    GDisplay        *display);
gboolean           gimp_context_display_defined    (GimpContext     *context);
void               gimp_context_define_display     (GimpContext     *context,
						    gboolean         defined);

/*  tool  */

ToolType           gimp_context_get_tool           (GimpContext     *context);
void               gimp_context_set_tool           (GimpContext     *context,
						    ToolType         tool_type);
gboolean           gimp_context_tool_defined       (GimpContext     *context);
void               gimp_context_define_tool        (GimpContext     *context,
						    gboolean         defined);

/*  foreground color  */

void               gimp_context_get_foreground     (GimpContext *context,
						    guchar       foreground[3]);
void               gimp_context_set_foreground     (GimpContext *context,
						    guchar       foreground[3]);
gboolean           gimp_context_foreground_defined (GimpContext *context);
void               gimp_context_define_foreground  (GimpContext *context,
						    gboolean     defined);

/*  background color  */

void               gimp_context_get_background     (GimpContext *context,
						    guchar       background[3]);
void               gimp_context_set_background     (GimpContext *context,
						    guchar       background[3]);
gboolean           gimp_context_background_defined (GimpContext *context);
void               gimp_context_define_background  (GimpContext *context,
						    gboolean     defined);

/*  opacity  */

gdouble            gimp_context_get_opacity        (GimpContext     *context);
void               gimp_context_set_opacity        (GimpContext     *context,
						    gdouble          opacity);
gboolean           gimp_context_opacity_defined    (GimpContext     *context);
void               gimp_context_define_opacity     (GimpContext     *context,
						    gboolean         defined);

/*  paint mode  */

LayerModeEffects   gimp_context_get_paint_mode     (GimpContext     *context);
void               gimp_context_set_paint_mode     (GimpContext     *context,
						    LayerModeEffects paint_mode);
gboolean           gimp_context_paint_mode_defined (GimpContext     *context);
void               gimp_context_define_paint_mode  (GimpContext     *context,
						    gboolean         defined);

/*  brush  */

GimpBrush        * gimp_context_get_brush          (GimpContext     *context);
void               gimp_context_set_brush          (GimpContext     *context,
						    GimpBrush       *brush);
gboolean           gimp_context_brush_defined      (GimpContext     *context);
void               gimp_context_define_brush       (GimpContext     *context,
						    gboolean         defined);

/*  pattern  */

GPattern         * gimp_context_get_pattern        (GimpContext     *context);
void               gimp_context_set_pattern        (GimpContext     *context,
						    GPattern        *pattern);
gboolean           gimp_context_pattern_defined    (GimpContext     *context);
void               gimp_context_define_pattern     (GimpContext     *context,
						    gboolean         defined);

/*  gradient  */

gradient_t       * gimp_context_get_gradient       (GimpContext     *context);
void               gimp_context_set_gradient       (GimpContext     *context,
						    gradient_t      *gradient);
gboolean           gimp_context_gradient_defined   (GimpContext     *context);
void               gimp_context_define_gradient    (GimpContext     *context,
						    gboolean         defined);

#endif /* __GIMP_CONTEXT_H__ */
