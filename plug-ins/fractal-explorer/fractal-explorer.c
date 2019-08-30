/**********************************************************************
    Fractal Explorer Plug-in (Version 2.00 Beta 2)
    Daniel Cotting (cotting@multimania.com)
 **********************************************************************
 **********************************************************************
    Official homepages: http://www.multimania.com/cotting
                        http://cotting.citeweb.net
 *********************************************************************/

/**********************************************************************
   GIMP - The GNU Image Manipulation Program
   Copyright (C) 1995 Spencer Kimball and Peter Mattis

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *********************************************************************/

/**********************************************************************
   Some code has been 'stolen' from:
        - Peter Kirchgessner (Pkirchg@aol.com)
        - Scott Draves (spot@cs.cmu.edu)
        - Andy Thomas (alt@picnic.demon.co.uk)
           .
           .
           .
 **********************************************************************
   "If you steal from one author it's plagiarism; if you steal from
   many it's research."  --Wilson Mizner
 *********************************************************************/


/**********************************************************************
 Include necessary files
 *********************************************************************/

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fractal-explorer.h"
#include "fractal-explorer-dialogs.h"

#include "libgimp/stdplugins-intl.h"


typedef struct _Explorer      Explorer;
typedef struct _ExplorerClass ExplorerClass;

struct _Explorer
{
  GimpPlugIn parent_instance;
};

struct _ExplorerClass
{
  GimpPlugInClass parent_class;
};


#define EXPLORER_TYPE  (explorer_get_type ())
#define EXPLORER (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXPLORER_TYPE, Explorer))

GType                   explorer_get_type         (void) G_GNUC_CONST;

static GList          * explorer_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * explorer_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * explorer_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable         *drawable,
                                                   const GimpValueArray *args,
                                                   gpointer              run_data);

static void       explorer                         (GimpDrawable       *drawable);

static void       delete_dialog_callback           (GtkWidget          *widget,
                                                    gboolean            value,
                                                    gpointer            data);
static gboolean   delete_fractal_callback          (GtkWidget          *widget,
                                                    gpointer            data);
static gint       fractalexplorer_list_pos         (fractalexplorerOBJ *feOBJ);
static gint       fractalexplorer_list_insert      (fractalexplorerOBJ *feOBJ);
static fractalexplorerOBJ *fractalexplorer_new     (void);
static void       fill_list_store                  (GtkListStore       *list_store);
static void       activate_fractal                 (fractalexplorerOBJ *sel_obj);
static void       activate_fractal_callback        (GtkTreeView        *view,
                                                    GtkTreePath        *path,
                                                    GtkTreeViewColumn  *col,
                                                    gpointer            data);
static gboolean   apply_fractal_callback           (GtkWidget          *widget,
                                                    gpointer            data);

static void       fractalexplorer_free             (fractalexplorerOBJ *feOBJ);
static void       fractalexplorer_free_everything  (fractalexplorerOBJ *feOBJ);
static void       fractalexplorer_list_free_all    (void);
static fractalexplorerOBJ * fractalexplorer_load   (const gchar *filename,
                                                    const gchar *name);

static void       fractalexplorer_list_load_all    (const gchar *path);
static void       fractalexplorer_rescan_list      (GtkWidget *widget,
                                                    gpointer   data);


