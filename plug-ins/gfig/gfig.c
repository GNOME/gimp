/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#  include <io.h>
#  ifndef W_OK
#    define W_OK 2
#  endif
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "gfig.h"
#include "gfig-arc.h"
#include "gfig-bezier.h"
#include "gfig-circle.h"
#include "gfig-dobject.h"
#include "gfig-ellipse.h"
#include "gfig-grid.h"
#include "gfig-line.h"
#include "gfig-poly.h"
#include "gfig-preview.h"
#include "gfig-spiral.h"
#include "gfig-star.h"
#include "gfig-stock.h"

#include "pix-data.h"


/***** Magic numbers *****/

#define RESPONSE_UNDO    1
#define RESPONSE_CLEAR   2
#define RESPONSE_SAVE    3
#define RESPONSE_PAINT   4

#define PREVIEW_SIZE     400

#define SCALE_WIDTH      120

#define MAX_UNDO         10
#define MIN_UNDO         1
#define SMALL_PREVIEW_SZ 48
#define BRUSH_PREVIEW_SZ 32
#define GFIG_HEADER      "GFIG Version 0.1\n"

#define PREVIEW_MASK  (GDK_EXPOSURE_MASK       | \
                       GDK_POINTER_MOTION_MASK | \
                       GDK_BUTTON_PRESS_MASK   | \
                       GDK_BUTTON_RELEASE_MASK | \
                       GDK_BUTTON_MOTION_MASK  | \
                       GDK_KEY_PRESS_MASK      | \
                       GDK_KEY_RELEASE_MASK)

static GtkWidget *top_level_dlg;
static GimpDrawable *gfig_select_drawable;
GtkWidget    *gfig_preview;
GtkWidget    *pic_preview;
static GtkWidget    *gfig_gtk_list;
gint32        gfig_image;
gint32        gfig_drawable;
static GtkWidget    *brush_page_pw;
static GtkWidget    *brush_sel_button;

static gint   img_width, img_height;

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static gint      gfig_dialog               (void);
static void      gfig_response             (GtkWidget *widget,
                                            gint       response_id,
                                            gpointer   data);
static void      gfig_paint_callback       (void);
static gboolean  pic_preview_expose        (GtkWidget *widget,
                                            GdkEvent  *event);

static gint      gfig_brush_preview_events (GtkWidget *widget,
                                            GdkEvent  *event);

static void      gfig_scale_update_scale   (GtkAdjustment *adjustment,
                                            gdouble       *value);

static void      gfig_scale2img_update     (GtkWidget *widget,
                                            gpointer   data);
static gint      gfig_scale_x              (gint       x);
static gint      gfig_scale_y              (gint       y);

static gint      list_button_press         (GtkWidget      *widget,
                                            GdkEventButton *event,
                                            gpointer        data);

static void      rescan_button_callback    (GtkWidget *widget,
                                            gpointer   data);
static void      load_button_callback      (GtkWidget *widget,
                                            gpointer   data);
static void      new_button_callback       (GtkWidget *widget,
                                            gpointer   data);
static void     gfig_do_delete_gfig_callback (GtkWidget *widget,
                                              gboolean   delete,
                                              gpointer   data);
static void      gfig_delete_gfig_callback (GtkWidget *widget,
                                            gpointer   data);
static void      edit_button_callback      (GtkWidget *widget,
                                            gpointer   data);
static void      merge_button_callback     (GtkWidget *widget,
                                            gpointer   data);
static void      about_button_callback     (GtkWidget *widget,
                                            gpointer   data);
static void      reload_button_callback    (GtkWidget *widget,
                                            gpointer   data);

static void      do_gfig                   (void);
static void      toggle_show_image         (void);
static void      gfig_new_gc               (void);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

#define GRID_TYPE_MENU   1
#define GRID_RENDER_MENU 2
#define GRID_IGNORE      0
#define GRID_HIGHTLIGHT  1
#define GRID_RESTORE     2

#define PAINT_LAYERS_MENU 1
#define PAINT_BGS_MENU    2
#define PAINT_TYPE_MENU   3

#define SELECT_TYPE_MENU      1
#define SELECT_ARCTYPE_MENU   2
#define SELECT_TYPE_MENU_FILL 3
#define SELECT_TYPE_MENU_WHEN 4

#define OBJ_SELECT_GT 1
#define OBJ_SELECT_LT 2
#define OBJ_SELECT_EQ 4

/* Must keep in step with the above */
typedef struct
{
  void      *gridspacing;
  GtkWidget *gridtypemenu;
  GtkWidget *drawgrid;
  GtkWidget *snap2grid;
  GtkWidget *lockongrid;
  GtkWidget *showcontrol;
} GfigOptWidgets;

static GfigOptWidgets gfig_opt_widget;

/* Values when first invoked */
SelectItVals selvals =
{
  {
    MIN_GRID + (MAX_GRID - MIN_GRID)/2, /* Gridspacing     */
    RECT_GRID,            /* Default to rectangle type     */
    FALSE,                /* drawgrid                      */
    FALSE,                /* snap2grid                     */
    FALSE,                /* lockongrid                    */
    TRUE                  /* show control points           */
  },
  FALSE,                  /* show image                    */
  MIN_UNDO + (MAX_UNDO - MIN_UNDO)/2,  /* Max level of undos */
  TRUE,                   /* Show pos updates              */
  0.0,                    /* Brush fade                    */
  0.0,                    /* Brush gradient                */
  20.0,                   /* Air bursh pressure            */
  ORIGINAL_LAYER,         /* Draw all objects on one layer */
  LAYER_TRANS_BG,         /* New layers background         */
  PAINT_BRUSH_TYPE,       /* Default to use brushes        */
  FALSE,                  /* reverse lines                 */
  TRUE,                   /* Scale to image when painting  */
  1.0,                    /* Scale to image fp             */
  FALSE,                  /* Approx circles by drawing lines */
  BRUSH_BRUSH_TYPE,       /* Default to use a brush        */
  LINE                    /* Initial object type           */
};

selection_option selopt =
{
  ADD,          /* type */
  FALSE,        /* Antia */
  FALSE,        /* Feather */
  10.0,         /* feather radius */
  ARC_SEGMENT,  /* Arc as a segment */
  FILL_PATTERN, /* Fill as pattern */
  FILL_EACH,    /* Fill after each selection */
  100.0,        /* Max opacity */
};


static gchar *gfig_path       = NULL;
static GList *gfig_list       = NULL;
gint   line_no;

gint obj_show_single   = -1; /* -1 all >= 0 object number */

/* Structures etc for the objects */
/* Points used to draw the object  */


Dobject *obj_creating; /* Object we are creating */
Dobject *tmp_line;     /* Needed when drawing lines */

typedef struct BrushDesc
{
  gchar                *name;
  gdouble               opacity;
  gint                  spacing;
  GimpLayerModeEffects  paint_mode;
  gint                  width;
  gint                  height;
  guchar               *pv_buf;  /* Buffer where brush placed */
  gint16                x_off;
  gint16                y_off;
  const gchar          *popup;
} BrushDesc;


GFigObj  *current_obj;
GFigObj  *pic_obj;
static DAllObjs *undo_table[MAX_UNDO];
gint      need_to_scale;
static gint32    brush_image_ID = -1;

static GtkWidget *gfig_op_menu;    /* Popup menu in the list box */
static GtkWidget *object_list;     /* Top preview frame window */
static GtkWidget *fade_out_hbox;   /* Fade out widget in brush page */
static GtkWidget *gradient_hbox;   /* Gradient widget in brush page */
static GtkWidget *pressure_hbox;   /* Pressure widget in brush page */
static GtkWidget *pencil_hbox;     /* Dummy widget in brush page */
static GtkWidget *brush_page_widget; /* Widget for the brush part of notebook */
static GtkWidget *select_page_widget; /* Widget for the selection part
                                       * of notebook */

static gint       undo_water_mark = -1; /* Last slot filled in -1 = no undo */
gboolean       drawing_pic = FALSE;  /* If true drawing to the small preview */
static GFigObj   *gfig_obj_for_menu; /* More static data -
                                      * need to know which object was selected*/
static GtkWidget *save_menu_item;


/* Don't up just like BIGGG source files? */

static GFigObj  * gfig_load               (const gchar *filename,
                                           const gchar *name);
static void       free_all_objs           (DAllObjs * objs);
static GFigObj  * gfig_new                (void);
static void       clear_undo              (void);
static void       gfig_obj_modified       (GFigObj *obj, gint stat_type);
static void       gfig_op_menu_create     (GtkWidget *window);
static void       gridtype_menu_callback  (GtkWidget *widget, gpointer data);

static void       new_obj_2edit           (GFigObj *obj);
static gint       load_options            (GFigObj *gfig, FILE *fp);
static gint       gfig_obj_counts         (DAllObjs * objs);

static void    gfig_brush_fill_preview_xy (GtkWidget *pw, gint x , gint y);

static void      brush_list_button_callback (BrushDesc *bdesc);


/* globals */

static gint    gfig_run;
GdkGC  *gfig_gc;

/* Stuff for the preview bit */
static gint    sel_x1, sel_y1, sel_x2, sel_y2;
static gint    sel_width, sel_height;
gint    preview_width, preview_height;
gdouble scale_x_factor, scale_y_factor;
static gdouble org_scale_x_factor, org_scale_y_factor;

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "dummy", "dummy" }
  };

  gimp_install_procedure ("plug_in_gfig",
                          "Create Geometrical shapes with the Gimp",
                          "More here later",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1997",
                          N_("<Image>/Filters/Render/_Gfig..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpParam         *values = g_new (GimpParam, 1);
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  gint pwidth, pheight;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;
  gfig_image = param[1].data.d_image;
  gfig_drawable = param[2].data.d_drawable;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gfig_select_drawable = drawable = gimp_drawable_get (param[2].data.d_drawable);

  /* TMP Hack - clear any selections */
  if (! gimp_selection_is_empty (gfig_image))
    gimp_selection_clear (gfig_image);

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  /* Calculate preview size */

  if (sel_width > sel_height)
    {
      pwidth  = MIN (sel_width, PREVIEW_SIZE);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN (sel_height, PREVIEW_SIZE);
      pwidth  = sel_width * pheight / sel_height;
    }

  preview_width  = MAX (pwidth, 2);  /* Min size is 2 */
  preview_height = MAX (pheight, 2);

  org_scale_x_factor = scale_x_factor =
    (gdouble) sel_width / (gdouble) preview_width;
  org_scale_y_factor = scale_y_factor =
    (gdouble) sel_height / (gdouble) preview_height;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*gimp_get_data ("plug_in_gfig", &selvals);*/
      if (! gfig_dialog ())
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*gimp_get_data ("plug_in_gfig", &selvals);*/
      break;

    default:
      break;
    }

  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      /* Set the tile cache size */
      gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
                              gimp_tile_width ());

      do_gfig ();

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

#if 0
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data ("plug_in_gfig", &selvals, sizeof (SelectItVals));
#endif /* 0 */
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*
  Translate SPACE to "\\040", etc.
  Taken from gflare plugin
 */
static void
gfig_name_encode (gchar *dest,
                  gchar *src)
{
  gint cnt = MAX_LOAD_LINE - 1;

  while (*src && cnt--)
    {
      if (g_ascii_iscntrl (*src) || g_ascii_isspace (*src) || *src == '\\')
        {
          sprintf (dest, "\\%03o", *src++);
          dest += 4;
        }
      else
        *dest++ = *src++;
    }
  *dest = '\0';
}

/*
  Translate "\\040" to SPACE, etc.
 */
static void
gfig_name_decode (gchar       *dest,
                  const gchar *src)
{
  gint  cnt = MAX_LOAD_LINE - 1;
  guint tmp;

  while (*src && cnt--)
    {
      if (*src == '\\' && *(src+1) && *(src+2) && *(src+3))
        {
          sscanf (src+1, "%3o", &tmp);
          *dest++ = tmp;
          src += 4;
        }
      else
        *dest++ = *src++;
    }
  *dest = '\0';
}


/*
 * Load all gfig, which are founded in gfig-path-list, into gfig_list.
 * gfig-path-list must be initialized first. (plug_in_parse_gfig_path ())
 * based on code from Gflare.
 */

static gint
gfig_list_pos (GFigObj *gfig)
{
  GFigObj *g;
  gint n;
  GList *tmp;

  n = 0;

  for (tmp = gfig_list; tmp; tmp = g_list_next (tmp))
    {
      g = tmp->data;

      if (strcmp (gfig->draw_name, g->draw_name) <= 0)
        break;

      n++;
    }
  return n;
}

/*
 *      Insert gfigs in alphabetical order
 */

static gint
gfig_list_insert (GFigObj *gfig)
{
  gint n;

  n = gfig_list_pos (gfig);

  gfig_list = g_list_insert (gfig_list, gfig, n);

  return n;
}

static void
gfig_free (GFigObj *gfig)
{
  g_assert (gfig != NULL);

  free_all_objs (gfig->obj_list);

  g_free (gfig->name);
  g_free (gfig->filename);
  g_free (gfig->draw_name);

  g_free (gfig);
}

static void
gfig_free_everything (GFigObj *gfig)
{
  g_assert (gfig != NULL);

  if (gfig->filename)
    {
      remove (gfig->filename);
    }
  gfig_free (gfig);
}

static void
gfig_list_free_all (void)
{
  g_list_foreach (gfig_list, (GFunc) gfig_free, NULL);
  g_list_free (gfig_list);
  gfig_list = NULL;
}

static void
gfig_list_load_one (const GimpDatafileData *file_data,
                    gpointer                user_data)
{
  GFigObj *gfig;

  gfig = gfig_load (file_data->filename, file_data->basename);

  if (gfig)
    {
      /* Read only ?*/
      if (access (file_data->filename, W_OK))
        gfig->obj_status |= GFIG_READONLY;

      gfig_list_insert (gfig);
    }
}

