/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-style.h"
#include "gfig-dialog.h"
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

#include "libgimp/stdplugins-intl.h"


#define GFIG_HEADER      "GFIG Version 0.2\n"

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


gint line_no;

gint obj_show_single   = -1; /* -1 all >= 0 object number */

/* Structures etc for the objects */
/* Points used to draw the object  */

GfigObject *obj_creating; /* Object we are creating */
GfigObject *tmp_line;     /* Needed when drawing lines */

gboolean need_to_scale;

static gint       load_options            (GFigObj *gfig,
                                           FILE    *fp);
/* globals */

GfigObjectClass dobj_class[10];
GFigContext  *gfig_context;
GtkWidget    *top_level_dlg;
GList        *gfig_list;
gdouble       org_scale_x_factor, org_scale_y_factor;


/* Stuff for the preview bit */
static gint  sel_x, sel_y;
static gint  sel_width, sel_height;
gint         preview_width, preview_height;
gdouble      scale_x_factor, scale_y_factor;
GdkPixbuf   *back_pixbuf = NULL;

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32,    "dummy",    "dummy" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create geometric shapes"),
                          "Draw Vector Graphics and paint them onto your images.  "
                          "Gfig allows you to draw many types of objects "
                          "including Lines, Circles, Ellipses, Curves, Polygons, "
                          "pointed stars, Bezier curves, and Spirals.  "
                          "Objects can be painted using Brushes or other tools"
                          "or filled using colors or patterns.  "
                          "Gfig objects can also be used to create selections.  ",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1997",
                          N_("_Gfig..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  gint32             drawable_id;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               pwidth, pheight;

  INIT_I18N ();

  gfig_context = g_new0 (GFigContext, 1);
  gfig_context->show_background = TRUE;
  gfig_context->selected_obj = NULL;

  drawable_id = param[2].data.d_drawable;

  run_mode = param[0].data.d_int32;

  gfig_context->image_id = param[1].data.d_image;
  gfig_context->drawable_id = drawable_id;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gimp_image_undo_group_start (gfig_context->image_id);

  gimp_context_push ();

  /* TMP Hack - clear any selections */
  if (! gimp_selection_is_empty (gfig_context->image_id))
    gimp_selection_none (gfig_context->image_id);

  if (! gimp_drawable_mask_intersect (drawable_id, &sel_x, &sel_y,
                                      &sel_width, &sel_height))
    {
      gimp_context_pop ();

      gimp_image_undo_group_end (gfig_context->image_id);
      return;
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


  preview_width  = MAX (pwidth, 2);  /* Min size is 2 */
  preview_height = MAX (pheight, 2);

  org_scale_x_factor = scale_x_factor =
    (gdouble) sel_width / (gdouble) preview_width;
  org_scale_y_factor = scale_y_factor =
    (gdouble) sel_height / (gdouble) preview_height;

  /* initialize */
  gfig_init_object_classes ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      /*gimp_get_data (PLUG_IN_PROC, &selvals);*/
      if (! gfig_dialog ())
        {
          gimp_image_undo_group_end (gfig_context->image_id);

          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      status = GIMP_PDB_CALLING_ERROR;
      break;

    default:
      break;
    }

  gimp_context_pop ();

  gimp_image_undo_group_end (gfig_context->image_id);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();
  else
#if 0
  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data (PLUG_IN_PROC, &selvals, sizeof (SelectItVals));
  else
#endif /* 0 */
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
}

/*
  Translate SPACE to "\\040", etc.
  Taken from gflare plugin
 */
void
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
void
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

gint
gfig_list_pos (GFigObj *gfig)
{
  GFigObj *g;
  gint     n;
  GList   *tmp;

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

gint
gfig_list_insert (GFigObj *gfig)
{
  gint n;

  n = gfig_list_pos (gfig);

  gfig_list = g_list_insert (gfig_list, gfig, n);

  return n;
}

void
gfig_free (GFigObj *gfig)
{
  g_assert (gfig != NULL);

  free_all_objs (gfig->obj_list);

  g_free (gfig->name);
  g_free (gfig->filename);
  g_free (gfig->draw_name);

  g_free (gfig);
}

GFigObj *
gfig_new (void)
{
  return g_new0 (GFigObj, 1);
}

static void
gfig_load_objs (GFigObj *gfig,
                gint     load_count,
                FILE    *fp)
{
  GfigObject *obj;
  gchar       load_buf[MAX_LOAD_LINE];
  glong       offset;
  glong       offset2;
  Style       style;

  while (load_count-- > 0)
    {
      obj = NULL;
      get_line (load_buf, MAX_LOAD_LINE, fp, 0);

      /* kludge */
      offset = ftell (fp);
      gfig_skip_style (&style, fp);

      obj = d_load_object (load_buf, fp);

      if (obj)
        {
          add_to_all_obj (gfig, obj);
          offset2 = ftell (fp);
          fseek (fp, offset, SEEK_SET);
          gfig_load_style (&obj->style, fp);
          fseek (fp, offset2, SEEK_SET);
        }
      else
        {
          g_message ("Failed to load object, load count = %d", load_count);
        }
    }
}

GFigObj *
gfig_load (const gchar *filename,
           const gchar *name)
{
  GFigObj *gfig;
  FILE    *fp;
  gchar    load_buf[MAX_LOAD_LINE];
  gchar    str_buf[MAX_LOAD_LINE];
  gint     chk_count;
  gint     load_count = 0;
  gdouble  version;
  gchar    magic1[20];
  gchar    magic2[20];

  g_assert (filename != NULL);

#ifdef DEBUG
  printf ("Loading %s (%s)\n", filename, name);
#endif /* DEBUG */

  fp = g_fopen (filename, "rb");
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

  sscanf (load_buf, "%10s %10s %lf", magic1, magic2, &version);

  if (strcmp (magic1, "GFIG") || strcmp (magic2, "Version"))
    {
      g_message ("File '%s' is not a gfig file",
                  gimp_filename_to_utf8 (gfig->filename));
      gfig_free (gfig);
      fclose (fp);
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
      gfig_free (gfig);
      fclose (fp);
      return NULL;
    }

  if (gfig_load_styles (gfig, fp))
    {
      g_message ("File '%s' corrupt file - Line %d Option section incorrect",
                 gimp_filename_to_utf8 (filename), line_no);
      gfig_free (gfig);
      fclose (fp);
      return NULL;
    }



  gfig_load_objs (gfig, load_count, fp);

  /* Check count ? */

  chk_count = g_list_length (gfig->obj_list);

  if (chk_count != load_count)
    {
      g_message ("File '%s' corrupt file - Line %d Object count to small",
                 gimp_filename_to_utf8 (filename), line_no);
      gfig_free (gfig);
      fclose (fp);
      return NULL;
    }

  fclose (fp);

  if (!gfig_context->current_obj)
    gfig_context->current_obj = gfig;

  gfig->obj_status = GFIG_OK;

  return gfig;
}

void
save_options (GString *string)
{
  /* Save options */
  g_string_append_printf (string, "<OPTIONS>\n");
  g_string_append_printf (string, "GridSpacing: %d\n",
                          selvals.opts.gridspacing);
  if (selvals.opts.gridtype == RECT_GRID)
    {
      g_string_append_printf (string, "GridType: RECT_GRID\n");
    }
  else if (selvals.opts.gridtype == POLAR_GRID)
    {
      g_string_append_printf (string, "GridType: POLAR_GRID\n");
    }
  else if (selvals.opts.gridtype == ISO_GRID)
    {
      g_string_append_printf (string, "GridType: ISO_GRID\n");
    }
  else
    {
      /* default to RECT_GRID */
      g_string_append_printf (string, "GridType: RECT_GRID\n");
    }

  g_string_append_printf (string, "DrawGrid: %s\n",
                          (selvals.opts.drawgrid) ? "TRUE" : "FALSE");
  g_string_append_printf (string, "Snap2Grid: %s\n",
                          (selvals.opts.snap2grid) ? "TRUE" : "FALSE");
  g_string_append_printf (string, "LockOnGrid: %s\n",
                          (selvals.opts.lockongrid) ? "TRUE" : "FALSE");
  g_string_append_printf (string, "ShowControl: %s\n",
                          (selvals.opts.showcontrol) ? "TRUE" : "FALSE");
  g_string_append_printf (string, "</OPTIONS>\n");
}

static void
gfig_save_obj_start (GfigObject *obj,
                     GString    *string)
{
  g_string_append_printf (string, "<%s ", obj->class->name);
  gfig_style_save_as_attributes (&obj->style, string);
  g_string_append_printf (string, ">\n");
}

static void
gfig_save_obj_end (GfigObject *obj,
                   GString    *string)
{
  g_string_append_printf (string, "</%s>\n",obj->class->name);
}

static gboolean
load_bool (gchar *opt_buf,
           gint  *toset)
{
  if (!strcmp (opt_buf, "TRUE"))
    *toset = 1;
  else if (!strcmp (opt_buf, "FALSE"))
    *toset = 0;
  else
    return TRUE;

  return FALSE;
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
      printf ("num = %d\n", sscanf (load_buf, "%255s %255s", str_buf, opt_buf));

      printf ("option %s val %s\n", str_buf, opt_buf);
#else
      sscanf (load_buf, "%255s %255s", str_buf, opt_buf);
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

GString *
gfig_save_as_string (void)
{
  GList    *objs;
  gint      count;
  gchar     buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar     conv_buf[MAX_LOAD_LINE * 3 + 1];
  GString  *string;

  string = g_string_new (GFIG_HEADER);

  gfig_name_encode (conv_buf, gfig_context->current_obj->draw_name);
  g_string_append_printf (string, "Name: %s\n", conv_buf);
  g_string_append_printf (string, "Version: %s\n",
                          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                                           gfig_context->current_obj->version));
  objs = gfig_context->current_obj->obj_list;

  count = g_list_length (objs);

  g_string_append_printf (string, "ObjCount: %d\n", count);

  save_options (string);

  gfig_save_styles (string);

  for (objs = gfig_context->current_obj->obj_list;
       objs;
       objs = g_list_next (objs))
    {
      GfigObject *object = objs->data;

      gfig_save_obj_start (object, string);

      gfig_save_style (&object->style, string);

      if (object->points)
        d_save_object (object, string);

      gfig_save_obj_end (object, string);
    }

  return string;
}


gboolean
gfig_save_as_parasite (void)
{
  GimpParasite *parasite;
  GString       *string;

  string = gfig_save_as_string ();

  parasite = gimp_parasite_new ("gfig",
                                GIMP_PARASITE_PERSISTENT |
                                GIMP_PARASITE_UNDOABLE,
                                string->len, string->str);

  g_string_free (string, TRUE);

  if (!gimp_item_attach_parasite (gfig_context->drawable_id, parasite))
    {
      g_message (_("Error trying to save figure as a parasite: "
                   "can't attach parasite to drawable."));
      gimp_parasite_free (parasite);
      return FALSE;
    }

  gimp_parasite_free (parasite);
  return TRUE;
}

GFigObj *
gfig_load_from_parasite (void)
{
  FILE         *fp;
  gchar        *fname;
  GimpParasite *parasite;
  GFigObj      *gfig;

  parasite = gimp_item_get_parasite (gfig_context->drawable_id, "gfig");
  if (! parasite)
    return NULL;

  fname = gimp_temp_name ("gfigtmp");

  fp = g_fopen (fname, "wb");
  if (!fp)
    {
      g_message (_("Error trying to open temporary file '%s' "
                   "for parasite loading: %s"),
                 gimp_filename_to_utf8 (fname), g_strerror (errno));
      return NULL;
    }

  fwrite (gimp_parasite_data (parasite),
          sizeof (guchar),
          gimp_parasite_data_size (parasite),
          fp);
  fclose (fp);

  gimp_parasite_free (parasite);

  gfig = gfig_load (fname, "(none)");

  g_unlink (fname);

  g_free (fname);

  return gfig;
}

void
gfig_save_callbk (void)
{
  FILE     *fp;
  gchar    *savename;
  GString  *string;

  savename = gfig_context->current_obj->filename;

  fp = g_fopen (savename, "w+b");

  if (!fp)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (savename), g_strerror (errno));
      return;
    }

  string = gfig_save_as_string ();

  fwrite (string->str, string->len, 1, fp);

  if (ferror (fp))
    g_message ("Failed to write file.");
  else
    gfig_context->current_obj->obj_status &= ~(GFIG_MODIFIED | GFIG_READONLY);

  fclose (fp);
}
