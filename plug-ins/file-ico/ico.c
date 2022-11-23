/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * LIGMA Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

/* #define ICO_DBG */

#include "ico.h"
#include "ico-load.h"
#include "ico-save.h"

#include "libligma/stdplugins-intl.h"

#define LOAD_PROC           "file-ico-load"
#define LOAD_CUR_PROC       "file-cur-load"
#define LOAD_ANI_PROC       "file-ani-load"
#define LOAD_THUMB_PROC     "file-ico-load-thumb"
#define LOAD_ANI_THUMB_PROC "file-ani-load-thumb"
#define SAVE_PROC           "file-ico-save"
#define SAVE_CUR_PROC       "file-cur-save"
#define SAVE_ANI_PROC       "file-ani-save"


typedef struct _Ico      Ico;
typedef struct _IcoClass IcoClass;

struct _Ico
{
  LigmaPlugIn      parent_instance;
};

struct _IcoClass
{
  LigmaPlugInClass parent_class;
};


#define ICO_TYPE  (ico_get_type ())
#define ICO (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ICO_TYPE, Ico))

GType                   ico_get_type         (void) G_GNUC_CONST;

static GList          * ico_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * ico_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * ico_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * ani_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * ico_load_thumb       (LigmaProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * ani_load_thumb       (LigmaProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * ico_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * cur_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * ani_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);


G_DEFINE_TYPE (Ico, ico, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (ICO_TYPE)
DEFINE_STD_SET_I18N


static void
ico_class_init (IcoClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = ico_query_procedures;
  plug_in_class->create_procedure = ico_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
ico_init (Ico *ico)
{
}

static GList *
ico_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_ANI_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_CUR_PROC));
  list = g_list_append (list, g_strdup (LOAD_ANI_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));
  list = g_list_append (list, g_strdup (SAVE_CUR_PROC));
  list = g_list_append (list, g_strdup (SAVE_ANI_PROC));

  return list;
}