G_DEFINE_TYPE (Explorer, explorer, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (EXPLORER_TYPE)


/**********************************************************************
  Global variables
 *********************************************************************/

gdouble              xmin = -2;
gdouble              xmax = 1;
gdouble              ymin = -1.5;
gdouble              ymax = 1.5;
gdouble              xbild;
gdouble              ybild;
gdouble              xdiff;
gdouble              ydiff;
gint                 sel_x;
gint                 sel_y;
gint                 preview_width;
gint                 preview_height;
gdouble             *gg;
gint                 line_no;
gchar               *filename;
clrmap               colormap;
vlumap               valuemap;
gchar               *fractalexplorer_path = NULL;

static gfloat        cx = -0.75;
static gfloat        cy = -0.2;
static GList        *fractalexplorer_list = NULL;

explorer_interface_t wint =
{
  NULL,                       /* preview */
  NULL,                       /* wimage */
  FALSE                       /* run */
};                              /* wint */

explorer_vals_t wvals =
{
  0,
  -2.0,
  2.0,
  -1.5,
  1.5,
  50.0,
  -0.75,
  -0.2,
  0,
  1.0,
  1.0,
  1.0,
  1,
  1,
  0,
  0,
  0,
  0,
  1,
  256,
  0,
  0
};                              /* wvals */

fractalexplorerOBJ *current_obj   = NULL;
static GtkWidget   *delete_dialog = NULL;


static void
explorer_class_init (ExplorerClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = explorer_query_procedures;
  plug_in_class->create_procedure = explorer_create_procedure;
}

static void
explorer_init (Explorer *explorer)
{
}

static GList *
explorer_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
explorer_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            explorer_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");

      gimp_procedure_set_menu_label (procedure, N_("_Fractal Explorer..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Render/Fractals");

      gimp_procedure_set_documentation (procedure,
                                        N_("Render fractal art"),
                                        "No help yet.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Daniel Cotting (cotting@multimania.com, "
                                      "www.multimania.com/cotting)",
                                      "Daniel Cotting (cotting@multimania.com, "
                                      "www.multimania.com/cotting)",
                                      "December, 1998");

      GIMP_PROC_ARG_INT (procedure, "fractal-type",
                         "Fractal type",
                         "0: Mandelbrot; 1: Julia; 2: Barnsley 1; "
                         "3: Barnsley 2; 4: Barnsley 3; 5: Spider; "
                         "6: ManOWar; 7: Lambda; 8: Sierpinski",
                         0, 8, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "xmin",
                            "X min",
                            "xmin fractal image delimiter",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "xmax",
                            "X max",
                            "xmax fractal image delimiter",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "ymin",
                            "Y min",
                            "ymin fractal image delimiter",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "ymax",
                            "Y max",
                            "ymax fractal image delimiter",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "iter",
                            "Iter",
                            "Iteration value",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "cx",
                            "CX",
                            "cx value (only Julia)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "cy",
                            "CY",
                            "cy value (only Julia)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "color-mode",
                         "Color mode",
                         "0: Apply colormap as specified by the parameters "
                         "below; 1: Apply active gradient to final image",
                         0, 1, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "red-stretch",
                            "Red stretch",
                            "Red stretching factor",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "green-stretch",
                            "Green stretch",
                            "Green stretching factor",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "blues-tretch",
                            "Blue stretch",
                            "Blue stretching factor",
                            -G_MAXDOUBLE,G_MAXDOUBLE, 0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "red-mode",
                         "Red mode",
                         "Red application mode (0:SIN; 1:COS; 2:NONE)",
                         0, 2, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "green-mode",
                         "Green mode",
                         "Green application mode (0:SIN; 1:COS; 2:NONE)",
                         0, 2, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "blue-mode",
                         "Blue mode",
                         "Blue application mode (0:SIN; 1:COS; 2:NONE)",
                         0, 2, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "red-invert",
                             "Red invert",
                             "Red inversion mode",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "green-invert",
                             "Green invert",
                             "Green inversion mode",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "blue-invert",
                             "Blue invert",
                             "Blue inversion mode",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "n-colors",
                         "N volors",
                         "Number of Colors for mapping",
                         2, 8192, 512,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
explorer_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable         *drawable,
              const GimpValueArray *args,
              gpointer              run_data)
{
  gint               pwidth;
  gint               pheight;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               sel_width;
  gint               sel_height;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (! gimp_drawable_mask_intersect (drawable,
                                      &sel_x, &sel_y,
                                      &sel_width, &sel_height))
    {
      return gimp_procedure_new_return_values (procedure, status, NULL);
    }

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

  preview_width  = MAX (pwidth, 2);
  preview_height = MAX (pheight, 2);

  /* See how we will run */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_fractalexplorer", &wvals);

      /* Get information from the dialog */
      if (! explorer_dialog ())
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      wvals.fractaltype  = GIMP_VALUES_GET_INT    (args, 0);
      wvals.xmin         = GIMP_VALUES_GET_DOUBLE (args, 1);
      wvals.xmax         = GIMP_VALUES_GET_DOUBLE (args, 2);
      wvals.ymin         = GIMP_VALUES_GET_DOUBLE (args, 4);
      wvals.ymax         = GIMP_VALUES_GET_DOUBLE (args, 4);
      wvals.iter         = GIMP_VALUES_GET_DOUBLE (args, 5);
      wvals.cx           = GIMP_VALUES_GET_DOUBLE (args, 6);
      wvals.cy           = GIMP_VALUES_GET_DOUBLE (args, 7);
      wvals.colormode    = GIMP_VALUES_GET_INT    (args, 8);
      wvals.redstretch   = GIMP_VALUES_GET_DOUBLE (args, 9);
      wvals.greenstretch = GIMP_VALUES_GET_DOUBLE (args, 10);
      wvals.bluestretch  = GIMP_VALUES_GET_DOUBLE (args, 11);
      wvals.redmode      = GIMP_VALUES_GET_INT    (args, 12);
      wvals.greenmode    = GIMP_VALUES_GET_INT    (args, 13);
      wvals.bluemode     = GIMP_VALUES_GET_INT    (args, 14);
      wvals.redinvert    = GIMP_VALUES_GET_INT    (args, 15);
      wvals.greeninvert  = GIMP_VALUES_GET_INT    (args, 16);
      wvals.blueinvert   = GIMP_VALUES_GET_INT    (args, 17);
      wvals.ncolors      = GIMP_VALUES_GET_INT    (args, 18);

      make_color_map ();
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_fractalexplorer", &wvals);
      make_color_map ();
      break;

    default:
      break;
    }

  xmin = wvals.xmin;
  xmax = wvals.xmax;
  ymin = wvals.ymin;
  ymax = wvals.ymax;
  cx = wvals.cx;
  cy = wvals.cy;

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_progress_init (_("Rendering fractal"));

      explorer (drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /* Store data */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data ("plug_in_fractalexplorer",
                       &wvals, sizeof (explorer_vals_t));
    }

  return gimp_procedure_new_return_values (procedure, status, NULL);
}

/**********************************************************************
 FUNCTION: explorer
 *********************************************************************/

static void
explorer (GimpDrawable *drawable)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  gint        width;
  gint        height;
  gint        bpp;
  gint        row;
  gint        x;
  gint        y;
  gint        w;
  gint        h;
  guchar     *src_row;
  guchar     *dest_row;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &w, &h))
    return;

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = gimp_drawable_width  (drawable);
  height = gimp_drawable_height (drawable);

  if (gimp_drawable_has_alpha (drawable))
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  bpp = babl_format_get_bytes_per_pixel (format);

  /*  allocate row buffers  */
  src_row  = g_new (guchar, bpp * w);
  dest_row = g_new (guchar, bpp * w);

  src_buffer  = gimp_drawable_get_buffer (drawable);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  xbild = width;
  ybild = height;
  xdiff = (xmax - xmin) / xbild;
  ydiff = (ymax - ymin) / ybild;

  for (row = y; row < y + h; row++)
    {
      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (x, row, w, 1), 1.0,
                       format, src_row,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      explorer_render_row (src_row,
                           dest_row,
                           row,
                           w,
                           bpp);

      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x, row, w, 1), 0,
                       format, dest_row,
                       GEGL_AUTO_ROWSTRIDE);

      if ((row % 10) == 0)
        gimp_progress_update ((double) row / (double) h);
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  g_free (src_row);
  g_free (dest_row);

  gimp_progress_update (1.0);

  /*  update the processed region  */
  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable, x, y, w, h);
}