static void
gfig_list_load_all (const gchar *path)
{
  /*  Make sure to clear any existing gfigs  */
  current_obj = pic_obj = NULL;
  gfig_list_free_all ();

  gimp_datafiles_read_directories (path, G_FILE_TEST_EXISTS,
                                   gfig_list_load_one,
                                   NULL);

  if (! gfig_list)
    {
      GFigObj *gfig;

      /* lets have at least one! */
      gfig = gfig_new ();
      gfig->draw_name = g_strdup (_("First Gfig"));
      gfig_list_insert (gfig);
    }

  pic_obj = current_obj = gfig_list->data;  /* set to first entry */
}

static GFigObj *
gfig_new (void)
{
  return g_new0 (GFigObj, 1);
}

static void
gfig_load_objs (GFigObj *gfig,
                gint     load_count,
                FILE    *fp)
{
  Dobject *obj;
  gchar load_buf[MAX_LOAD_LINE];

  /* Loading object */
  /*kill (getpid (), 19);*/
  /* Read first line */
  while (load_count-- > 0)
    {
      obj = NULL;
      get_line (load_buf, MAX_LOAD_LINE, fp, 0);

      if (!strcmp (load_buf, "<LINE>"))
        {
          obj = d_load_line (fp);
        }
      else if (!strcmp (load_buf, "<CIRCLE>"))
        {
          obj = d_load_circle (fp);
        }
      else if (!strcmp (load_buf, "<ELLIPSE>"))
        {
          obj = d_load_ellipse (fp);
        }
      else if (!strcmp (load_buf, "<POLY>"))
        {
          obj = d_load_poly (fp);
        }
      else if (!strcmp (load_buf, "<STAR>"))
        {
          obj = d_load_star (fp);
        }
      else if (!strcmp (load_buf, "<SPIRAL>"))
        {
          obj = d_load_spiral (fp);
        }
      else if (!strcmp (load_buf, "<BEZIER>"))
        {
          obj = d_load_bezier (fp);
        }
      else if (!strcmp (load_buf, "<ARC>"))
        {
          obj = d_load_arc (fp);
        }
      else
        {
          g_warning ("Unknown obj type file %s line %d\n", gfig->filename, line_no);
        }

      if (obj)
        {
          add_to_all_obj (gfig, obj);
        }
    }
}

static GFigObj *
gfig_load (const gchar *filename,
           const gchar *name)
{
  GFigObj *gfig;
  FILE    *fp;
  gchar    load_buf[MAX_LOAD_LINE];
  gchar    str_buf[MAX_LOAD_LINE];
  gint     chk_count;
  gint     load_count = 0;

  g_assert (filename != NULL);

#ifdef DEBUG
  printf ("Loading %s (%s)\n", filename, name);
#endif /* DEBUG */

  fp = fopen (filename, "r");
  if (!fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                  gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  gfig = gfig_new ();

  gfig->name = g_strdup (name);
  gfig->filename = g_strdup (filename);


  /* HEADER
   * draw_name
   * version
   * obj_list
   */

  get_line (load_buf, MAX_LOAD_LINE, fp, 1);

  if (strncmp (GFIG_HEADER, load_buf, strlen (load_buf)))
    {
      g_message ("File '%s' is not a gfig file",
                  gimp_filename_to_utf8 (gfig->filename));
      return NULL;
    }

  get_line (load_buf, MAX_LOAD_LINE, fp, 0);
  sscanf (load_buf, "Name: %100s", str_buf);
  gfig_name_decode (load_buf, str_buf);
  gfig->draw_name = g_strdup (load_buf);

  get_line (load_buf, MAX_LOAD_LINE, fp, 0);
  if (strncmp (load_buf, "Version: ", 9) == 0)
    gfig->version = g_ascii_strtod (load_buf + 9, NULL);

  get_line (load_buf, MAX_LOAD_LINE, fp, 0);
  sscanf (load_buf, "ObjCount: %d", &load_count);

  if (load_options (gfig, fp))
    {
      g_message ("File '%s' corrupt file - Line %d Option section incorrect",
                  gimp_filename_to_utf8 (filename), line_no);
      return NULL;
    }

  /*return (NULL);*/

  gfig_load_objs (gfig, load_count, fp);

  /* Check count ? */

  chk_count = gfig_obj_counts (gfig->obj_list);

  if (chk_count != load_count)
    {
      g_message ("File '%s' corrupt file - Line %d Object count to small",
                  gimp_filename_to_utf8 (filename), line_no);
      return NULL;
    }

  fclose (fp);

  if (!pic_obj)
    pic_obj = gfig;

  gfig->obj_status = GFIG_OK;

  return gfig;
}

static void
save_options (FILE *fp)
{
  /* Save options */
  fprintf (fp, "<OPTIONS>\n");
  fprintf (fp, "GridSpacing: %d\n", selvals.opts.gridspacing);
  if (selvals.opts.gridtype == RECT_GRID)
    fprintf (fp, "GridType: RECT_GRID\n");
  else if (selvals.opts.gridtype == POLAR_GRID)
    fprintf (fp, "GridType: POLAR_GRID\n");
  else if (selvals.opts.gridtype == ISO_GRID)
    fprintf (fp, "GridType: ISO_GRID\n");
  else fprintf (fp, "GridType: RECT_GRID\n"); /* If in doubt, default to RECT_GRID */
  fprintf (fp, "DrawGrid: %s\n", (selvals.opts.drawgrid)?"TRUE":"FALSE");
  fprintf (fp, "Snap2Grid: %s\n", (selvals.opts.snap2grid)?"TRUE":"FALSE");
  fprintf (fp, "LockOnGrid: %s\n", (selvals.opts.lockongrid)?"TRUE":"FALSE");
  /*  fprintf (fp, "ShowImage: %s\n", (selvals.opts.showimage)?"TRUE":"FALSE");*/
  fprintf (fp, "ShowControl: %s\n", (selvals.opts.showcontrol)?"TRUE":"FALSE");
  fprintf (fp, "</OPTIONS>\n");
}

static gint
load_bool (gchar *opt_buf,
           gint  *toset)
{
  if (!strcmp (opt_buf, "TRUE"))
    *toset = 1;
  else if (!strcmp (opt_buf, "FALSE"))
    *toset = 0;
  else
    return (-1);

  return (0);
}

static void
update_options (GFigObj *old_obj)
{
  /* Save old vals */
  if (selvals.opts.gridspacing != old_obj->opts.gridspacing)
    {
      old_obj->opts.gridspacing = selvals.opts.gridspacing;
    }
  if (selvals.opts.gridtype != old_obj->opts.gridtype)
    {
      old_obj->opts.gridtype = selvals.opts.gridtype;
    }
  if (selvals.opts.drawgrid != old_obj->opts.drawgrid)
    {
      old_obj->opts.drawgrid = selvals.opts.drawgrid;
    }
  if (selvals.opts.snap2grid != old_obj->opts.snap2grid)
    {
      old_obj->opts.snap2grid = selvals.opts.snap2grid;
    }
  if (selvals.opts.lockongrid != old_obj->opts.lockongrid)
    {
      old_obj->opts.lockongrid = selvals.opts.lockongrid;
    }
  if (selvals.opts.showcontrol != old_obj->opts.showcontrol)
    {
      old_obj->opts.showcontrol = selvals.opts.showcontrol;
    }

  /* New vals */
  if (selvals.opts.gridspacing != current_obj->opts.gridspacing)
    {
      gtk_adjustment_set_value
        (GTK_ADJUSTMENT (gfig_opt_widget.gridspacing),
         current_obj->opts.gridspacing);
    }
  if (selvals.opts.drawgrid != current_obj->opts.drawgrid)
    {
      gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON (gfig_opt_widget.drawgrid),
         current_obj->opts.drawgrid);
    }
  if (selvals.opts.snap2grid != current_obj->opts.snap2grid)
    {
      gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON (gfig_opt_widget.snap2grid),
         current_obj->opts.snap2grid);
    }
  if (selvals.opts.lockongrid != current_obj->opts.lockongrid)
    {
#if 0
      /* Maurits: code not implemented */
      gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON (gfig_opt_widget.lockongrid),
         current_obj->opts.lockongrid);
#endif
    }
  if (selvals.opts.showcontrol != current_obj->opts.showcontrol)
    {
      gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON (gfig_opt_widget.showcontrol),
         current_obj->opts.showcontrol);
    }
  if (selvals.opts.gridtype != current_obj->opts.gridtype)
    {
      gtk_option_menu_set_history
        (GTK_OPTION_MENU (gfig_opt_widget.gridtypemenu),
         current_obj->opts.gridtype);

      gridtype_menu_callback
        (gtk_menu_get_active
         (GTK_MENU (gtk_option_menu_get_menu
                    (GTK_OPTION_MENU (gfig_opt_widget.gridtypemenu)))),
         GINT_TO_POINTER (GRID_TYPE_MENU));

#ifdef DEBUG
      printf ("Gridtype set in options to ");
      if (current_obj->opts.gridtype == RECT_GRID)
        printf ("RECT_GRID\n");
      else if (current_obj->opts.gridtype == POLAR_GRID)
        printf ("POLAR_GRID\n");
      else if (current_obj->opts.gridtype == ISO_GRID)
        printf ("ISO_GRID\n");
      else printf ("NONE\n");
#endif /* DEBUG */
    }
}

static gint
load_options (GFigObj *gfig,
              FILE    *fp)
{
  gchar load_buf[MAX_LOAD_LINE];
  gchar str_buf[MAX_LOAD_LINE];
  gchar opt_buf[MAX_LOAD_LINE];

  get_line (load_buf, MAX_LOAD_LINE, fp, 0);

#ifdef DEBUG
  printf ("load '%s'\n", load_buf);
#endif /* DEBUG */

  if (strcmp (load_buf, "<OPTIONS>"))
    return (-1);

  get_line (load_buf, MAX_LOAD_LINE, fp, 0);

#ifdef DEBUG
  printf ("opt line '%s'\n", load_buf);
#endif /* DEBUG */

  while (strcmp (load_buf, "</OPTIONS>"))
    {
      /* Get option name */
#ifdef DEBUG
      printf ("num = %d\n", sscanf (load_buf, "%s %s", str_buf, opt_buf));

      printf ("option %s val %s\n", str_buf, opt_buf);
#else
      sscanf (load_buf, "%s %s", str_buf, opt_buf);
#endif /* DEBUG */

      if (!strcmp (str_buf, "GridSpacing:"))
        {
          /* Value is decimal */
          int sp = 0;
          sp = atoi (opt_buf);
          if (sp <= 0)
            return (-1);
          gfig->opts.gridspacing = sp;
        }
      else if (!strcmp (str_buf, "DrawGrid:"))
        {
          /* Value is bool */
          if (load_bool (opt_buf, &gfig->opts.drawgrid))
            return (-1);
        }
      else if (!strcmp (str_buf, "Snap2Grid:"))
        {
          /* Value is bool */
          if (load_bool (opt_buf, &gfig->opts.snap2grid))
            return (-1);
        }
      else if (!strcmp (str_buf, "LockOnGrid:"))
        {
          /* Value is bool */
          if (load_bool (opt_buf, &gfig->opts.lockongrid))
            return (-1);
        }
      else if (!strcmp (str_buf, "ShowControl:"))
        {
          /* Value is bool */
          if (load_bool (opt_buf, &gfig->opts.showcontrol))
            return (-1);
        }
      else if (!strcmp (str_buf, "GridType:"))
        {
          /* Value is string */
          if (!strcmp (opt_buf, "RECT_GRID"))
            gfig->opts.gridtype = RECT_GRID;
          else if (!strcmp (opt_buf, "POLAR_GRID"))
            gfig->opts.gridtype = POLAR_GRID;
          else if (!strcmp (opt_buf, "ISO_GRID"))
            gfig->opts.gridtype = ISO_GRID;
          else
            return (-1);
        }

      get_line (load_buf, MAX_LOAD_LINE, fp, 0);

#ifdef DEBUG
      printf ("opt line '%s'\n", load_buf);
#endif /* DEBUG */
    }
  return (0);
}

static gint
gfig_obj_counts (DAllObjs *objs)
{
  gint count = 0;

  for (; objs; objs = objs->next)
    count++;

  return count;
}

static void
gfig_save_callbk (void)
{
  FILE     *fp;
  DAllObjs *objs;
  gint      count = 0;
  gchar    *savename;
  gchar     buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar     conv_buf[MAX_LOAD_LINE*3 +1];

  savename = current_obj->filename;

  fp = fopen (savename, "w+");

  if (!fp)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (savename), g_strerror (errno));
      return;
    }

  /* Write header out */
  fputs (GFIG_HEADER, fp);

  /*
   * draw_name
   * version
   * obj_list
   *
   */

  gfig_name_encode (conv_buf, current_obj->draw_name);
  fprintf (fp, "Name: %s\n", conv_buf);
  fprintf (fp, "Version: %s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            current_obj->version));
  objs = current_obj->obj_list;

  count = gfig_obj_counts (objs);

  fprintf (fp, "ObjCount: %d\n", count);

  save_options (fp);

  for (objs = current_obj->obj_list; objs; objs = objs->next)
    {
      if (objs->obj->points)
        {
          objs->obj->savefunc (objs->obj, fp);
        }
    }

  if (ferror (fp))
    g_message ("Failed to write file\n");
  else
    {
      gfig_obj_modified (current_obj, GFIG_OK);
      current_obj->obj_status &= ~(GFIG_MODIFIED | GFIG_READONLY);
    }

  fclose (fp);

  gfig_update_stat_labels ();
}

static void
file_chooser_response (GtkFileChooser *chooser,
                       gint            response_id,
                       GFigObj        *obj)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar   *filename;
      GFigObj *real_current;

      filename = gtk_file_chooser_get_filename (chooser);

      obj->filename = filename;

      real_current = current_obj;
      current_obj = obj;
      gfig_save_callbk ();
      current_obj = current_obj;
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));
}

