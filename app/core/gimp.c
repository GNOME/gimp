/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/tile-manager.h"

#include "pdb/procedural_db.h"

#include "gimp.h"
#include "gimpbrush.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushpipe.h"
#include "gimpbuffer.h"
#include "gimpcontext.h"
#include "gimpcoreconfig.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpimage-new.h"
#include "gimplist.h"
#include "gimppalette.h"
#include "gimppattern.h"
#include "gimptoolinfo.h"

#include "appenv.h"
#include "app_procs.h"
#include "gimpparasite.h"

#include "libgimp/gimpintl.h"


static void   gimp_class_init (GimpClass *klass);
static void   gimp_init       (Gimp      *gimp);

static void   gimp_destroy    (GtkObject *object);


static GimpObjectClass *parent_class = NULL;


GtkType 
gimp_get_type (void)
{
  static GtkType object_type = 0;

  if (! object_type)
    {
      GtkTypeInfo object_info =
      {
        "Gimp",
        sizeof (Gimp),
        sizeof (GimpClass),
        (GtkClassInitFunc) gimp_class_init,
        (GtkObjectInitFunc) gimp_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      object_type = gtk_type_unique (GIMP_TYPE_OBJECT, &object_info);
    }

  return object_type;
}

static void
gimp_class_init (GimpClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  object_class->destroy = gimp_destroy;
}

static void
gimp_init (Gimp *gimp)
{
  GimpContext *context;

  static const GimpDataFactoryLoaderEntry brush_loader_entries[] =
  {
    { gimp_brush_load,           GIMP_BRUSH_FILE_EXTENSION           },
    { gimp_brush_load,           GIMP_BRUSH_PIXMAP_FILE_EXTENSION    },
    { gimp_brush_generated_load, GIMP_BRUSH_GENERATED_FILE_EXTENSION },
    { gimp_brush_pipe_load,      GIMP_BRUSH_PIPE_FILE_EXTENSION      }
  };
  static gint n_brush_loader_entries = (sizeof (brush_loader_entries) /
					sizeof (brush_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry pattern_loader_entries[] =
  {
    { gimp_pattern_load, GIMP_PATTERN_FILE_EXTENSION }
  };
  static gint n_pattern_loader_entries = (sizeof (pattern_loader_entries) /
					  sizeof (pattern_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry gradient_loader_entries[] =
  {
    { gimp_gradient_load, GIMP_GRADIENT_FILE_EXTENSION },
    { gimp_gradient_load, NULL /* legacy loader */     }
  };
  static gint n_gradient_loader_entries = (sizeof (gradient_loader_entries) /
					   sizeof (gradient_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry palette_loader_entries[] =
  {
    { gimp_palette_load, GIMP_PALETTE_FILE_EXTENSION },
    { gimp_palette_load, NULL /* legacy loader */    }
  };
  static gint n_palette_loader_entries = (sizeof (palette_loader_entries) /
					  sizeof (palette_loader_entries[0]));

  gimp->images = gimp_list_new (GIMP_TYPE_IMAGE,
				GIMP_CONTAINER_POLICY_WEAK);
  gtk_object_ref (GTK_OBJECT (gimp->images));
  gtk_object_sink (GTK_OBJECT (gimp->images));

  gimp->global_buffer = NULL;
  gimp->named_buffers = gimp_list_new (GIMP_TYPE_BUFFER,
				       GIMP_CONTAINER_POLICY_STRONG);
  gtk_object_ref (GTK_OBJECT (gimp->named_buffers));
  gtk_object_sink (GTK_OBJECT (gimp->named_buffers));

  gimp_parasites_init (gimp);

  gimp->brush_factory =
    gimp_data_factory_new (GIMP_TYPE_BRUSH,
			   (const gchar **) &core_config->brush_path,
			   brush_loader_entries,
			   n_brush_loader_entries,
			   gimp_brush_new,
			   gimp_brush_get_standard);
  gtk_object_ref (GTK_OBJECT (gimp->brush_factory));
  gtk_object_sink (GTK_OBJECT (gimp->brush_factory));

  gimp->pattern_factory =
    gimp_data_factory_new (GIMP_TYPE_PATTERN,
			   (const gchar **) &core_config->pattern_path,
			   pattern_loader_entries,
			   n_pattern_loader_entries,
			   gimp_pattern_new,
			   gimp_pattern_get_standard);
  gtk_object_ref (GTK_OBJECT (gimp->pattern_factory));
  gtk_object_sink (GTK_OBJECT (gimp->pattern_factory));

  gimp->gradient_factory =
    gimp_data_factory_new (GIMP_TYPE_GRADIENT,
			   (const gchar **) &core_config->gradient_path,
			   gradient_loader_entries,
			   n_gradient_loader_entries,
			   gimp_gradient_new,
			   gimp_gradient_get_standard);
  gtk_object_ref (GTK_OBJECT (gimp->gradient_factory));
  gtk_object_sink (GTK_OBJECT (gimp->gradient_factory));

  gimp->palette_factory =
    gimp_data_factory_new (GIMP_TYPE_PALETTE,
			   (const gchar **) &core_config->palette_path,
			   palette_loader_entries,
			   n_palette_loader_entries,
			   gimp_palette_new,
			   gimp_palette_get_standard);
  gtk_object_ref (GTK_OBJECT (gimp->palette_factory));
  gtk_object_sink (GTK_OBJECT (gimp->palette_factory));

  procedural_db_init (gimp);

  gimp->tool_info_list = gimp_list_new (GIMP_TYPE_TOOL_INFO,
					GIMP_CONTAINER_POLICY_STRONG);
  gtk_object_ref (GTK_OBJECT (gimp->tool_info_list));
  gtk_object_sink (GTK_OBJECT (gimp->tool_info_list));

  gimp_image_new_init (gimp);

  gimp->standard_context = gimp_create_context (gimp, "Standard", NULL);
  gtk_object_ref (GTK_OBJECT (gimp->standard_context));
  gtk_object_sink (GTK_OBJECT (gimp->standard_context));

  /*  TODO: load from disk  */
  context = gimp_create_context (gimp, "Default", NULL);
  gimp_set_default_context (gimp, context);

  context = gimp_create_context (gimp, "User", context);
  gimp_set_user_context (gimp, context);

  gimp_set_current_context (gimp, context);
}

static void
gimp_destroy (GtkObject *object)
{
  Gimp *gimp;

  gimp = GIMP (object);

  gimp_set_current_context (gimp, NULL);

  gimp_set_user_context (gimp, NULL);
  gimp_set_default_context (gimp, NULL);

  if (gimp->standard_context)
    {
      gtk_object_unref (GTK_OBJECT (gimp->standard_context));
      gimp->standard_context = NULL;
    }

  gimp_image_new_exit (gimp);

  if (gimp->tool_info_list)
    {
      gtk_object_unref (GTK_OBJECT (gimp->tool_info_list));
      gimp->tool_info_list = NULL;
    }

  procedural_db_free (gimp);

  if (gimp->brush_factory)
    {
      gimp_data_factory_data_free (gimp->brush_factory);
      gtk_object_unref (GTK_OBJECT (gimp->brush_factory));
      gimp->brush_factory = NULL;
    }

  if (gimp->pattern_factory)
    {
      gimp_data_factory_data_free (gimp->pattern_factory);
      gtk_object_unref (GTK_OBJECT (gimp->pattern_factory));
      gimp->pattern_factory = NULL;
    }

  if (gimp->gradient_factory)
    {
      gimp_data_factory_data_free (gimp->gradient_factory);
      gtk_object_unref (GTK_OBJECT (gimp->gradient_factory));
      gimp->gradient_factory = NULL;
    }

  if (gimp->palette_factory)
    {
      gimp_data_factory_data_free (gimp->palette_factory);
      gtk_object_unref (GTK_OBJECT (gimp->palette_factory));
      gimp->palette_factory = NULL;
    }

  gimp_parasites_exit (gimp);

  if (gimp->named_buffers)
    {
      gtk_object_unref (GTK_OBJECT (gimp->named_buffers));
      gimp->named_buffers = NULL;
    }

  if (gimp->global_buffer)
    {
      tile_manager_destroy (gimp->global_buffer);
      gimp->global_buffer = NULL;
    }

  if (gimp->images)
    {
      gtk_object_unref (GTK_OBJECT (gimp->images));
      gimp->images = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

Gimp *
gimp_new (void)
{
  Gimp *gimp;

  gimp = gtk_type_new (GIMP_TYPE_GIMP);

  return gimp;
}

void
gimp_restore (Gimp *gimp)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  initialize  the global parasite table  */
  app_init_update_status (_("Looking for data files"), _("Parasites"), 0.00);
  gimp_parasiterc_load (gimp);

  /*  initialize the list of gimp brushes    */
  app_init_update_status (NULL, _("Brushes"), 0.20);
  gimp_data_factory_data_init (gimp->brush_factory, no_data); 

  /*  initialize the list of gimp patterns   */
  app_init_update_status (NULL, _("Patterns"), 0.40);
  gimp_data_factory_data_init (gimp->pattern_factory, no_data); 

  /*  initialize the list of gimp palettes   */
  app_init_update_status (NULL, _("Palettes"), 0.60);
  gimp_data_factory_data_init (gimp->palette_factory, no_data); 

  /*  initialize the list of gimp gradients  */
  app_init_update_status (NULL, _("Gradients"), 0.80);
  gimp_data_factory_data_init (gimp->gradient_factory, no_data); 

  app_init_update_status (NULL, NULL, 1.00);
}

void
gimp_shutdown (Gimp *gimp)
{
  gimp_data_factory_data_save (gimp->brush_factory);
  gimp_data_factory_data_save (gimp->pattern_factory);
  gimp_data_factory_data_save (gimp->gradient_factory);
  gimp_data_factory_data_save (gimp->palette_factory);
  gimp_parasiterc_save (gimp);
}

GimpImage *
gimp_create_image (Gimp              *gimp,
		   gint               width,
		   gint               height,
		   GimpImageBaseType  type,
		   gboolean           attach_comment)
{
  GimpImage *gimage;

  g_return_val_if_fail (gimp != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  gimage = gimp_image_new (gimp, width, height, type);

  gimp_container_add (gimp->images, GIMP_OBJECT (gimage));

  if (attach_comment && core_config->default_comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment",
				    GIMP_PARASITE_PERSISTENT,
				    strlen (core_config->default_comment) + 1,
				    core_config->default_comment);
      gimp_image_parasite_attach (gimage, parasite);
      gimp_parasite_free (parasite);
    }

  return gimage;
}

void
gimp_create_display (Gimp      *gimp,
		     GimpImage *gimage)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->create_display_func)
    gimp->create_display_func (gimage);
}

/*
void
gimp_open_file (Gimp        *gimp,
		const gchar *filename,
		gboolean     with_display)
{
  GimpImage *gimage;
  gint       status;

  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (filename != NULL);

  gimage = file_open_image (gimp,
			    filename,
			    filename,
			    _("Open"),
			    NULL,
			    RUN_INTERACTIVE,
			    &status);

  if (gimage)
    {
      gchar *absolute;

      * enable & clear all undo steps *
      gimp_image_undo_enable (gimage);

      * set the image to clean  *
      gimp_image_clean_all (gimage);

      if (with_display)
	gimp_create_display (gimage->gimp, gimage);

      absolute = file_open_absolute_filename (filename);

      document_index_add (absolute);
      menus_last_opened_add (absolute);

      g_free (absolute);
    }
}
*/

GimpContext *
gimp_create_context (Gimp        *gimp,
		     const gchar *name,
		     GimpContext *template)
{
  GimpContext *context;

  g_return_val_if_fail (gimp != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (!template || GIMP_IS_CONTEXT (template), NULL);

  /*  FIXME: need unique names here  */
  if (! name)
    name = "Unnamed";

  context = gimp_context_new (gimp, name, template);

  gimp->context_list = g_list_prepend (gimp->context_list, context);

  return context;
}

GimpContext *
gimp_get_standard_context (Gimp *gimp)
{
  return gimp->standard_context;
}

void
gimp_set_default_context (Gimp        *gimp,
			  GimpContext *context)
{
  gimp->default_context = context;
}

GimpContext *
gimp_get_default_context (Gimp *gimp)
{
  return gimp->default_context;
}

void
gimp_set_user_context (Gimp        *gimp,
		       GimpContext *context)
{
  gimp->user_context = context;
}

GimpContext *
gimp_get_user_context (Gimp *gimp)
{
  return gimp->user_context;
}

void
gimp_set_current_context (Gimp        *gimp,
			  GimpContext *context)
{
  gimp->current_context = context;
}

GimpContext *
gimp_get_current_context (Gimp *gimp)
{
  return gimp->current_context;
}