/**********************************************************************
 FUNCTION: explorer_render_row
 *********************************************************************/

void
explorer_render_row (const guchar *src_row,
                     guchar       *dest_row,
                     gint          row,
                     gint          row_width,
                     gint          bpp)
{
  gint    col;
  gdouble a;
  gdouble b;
  gdouble x;
  gdouble y;
  gdouble oldx;
  gdouble oldy;
  gdouble tempsqrx;
  gdouble tempsqry;
  gdouble tmpx = 0;
  gdouble tmpy = 0;
  gdouble foldxinitx;
  gdouble foldxinity;
  gdouble foldyinitx;
  gdouble foldyinity;
  gdouble xx = 0;
  gdouble adjust;
  gdouble cx;
  gdouble cy;
  gint    counter;
  gint    color;
  gint    iteration;
  gint    useloglog;
  gdouble log2;

  cx = wvals.cx;
  cy = wvals.cy;
  useloglog = wvals.useloglog;
  iteration = wvals.iter;
  log2 = log (2.0);

  for (col = 0; col < row_width; col++)
    {
      a = xmin + (double) col * xdiff;
      b = ymin + (double) row * ydiff;
      if (wvals.fractaltype != 0)
        {
          tmpx = x = a;
          tmpy = y = b;
        }
      else
        {
          x = 0;
          y = 0;
        }

      for (counter = 0; counter < iteration; counter++)
        {
          oldx=x;
          oldy=y;

          switch (wvals.fractaltype)
            {
            case TYPE_MANDELBROT:
              xx = x * x - y * y + a;
              y = 2.0 * x * y + b;
              break;

            case TYPE_JULIA:
              xx = x * x - y * y + cx;
              y = 2.0 * x * y + cy;
              break;

            case TYPE_BARNSLEY_1:
              foldxinitx = oldx * cx;
              foldyinity = oldy * cy;
              foldxinity = oldx * cy;
              foldyinitx = oldy * cx;
              /* orbit calculation */
              if (oldx >= 0)
                {
                  xx = (foldxinitx - cx - foldyinity);
                  y  = (foldyinitx - cy + foldxinity);
                }
              else
                {
                  xx = (foldxinitx + cx - foldyinity);
                  y  = (foldyinitx + cy + foldxinity);
                }
              break;

            case TYPE_BARNSLEY_2:
              foldxinitx = oldx * cx;
              foldyinity = oldy * cy;
              foldxinity = oldx * cy;
              foldyinitx = oldy * cx;
              /* orbit calculation */
              if (foldxinity + foldyinitx >= 0)
                {
                  xx = foldxinitx - cx - foldyinity;
                  y  = foldyinitx - cy + foldxinity;
                }
              else
                {
                  xx = foldxinitx + cx - foldyinity;
                  y  = foldyinitx + cy + foldxinity;
                }
              break;

            case TYPE_BARNSLEY_3:
              foldxinitx  = oldx * oldx;
              foldyinity  = oldy * oldy;
              foldxinity  = oldx * oldy;
              /* orbit calculation */
              if (oldx > 0)
                {
                  xx = foldxinitx - foldyinity - 1.0;
                  y  = foldxinity * 2;
                }
              else
                {
                  xx = foldxinitx - foldyinity -1.0 + cx * oldx;
                  y  = foldxinity * 2;
                  y += cy * oldx;
                }
              break;

            case TYPE_SPIDER:
              /* { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
              xx = x*x - y*y + tmpx + cx;
              y = 2 * oldx * oldy + tmpy +cy;
              tmpx = tmpx/2 + xx;
              tmpy = tmpy/2 + y;
              break;

            case TYPE_MAN_O_WAR:
              xx = x*x - y*y + tmpx + cx;
              y = 2.0 * x * y + tmpy + cy;
              tmpx = oldx;
              tmpy = oldy;
              break;

            case TYPE_LAMBDA:
              tempsqrx = x * x;
              tempsqry = y * y;
              tempsqrx = oldx - tempsqrx + tempsqry;
              tempsqry = -(oldy * oldx);
              tempsqry += tempsqry + oldy;
              xx = cx * tempsqrx - cy * tempsqry;
              y = cx * tempsqry + cy * tempsqrx;
              break;

            case TYPE_SIERPINSKI:
              xx = oldx + oldx;
              y = oldy + oldy;
              if (oldy > .5)
                y = y - 1;
              else if (oldx > .5)
                xx = xx - 1;
              break;

            default:
              break;
            }

          x = xx;

          if (((x * x) + (y * y)) >= 4.0)
            break;
        }

      if (useloglog)
        {
          gdouble modulus_square = (x * x) + (y * y);

          if (modulus_square > (G_E * G_E))
              adjust = log (log (modulus_square) / 2.0) / log2;
          else
              adjust = 0.0;
        }
      else
        {
          adjust = 0.0;
        }

      color = (int) (((counter - adjust) * (wvals.ncolors - 1)) / iteration);
      if (bpp >= 3)
        {
          dest_row[col * bpp + 0] = colormap[color].r;
          dest_row[col * bpp + 1] = colormap[color].g;
          dest_row[col * bpp + 2] = colormap[color].b;
        }
      else
          dest_row[col * bpp + 0] = valuemap[color];

      if (! ( bpp % 2))
        dest_row [col * bpp + bpp - 1] = 255;

    }
}

static void
delete_dialog_callback (GtkWidget *widget,
                        gboolean   delete,
                        gpointer   data)
{
  GtkWidget          *view = (GtkWidget *) data;
  GtkTreeSelection   *selection;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  gboolean            valid;
  fractalexplorerOBJ *sel_obj;

  if (delete)
    {
      /* Must update which object we are editing */
      /* Get the list and which item is selected */
      /* Only allow single selections */
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
      gtk_tree_selection_get_selected (selection, &model, &iter);

      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);

      /* Delete the current item + associated file */
      valid = gtk_list_store_remove (GTK_LIST_STORE(model), &iter);

      /* Try to select first item if last one was deleted */
      if (!valid)
        valid = gtk_tree_model_get_iter_first (model, &iter);

      /* Shadow copy for ordering info */
      fractalexplorer_list = g_list_remove (fractalexplorer_list, sel_obj);
      /*
        if(sel_obj == current_obj)
        {
        clear_undo();
        }
      */
      /* Free current obj */
      fractalexplorer_free_everything (sel_obj);

      /* Check whether there are items left */
      if (valid)
        {
          gtk_tree_selection_select_iter (selection, &iter);

          gtk_tree_model_get (model, &iter, 1, &current_obj, -1);
        }
    }

  delete_dialog = NULL;
}

static gboolean
delete_fractal_callback (GtkWidget *widget,
                         gpointer   data)
{
  gchar              *str;
  GtkWidget          *view = (GtkWidget *) data;
  GtkTreeSelection   *selection;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  fractalexplorerOBJ *sel_obj;

  if (delete_dialog)
    return FALSE;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);

      str = g_strdup_printf (_("Are you sure you want to delete "
                               "\"%s\" from the list and from disk?"),
                             sel_obj->draw_name);

      delete_dialog = gimp_query_boolean_box (_("Delete Fractal"),
                                              gtk_widget_get_toplevel (view),
                                              gimp_standard_help_func, NULL,
                                              GIMP_ICON_DIALOG_QUESTION,
                                              str,
                                              _("_Delete"), _("_Cancel"),
                                              G_OBJECT (widget), "destroy",
                                              delete_dialog_callback,
                                              data, NULL);
      g_free (str);

      gtk_widget_show (delete_dialog);
    }

  return FALSE;
}

