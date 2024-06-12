/*
 *  Guillotine plug-in v0.9 by Adam D. Moss, adam@foxbox.org.  1998/09/01
 */

/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC "plug-in-guillotine"


typedef struct _Guillotine      Guillotine;
typedef struct _GuillotineClass GuillotineClass;

struct _Guillotine
{
  GimpPlugIn      parent_instance;
};

struct _GuillotineClass
{
  GimpPlugInClass parent_class;
};


#define GUILLOTINE_TYPE  (guillotine_get_type ())
#define GUILLOTINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GUILLOTINE_TYPE, Guillotine))

GType                   guillotine_get_type         (void) G_GNUC_CONST;

static GList          * guillotine_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * guillotine_create_procedure (GimpPlugIn           *plug_in,
                                                     const gchar          *name);

static GimpValueArray * guillotine_run              (GimpProcedure        *procedure,
                                                     GimpRunMode           run_mode,
                                                     GimpImage            *image,
                                                     gint                  n_drawables,
                                                     GimpDrawable        **drawables,
                                                     GimpProcedureConfig  *config,
                                                     gpointer              run_data);
static GList          * guillotine                  (GimpImage            *image,
                                                     gboolean              interactive);


G_DEFINE_TYPE (Guillotine, guillotine, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GUILLOTINE_TYPE)
DEFINE_STD_SET_I18N