static void
create_save_file_chooser (GFigObj   *obj,
                          gchar     *tpath,
                          GtkWidget *parent)
{
  static GtkWidget *window = NULL;

  if (! window)
    {
      window =
        gtk_file_chooser_dialog_new (_("Save Gfig Drawing"),
                                     GTK_WINDOW (parent),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                     NULL);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &window);
      g_signal_connect (window, "response",
                        G_CALLBACK (file_chooser_response),
                        obj);
    }

  if (tpath)
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (window), tpath);
    }
  else if (gfig_path)
    {
      GList *list;
      gchar *dir;

      list = gimp_path_parse (gfig_path, 16, FALSE, 0);
      dir = gimp_path_get_user_writable_dir (list);
      gimp_path_free (list);

      if (! dir)
        dir = g_strdup (gimp_directory ());

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (window), dir);

      g_free (dir);
    }
  else
    {
      const gchar *tmp = g_get_tmp_dir ();

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (window), tmp);
    }

  gtk_window_present (GTK_WINDOW (window));
}

static void
gfig_save (GtkWidget *parent)
{
  /* Save the current object */
  if (!current_obj->filename)
   {
     create_save_file_chooser (current_obj, NULL, parent);
     return;
   }
  gfig_save_callbk ();
}

static GtkWidget *
gfig_list_item_new_with_label_and_pixmap (GFigObj   *obj,
                                          gchar     *label,
                                          GtkWidget *pix_widget)
{
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *hbox;

  list_item = gtk_list_item_new ();

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), hbox);
  gtk_widget_show (hbox);

  gtk_box_pack_start (GTK_BOX (hbox), pix_widget, FALSE, FALSE, 0);

  label_widget = gtk_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (label_widget), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (hbox), label_widget);

  gtk_widget_show (obj->label_widget = label_widget);
  gtk_widget_show (obj->pixmap_widget = pix_widget);
  gtk_widget_show (obj->list_item = list_item);

  return list_item;
}

static void
gfig_obj_modified (GFigObj *obj,
                   gint     stat_type)
{
  g_assert (obj != NULL);

  if (obj->obj_status == stat_type)
    return;

  /* Set the new one up */
  if (stat_type == GFIG_MODIFIED)
    gimp_pixmap_set (GIMP_PIXMAP (obj->pixmap_widget), Floppy6_xpm);
  else
    gimp_pixmap_set (GIMP_PIXMAP (obj->pixmap_widget), blank_xpm);
}

static void
select_button_clicked (GtkWidget *widget,
                       gpointer   data)
{
  gint      type  = GPOINTER_TO_INT (data);
  gint      count = 0;
  DAllObjs *objs;

  if (current_obj)
    {
      for (objs = current_obj->obj_list; objs; objs = objs->next)
        count++;
    }

  switch (type)
    {
    case OBJ_SELECT_LT:
      obj_show_single--;
      if (obj_show_single < 0)
        obj_show_single = count - 1;
      break;

    case OBJ_SELECT_GT:
      obj_show_single++;
      if (obj_show_single >= count)
        obj_show_single = 0;
      break;

    case OBJ_SELECT_EQ:
      obj_show_single = -1; /* Reset to show all */
      break;

    default:
      break;
    }

  draw_grid_clear ();
}

static GtkWidget *
obj_select_buttons (void)
{
  GtkWidget *button;
  GtkWidget *image;
  GtkWidget *hbox, *vbox;

  vbox = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new ();
  gimp_help_set_help_data (button, _("Show previous object"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (select_button_clicked),
                    GINT_TO_POINTER (OBJ_SELECT_LT));
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  button = gtk_button_new ();
  gimp_help_set_help_data (button, _("Show next object"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (select_button_clicked),
                    GINT_TO_POINTER (OBJ_SELECT_GT));
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  button = gtk_button_new_with_label (_("All"));
  gimp_help_set_help_data (button, _("Show all objects"), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (select_button_clicked),
                    GINT_TO_POINTER (OBJ_SELECT_EQ));
  gtk_widget_show (button);

  return vbox;
}

static GtkWidget *
but_with_pix (const gchar  *stock_id,
              GSList      **group,
              gint          baction)
{
  GtkWidget *button;

  button = gtk_radio_button_new_with_label (*group, stock_id);
  gtk_button_set_use_stock (GTK_BUTTON (button), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (toggle_obj_type),
                    GINT_TO_POINTER (baction));
  gtk_widget_show (button);

  *group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

  return button;
}

static GtkWidget *
small_preview (void)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  gint       y;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  pic_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (pic_preview),
                    SMALL_PREVIEW_SZ, SMALL_PREVIEW_SZ);
  gtk_container_add (GTK_CONTAINER (frame), pic_preview);
  gtk_widget_show (pic_preview);

  /* Fill with white */
  for (y = 0; y < SMALL_PREVIEW_SZ; y++)
    {
      guchar prow[SMALL_PREVIEW_SZ*3];
      memset (prow, 255, SMALL_PREVIEW_SZ * 3);
      gtk_preview_draw_row (GTK_PREVIEW (pic_preview), prow,
                            0, y, SMALL_PREVIEW_SZ);
    }

  g_signal_connect_after (pic_preview, "expose_event",
                          G_CALLBACK (pic_preview_expose),
                          NULL);

  return vbox;
}

/* Special case for now - options on poly/star/spiral button */

void
num_sides_dialog (gchar *d_title,
                  gint  *num_sides,
                  gint  *which_way,
                  gint   adj_min,
                  gint   adj_max)
{
  GtkWidget *window;
  GtkWidget *table;
  GtkObject *size_data;

  window = gimp_dialog_new (d_title, "gfig",
                            NULL, 0,
                            gimp_standard_help_func, HELP_ID,

                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                            NULL);

  g_signal_connect (window, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  table = gtk_table_new (which_way ? 2 : 1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  size_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                    _("Number of Sides/Points/Turns:"), 0, 0,
                                    *num_sides, adj_min, adj_max, 1, 10, 0,
                                    TRUE, 0, 0,
                                    NULL, NULL);
  g_signal_connect (size_data, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    num_sides);

  if (which_way)
    {
      GtkWidget *option_menu;

      option_menu =
        gimp_int_option_menu_new (FALSE, G_CALLBACK (gimp_menu_item_update),
                                  which_way, *which_way,

                                  _("Clockwise"),      0, NULL,
                                  _("Anti-Clockwise"), 1, NULL,

                                  NULL);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                 _("Orientation:"), 1.0, 0.5,
                                 option_menu, 1, TRUE);
    }

  gtk_widget_show (window);
}

static gint
bezier_button_press (GtkWidget      *widget,
                     GdkEventButton *event,
                     gpointer        data)
{
  if ((event->type == GDK_2BUTTON_PRESS) &&
      (event->button == 1))
    bezier_dialog ();
  return FALSE;
}

static GtkWidget *
draw_buttons (GtkWidget *ww)
{
  GtkWidget *button;
  GtkWidget *vbox;
  GSList    *group = NULL;

  /* Create group */
  vbox = gtk_vbox_new (FALSE, 0);

  /* Put buttons in */
  button = but_with_pix (GFIG_STOCK_LINE, &group, LINE);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Create line"), NULL);

  button = but_with_pix (GFIG_STOCK_CIRCLE, &group, CIRCLE);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Create circle"), NULL);

  button = but_with_pix (GFIG_STOCK_ELLIPSE, &group, ELLIPSE);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Create ellipse"), NULL);

  button = but_with_pix (GFIG_STOCK_CURVE, &group, ARC);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Create arch"), NULL);

  button = but_with_pix (GFIG_STOCK_POLYGON, &group, POLY);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "button_press_event",
                    G_CALLBACK (poly_button_press),
                    NULL);
  gimp_help_set_help_data (button, _("Create reg polygon"), NULL);

  button = but_with_pix (GFIG_STOCK_STAR, &group, STAR);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  g_signal_connect (button, "button_press_event",
                    G_CALLBACK (star_button_press),
                    NULL);
  gimp_help_set_help_data (button, _("Create star"), NULL);

  button = but_with_pix (GFIG_STOCK_SPIRAL, &group, SPIRAL);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "button_press_event",
                    G_CALLBACK (spiral_button_press),
                    NULL);
  gimp_help_set_help_data (button, _("Create spiral"), NULL);

  button = but_with_pix (GFIG_STOCK_BEZIER, &group, BEZIER);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  g_signal_connect (button, "button_press_event",
                    G_CALLBACK (bezier_button_press),
                    NULL);

  gimp_help_set_help_data (button,
                        _("Create bezier curve. "
                          "Shift + Button ends object creation."), NULL);

  button = but_with_pix (GFIG_STOCK_MOVE_OBJECT, &group, MOVE_OBJ);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Move an object"), NULL);

  button = but_with_pix (GFIG_STOCK_MOVE_POINT, &group, MOVE_POINT);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Move a single point"), NULL);

  button = but_with_pix (GFIG_STOCK_COPY_OBJECT, &group, COPY_OBJ);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Copy an object"), NULL);

  button = but_with_pix (GFIG_STOCK_DELETE_OBJECT, &group, DEL_OBJ);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Delete an object"), NULL);

  button = obj_select_buttons ();
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  gtk_widget_show (vbox);

  return vbox;
}