static gint
fractalexplorer_list_pos (fractalexplorerOBJ *fractalexplorer)
{
  fractalexplorerOBJ *g;
  gint                n;
  GList              *tmp;

  n = 0;

  for (tmp = fractalexplorer_list; tmp; tmp = g_list_next (tmp))
    {
      g = tmp->data;

      if (strcmp (fractalexplorer->draw_name, g->draw_name) <= 0)
        break;

      n++;
    }
  return n;
}

static gint
fractalexplorer_list_insert (fractalexplorerOBJ *fractalexplorer)
{
  gint n = fractalexplorer_list_pos (fractalexplorer);

  /*
   *    Insert fractalexplorers in alphabetical order
   */

  fractalexplorer_list = g_list_insert (fractalexplorer_list,
                                        fractalexplorer, n);

  return n;
}

static fractalexplorerOBJ *
fractalexplorer_new (void)
{
  return g_new0 (fractalexplorerOBJ, 1);
}

static void
fill_list_store (GtkListStore *list_store)
{
  GList         *tmp;
  GtkTreeIter    iter;


  for (tmp = fractalexplorer_list; tmp; tmp = tmp->next)
    {
      fractalexplorerOBJ *g;
      g = tmp->data;

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, g->draw_name, 1, g, -1);
    }
}

