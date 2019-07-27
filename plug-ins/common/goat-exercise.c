/*
 *  Goat exercise plug-in by Øyvind Kolås, pippin@gimp.org
 */

/*
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

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC "plug-in-goat-exercise"


typedef struct _Goat      Goat;
typedef struct _GoatClass GoatClass;

struct _Goat
{
  GimpPlugIn parent_instance;
};

struct _GoatClass
{
  GimpPlugInClass parent_class;
};


/* Declare local functions.
 */

#define GOAT_TYPE  (goat_get_type ())
#define GOAT (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOAT_TYPE, Goat))

GType                   goat_get_type         (void) G_GNUC_CONST;

static gchar         ** goat_query_procedures (GimpPlugIn           *plug_in,
                                               gint                 *n_procedures);
static GimpProcedure  * goat_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * goat_run              (GimpProcedure        *procedure,
                                               const GimpValueArray *args);


G_DEFINE_TYPE (Goat, goat, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GOAT_TYPE)


static void
goat_class_init (GoatClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = goat_query_procedures;
  plug_in_class->create_procedure = goat_create_procedure;
}

static void
goat_init (Goat *goat)
{
}

static gchar **
goat_query_procedures (GimpPlugIn *plug_in,
                       gint       *n_procedures)
{
  gchar **procedures = g_new0 (gchar *, 2);

  procedures[0] = g_strdup (PLUG_IN_PROC);

  *n_procedures = 1;

  return procedures;
}

static GimpProcedure *
goat_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (name, goat_run);

      gimp_procedure_set_strings (procedure,
                                  N_("Goat-exercise"),
                                  N_("Exercise a goat"),
                                  "takes a goat for a walk",
                                  PLUG_IN_PROC,
                                  "Øyvind Kolås <pippin@gimp.org>",
                                  "Øyvind Kolås <pippin@gimp.org>",
                                  "21march 2012",
                                  "RGB*, INDEXED*, GRAY*");

      gimp_procedure_add_menu_path (procedure, "<Image>/Filters");

      gimp_procedure_add_argument (procedure,
                                   g_param_spec_enum ("run-mode",
                                                      "Run mode",
                                                      "The run mode",
                                                      GIMP_TYPE_RUN_MODE,
                                                      GIMP_RUN_NONINTERACTIVE,
                                                      G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_image_id ("image",
                                                             "Image",
                                                             "The input image",
                                                             FALSE,
                                                             G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_drawable_id ("drawable",
                                                                "Drawable",
                                                                "The input drawable",
                                                                FALSE,
                                                                G_PARAM_READWRITE));
    }

  return procedure;
}

static GimpValueArray *
goat_run (GimpProcedure        *procedure,
          const GimpValueArray *args)
{
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            drawable_id;
  gint              x, y, width, height;

  INIT_I18N();
  gegl_init (NULL, NULL);

  g_printerr ("goat run %d %d %d\n",
              g_value_get_enum           (gimp_value_array_index (args, 0)),
              gimp_value_get_image_id    (gimp_value_array_index (args, 1)),
              gimp_value_get_drawable_id (gimp_value_array_index (args, 2)));

  drawable_id = gimp_value_get_drawable_id (gimp_value_array_index (args, 2));

  if (gimp_drawable_mask_intersect (drawable_id, &x, &y, &width, &height))
    {
      GeglBuffer *buffer;
      GeglBuffer *shadow_buffer;

      buffer        = gimp_drawable_get_buffer (drawable_id);
      shadow_buffer = gimp_drawable_get_shadow_buffer (drawable_id);

      gegl_render_op (buffer, shadow_buffer, "gegl:invert", NULL);

      g_object_unref (shadow_buffer); /* flushes the shadow tiles */
      g_object_unref (buffer);

      gimp_drawable_merge_shadow (drawable_id, TRUE);
      gimp_drawable_update (drawable_id, x, y, width, height);
      gimp_displays_flush ();
    }

  gegl_exit ();

  return gimp_procedure_new_return_values (procedure, status, NULL);
}