/* Brush preview stuff */
static gboolean
gfig_brush_preview_events (GtkWidget *widget,
                           GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  static GdkPoint point;
  static int have_start = 0;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      point.x = bevent->x;
      point.y = bevent->y;
      have_start = 1;
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      have_start = 0;
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      if (!have_start || !(mevent->state & GDK_BUTTON1_MASK))
        break;

      gfig_brush_fill_preview_xy (widget,
                                  point.x - mevent->x,
                                  point.y - mevent->y);
      gtk_widget_queue_draw (widget);
      point.x = mevent->x;
      point.y = mevent->y;
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gfig_brush_update_preview (GtkWidget *widget,
                           gpointer   data)
{
  BrushDesc *bdesc = g_object_get_data (G_OBJECT (data), "brush-desc");

  brush_list_button_callback (bdesc);
}

static void
gfig_brush_menu_callback (GtkWidget *widget,
                          gpointer   data)
{
  gimp_menu_item_update (widget, &selvals.brshtype);

  switch (selvals.brshtype)
    {
    case BRUSH_BRUSH_TYPE:
      gtk_widget_hide (pressure_hbox);
      gtk_widget_hide (pencil_hbox);
      gtk_widget_show (fade_out_hbox);
      gtk_widget_show (gradient_hbox);
      break;

    case BRUSH_PENCIL_TYPE:
      gtk_widget_hide (fade_out_hbox);
      gtk_widget_hide (gradient_hbox);
      gtk_widget_hide (pressure_hbox);
      gtk_widget_show (pencil_hbox);
      break;

    case BRUSH_AIRBRUSH_TYPE:
      gtk_widget_hide (fade_out_hbox);
      gtk_widget_hide (gradient_hbox);
      gtk_widget_hide (pencil_hbox);
      gtk_widget_show (pressure_hbox);
      break;

    case BRUSH_PATTERN_TYPE:
      gtk_widget_hide (fade_out_hbox);
      gtk_widget_hide (gradient_hbox);
      gtk_widget_hide (pressure_hbox);
      gtk_widget_show (pencil_hbox);
      break;

    default:
      g_warning ("Internal error - invalid brush type");
      break;
    }

  gfig_brush_update_preview (widget, data);
}

static GtkWidget *
gfig_brush_preview (GtkWidget **pv)
{
  GtkWidget *option_menu;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  gint y;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  gtk_widget_show (frame);

  *pv = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_widget_show (*pv);
  gtk_widget_set_events (GTK_WIDGET (*pv), PREVIEW_MASK);
  g_signal_connect (*pv, "event",
                    G_CALLBACK (gfig_brush_preview_events),
                    NULL);
  gtk_preview_size (GTK_PREVIEW (*pv), BRUSH_PREVIEW_SZ, BRUSH_PREVIEW_SZ);
  gtk_container_add (GTK_CONTAINER (frame), *pv);

  /* Fill with white */
  for (y = 0; y < BRUSH_PREVIEW_SZ; y++)
    {
      guchar prow[BRUSH_PREVIEW_SZ*3];
      memset (prow, -1, BRUSH_PREVIEW_SZ * 3);
      gtk_preview_draw_row (GTK_PREVIEW (*pv), prow, 0, y, BRUSH_PREVIEW_SZ);
    }

  /* Now the buttons */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  option_menu =
    gimp_int_option_menu_new (FALSE, G_CALLBACK (gfig_brush_menu_callback),
                              *pv, selvals.brshtype,

                              _("Brush"),    BRUSH_BRUSH_TYPE,    NULL,
                              _("Airbrush"), BRUSH_AIRBRUSH_TYPE, NULL,
                              _("Pencil"),   BRUSH_PENCIL_TYPE,   NULL,
                              _("Pattern"),  BRUSH_PATTERN_TYPE,  NULL,

                              NULL);
  gtk_widget_show (option_menu);

  gtk_container_add (GTK_CONTAINER (vbox), option_menu);
  gimp_help_set_help_data (option_menu,
                        _("Use the brush/pencil or the airbrush when drawing "
                          "on the image. Pattern paints with currently "
                          "selected brush with a pattern. Only applies to "
                          "circles/ellipses if Approx. Circles/Ellipses "
                          "toggle is set."), NULL);

  gtk_container_add (GTK_CONTAINER (hbox), vbox);
  gtk_container_add (GTK_CONTAINER (hbox), frame);

  return hbox;
}

static void
gfig_brush_fill_preview_xy (GtkWidget *pw,
                            gint       x1,
                            gint       y1)
{
  gint row_count;
  BrushDesc *bdesc = (BrushDesc *) g_object_get_data (G_OBJECT (pw),
                                                     "brush-desc");

  /* Adjust start position */
  bdesc->x_off += x1;
  bdesc->y_off += y1;

  if (bdesc->y_off < 0)
    bdesc->y_off = 0;
  if (bdesc->y_off > (bdesc->height - BRUSH_PREVIEW_SZ))
    bdesc->y_off = bdesc->height - BRUSH_PREVIEW_SZ;

  if (bdesc->x_off < 0)
    bdesc->x_off = 0;
  if (bdesc->x_off > (bdesc->width - BRUSH_PREVIEW_SZ))
    bdesc->x_off = bdesc->width - BRUSH_PREVIEW_SZ;

  /* Given an x and y fill preview in correctly offsetted */
  for (row_count = 0; row_count < BRUSH_PREVIEW_SZ; row_count++)
    gtk_preview_draw_row (GTK_PREVIEW (pw),
                          &bdesc->pv_buf[bdesc->x_off * 3
                                        + (bdesc->width
                                           * 3 * (row_count + bdesc->y_off))],
                          0,
                          row_count,
                          BRUSH_PREVIEW_SZ);
}

static void
gfig_brush_fill_preview (GtkWidget *pw,
                         gint32     layer_ID,
                         BrushDesc *bdesc)
{
  GimpPixelRgn  src_rgn;
  GimpDrawable *brushdrawable;

  g_free (bdesc->pv_buf); /* Free old area */

  brushdrawable = gimp_drawable_get (layer_ID);

  /* Fill the preview with the current brush name */
  gimp_pixel_rgn_init (&src_rgn, brushdrawable,
                       0, 0, bdesc->width, bdesc->height, FALSE, FALSE);

  bdesc->pv_buf = g_new (guchar, bdesc->width * bdesc->height * 3);
  bdesc->x_off = bdesc->y_off = 0; /* Start from top left */

  gimp_pixel_rgn_get_rect (&src_rgn, bdesc->pv_buf,
                           0, 0, bdesc->width, bdesc->height);

  /* Dump the pv_buf into the preview area */
  gfig_brush_fill_preview_xy (pw, 0, 0);
}

static void
mygimp_brush_set (const gchar *name)
{
  if (!gimp_brushes_set_brush (name))
    {
      g_message ("Can't set brush...(1)");
    }
}

static gchar *
mygimp_brush_get (void)
{
  gint width, height, spacing;

  return gimp_brushes_get_brush (&width, &height, &spacing);
}

static void
mygimp_brush_info (gint *width,
                   gint *height)
{
  char *name;
  gint spacing;

  name = gimp_brushes_get_brush (width, height, &spacing);
  if (name)
    {
      *width  = MAX (*width, 32);
      *height = MAX (*height, 32);
      g_free (name);
    }
  else
    {
      g_message ("Failed to get brush info");
      *width = *height = 48;
    }
}

void
gfig_paint (BrushType brush_type,
            gint32    drawable_ID,
            gint      seg_count,
            gdouble   line_pnts[])
{
  switch (brush_type)
    {
    case BRUSH_BRUSH_TYPE:
      gimp_paintbrush (drawable_ID,
                       selvals.brushfade,
                       seg_count, line_pnts,
                       GIMP_PAINT_CONSTANT,
                       selvals.brushgradient);
      break;

    case BRUSH_PENCIL_TYPE:
      gimp_pencil (drawable_ID,
                   seg_count, line_pnts);
      break;

    case BRUSH_AIRBRUSH_TYPE:
      gimp_airbrush (drawable_ID,
                     selvals.airbrushpressure,
                     seg_count, line_pnts);
      break;

    case BRUSH_PATTERN_TYPE:
      gimp_clone (drawable_ID,
                  drawable_ID,
                  GIMP_PATTERN_CLONE,
                  0.0, 0.0,
                  seg_count, line_pnts);
      break;
    }
}

static gint32
gfig_gen_brush_preview (BrushDesc *bdesc)
{
  /* Given the name of a brush then paint it and return the ID of the image
   * the preview can be got from
   */
  static  gint32 layer_ID = -1;
  gchar  *saved_name;
  gint32  width, height;
  gdouble line_pnts[2];
  GimpRGB foreground;
  GimpRGB background;
  GimpRGB color;

  if (brush_image_ID == -1)
    {
      /* Create a new image */
      brush_image_ID = gimp_image_new (48, 48, 0);
      if (brush_image_ID < 0)
        {
          g_message ("Failed to generate brush preview");
          return -1;
        }
      if ((layer_ID = gimp_layer_new (brush_image_ID,
                                      "Brush preview",
                                      48,
                                      48,
                                      GIMP_RGB_IMAGE,
                                      100.0, /* opacity */
                                      GIMP_NORMAL_MODE)) < 0)
        {
          g_message ("Error in creating layer for brush preview");
          return -1;
        }
      gimp_image_add_layer (brush_image_ID, layer_ID, -1);
    }

  /* Need this later to delete it */

  /* Store foreground & backgroud colours set to black/white
   * paint with brush
   * restore colours
   */

  gimp_palette_get_foreground (&foreground);
  gimp_palette_get_background (&background);

  saved_name = mygimp_brush_get ();

  gimp_rgba_set (&color, 1.0, 1.0, 1.0, 1.0);
  gimp_palette_set_background (&color);
  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
  gimp_palette_set_foreground (&color);

  mygimp_brush_set (bdesc->name);

  mygimp_brush_info (&width, &height);
  bdesc->width = width;
  bdesc->height = height;
  line_pnts[0] = (gdouble) width / 2;
  line_pnts[1] = (gdouble) height / 2;

  gimp_layer_resize (layer_ID, width, height, 0, 0);
  gimp_image_resize (brush_image_ID, width, height, 0, 0);

  gimp_drawable_fill (layer_ID, GIMP_BACKGROUND_FILL);

  /* Blob of paint */
  gfig_paint (selvals.brshtype,
              layer_ID,
              2, line_pnts);

  gimp_palette_set_background (&background);
  gimp_palette_set_foreground (&foreground);

  mygimp_brush_set (saved_name);

  g_free (saved_name);

  return layer_ID;
}

static void
brush_list_button_callback (BrushDesc *bdesc)
{
  gint32 layer_ID;

  if ((layer_ID = gfig_gen_brush_preview (bdesc)) != -1)
    {
      g_object_set_data (G_OBJECT (brush_page_pw), "brush-desc", bdesc);
      gfig_brush_fill_preview (brush_page_pw, layer_ID, bdesc);
      gtk_widget_queue_draw (brush_page_pw);
    }
}

/* Build the dialog up. This was the hard part! */

static GtkWidget *page_menu_bg;
static GtkWidget *page_menu_layers;

static void
paint_menu_callback (GtkWidget *widget,
                     gpointer   data)
{
  gint mtype = GPOINTER_TO_INT (data);

  if (mtype == PAINT_LAYERS_MENU)
    {
      selvals.onlayers = (DrawonLayers)
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));

#ifdef DEBUG
      printf ("layer type set to %s\n",
              selvals.onlayers == SINGLE_LAYER ? "SINGLE_LAYER" : "MULTI_LAYER");
#endif /* DEBUG */

      /* Type only meaningful if creating new layers */
      if (selvals.onlayers == ORIGINAL_LAYER)
        gtk_widget_set_sensitive (page_menu_bg, FALSE);
      else
        gtk_widget_set_sensitive (page_menu_bg, TRUE);
    }
  else if (mtype == PAINT_BGS_MENU)
    {
      selvals.onlayerbg = (LayersBGType)
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));
#ifdef DEBUG
      printf ("BG type = %d\n", selvals.onlayerbg);
#endif /* DEBUG */
    }
  else if (mtype == PAINT_TYPE_MENU)
    {
      selvals.painttype = (PaintType)
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));

#ifdef DEBUG
      printf ("Got type menu = %d\n", selvals.painttype);
#endif /* DEBUG */

      switch (selvals.painttype)
        {
        case PAINT_BRUSH_TYPE:
          gtk_widget_set_sensitive (select_page_widget, FALSE);
          gtk_widget_set_sensitive (brush_page_widget, TRUE);
          gtk_widget_set_sensitive (page_menu_layers, TRUE);
          if (selvals.onlayers == ORIGINAL_LAYER)
            gtk_widget_set_sensitive (page_menu_bg, FALSE);
          else
            gtk_widget_set_sensitive (page_menu_bg, TRUE);
          break;
        case PAINT_SELECTION_TYPE:
          gtk_widget_set_sensitive (select_page_widget, TRUE);
          gtk_widget_set_sensitive (brush_page_widget, FALSE);
          gtk_widget_set_sensitive (page_menu_layers, FALSE);
          gtk_widget_set_sensitive (page_menu_bg, FALSE);
          break;
        case PAINT_SELECTION_FILL_TYPE:
          gtk_widget_set_sensitive (select_page_widget, TRUE);
          gtk_widget_set_sensitive (brush_page_widget, FALSE);
          gtk_widget_set_sensitive (page_menu_layers, TRUE);
          if (selvals.onlayers == ORIGINAL_LAYER)
            gtk_widget_set_sensitive (page_menu_bg, FALSE);
          else
            gtk_widget_set_sensitive (page_menu_bg, TRUE);
          break;
        default:
          break;
        }
    }
}

static GtkWidget *
paint_page (void)
{
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *toggle;
  GtkWidget *page_menu_type;
  GtkWidget *scale_scale;
  GtkObject *scale_scale_data;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  page_menu_layers =
    gimp_int_option_menu_new (FALSE, G_CALLBACK (paint_menu_callback),
                              GINT_TO_POINTER (PAINT_LAYERS_MENU), 0,

                              _("Original"), ORIGINAL_LAYER, NULL,
                              _("New"),      SINGLE_LAYER,   NULL,
                              _("Multiple"), MULTI_LAYER,    NULL,

                              NULL);

  gimp_help_set_help_data (page_menu_layers,
                        _("Draw all objects on one layer (original or new) "
                          "or one object per layer"), NULL);
  if (gimp_drawable_is_channel (gfig_drawable))
      gtk_widget_set_sensitive (page_menu_layers, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Draw on:"), 1.0, 0.5,
                             page_menu_layers, 1, TRUE);

  page_menu_type =
    gimp_int_option_menu_new (FALSE, G_CALLBACK (paint_menu_callback),
                              GINT_TO_POINTER (PAINT_TYPE_MENU), 0,

                              _("Brush"),          PAINT_BRUSH_TYPE,          NULL,
                              _("Selection"),      PAINT_SELECTION_TYPE,      NULL,
                              _("Selection+Fill"), PAINT_SELECTION_FILL_TYPE, NULL,

                             NULL);

  gimp_help_set_help_data (page_menu_type,
                        _("Draw type. Either a brush or a selection. "
                          "See brush page or selection page for more options"),
                        NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Using:"), 1.0, 0.5,
                             page_menu_type, 1, TRUE);

  page_menu_bg =
    gimp_int_option_menu_new (FALSE, G_CALLBACK (paint_menu_callback),
                              GINT_TO_POINTER (PAINT_BGS_MENU), 0,

                              _("Transparent"), LAYER_TRANS_BG, NULL,
                              _("Background"),  LAYER_BG_BG,    NULL,
                              _("Foreground"),  LAYER_FG_BG,    NULL,
                              _("White"),       LAYER_WHITE_BG, NULL,
                              _("Copy"),        LAYER_COPY_BG,  NULL,

                              NULL);
  gimp_help_set_help_data (page_menu_bg,
                        _("Layer background type. Copy causes previous "
                          "layer to be copied before the draw is performed"),
                        NULL);
  gtk_widget_set_sensitive (page_menu_bg, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("With BG of:"), 1.0, 0.5,
                             page_menu_bg, 1, TRUE);

  toggle = gtk_check_button_new_with_label (_("Reverse Line"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 4, 5,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.reverselines);
  gimp_help_set_help_data (toggle,
                        _("Draw lines in reverse order"), NULL);
  gtk_widget_show (toggle);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), vbox2, 0, 1, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (vbox2);

  toggle = gtk_check_button_new_with_label (_("Scale to Image"));
  gtk_box_pack_end (GTK_BOX (vbox2), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                selvals.scaletoimage);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gfig_scale2img_update),
                    &selvals.scaletoimage);
  gimp_help_set_help_data (toggle,
                        _("Scale drawings to images size"), NULL);
  gtk_widget_show (toggle);

  hbox = gtk_hbox_new (FALSE, 1);
  scale_scale_data = gtk_adjustment_new (1.0, 0.1, 5.0, 0.01, 0.01, 0.0);
  scale_scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), scale_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale_scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale_scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale_scale), GTK_UPDATE_CONTINUOUS);
  g_signal_connect (scale_scale_data, "value_changed",
                    G_CALLBACK (gfig_scale_update_scale),
                    &selvals.scaletoimagefp);
  gtk_widget_show (scale_scale);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 3, 4,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  gtk_widget_set_sensitive (GTK_WIDGET (scale_scale), FALSE);
  g_object_set_data (G_OBJECT (toggle), "inverse_sensitive", scale_scale);
  g_object_set_data (G_OBJECT (toggle), "user_data", scale_scale_data);

  toggle = gtk_check_button_new_with_label (_("Approx. Circles/Ellipses"));
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 4, 5,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.approxcircles);
  gimp_help_set_help_data (toggle,
                        _("Approx. circles & ellipses using lines. Allows "
                          "the use of brush fading with these types of "
                          "objects."), NULL);
  gtk_widget_show (toggle);

  return vbox;
}

static void
gfig_brush_invoker (const gchar          *name,
                    gdouble               opacity,
                    gint                  spacing,
                    GimpLayerModeEffects  paint_mode,
                    gint                  width,
                    gint                  height,
                    const guchar         *mask_data,
                    gboolean              closing,
                    gpointer              data)
{
  BrushDesc *bdesc = (BrushDesc *) data;

  g_free (bdesc->name);

  bdesc->name       = g_strdup (name);
  bdesc->width      = width;
  bdesc->height     = height;
  bdesc->opacity    = opacity;
  bdesc->spacing    = spacing;
  bdesc->paint_mode = paint_mode;

  brush_list_button_callback (bdesc);

  if (closing)
    bdesc->popup = NULL;
}