static void
activate_fractal (fractalexplorerOBJ *sel_obj)
{
  current_obj = sel_obj;
  wvals = current_obj->opts;
  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}

static void
activate_fractal_callback (GtkTreeView       *view,
                           GtkTreePath       *path,
                           GtkTreeViewColumn *col,
                           gpointer           data)
{
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  fractalexplorerOBJ *sel_obj;

  model = gtk_tree_view_get_model (view);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);
      activate_fractal (sel_obj);
    }

}

static gboolean
apply_fractal_callback (GtkWidget *widget,
                        gpointer   data)
{
  GtkWidget          *view = (GtkWidget *) data;
  GtkTreeSelection   *selection;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  fractalexplorerOBJ *sel_obj;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &sel_obj, -1);
      activate_fractal (sel_obj);
    }

  return FALSE;
}

static void
fractalexplorer_free (fractalexplorerOBJ *fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  g_free (fractalexplorer->name);
  g_free (fractalexplorer->filename);
  g_free (fractalexplorer->draw_name);
  g_free (fractalexplorer);
}

static void
fractalexplorer_free_everything (fractalexplorerOBJ *fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  if (fractalexplorer->filename)
    g_remove (fractalexplorer->filename);

  fractalexplorer_free (fractalexplorer);
}

static void
fractalexplorer_list_free_all (void)
{
  g_list_free_full (fractalexplorer_list, (GDestroyNotify) fractalexplorer_free);
  fractalexplorer_list = NULL;
}