static LigmaProcedure *
ico_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           ico_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Microsoft Windows icon"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "Loads files of Windows ICO file format",
                                        "Loads files of Windows ICO file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>",
                                      "Christian Kreibich <christian@whoop.org>",
                                      "2002");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-ico");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "ico");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,\\0\\0\\1\\0");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_CUR_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           ico_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Microsoft Windows cursor"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "Loads files of Windows CUR file format",
                                        "Loads files of Windows CUR file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "2002-2022");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/vnd.microsoft.icon");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "cur");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,\\0\\0\\2\\0");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_ANI_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           ani_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Microsoft Windows animated cursor"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        _("Loads files of Windows ANI file format"),
                                        "Loads files of Windows ANI file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "2007-2022");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "application/x-navi-animation");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "ani");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "8,string,ACON");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_ANI_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                ico_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Loads a preview from a Windows ICO or CUR files",
                                        "",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Sven Neumann <sven@ligma.org>",
                                      "2005");
    }
  else if (! strcmp (name, LOAD_ANI_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                ani_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        _("Loads a preview from a Windows ANI files"),
                                        "",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann, James Huang, "
                                      "Alex S.",
                                      "Dom Lachowicz, "
                                      "Sven Neumann <sven@ligma.org>, "
                                      "James Huang, Alex S.",
                                      "2007-2022");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           ico_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("Microsoft Windows icon"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "Saves files in Windows ICO file format",
                                        "Saves files in Windows ICO file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>",
                                      "Christian Kreibich <christian@whoop.org>",
                                      "2002");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-ico");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "ico");
    }
  else if (! strcmp (name, SAVE_CUR_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           cur_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("Microsoft Windows cursor"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "Saves files in Windows CUR file format",
                                        "Saves files in Windows CUR file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "2002-2022");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/vnd.microsoft.icon");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "cur");

      LIGMA_PROC_ARG_INT (procedure, "n-hot-spot-x",
                         "Number of hot spot's X coordinates",
                         "Number of hot spot's X coordinates",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);
      LIGMA_PROC_ARG_INT32_ARRAY (procedure, "hot-spot-x",
                                 "Hot spot X",
                                 "X coordinates of hot spot (one per layer)",
                                 G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "n-hot-spot-y",
                         "Number of hot spot's Y coordinates",
                         "Number of hot spot's Y coordinates",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);
      LIGMA_PROC_ARG_INT32_ARRAY (procedure, "hot-spot-y",
                                 "Hot spot Y",
                                 "Y coordinates of hot spot (one per layer)",
                                 G_PARAM_READWRITE);
    }
  else if (! strcmp (name, SAVE_ANI_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           ani_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("Microsoft Windows animated cursor"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        _("Saves files in Windows ANI file format"),
                                        _("Saves files in Windows ANI file format"),
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "2007-2022");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "application/x-navi-animation");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "ani");

      LIGMA_PROC_ARG_STRING (procedure, "cursor-name",
                            "Cursor Name",
                            _("Cursor Name (Optional)"),
                            NULL,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "author-name",
                            "Cursor Author",
                            _("Cursor Author (Optional)"),
                            NULL,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "default-delay",
                         "Default delay",
                         "Default delay between frames "
                         "in jiffies (1/60 of a second)",
                         0, G_MAXINT, 8,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "n-hot-spot-x",
                         "Number of hot spot's X coordinates",
                         "Number of hot spot's X coordinates",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);
      LIGMA_PROC_ARG_INT32_ARRAY (procedure, "hot-spot-x",
                                 "Hot spot X",
                                 "X coordinates of hot spot (one per layer)",
                                 G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "n-hot-spot-y",
                         "Number of hot spot's Y coordinates",
                         "Number of hot spot's Y coordinates",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);
      LIGMA_PROC_ARG_INT32_ARRAY (procedure, "hot-spot-y",
                                 "Hot spot Y",
                                 "Y coordinates of hot spot (one per layer)",
                                 G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
ico_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = ico_load_image (file, NULL, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
ani_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = ani_load_image (file, FALSE,
                          NULL, NULL, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
ico_load_thumb (LigmaProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const LigmaValueArray *args,
                gpointer              run_data)
{
  LigmaValueArray *return_vals;
  gint            width;
  gint            height;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  width  = size;
  height = size;

  image = ico_load_thumbnail_image (file,
                                    &width, &height, 0, &error);

  if (image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, width);
  LIGMA_VALUES_SET_INT   (return_vals, 3, height);

  ligma_value_array_truncate (return_vals, 4);

  return return_vals;
}

static LigmaValueArray *
ani_load_thumb (LigmaProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const LigmaValueArray *args,
                gpointer              run_data)
{
  LigmaValueArray *return_vals;
  gint            width;
  gint            height;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  width  = size;
  height = size;

  image = ani_load_image (file, TRUE,
                          &width, &height, &error);

  if (image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, width);
  LIGMA_VALUES_SET_INT   (return_vals, 3, height);

  ligma_value_array_truncate (return_vals, 4);

  return return_vals;
}

static LigmaValueArray *
ico_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaPDBStatusType  status;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  status = ico_save_image (file, image, run_mode, &error);

  return ligma_procedure_new_return_values (procedure, status, error);
}

static LigmaValueArray *
cur_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaPDBStatusType    status;
  GError              *error        = NULL;
  gint32              *hot_spot_x   = NULL;
  gint32              *hot_spot_y   = NULL;
  gint                 n_hot_spot_x = 0;
  gint                 n_hot_spot_y = 0;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  g_object_get (config,
                "n-hot-spot-x", &n_hot_spot_x,
                "n-hot-spot-y", &n_hot_spot_y,
                "hot-spot-x",   &hot_spot_x,
                "hot-spot-y",   &hot_spot_y,
                NULL);

  status = cur_save_image (file, image, run_mode,
                           &n_hot_spot_x, &hot_spot_x,
                           &n_hot_spot_y, &hot_spot_y,
                           &error);

  if (status == LIGMA_PDB_SUCCESS)
    {
      /* XXX: seems libligmaconfig is not able to serialize
       * LigmaInt32Array args yet anyway. Still leave this here for now,
       * as reminder of missing feature when we see the warnings.
       */
      g_object_set (config,
                    "n-hot-spot-x", n_hot_spot_x,
                    "n-hot-spot-y", n_hot_spot_y,
                    /*"hot-spot-x",   hot_spot_x,*/
                    /*"hot-spot-y",   hot_spot_y,*/
                    NULL);
      g_free (hot_spot_x);
      g_free (hot_spot_y);
    }

  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  return ligma_procedure_new_return_values (procedure, status, error);
}

static LigmaValueArray *
ani_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaPDBStatusType    status;
  GError              *error        = NULL;
  gchar               *inam         = NULL;
  gchar               *iart         = NULL;
  gint                 jif_rate     = 0;
  gint32              *hot_spot_x   = NULL;
  gint32              *hot_spot_y   = NULL;
  gint                 n_hot_spot_x = 0;
  gint                 n_hot_spot_y = 0;
  AniFileHeader        header;
  AniSaveInfo          ani_info;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  g_object_get (config,
                "cursor-name",   &inam,
                "author-name",   &iart,
                "default-delay", &jif_rate,
                "n-hot-spot-x",  &n_hot_spot_x,
                "n-hot-spot-y",  &n_hot_spot_y,
                "hot-spot-x",    &hot_spot_x,
                "hot-spot-y",    &hot_spot_y,
                NULL);

  /* Jiffies (1/60th of a second) used if rate chunk not present. */
  header.jif_rate = jif_rate;
  ani_info.inam = inam;
  ani_info.iart = iart;

  status = ani_save_image (file, image, run_mode,
                           &n_hot_spot_x, &hot_spot_x,
                           &n_hot_spot_y, &hot_spot_y,
                           &header, &ani_info, &error);

  if (status == LIGMA_PDB_SUCCESS)
    {
      /* XXX: seems libligmaconfig is not able to serialize
       * LigmaInt32Array args yet anyway. Still leave this here for now,
       * as reminder of missing feature when we see the warnings.
       */
      g_object_set (config,
                    "cursor-name",   NULL,
                    "author-name",   NULL,
                    "default-delay", header.jif_rate,
                    "n-hot-spot-x",  n_hot_spot_x,
                    "n-hot-spot-y",  n_hot_spot_y,
                    /*"hot-spot-x",   hot_spot_x,*/
                    /*"hot-spot-y",   hot_spot_y,*/
                    NULL);
      g_free (hot_spot_x);
      g_free (hot_spot_y);

      g_free (inam);
      g_free (iart);
      g_free (ani_info.inam);
      g_free (ani_info.iart);
      memset (&ani_info, 0, sizeof (AniSaveInfo));
    }

  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  return ligma_procedure_new_return_values (procedure, status, error);
}

gint
ico_rowstride (gint width,
               gint bpp)
{
  switch (bpp)
    {
    case 1:
      if ((width % 32) == 0)
        return width / 8;
      else
        return 4 * (width/32 + 1);
      break;

    case 4:
      if ((width % 8) == 0)
        return width / 2;
      else
        return 4 * (width/8 + 1);
      break;

    case 8:
      if ((width % 4) == 0)
        return width;
      else
        return 4 * (width/4 + 1);
      break;

    case 24:
      if (((width*3) % 4) == 0)
        return width * 3;
      else
        return 4 * (width*3/4+1);

    case 32:
      return width * 4;

    default:
      g_warning ("invalid bitrate: %d\n", bpp);
      g_assert_not_reached ();
      return width * (bpp/8);
    }
}

guint8 *
ico_alloc_map (gint  width,
               gint  height,
               gint  bpp,
               gint *length)
{
  gint    len = 0;
  guint8 *map = NULL;

  len = ico_rowstride (width, bpp) * height;

  *length = len;
  map = g_new0 (guint8, len);

  return map;
}