static void
select_brush_callback (GtkWidget *widget,
                       gpointer   data)
{
  BrushDesc *bdesc = (BrushDesc *) data;

  if (bdesc->popup)
    /*  calling gimp_brushes_set_popup() raises the dialog  */
    gimp_brushes_set_popup (bdesc->popup,
                            bdesc->name,
                            bdesc->opacity,
                            bdesc->spacing,
                            bdesc->paint_mode);
  else
    bdesc->popup = gimp_brush_select_new (_("Gfig Brush Selection"),
                                          bdesc->name,
                                          100.0,  /*  opacity  */
                                          -1,     /*  spacing  */
                                          GIMP_NORMAL_MODE,
                                          gfig_brush_invoker,
                                          bdesc);

  brush_list_button_callback (bdesc);
}

static GtkWidget *
brush_page (void)
{
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *pw;
  GtkWidget *scale;
  GtkObject *fade_out_scale_data;
  GtkObject *gradient_scale_data;
  GtkObject *pressure_scale_data;
  GtkWidget *vbox;
  GtkWidget *button;
  BrushDesc *bdesc = g_new0 (BrushDesc, 1); /* Initial brush settings */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Fade option */
  /*  the fade-out scale  From GIMP itself*/
  fade_out_hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Fade out:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (fade_out_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  fade_out_scale_data = gtk_adjustment_new (0.0, 0.0, 3000.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (fade_out_scale_data));
  gtk_box_pack_start (GTK_BOX (fade_out_hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  g_signal_connect (fade_out_scale_data, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &selvals.brushfade);
  gtk_widget_show (scale);

  gtk_table_attach (GTK_TABLE (table), fade_out_hbox, 0, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (fade_out_hbox);


  /* Gradient drawing */
  gradient_hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Gradient:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (gradient_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gradient_scale_data = gtk_adjustment_new (0.0, 0.0, 3000.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (gradient_scale_data));
  gtk_box_pack_start (GTK_BOX (gradient_hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  g_signal_connect (gradient_scale_data, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &selvals.brushgradient);
  gtk_widget_show (scale);
  gtk_table_attach (GTK_TABLE (table), gradient_hbox, 0, 2, 2, 3,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (gradient_hbox);


  pressure_hbox = gtk_hbox_new (FALSE, 4);
  label = gtk_label_new (_("Pressure:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (pressure_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pressure_scale_data = gtk_adjustment_new (20.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (pressure_scale_data));
  gtk_box_pack_start (GTK_BOX (pressure_hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  g_signal_connect (pressure_scale_data, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &selvals.airbrushpressure);
  gtk_table_attach (GTK_TABLE (table), pressure_hbox, 0, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  pencil_hbox = gtk_hbox_new (FALSE, 4);
  label = gtk_label_new (_("No Options..."));
  gtk_box_pack_start (GTK_BOX (pencil_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), pencil_hbox, 0, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  /* Preview widget */
  pw = gfig_brush_preview (&brush_page_pw);
  gtk_table_attach (GTK_TABLE (table), pw, 0, 1, 0, 1, 0, 0, 0, 0);

  g_signal_connect (pressure_scale_data, "value_changed",
                    G_CALLBACK (gfig_brush_update_preview),
                    brush_page_pw);

  /* Start of new brush selection code */
  brush_sel_button = button = gtk_button_new_with_label (_("Set Brush..."));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (brush_sel_button)->child), 2, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (select_brush_callback),
                    bdesc);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             NULL, 0, 0,
                             button, 1,  TRUE);

  /* Setup initial brush settings */
  bdesc->name = mygimp_brush_get ();
  brush_list_button_callback (bdesc);

  return vbox;
}

static void
select_menu_callback (GtkWidget *widget,
                      gpointer   data)
{
  gint mtype = GPOINTER_TO_INT (data);

  if (mtype == SELECT_TYPE_MENU)
    {
      SelectionType type = (SelectionType)
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));

      selopt.type = type;
    }
  else if (mtype == SELECT_ARCTYPE_MENU)
    {
      ArcType type = (ArcType)
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));

      selopt.as_pie = type;
    }
  else if (mtype == SELECT_TYPE_MENU_FILL)
    {
      FillType type = (FillType)
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));

      selopt.fill_type = type;
    }
  else if (mtype == SELECT_TYPE_MENU_WHEN)
    {
      FillWhen type = (FillWhen)
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));
      selopt.fill_when = type;
    }
}

static GtkWidget *
select_page (void)
{
  GtkWidget *menu;
  GtkWidget *toggle;
  GtkWidget *scale;
  GtkObject *scale_data;
  GtkWidget *table;
  GtkWidget *vbox;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

  table = gtk_table_new (4, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* The secltion settings -
   * 1) Type (option menu)
   * 2) Anti A (toggle)
   * 3) Feather (toggle)
   * 4) F radius (slider)
   * 5) Fill type (option menu)
   * 6) Opacity (slider)
   * 7) When to fill (toggle)
   * 8) Arc as segment/sector
   */

  /* 1 */
  menu = gimp_int_option_menu_new (FALSE, G_CALLBACK (select_menu_callback),
                                   GINT_TO_POINTER (SELECT_TYPE_MENU), 0,

                                   _("Add"),       ADD, NULL,
                                   _("Subtract"),  SUBTRACT, NULL,
                                   _("Replace"),   REPLACE, NULL,
                                   _("Intersect"), INTERSECT, NULL,

                                   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Selection Type:"), 1.0, 0.5,
                             menu, 1, FALSE);

  /* 2 */
  toggle = gtk_check_button_new_with_label (_("Antialiasing"));
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 4, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selopt.antia);
  gtk_widget_show (toggle);

  /* 3 */
  toggle = gtk_check_button_new_with_label (_("Feather"));
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 4, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selopt.feather);
  gtk_widget_show (toggle);

  /* 4 */
  scale_data =
    gtk_adjustment_new (selopt.feather_radius, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  g_signal_connect (scale_data, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &selopt.feather_radius);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 3,
                             _("Radius:"), 1.0, 1.0,
                             scale, 1, FALSE);

  /* 5 */
  menu =
    gimp_int_option_menu_new (FALSE, G_CALLBACK (select_menu_callback),
                              GINT_TO_POINTER (SELECT_TYPE_MENU_FILL), 0,

                              _("Pattern"),    FILL_PATTERN,    NULL,
                              _("Foreground"), FILL_FOREGROUND, NULL,
                              _("Background"), FILL_BACKGROUND, NULL,

                              NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Fill Type:"), 1.0, 0.5,
                             menu, 1, FALSE);

  /* 6 */
  scale_data =
    gtk_adjustment_new (selopt.fill_opacity, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  g_signal_connect (scale_data, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &selopt.fill_opacity);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 1,
                             _("Fill Opacity:"), 1.0, 1.0,
                             scale, 1, FALSE);

  /* 7 */
  menu =
    gimp_int_option_menu_new (FALSE, G_CALLBACK (select_menu_callback),
                              GINT_TO_POINTER (SELECT_TYPE_MENU_WHEN), 0,

                              _("Each Selection"), FILL_EACH,  NULL,
                              _("All Selections"), FILL_AFTER, NULL,

                              NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Fill after:"), 1.0, 0.5,
                             menu, 1, FALSE);

  /* 8 */
  menu = gimp_int_option_menu_new (FALSE, G_CALLBACK (select_menu_callback),
                                   GINT_TO_POINTER (SELECT_ARCTYPE_MENU), 0,

                                   _("Segment"), ARC_SEGMENT, NULL,
                                   _("Sector"),  ARC_SECTOR,  NULL,

                                   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Arc as:"), 1.0, 0.5,
                             menu, 1, FALSE);

  return vbox;
}

static void
gridtype_menu_callback (GtkWidget *widget,
                        gpointer   data)
{
  gint mtype = GPOINTER_TO_INT (data);

  if (mtype == GRID_TYPE_MENU)
    {
      selvals.opts.gridtype =
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));
    }
  else
    {
      grid_gc_type =
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));
    }

  draw_grid_clear ();
}

static GtkWidget *
options_page (void)
{
  GtkWidget *table;
  GtkWidget *menu;
  GtkWidget *toggle;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkObject *size_data;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Put buttons in */
  toggle = gtk_check_button_new_with_label (_("Show Image"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.showimage);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_show_image),
                    NULL);
  gtk_widget_show (toggle);

  button = gtk_button_new_with_label (_("Reload Image"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (reload_button_callback),
                    NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             NULL, 0, 0,
                             button, 1, TRUE);

  menu = gimp_int_option_menu_new (FALSE, G_CALLBACK (gridtype_menu_callback),
                                   GINT_TO_POINTER (GRID_TYPE_MENU), 0,

                                   _("Rectangle"), RECT_GRID,  NULL,
                                   _("Polar"),     POLAR_GRID, NULL,
                                   _("Isometric"), ISO_GRID,   NULL,

                                   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Grid Type:"), 1.0, 0.5,
                             menu, 1, TRUE);

  gfig_opt_widget.gridtypemenu = menu;

  menu =
    gimp_int_option_menu_new (FALSE, G_CALLBACK (gridtype_menu_callback),
                              GINT_TO_POINTER (GRID_RENDER_MENU), 0,

                              _("Normal"),     GTK_STATE_NORMAL,   NULL,
                              _("Black"),      GFIG_BLACK_GC,      NULL,
                              _("White"),      GFIG_WHITE_GC,      NULL,
                              _("Grey"),       GFIG_GREY_GC,       NULL,
                              _("Darker"),     GTK_STATE_ACTIVE,   NULL,
                              _("Lighter"),    GTK_STATE_PRELIGHT, NULL,
                              _("Very Dark"),  GTK_STATE_SELECTED, NULL,

                              NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Grid Color:"), 1.0, 0.5,
                             menu, 1, TRUE);

  size_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                                    _("Max Undo:"), 0, 50,
                                    selvals.maxundo, MIN_UNDO, MAX_UNDO, 1, 2, 0,
                                    TRUE, 0, 0,
                                    NULL, NULL);
  g_signal_connect (size_data, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &selvals.maxundo);

  toggle = gtk_check_button_new_with_label (_("Show Position"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 4, 5,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.showpos);
  g_signal_connect_after (toggle, "toggled",
                          G_CALLBACK (gfig_pos_enable),
                          NULL);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Hide Control Points"));
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 3, 4, 5,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.opts.showcontrol);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_show_image),
                    NULL);
  gtk_widget_show (toggle);
  gfig_opt_widget.showcontrol = toggle;

  button = gtk_button_new_with_label (_("About"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 8, 0);
  gtk_table_attach (GTK_TABLE (table), button, 1, 3, 5, 6,
                    0, 0, 0, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (about_button_callback),
                    NULL);
  gtk_widget_show (button);

  return vbox;
}

static GtkWidget *
grid_frame (void)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkObject *size_data;

  frame = gtk_frame_new (_("Grid"));

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle = gtk_check_button_new_with_label (_("Show Grid"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.opts.drawgrid);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (draw_grid_clear),
                    NULL);
  gtk_widget_show (toggle);
  gfig_opt_widget.drawgrid = toggle;

  toggle = gtk_check_button_new_with_label (_("Snap to Grid"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.opts.snap2grid);
  gtk_widget_show (toggle);
  gfig_opt_widget.snap2grid = toggle;

#if 0
  /* 17/10/2003 (Maurits): this option is not implemented. Therefore removing
     it from the user interface */

  toggle = gtk_check_button_new_with_label (_("Lock on Grid"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.opts.lockongrid);
  gtk_widget_show (toggle);
  gfig_opt_widget.lockongrid = toggle;
#endif
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  size_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                    _("Grid spacing:"), 0, 50,
                                    selvals.opts.gridspacing,
                                    MIN_GRID, MAX_GRID, 1, 10, 0,
                                    TRUE, 0, 0,
                                    NULL, NULL);
  g_signal_connect (size_data, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &selvals.opts.gridspacing);
  g_signal_connect (size_data, "value_changed",
                    G_CALLBACK (draw_grid_clear),
                    NULL);
  gfig_opt_widget.gridspacing = size_data;

  gtk_widget_show (frame);

  return frame;
}

static void
clear_list_items (GtkList *list)
{
  gtk_list_clear_items (list, 0, -1);
}

static void
build_list_items (GtkWidget *list)
{
  GList     *tmp;
  GtkWidget *list_item;
  GtkWidget *list_pix;
  GFigObj   *g;

  for (tmp = gfig_list; tmp; tmp = g_list_next (tmp))
    {
      g = tmp->data;

      if (g->obj_status & GFIG_READONLY)
        list_pix = gimp_pixmap_new (mini_cross_xpm);
      else
        list_pix = gimp_pixmap_new (blank_xpm);

      list_item =
        gfig_list_item_new_with_label_and_pixmap (g, g->draw_name, list_pix);

      g_object_set_data (G_OBJECT (list_item), "user_data", g);
      gtk_list_append_items (GTK_LIST (list), g_list_append (NULL, list_item));

      g_signal_connect (list_item, "button_press_event",
                        G_CALLBACK (list_button_press),
                        g);
      gtk_widget_show (list_item);
    }
}