static void
guillotine_class_init (GuillotineClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = guillotine_query_procedures;
  plug_in_class->create_procedure = guillotine_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
guillotine_init (Guillotine *film)
{
}

static GList *
guillotine_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
guillotine_create_procedure (GimpPlugIn  *plug_in,
                             const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            guillotine_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("Slice Using G_uides"));
      gimp_procedure_add_menu_path (procedure, "<Image>/Image/[Crop]");

      gimp_procedure_set_documentation (procedure,
                                        _("Slice the image into subimages "
                                          "using guides"),
                                        "This function takes an image and "
                                        "slices it along its guides, creating "
                                        "new images. The original image is "
                                        "not modified.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Adam D. Moss (adam@foxbox.org)",
                                      "Adam D. Moss (adam@foxbox.org)",
                                      "1998");

      gimp_procedure_add_int_return_value (procedure, "image-count",
                                           "Number of images created",
                                           "Number of images created",
                                           0, G_MAXINT, 0,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_object_array_return_value (procedure, "images",
                                                    "Output images",
                                                    "Output images",
                                                    GIMP_TYPE_IMAGE,
                                                    G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
guillotine_run (GimpProcedure        *procedure,
                GimpRunMode           run_mode,
                GimpImage            *image,
                gint                  n_drawables,
                GimpDrawable        **drawables,
                GimpProcedureConfig  *config,
                gpointer              run_data)
{
  GimpValueArray    *return_vals = NULL;
  GimpPDBStatusType  status      = GIMP_PDB_SUCCESS;

  return_vals = gimp_procedure_new_return_values (procedure, status,
                                                  NULL);
  if (status == GIMP_PDB_SUCCESS)
    {
      GList      *image_list;
      GList      *list;
      GimpImage **images;
      gint        num_images;
      gint        i;

      gimp_progress_init (_("Guillotine"));

      image_list = guillotine (image, run_mode == GIMP_RUN_INTERACTIVE);

      num_images = g_list_length (image_list);
      images     = g_new (GimpImage *, num_images);

      for (list = image_list, i = 0;
           list;
           list = g_list_next (list), i++)
        {
          images[i] = g_object_ref (list->data);
        }

      g_list_free (image_list);

      GIMP_VALUES_SET_INT           (return_vals, 1, num_images);
      GIMP_VALUES_TAKE_OBJECT_ARRAY (return_vals, 2, GIMP_TYPE_IMAGE, images, num_images);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_displays_flush ();
    }

  return return_vals;
}


static gint
guide_sort_func (gconstpointer a,
                 gconstpointer b)
{
  return GPOINTER_TO_INT (a) - GPOINTER_TO_INT (b);
}

static GList *
guillotine (GimpImage *image,
            gboolean   interactive)
{
  GList    *images = NULL;
  gint      guide;
  gint      image_width;
  gint      image_height;
  gboolean  guides_found = FALSE;
  GList    *hguides, *hg;
  GList    *vguides, *vg;

  image_width  = gimp_image_get_width (image);
  image_height = gimp_image_get_height (image);

  hguides = g_list_append (NULL,    GINT_TO_POINTER (0));
  hguides = g_list_append (hguides, GINT_TO_POINTER (image_height));

  vguides = g_list_append (NULL,    GINT_TO_POINTER (0));
  vguides = g_list_append (vguides, GINT_TO_POINTER (image_width));

  for (guide = gimp_image_find_next_guide (image, 0);
       guide > 0;
       guide = gimp_image_find_next_guide (image, guide))
    {
      gint position = gimp_image_get_guide_position (image, guide);

      switch (gimp_image_get_guide_orientation (image, guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          if (! g_list_find (hguides, GINT_TO_POINTER (position)))
            {
              hguides = g_list_insert_sorted (hguides,
                                              GINT_TO_POINTER (position),
                                              guide_sort_func);
              guides_found = TRUE;
            }
          break;

        case GIMP_ORIENTATION_VERTICAL:
          if (! g_list_find (vguides, GINT_TO_POINTER (position)))
            {
              vguides = g_list_insert_sorted (vguides,
                                              GINT_TO_POINTER (position),
                                              guide_sort_func);
              guides_found = TRUE;
            }
          break;

        case GIMP_ORIENTATION_UNKNOWN:
          g_assert_not_reached ();
          break;
        }
    }

  if (guides_found)
    {
      GFile *file;
      gint   h, v, hpad, vpad;
      gint   x, y;
      gchar *hformat;
      gchar *format;

      file = gimp_image_get_file (image);

      if (! file)
        file = g_file_new_for_uri (_("Untitled"));

      /* get the number horizontal and vertical slices */
      h = g_list_length (hguides);
      v = g_list_length (vguides);

      /* need the number of digits of h and v for the padding */
      hpad = log10(h) + 1;
      vpad = log10(v) + 1;

      /* format for the x-y coordinates in the filename */
      hformat = g_strdup_printf ("%%0%i", MAX (hpad, vpad));
      format  = g_strdup_printf ("-%si-%si", hformat, hformat);

      /*  Do the actual dup'ing and cropping... this isn't a too naive a
       *  way to do this since we got copy-on-write tiles, either.
       */
      for (y = 0, hg = hguides; hg && hg->next; y++, hg = hg->next)
        {
          for (x = 0, vg = vguides; vg && vg->next; x++, vg = vg->next)
            {
              GimpImage *new_image = gimp_image_duplicate (image);
              GString   *new_uri;
              GFile     *new_file;
              gchar     *fileextension;
              gchar     *fileindex;

              if (! new_image)
                {
                  g_warning ("Couldn't create new image.");
                  g_free (hformat);
                  g_free (format);
                  return images;
                }

              gimp_image_undo_disable (new_image);

              gimp_image_crop (new_image,
                               GPOINTER_TO_INT (vg->next->data) -
                               GPOINTER_TO_INT (vg->data),
                               GPOINTER_TO_INT (hg->next->data) -
                               GPOINTER_TO_INT (hg->data),
                               GPOINTER_TO_INT (vg->data),
                               GPOINTER_TO_INT (hg->data));


              new_uri = g_string_new (g_file_get_uri (file));

              /* show the rough coordinates of the image in the title */
              fileindex    = g_strdup_printf (format, x, y);

              /* get the position of the file extension - last . in filename */
              fileextension = strrchr (new_uri->str, '.');
              if (fileextension)
                {
                  gint pos;

                  /* Truncate the extension. */
                  pos = fileextension - new_uri->str;
                  g_string_truncate (new_uri, pos);
                }

              /* insert the coordinates before the extension */
              g_string_append (new_uri, fileindex);
              g_free (fileindex);

              g_string_append (new_uri, ".xcf");

              new_file = g_file_new_for_uri (new_uri->str);
              g_string_free (new_uri, TRUE);

              gimp_image_set_file (new_image, new_file);
              g_object_unref (new_file);

              while ((guide = gimp_image_find_next_guide (new_image, 0)))
                gimp_image_delete_guide (new_image, guide);

              gimp_image_undo_enable (new_image);

              if (interactive)
                gimp_display_new (new_image);

              images = g_list_prepend (images, GINT_TO_POINTER (new_image));
            }
        }

      g_object_unref (file);
      g_free (hformat);
      g_free (format);
    }

  g_list_free (hguides);
  g_list_free (vguides);

  return g_list_reverse (images);
}