static fractalexplorerOBJ *
fractalexplorer_load (const gchar *filename,
                      const gchar *name)
{
  fractalexplorerOBJ * fractalexplorer;
  FILE * fp;
  gchar load_buf[MAX_LOAD_LINE];

  g_assert (filename != NULL);

  fp = g_fopen (filename, "rt");
  if (!fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  fractalexplorer = fractalexplorer_new ();

  fractalexplorer->name = g_strdup (name);
  fractalexplorer->draw_name = g_strdup (name);
  fractalexplorer->filename = g_strdup (filename);

  /* HEADER
   * draw_name
   * version
   * obj_list
   */

  get_line (load_buf, MAX_LOAD_LINE, fp, 1);

  if (strncmp (fractalexplorer_HEADER, load_buf, strlen (load_buf)))
    {
      g_message (_("File '%s' is not a FractalExplorer file"),
                 gimp_filename_to_utf8 (filename));
      fclose (fp);
      fractalexplorer_free (fractalexplorer);

      return NULL;
    }

  if (load_options (fractalexplorer, fp))
    {
      g_message (_("File '%s' is corrupt.\nLine %d Option section incorrect"),
                 gimp_filename_to_utf8 (filename), line_no);
      fclose (fp);
      fractalexplorer_free (fractalexplorer);

      return NULL;
    }

  fclose (fp);

  fractalexplorer->obj_status = fractalexplorer_OK;

  return fractalexplorer;
}

static void
fractalexplorer_list_load_all (const gchar *explorer_path)
{
  GList *path;
  GList *list;

  /*  Make sure to clear any existing fractalexplorers  */
  current_obj = NULL;
  fractalexplorer_list_free_all ();

  path = gimp_config_path_expand_to_files (explorer_path, NULL);

  for (list = path; list; list = g_list_next (list))
    {
      GFileEnumerator *enumerator;

      enumerator = g_file_enumerate_children (list->data,
                                              G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                              G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                              G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                              G_FILE_QUERY_INFO_NONE,
                                              NULL, NULL);

      if (enumerator)
        {
          GFileInfo *info;

          while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
            {
              GFileType file_type = g_file_info_get_file_type (info);

              if (file_type == G_FILE_TYPE_REGULAR &&
                  ! g_file_info_get_is_hidden (info))
                {
                  fractalexplorerOBJ *fractalexplorer;
                  GFile              *child;
                  gchar              *filename;
                  gchar              *basename;

                  child = g_file_enumerator_get_child (enumerator, info);

                  filename = g_file_get_path (child);
                  basename = g_file_get_basename (child);

                  fractalexplorer = fractalexplorer_load (filename,
                                                          basename);

                  g_free (filename);
                  g_free (basename);

                  if (fractalexplorer)
                    fractalexplorer_list_insert (fractalexplorer);

                  g_object_unref (child);
                }

              g_object_unref (info);
            }

          g_object_unref (enumerator);
        }
    }

  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  if (!fractalexplorer_list)
    {
      fractalexplorerOBJ *fractalexplorer;

      /* lets have at least one! */
      fractalexplorer = fractalexplorer_new ();
      fractalexplorer->draw_name = g_strdup (_("My first fractal"));
      fractalexplorer_list_insert (fractalexplorer);
    }
  current_obj = fractalexplorer_list->data;  /* set to first entry */
}


GtkWidget *
add_objects_list (void)
{
  GtkWidget         *grid;
  GtkWidget         *scrolled_win;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *renderer;
  GtkWidget         *view;
  GtkTreeSelection  *selection;
  GtkListStore      *list_store;
  GtkWidget         *button;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_widget_show (grid);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_hexpand (scrolled_win, TRUE);
  gtk_widget_set_vexpand (scrolled_win, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_grid_attach (GTK_GRID (grid), scrolled_win, 0, 0, 3, 1);
  gtk_widget_show (scrolled_win);

  view = gtk_tree_view_new ();
  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", 0);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (view, "row-activated",
                    G_CALLBACK (activate_fractal_callback),
                    NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), view);
  gtk_widget_show (view);

  fractalexplorer_list_load_all (fractalexplorer_path);
  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  fill_list_store (list_store);
  gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (list_store));
  g_object_unref (list_store); /* destroy model automatically with view */

  /* Put buttons in */
  button = gtk_button_new_with_mnemonic (_("_Refresh"));
  gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 1, 1);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Select folder and rescan collection"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (fractalexplorer_rescan_list),
                    view);

  button = gtk_button_new_with_mnemonic (_("_Apply"));
  gtk_grid_attach (GTK_GRID (grid), button, 1, 1, 1, 1);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Apply currently selected fractal"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (apply_fractal_callback),
                    view);

  button = gtk_button_new_with_mnemonic (_("_Delete"));
  gtk_grid_attach (GTK_GRID (grid), button, 2, 1, 1, 1);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Delete currently selected fractal"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (delete_fractal_callback),
                    view);

  return grid;
}