static GtkWidget *
add_objects_list (void)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *scrolled_win;
  GtkWidget *list;
  GtkWidget *button;

  frame = gtk_frame_new (_("Object"));
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  gfig_gtk_list = list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);

  gtk_box_pack_start (GTK_BOX (hbox), small_preview (), FALSE, FALSE, 0);

  object_list = scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
                                         list);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);
  gtk_widget_show (list);

  /* Load saved objects */
  gfig_list_load_all (gfig_path);

  /* Put list in */
  build_list_items (list);

  /* Put buttons in */
  vbox = gtk_vbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (new_button_callback),
                    NULL);
  gimp_help_set_help_data (button, _("Create a new Gfig object collection "
                                     "for editing"), NULL);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (load_button_callback),
                    list);
  gimp_help_set_help_data (button,
                           _("Load a single Gfig object collection"), NULL);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GIMP_STOCK_EDIT);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (edit_button_callback),
                    list);
  gimp_help_set_help_data (button, _("Edit Gfig object collection"), NULL);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  button = gtk_button_new_with_mnemonic (_("_Merge"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (merge_button_callback),
                    list);
  gimp_help_set_help_data (button, _("Merge Gfig Object collection into the "
                                     "current edit session"), NULL);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (gfig_delete_gfig_callback),
                    list);
  gimp_help_set_help_data (button, _("Delete currently selected Gfig Object "
                                     "collection"), NULL);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (rescan_button_callback),
                    NULL);
  gimp_help_set_help_data (button,
                        _("Select folder and rescan Gfig object collections"),
                           NULL);
  gtk_container_add (GTK_CONTAINER (vbox), button);
  gtk_widget_show (button);

  return frame;
}

#if 0 /* NOT USED */
static void
gfig_obj_size_update (gint sz)
{
  static gchar buf[256];

  sprintf (buf, "%6d", sz);
  gtk_label_set_text (GTK_LABEL (obj_size_label), buf);
}

static GtkWidget *
gfig_obj_size_label (void)
{
  GtkWidget *label;
  GtkWidget *hbox;

  hbox = gtk_hbox_new (TRUE, 6);

  /* Position labels */
  label = gtk_label_new (_("Size:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  obj_size_label = gtk_label_new ("0");
  gtk_misc_set_alignment (GTK_MISC (obj_size_label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), obj_size_label, FALSE, FALSE, 0);
  gtk_widget_show (obj_size_label);

  gtk_widget_show (hbox);

  return (hbox);
}

#endif /* NOT USED */

static gint
gfig_dialog (void)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *notebook;
  GtkWidget *page;

  gimp_ui_init ("gfig", TRUE);
  gfig_stock_init ();

  gfig_path = gimp_gimprc_query ("gfig-path");

  if (! gfig_path)
    {
      gchar *gimprc = gimp_personal_rc_file ("gimprc");
      gchar *full_path;
      gchar *esc_path;

      full_path =
        g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, "gfig",
                     G_SEARCHPATH_SEPARATOR_S,
                     "${gimp_data_dir}", G_DIR_SEPARATOR_S, "gfig",
                     NULL);
      esc_path = g_strescape (full_path, NULL);
      g_free (full_path);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                 "gfig-path", "gfig-path", esc_path,
                 gimp_filename_to_utf8 (gimprc));

      g_free (gimprc);
      g_free (esc_path);
    }

  img_width  = gimp_drawable_width (gfig_select_drawable->drawable_id);
  img_height = gimp_drawable_height (gfig_select_drawable->drawable_id);

  /* Start building the dialog up */
  top_level_dlg = gimp_dialog_new (_("Gfig"), "gfig",
                                   NULL, 0,
                                   gimp_standard_help_func, HELP_ID,

                                   GTK_STOCK_UNDO,   RESPONSE_UNDO,
                                   GTK_STOCK_CLEAR,  RESPONSE_CLEAR,
                                   GTK_STOCK_SAVE,   RESPONSE_SAVE,
                                   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                   GTK_STOCK_CLOSE,  GTK_RESPONSE_OK,
                                   _("Paint"),       RESPONSE_PAINT,

                                   NULL);

  g_signal_connect (top_level_dlg, "response",
                    G_CALLBACK (gfig_response),
                    top_level_dlg);
  g_signal_connect (top_level_dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (top_level_dlg),
                                     RESPONSE_UNDO, undo_water_mark >= 0);

  main_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (top_level_dlg)->vbox), main_hbox,
                      TRUE, TRUE, 0);

  /* Add buttons beside the preview frame */
  gtk_box_pack_start (GTK_BOX (main_hbox),
                      draw_buttons (top_level_dlg), FALSE, FALSE, 0);

  /* Preview itself */
  gtk_box_pack_start (GTK_BOX (main_hbox), make_preview (), FALSE, FALSE, 0);

  gtk_widget_show (gfig_preview);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* listbox + entry */
  gtk_box_pack_start (GTK_BOX (vbox), add_objects_list (), TRUE, TRUE, 0);

  /* Grid entry */
  gtk_box_pack_start (GTK_BOX (vbox), grid_frame (), FALSE, FALSE, 0);

  /* The notebook */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  page = paint_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                            gtk_label_new (_("Paint")));
  gtk_widget_show (page);

  brush_page_widget = brush_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), brush_page_widget,
                            gtk_label_new (_("Brush")));
  gtk_widget_show (brush_page_widget);

  /* Sometime maybe allow all objects to be done by selections - this
   * would adjust the selection options.
   */
  select_page_widget = select_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), select_page_widget,
                            gtk_label_new (_("Select")));
  gtk_widget_show (select_page_widget);
  gtk_widget_set_sensitive (select_page_widget, FALSE);

  page = options_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                            gtk_label_new (_("Options")));
  gtk_widget_show (page);

  gtk_widget_show (main_hbox);

  gtk_widget_show (top_level_dlg);

  dialog_update_preview (gfig_select_drawable);
  gfig_new_gc (); /* Need this for drawing */
  gfig_update_stat_labels ();

  gfig_grid_colours (gfig_preview);
  /* Popup for list area */
  gfig_op_menu_create (top_level_dlg);

  gtk_main ();

  gimp_image_delete (brush_image_ID);
  brush_image_ID = -1;

  return gfig_run;
}

static void
gfig_really_ok_callback (GtkWidget *widget,
                         gboolean   doit,
                         gpointer   data)
{
  if (doit)
    {
      gfig_run = TRUE;
      gtk_widget_destroy (GTK_WIDGET (data));
    }
}

static void
gfig_response (GtkWidget *widget,
               gint       response_id,
               gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_UNDO:
      if (undo_water_mark >= 0)
        {
          /* Free current objects an reinstate previous */
          free_all_objs (current_obj->obj_list);
          current_obj->obj_list = NULL;
          tmp_bezier = tmp_line = obj_creating = NULL;
          current_obj->obj_list = undo_table[undo_water_mark];
          undo_water_mark--;
          /* Update the screen */
          gtk_widget_queue_draw (gfig_preview);
          /* And preview */
          list_button_update (current_obj);
          gfig_obj_modified (current_obj, GFIG_MODIFIED);
          current_obj->obj_status |= GFIG_MODIFIED;
        }

      gtk_dialog_set_response_sensitive (GTK_DIALOG (widget),
                                         RESPONSE_UNDO, undo_water_mark >= 0);
      break;

    case RESPONSE_CLEAR:
      /* Make sure we can get back - if we have some objects to get back to */
      if (!current_obj->obj_list)
        return;

      setup_undo ();
      /* Free all objects */
      free_all_objs (current_obj->obj_list);
      current_obj->obj_list = NULL;
      obj_creating = NULL;
      tmp_line = NULL;
      tmp_bezier = NULL;
      gtk_widget_queue_draw (gfig_preview);
      /* And preview */
      list_button_update (current_obj);
      break;

    case RESPONSE_SAVE:
      gfig_save (widget);  /* Save current object */
      break;

    case GTK_RESPONSE_CLOSE:
      {
        /* Check if outstanding saves */
        GList   *list;
        GFigObj *gfig;
        gint     count = 0;

        for (list = gfig_list; list; list = g_list_next (list))
          {
            gfig = (GFigObj *) list->data;
            if (gfig->obj_status & GFIG_MODIFIED)
              count++;
          }

        if (count)
          {
            GtkWidget *dialog;
            gchar     *message;

            message =
              g_strdup_printf (_("%d unsaved Gfig objects. "
                                 "Continue with exiting?"), count);

            dialog = gimp_query_boolean_box (_("Warning"),
                                             widget,
                                             gimp_standard_help_func, HELP_ID,
                                             GTK_STOCK_DIALOG_WARNING,
                                             message,
                                             GTK_STOCK_OK, GTK_STOCK_CANCEL,
                                             NULL, NULL,
                                             gfig_really_ok_callback,
                                             data);
            g_free (message);

            gtk_widget_show (dialog);
          }
        else
          {
            gfig_run = TRUE;
            gtk_widget_destroy (GTK_WIDGET (data));
          }
      }
      break;

    case RESPONSE_PAINT:
      gfig_paint_callback ();
      break;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static gboolean
pic_preview_expose (GtkWidget *widget,
                    GdkEvent  *event)
{
  if (pic_obj)
    {
      drawing_pic = TRUE;
      draw_objects (pic_obj->obj_list, FALSE);
      drawing_pic = FALSE;
    }
  return FALSE;
}

static gint
adjust_pic_coords (gint coord,
                   gint ratio)
{
  /*return ((SMALL_PREVIEW_SZ * coord)/PREVIEW_SIZE);*/
  static gint pratio = -1;

  if (pratio == -1)
    {
      pratio = MAX (preview_width, preview_height);
    }

  return (SMALL_PREVIEW_SZ * coord) / pratio;
}

/*
 *  The edit gfig name attributes dialog
 *  Modified from Gimp source - layer edit.
 */

typedef struct _GfigListOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *list_entry;
  GFigObj   *obj;
  gboolean   created;
} GfigListOptions;

static GtkWidget *
gfig_list_add (GFigObj *obj)
{
  GList     *list;
  gint       pos;
  GtkWidget *list_item;
  GtkWidget *list_pix;

  list_pix  = gimp_pixmap_new (Floppy6_xpm);
  list_item =
    gfig_list_item_new_with_label_and_pixmap (obj, obj->draw_name, list_pix);

  g_object_set_data (G_OBJECT (list_item), "user_data", obj);

  pos = gfig_list_insert (obj);

  list = g_list_append (NULL, list_item);
  gtk_list_insert_items (GTK_LIST (gfig_gtk_list), list, pos);
  gtk_widget_show (list_item);
  gtk_list_select_item (GTK_LIST (gfig_gtk_list), pos);

  g_signal_connect (list_item, "button_press_event",
                    G_CALLBACK (list_button_press),
                    obj);

  return list_item;
}

static void
gfig_list_response (GtkWidget       *widget,
                    gint             response_id,
                    GfigListOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkWidget *list;
      gint       pos;

      list = options->list_entry;

      /*  Set the new layer name  */

      g_free (options->obj->draw_name);
      options->obj->draw_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

      /* Need to reorder the list */
      /* gtk_label_set_text (GTK_LABEL (options->layer_widget->label), layer->name);*/
      pos = gtk_list_child_position (GTK_LIST (gfig_gtk_list), list);

      gtk_list_clear_items (GTK_LIST (gfig_gtk_list), pos, pos + 1);

      /* remove/Add again */
      gfig_list = g_list_remove (gfig_list, options->obj);
      gfig_list_add (options->obj);

      options->obj->obj_status |= GFIG_MODIFIED;

      gfig_update_stat_labels ();
    }
  else
    {
      if (options->created)
        {
          /* We are creating an entry so if cancelled
           * must del the list item as well
           */
          gfig_do_delete_gfig_callback (widget, TRUE, gfig_gtk_list);
        }
   }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
gfig_dialog_edit_list (GtkWidget *lwidget,
                       GFigObj   *obj,
                       gboolean   created)
{
  GfigListOptions *options;
  GtkWidget       *vbox;
  GtkWidget       *hbox;
  GtkWidget       *label;

  /*  the new options structure  */
  options = g_new (GfigListOptions, 1);
  options->list_entry = lwidget;
  options->obj        = obj;
  options->created    = created;

  /*  the dialog  */
  options->query_box =
    gimp_dialog_new (_("Enter Gfig Object Name"), "gfig",
                     NULL, 0,
                     gimp_standard_help_func, HELP_ID,

                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                     NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (gfig_list_response),
                    options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Gfig Object Name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), obj->draw_name);
  gtk_widget_show (options->name_entry);

  gtk_widget_show (options->query_box);
}

static void
gfig_rescan_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkWidget *patheditor;

      gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);

      patheditor = GTK_WIDGET (g_object_get_data (G_OBJECT (data),
                                              "patheditor"));

      g_free (gfig_path);
      gfig_path = gimp_path_editor_get_path (GIMP_PATH_EDITOR (patheditor));

      if (gfig_path)
        {
          clear_list_items (GTK_LIST (gfig_gtk_list));
          gfig_list_load_all (gfig_path);
          build_list_items (gfig_gtk_list);
          list_button_update (current_obj);
        }
    }

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
gfig_rescan_list (void)
{
  static GtkWidget *dlg = NULL;

  GtkWidget *patheditor;

  if (dlg)
    {
      gtk_window_present (GTK_WINDOW (dlg));
      return;
    }

  /*  the dialog  */
  dlg = gimp_dialog_new (_("Rescan for Gfig Objects"), "gfig",
                         NULL, 0,
                         gimp_standard_help_func, HELP_ID,

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (gfig_rescan_response),
                    dlg);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &dlg);

  patheditor = gimp_path_editor_new (_("Add Gfig Path"), gfig_path);
  gtk_container_set_border_width (GTK_CONTAINER (patheditor), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), patheditor,
                      TRUE, TRUE, 0);
  gtk_widget_show (patheditor);

  g_object_set_data (G_OBJECT (dlg), "patheditor", patheditor);

  gtk_widget_show (dlg);
}

void
list_button_update (GFigObj *obj)
{
  g_return_if_fail (obj != NULL);

  pic_obj = (GFigObj *) obj;

  gtk_widget_queue_draw (pic_preview);

  drawing_pic = TRUE;
  draw_objects (pic_obj->obj_list, FALSE);
  drawing_pic = FALSE;
}


static void
gfig_load_file_chooser_response (GtkFileChooser *chooser,
                                 gint            response_id,
                                 gpointer        data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar   *filename;
      GFigObj *gfig;
      GFigObj *current_saved;

      filename = gtk_file_chooser_get_filename (chooser);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          /* Hack - current object MUST be NULL to prevent setup_undo ()
           * from kicking in.
           */
          current_saved = current_obj;
          current_obj = NULL;
          gfig = gfig_load (filename, filename);
          current_obj = current_saved;

          if (gfig)
            {
              /* Read only ?*/
              if (access (filename, W_OK))
                gfig->obj_status |= GFIG_READONLY;

              gfig_list_add (gfig);
              new_obj_2edit (gfig);
            }
        }

      g_free (filename);
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));
}

static void
load_button_callback (GtkWidget *widget,
                      gpointer   data)
{
  static GtkWidget *window = NULL;

  /* Load a single object */
  window = gtk_file_chooser_dialog_new (_("Load Gfig object collection"),
                                        GTK_WINDOW (gtk_widget_get_toplevel (widget)),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,

                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                        NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &window);
  g_signal_connect (window, "response",
                    G_CALLBACK (gfig_load_file_chooser_response),
                    window);

  gtk_widget_show (window);
}

static void
paint_layer_copy (gchar *new_name)
{
  gint32 old_drawable = gfig_drawable;

  if ((gfig_drawable = gimp_layer_copy (gfig_drawable)) < 0)
    {
      g_warning (_("Error in copy layer for onlayers"));
      gfig_drawable = old_drawable;
      return;
    }

  gimp_drawable_set_name (gfig_drawable, new_name);
  gimp_image_add_layer (gfig_image, gfig_drawable, -1);
}

static void
paint_layer_new (gchar *new_name)
{
  GimpFillType   fill_type;
  GimpImageType  type;
  gint32         layer_id;

  switch (gimp_drawable_type (gfig_select_drawable->drawable_id))
    {
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      type = GIMP_GRAYA_IMAGE;
      break;

    default:
      type = GIMP_RGBA_IMAGE;
      break;
    }

  if ((layer_id = gimp_layer_new (gfig_image,
                                  new_name,
                                  img_width,
                                  img_height,
                                  type,
                                  100.0, /* opacity */
                                  GIMP_NORMAL_MODE)) < 0)
    {
      g_warning ("Error in creating layer");
      return;
    }

  gimp_image_add_layer (gfig_image, layer_id, -1);

  gfig_drawable = layer_id;

  switch (selvals.onlayerbg)
    {
    case LAYER_TRANS_BG:
      fill_type = GIMP_TRANSPARENT_FILL;
      break;
    case LAYER_BG_BG:
      fill_type = GIMP_BACKGROUND_FILL;
      break;
    case LAYER_FG_BG:
      fill_type = GIMP_FOREGROUND_FILL;
      break;
    case LAYER_WHITE_BG:
      fill_type = GIMP_WHITE_FILL;
      break;
    default:
      fill_type = GIMP_BACKGROUND_FILL;
      g_warning ("Paint layer new internal error %d\n", selvals.onlayerbg);
      break;
    }
  /* Have to clear layer out since creating transparent layer
   * seems to leave rubbish in it.
   */

  gimp_drawable_fill (layer_id, fill_type);

}

static void
paint_layer_fill (void)
{
  gimp_edit_bucket_fill (gfig_drawable,
                         selopt.fill_type,    /* Fill mode */
                         GIMP_NORMAL_MODE,
                         selopt.fill_opacity, /* Fill opacity */
                         0.0,                 /* threshold - ignored */
                         FALSE,               /* Sample merged - ignored */
                         0.0,                 /* x - ignored */
                         0.0);                /* y - ignored */
}

static void
gfig_paint_callback (void)
{
  DAllObjs  *objs;
  gint       layer_count = 0;
  gchar      buf[128];
  gint       count;
  gint       ccount = 0;
  BrushDesc *bdesc;

  objs = current_obj->obj_list;

  count = gfig_obj_counts (objs);
#if 0
  gtk_progress_bar_update (GTK_PROGRESS_BAR (progress_widget), (gfloat) 0.0);
#endif

  /* Set the brush up */
  bdesc = g_object_get_data (G_OBJECT (brush_page_pw), "brush-desc");

  if (bdesc)
    mygimp_brush_set (bdesc->name);

  gimp_image_undo_group_start (gfig_image);

  while (objs)
    {
      if (ccount == obj_show_single || obj_show_single == -1)
        {
          sprintf (buf, _("Gfig Layer %d"), layer_count++);

          if (selvals.painttype != PAINT_SELECTION_TYPE)
            {
              switch (selvals.onlayers)
                {
                case SINGLE_LAYER:
                  if (layer_count == 1)
                    {
                      if (selvals.onlayerbg == LAYER_COPY_BG)
                        paint_layer_copy (buf);
                      else
                        paint_layer_new (buf);
                    }
                  break;

                case MULTI_LAYER:
                  if (selvals.onlayerbg == LAYER_COPY_BG)
                    paint_layer_copy (buf);
                  else
                    paint_layer_new (buf);
                  break;

                case ORIGINAL_LAYER:
                  /* Just use the given layer */
                  break;

                default:
                  g_warning ("Error in onlayers val %d", selvals.onlayers);
                  break;
                }
            }

          objs->obj->paintfunc (objs->obj);

          /* Fill layer if required */
          if (selvals.painttype == PAINT_SELECTION_FILL_TYPE
              && selopt.fill_when == FILL_EACH)
            paint_layer_fill ();
        }

      objs = objs->next;

      ccount++;
    }

  /* Fill layer if required */
  if (selvals.painttype == PAINT_SELECTION_FILL_TYPE
      && selopt.fill_when == FILL_AFTER)
    paint_layer_fill ();

  gimp_image_undo_group_end (gfig_image);

  gimp_displays_flush ();
}

static void
reload_button_callback (GtkWidget *widget,
                        gpointer   data)
{
  refill_cache (gfig_select_drawable);
  draw_grid_clear ();
}

static void
about_button_callback (GtkWidget *widget,
                       gpointer   data)
{
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *image;

  window = gimp_dialog_new (_("About Gfig"), "gfig",
                            NULL, 0,
                            gimp_standard_help_func, HELP_ID,

                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                            NULL);

  g_signal_connect (window, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  image = gtk_image_new_from_stock (GFIG_STOCK_LOGO, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_widget_show (vbox);

  gtk_box_pack_start (GTK_BOX (hbox), image, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
                      hbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Gfig - GIMP plug-in"));
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  label = gtk_label_new (_("Release 2.0"));
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Andy Thomas");
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Isometric grid By Rob Saunders");
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  gtk_widget_show (window);
}

static void
rescan_button_callback (GtkWidget *widget,
                        gpointer   data)
{
  gfig_rescan_list ();
}

static GtkWidget *
new_gfig_obj (const gchar *name)
{
  GFigObj   *gfig;
  GtkWidget *new_list_item;
  /* Create a new entry */

  gfig = gfig_new ();

  if (!name)
    name = _("New Gfig Object");

  gfig->draw_name = g_strdup (name);

  /* Leave options as before */
  pic_obj = current_obj = gfig;

  new_list_item = gfig_list_add (gfig);

  tmp_bezier = obj_creating = tmp_line = NULL;

  /* Redraw areas */
  gtk_widget_queue_draw (gfig_preview);
  list_button_update (gfig);

  return new_list_item;
}

static void
new_button_callback (GtkWidget *widget,
                     gpointer   data)
{
  GtkWidget *new_list_item;

  new_list_item = new_gfig_obj (NULL);
  gfig_dialog_edit_list (new_list_item, current_obj, TRUE);
}

static GtkWidget *delete_dialog = NULL;

static void
gfig_do_delete_gfig_callback (GtkWidget *widget,
                              gboolean   delete,
                              gpointer   data)
{
  gint       pos;
  GList     *sellist;
  GFigObj   *sel_obj;
  GtkWidget *list = (GtkWidget *) data;

  if (!delete)
    {
      gtk_widget_set_sensitive (object_list, TRUE);
      return;
    }

  /* Must update which object we are editing */
  /* Get the list and which item is selected */
  /* Only allow single selections */

  sellist = GTK_LIST (list)->selection;

  sel_obj = (GFigObj *) g_object_get_data (G_OBJECT (sellist->data),
                                           "user_data");

  pos = gtk_list_child_position (GTK_LIST (gfig_gtk_list), sellist->data);

  /* Delete the current  item + asssociated file */
  gtk_list_clear_items (GTK_LIST (gfig_gtk_list), pos, pos + 1);
  /* Shadow copy for ordering info */
  gfig_list = g_list_remove (gfig_list, sel_obj);

  if (sel_obj == current_obj)
    {
      clear_undo ();
    }

  /* Free current obj */
  gfig_free_everything (sel_obj);

  /* Select previous one */
  pos--;

  if (pos < 0)
    {
      if (g_list_length (gfig_list) == 0)
        {
          /* Warning - we have a problem here
           * since we are not really "creating an entry"
           * why call gfig_new?
           */
          new_button_callback (NULL, NULL);
        }

      pos = 0;
    }

  gtk_widget_set_sensitive (object_list, TRUE);

  gtk_list_select_item (GTK_LIST (gfig_gtk_list), pos);

  current_obj = g_list_nth (gfig_list, pos)->data;

  gtk_widget_queue_draw (gfig_preview);

  list_button_update (current_obj);

  gfig_update_stat_labels ();
}

static void
gfig_delete_gfig_callback (GtkWidget *widget,
                           gpointer   data)
{
  gchar     *str;
  GtkWidget *list = (GtkWidget *) data;
  GList     *sellist;
  GFigObj   *sel_obj;

  sellist = GTK_LIST (list)->selection;

  sel_obj = (GFigObj *) g_object_get_data (G_OBJECT (sellist->data),
                                           "user_data");

  if (delete_dialog)
    return;

  str = g_strdup_printf (_("Are you sure you want to delete "
                           "\"%s\" from the list and from disk?"),
                         sel_obj->draw_name);

  delete_dialog = gimp_query_boolean_box (_("Delete Gfig Drawing"),
                                          gtk_widget_get_toplevel (list),
                                          gimp_standard_help_func, HELP_ID,
                                          FALSE,
                                          str,
                                          GTK_STOCK_DELETE, GTK_STOCK_CANCEL,
                                          NULL, NULL,
                                          gfig_do_delete_gfig_callback,
                                          data);

  g_free (str);

  g_signal_connect (delete_dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &delete_dialog);

  gtk_widget_set_sensitive (GTK_WIDGET (object_list), FALSE);
  gtk_widget_show (delete_dialog);
}

static void
new_obj_2edit (GFigObj *obj)
{
  GFigObj *old_current = current_obj;

  /* Clear undo levels */
  /* redraw the preview */
  /* Set up options as define in the selected object */

  clear_undo ();

  /* Point at this one */
  current_obj = obj;

  /* Show all objects to start with */
  obj_show_single = -1;

  /* Change options */
  update_options (old_current);

  /* If have old object and NOT scaleing currently then force
   * back to saved coord type.
   */
  gfig_update_stat_labels ();

  /* redraw with new */
  gtk_widget_queue_draw (gfig_preview);
  /* And preview */
  list_button_update (current_obj);

  if (obj->obj_status & GFIG_READONLY)
    {
      g_message (_("Editing read-only object - "
                   "you will not be able to save it"));
      gtk_dialog_set_response_sensitive (GTK_DIALOG (top_level_dlg),
                                         RESPONSE_SAVE, FALSE);
    }
  else
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (top_level_dlg),
                                         RESPONSE_SAVE, TRUE);
    }
}

static void
edit_button_callback (GtkWidget *widget,
                      gpointer   data)
{
  GList     *sellist;
  GFigObj   *sel_obj;
  GtkWidget *list = (GtkWidget *) data;

  /* Must update which object we are editing */
  /* Get the list and which item is selected */
  /* Only allow single selections */

  sellist = GTK_LIST (list)->selection;

  sel_obj = (GFigObj *) g_object_get_data (G_OBJECT (sellist->data),
                                           "user_data");

  if (sel_obj)
    new_obj_2edit (sel_obj);
}

static void
merge_button_callback (GtkWidget *widget,
                       gpointer   data)
{
  GList     *sellist;
  GFigObj   *sel_obj;
  DAllObjs  *obj_copies;
  GtkWidget *list = (GtkWidget *) data;

  /* Must update which object we are editing */
  /* Get the list and which item is selected */
  /* Only allow single selections */

  sellist = GTK_LIST (list)->selection;

  sel_obj = (GFigObj *) g_object_get_data (G_OBJECT (sellist->data),
                                           "user_data");

  if (sel_obj && sel_obj->obj_list && sel_obj != current_obj)
    {
      /* Copy list tag onto current & redraw */
      obj_copies = copy_all_objs (sel_obj->obj_list);
      prepend_to_all_obj (current_obj, obj_copies);

      /* redraw all */
      gtk_widget_queue_draw (gfig_preview);
      /* And preview */
      list_button_update (current_obj);
    }
}

static void
gfig_save_menu_callback (GtkWidget *widget,
                         gpointer   data)
{
  GFigObj * real_current = current_obj;
  /* Fiddle the current object and save it */
  /* What happens if we get a redraw here ? */

  current_obj = gfig_obj_for_menu;

  gfig_save (GTK_WIDGET (data));  /* Save current object */

  current_obj = real_current;
}

static void
gfig_edit_menu_callback (GtkWidget *widget,
                         gpointer   data)
{
  new_obj_2edit (gfig_obj_for_menu);
}

static void
gfig_rename_menu_callback (GtkWidget *widget,
                           gpointer   data)
{
  create_save_file_chooser (gfig_obj_for_menu, gfig_obj_for_menu->filename,
                            gtk_widget_get_toplevel (widget));
}

static void
gfig_copy_menu_callback (GtkWidget *widget,
                         gpointer   data)
{
  /* Create new entry with name + copy at end & copy object into it */
  gchar *new_name = g_strdup_printf (_("%s copy"),
                                     gfig_obj_for_menu->draw_name);
  new_gfig_obj (new_name);
  g_free (new_name);

  /* Copy objs across */
  current_obj->obj_list = copy_all_objs (gfig_obj_for_menu->obj_list);
  current_obj->opts = gfig_obj_for_menu->opts; /* Structure copy */

  /* redraw all */
  gtk_widget_queue_draw (gfig_preview);
  /* And preview */
  list_button_update (current_obj);
}

static void
gfig_op_menu_create (GtkWidget *window)
{
  GtkWidget *menu_item;
#if 0
  GtkAcceleratorTable *accelerator_table;
#endif /* 0 */

  gfig_op_menu = gtk_menu_new ();

#if 0
  accelerator_table = gtk_accelerator_table_new ();
  gtk_menu_set_accelerator_table (GTK_MENU (gfig_op_menu),
                                  accelerator_table);
  gtk_window_add_accelerator_table (GTK_WINDOW (window), accelerator_table);
#endif /* 0 */

  menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE, NULL);
  save_menu_item = menu_item;
  gtk_menu_shell_append (GTK_MENU_SHELL (gfig_op_menu), menu_item);
  gtk_widget_show (menu_item);

  g_signal_connect (menu_item, "activate",
                    G_CALLBACK (gfig_save_menu_callback),
                    window);

#if 0
  gtk_widget_install_accelerator (menu_item,
                                  accelerator_table,
                                  "activate", 'S', 0);
#endif /* 0 */

  menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE_AS, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (gfig_op_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
                    G_CALLBACK (gfig_rename_menu_callback),
                    NULL);

#if 0
  gtk_widget_install_accelerator (menu_item,
                                  accelerator_table,
                                  "activate", 'A', 0);
#endif /* 0 */

  menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (gfig_op_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
                    G_CALLBACK (gfig_copy_menu_callback),
                    NULL);

#if 0
  gtk_widget_install_accelerator (menu_item,
                                  accelerator_table,
                                  "activate", 'C', 0);
#endif /* 0 */

  menu_item = gtk_image_menu_item_new_from_stock (GIMP_STOCK_EDIT, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (gfig_op_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
                    G_CALLBACK (gfig_edit_menu_callback),
                    NULL);

#if 0
  gtk_widget_install_accelerator (menu_item,
                                  accelerator_table,
                                  "activate", 'E', 0);
#endif /* 0 */

}

static void
gfig_op_menu_popup (gint     button,
                    guint32  activate_time,
                    GFigObj *obj)
{
  gfig_obj_for_menu = obj; /* Static data again!*/

  if (obj->obj_status & GFIG_READONLY)
    {
      gtk_widget_set_sensitive (save_menu_item, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (save_menu_item, TRUE);
    }

  gtk_menu_popup (GTK_MENU (gfig_op_menu),
                  NULL, NULL, NULL, NULL,
                  button, activate_time);
}

static gboolean
list_button_press (GtkWidget      *widget,
                   GdkEventButton *event,
                   gpointer        data)
{
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button == 3)
        {
          gfig_op_menu_popup (event->button, event->time, (GFigObj *) data);
          return FALSE;
        }
      list_button_update ((GFigObj *) data);
      break;

    case GDK_2BUTTON_PRESS:
      gfig_dialog_edit_list (widget, data, FALSE);
      break;

    default:
      g_warning ("gfig: unknown event.\n");
      break;
    }

  return FALSE;
}