static void
fractalexplorer_rescan_list (GtkWidget *widget,
                             gpointer   data)
{
  static GtkWidget *dlg  = NULL;
  GtkWidget        *view = data;
  GtkWidget        *patheditor;

  if (dlg)
    {
      gtk_window_present (GTK_WINDOW (dlg));
      return;
    }

  dlg = gimp_dialog_new (_("Rescan for Fractals"), PLUG_IN_ROLE,
                         gtk_widget_get_toplevel (view),
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &dlg);

  patheditor = gimp_path_editor_new (_("Add FractalExplorer Path"),
                                     fractalexplorer_path);
  gtk_container_set_border_width (GTK_CONTAINER (patheditor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      patheditor, TRUE, TRUE, 0);
  gtk_widget_show (patheditor);

  if (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
      g_free (fractalexplorer_path);
      fractalexplorer_path =
        gimp_path_editor_get_path (GIMP_PATH_EDITOR (patheditor));

      if (fractalexplorer_path)
        {
          GtkTreeModel     *model;
          GtkTreeSelection *selection;
          GtkTreePath      *path;
          GtkTreeIter       iter;

          fractalexplorer_list_load_all (fractalexplorer_path);

          model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
          gtk_list_store_clear (GTK_LIST_STORE (model));
          fill_list_store (GTK_LIST_STORE (model));

          /* select active fractal, otherwise first fractal */
          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
          if (gtk_tree_model_get_iter_first (model, &iter))
            {
              gtk_tree_selection_select_iter (selection, &iter);
              path = gtk_tree_model_get_path (model, &iter);
              current_obj = fractalexplorer_list->data;
              gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path, NULL,
                                            FALSE, 0.0, 0.0);
              gtk_tree_path_free (path);
            }
        }
    }

  gtk_widget_destroy (dlg);
}