static void
gfig_scale_update_scale (GtkAdjustment *adjustment,
                         gdouble       *value)
{
  gimp_double_adjustment_update (adjustment, value);

  if (!selvals.scaletoimage)
    {
      scale_x_factor = (1 / (*value)) * org_scale_x_factor;
      scale_y_factor = (1 / (*value)) * org_scale_y_factor;
      gtk_widget_queue_draw (gfig_preview);
    }
}

/* Use to toggle the toggles */
static void
gfig_scale2img_update (GtkWidget *widget,
                       gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  if (*((gint *) data))
    {
      GtkObject *adj;

      adj = g_object_get_data (G_OBJECT (widget), "user_data");

      scale_x_factor = org_scale_x_factor;
      scale_y_factor = org_scale_y_factor;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), 1.0);

      gtk_widget_queue_draw (gfig_preview);
    }
}

static void
gfig_new_gc (void)
{
  GdkColor fg, bg;

  /*  create a new graphics context  */
  gfig_gc = gdk_gc_new (gfig_preview->window);

  gdk_gc_set_function (gfig_gc, GDK_INVERT);

  fg.pixel = 0xFFFFFFFF;
  bg.pixel = 0x00000000;
  gdk_gc_set_foreground (gfig_gc, &fg);
  gdk_gc_set_background (gfig_gc, &bg);

  gdk_gc_set_line_attributes (gfig_gc, 1,
                              GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
}

/* Given a point x, y draw a circle */
void
draw_circle (GdkPoint *p)
{
  if (!selvals.opts.showcontrol || drawing_pic)
    return;

  gdk_draw_arc (gfig_preview->window,
                gfig_gc,
                0,
                p->x - SQ_SIZE/2,
                p->y - SQ_SIZE/2,
                SQ_SIZE,
                SQ_SIZE,
                0,
                360*64);
}

/* Given a point x, y draw a square around it */
void
draw_sqr (GdkPoint *p)
{
  if (!selvals.opts.showcontrol || drawing_pic)
    return;

  gdk_draw_rectangle (gfig_preview->window,
                      gfig_gc,
                      0,
                      gfig_scale_x (p->x) - SQ_SIZE/2,
                      gfig_scale_y (p->y) - SQ_SIZE/2,
                      SQ_SIZE,
                      SQ_SIZE);
}

/* Draw the grid on the screen
 */

void
draw_grid_clear ()
{
  /* wipe slate and start again */
  dialog_update_preview (gfig_select_drawable);
  gtk_widget_queue_draw (gfig_preview);
}

static void
toggle_show_image (void)
{
  /* wipe slate and start again */
  draw_grid_clear ();
}

void
toggle_obj_type (GtkWidget *widget,
                 gpointer   data)
{
  static GdkCursor *p_cursors[DEL_OBJ + 1];
  GdkCursorType     ctype = GDK_LAST_CURSOR;

  if (selvals.otype != (DobjType) GPOINTER_TO_INT (data))
    {
      /* Mem leak */
      obj_creating = NULL;
      tmp_line = NULL;
      tmp_bezier = NULL;

      if ((DobjType)data < MOVE_OBJ)
        {
          obj_show_single = -1; /* Cancel select preview */
        }
      /* Update draw areas */
      gtk_widget_queue_draw (gfig_preview);
      /* And preview */
      list_button_update (current_obj);
    }

  selvals.otype = (DobjType) GPOINTER_TO_INT (data);

  switch (selvals.otype)
    {
    case LINE:
    case CIRCLE:
    case ELLIPSE:
    case ARC:
    case POLY:
    case STAR:
    case SPIRAL:
    case BEZIER:
    default:
      ctype = GDK_CROSSHAIR;
      break;
    case MOVE_OBJ:
    case MOVE_POINT:
    case COPY_OBJ:
    case MOVE_COPY_OBJ:
      ctype = GDK_DIAMOND_CROSS;
      break;
    case DEL_OBJ:
      ctype = GDK_PIRATE;
      break;
    }

  if (!p_cursors[selvals.otype])
    {
      GdkDisplay *display = gtk_widget_get_display (widget);

      p_cursors[selvals.otype] = gdk_cursor_new_for_display (display, ctype);
    }

  gdk_window_set_cursor (gfig_preview->window, p_cursors[selvals.otype]);
}

static void
do_gfig (void)
{
  /* Not sure if requre post proc - leave stub in */
}

/* This could belong in a separate file ... but makes it easier to lump into
 * one when compiling the plugin.
 */

/* Stuff for the generation/deletion of objects. */

/* Objects are easy one they are created - you just go down the object
 * list calling the draw function for each object but... when they
 * are been created we have to be a little more careful. When
 * the first point is placed on the canvas we create the object,
 * the mouse position then defines the next point that can move around.
 * careful how we draw this position.
 */

void
free_one_obj (Dobject *obj)
{
  d_delete_dobjpoints (obj->points);
  g_free (obj);
}

static void
free_all_objs (DAllObjs * objs)
{
  /* Free all objects */
  DAllObjs * next;

  while (objs)
    {
      free_one_obj (objs->obj);
      next = objs->next;
      g_free (objs);
      objs = next;
    }
}

gchar *
get_line (gchar *buf,
          gint   s,
          FILE  *from,
          gint   init)
{
  gint slen;
  char * ret;

  if (init)
    line_no = 1;
  else
    line_no++;

  do
    {
      ret = fgets (buf, s, from);
    } while (!ferror (from) && buf[0] == '#');

  slen = strlen (buf);

  /* The last newline is a pain */
  if (slen > 0)
    buf[slen - 1] = '\0';

  if (ferror (from))
    {
      g_warning (_("Error reading file"));
      return (0);
    }

#ifdef DEBUG
  printf ("Processing line '%s'\n", buf);
#endif /* DEBUG */

  return (ret);
}

static void
clear_undo (void)
{
  int lv;

  for (lv = undo_water_mark; lv >= 0; lv--)
    {
      free_all_objs (undo_table[lv]);
      undo_table[lv] = NULL;
    }

  undo_water_mark = -1;

  gtk_dialog_set_response_sensitive (GTK_DIALOG (top_level_dlg),
                                     RESPONSE_UNDO, FALSE);
}

void
setup_undo (void)
{
  /* Copy object list to undo buffer */
#if DEBUG
  printf ("setup undo level [%d]\n", undo_water_mark);
#endif /*DEBUG*/

  if (!current_obj)
    {
      /* If no current_obj must be loading -> no undo */
      return;
    }

  if (undo_water_mark >= selvals.maxundo - 1)
    {
      int loop;
      /* the little one in the bed said "roll over".. */
      if (undo_table[0])
        free_one_obj (undo_table[0]->obj);
      for (loop = 0; loop < undo_water_mark; loop++)
        {
          undo_table[loop] = undo_table[loop + 1];
        }
    }
  else
    {
      undo_water_mark++;
    }
  undo_table[undo_water_mark] = copy_all_objs (current_obj->obj_list);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (top_level_dlg),
                                     RESPONSE_UNDO, TRUE);

  gfig_obj_modified (current_obj, GFIG_MODIFIED);
  current_obj->obj_status |= GFIG_MODIFIED;
}

/* Given a number of float co-ords adjust for scaling back to org size */
/* Size is number of PAIRS of points */
/* FP + int varients */

static void
scale_to_orginal_x (gdouble *list)
{
  *list *= scale_x_factor;
}

static gint
gfig_scale_x (gint x)
{
  if (!selvals.scaletoimage)
    return (gint) (x * (1 / scale_x_factor));
  else
    return x;
}

static void
scale_to_orginal_y (gdouble *list)
{
  *list *= scale_y_factor;
}

static gint
gfig_scale_y (gint y)
{
  if (!selvals.scaletoimage)
    return (gint) (y * (1 / scale_y_factor));
  else
    return y;
}

/* Pairs x followed by y */
void
scale_to_original_xy (gdouble *list,
                      gint     size)
{
  gint i;

  for (i = 0; i < size * 2; i += 2)
    {
      scale_to_orginal_x (&list[i]);
      scale_to_orginal_y (&list[i + 1]);
    }
}

/* Pairs x followed by y */
void
scale_to_xy (gdouble *list,
             gint     size)
{
  gint i;

  for (i = 0; i < size * 2; i += 2)
    {
      list[i] *= (org_scale_x_factor / scale_x_factor);
      list[i + 1] *= (org_scale_y_factor / scale_y_factor);
    }
}

/* Given an list of PAIRS of doubles reverse the list */
/* Size is number of pairs to swap */
void
reverse_pairs_list (gdouble *list,
                    gint     size)
{
  gint i;

  struct cs
  {
    gdouble i1;
    gdouble i2;
  } copyit, *orglist;

  orglist = (struct cs *) list;

  /* Uses struct copies */
  for (i = 0; i < size / 2; i++)
    {
      copyit = orglist[i];
      orglist[i] = orglist[size - 1 - i];
      orglist[size - 1 - i] = copyit;
    }
}

void
gfig_draw_arc (gint x, gint y, gint width, gint height, gint angle1,
               gint angle2)
{
  if (drawing_pic)
    {
      gdk_draw_arc (pic_preview->window,
                    pic_preview->style->black_gc,
                    FALSE,
                    adjust_pic_coords (x - width, preview_width),
                    adjust_pic_coords (y - height, preview_height),
                    adjust_pic_coords (2 * width, preview_width),
                    adjust_pic_coords (2 * height, preview_height),
                    angle1 * 64,
                    angle2 * 64);
    }
  else
    {
      gdk_draw_arc (gfig_preview->window,
                    gfig_gc,
                    FALSE,
                    gfig_scale_x (x - width),
                    gfig_scale_y (y - height),
                    gfig_scale_x (2 * width),
                    gfig_scale_y (2 * height),
                    angle1 * 64,
                    angle2 * 64);
    }

}

void
gfig_draw_line (gint x0, gint y0, gint x1, gint y1)
{
  if (drawing_pic)
    {
      gdk_draw_line (pic_preview->window,
                     pic_preview->style->black_gc,
                     adjust_pic_coords (x0, preview_width),
                     adjust_pic_coords (y0, preview_height),
                     adjust_pic_coords (x1, preview_width),
                     adjust_pic_coords (y1, preview_height));
    }
  else
    {
      gdk_draw_line (gfig_preview->window,
                     gfig_gc,
                     gfig_scale_x (x0),
                     gfig_scale_y (y0),
                     gfig_scale_x (x1),
                     gfig_scale_y (y1));
    }
}
