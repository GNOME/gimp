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

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpconfig-params.h"

#include "gimpenummenu.h"
#include "gimppropwidgets.h"

#include "libgimp/gimpintl.h"


/******************/
/*  check button  */
/******************/

static void   gimp_prop_check_button_callback (GtkWidget  *widget,
                                               GObject    *config);
static void   gimp_prop_check_button_notify   (GObject    *config,
                                               GParamSpec *param_spec,
                                               GtkWidget  *button);

GtkWidget *
gimp_prop_check_button_new (GObject     *config,
                            const gchar *property_name,
                            const gchar *label)
{
  GParamSpec *param_spec;
  GtkWidget  *button;
  gchar      *notify_name;
  gboolean    value;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! G_IS_PARAM_SPEC_BOOLEAN (param_spec))
    {
      g_warning (G_STRLOC ": property '%s' of %s is not boolean",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_get (config,
                property_name, &value,
                NULL);

  button = gtk_check_button_new_with_label (label);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), value);

  g_object_set_data (G_OBJECT (button), "gimp-config-param-spec", param_spec);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (gimp_prop_check_button_callback),
		    config);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_check_button_notify),
                           button, 0);

  g_free (notify_name);

  return button;
}

static void
gimp_prop_check_button_callback (GtkWidget *widget,
                                 GObject   *config)
{
  GParamSpec *param_spec;

  param_spec = g_object_get_data (G_OBJECT (widget), "gimp-config-param-spec");

  if (! param_spec)
    return;

  g_object_set (config,
                param_spec->name, GTK_TOGGLE_BUTTON (widget)->active,
                NULL);
}

static void
gimp_prop_check_button_notify (GObject    *config,
                               GParamSpec *param_spec,
                               GtkWidget  *button)
{
  gboolean value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (GTK_TOGGLE_BUTTON (button)->active != value)
    {
      g_signal_handlers_block_by_func (G_OBJECT (button),
                                       gimp_prop_check_button_callback,
                                       config);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), value);

      g_signal_handlers_unblock_by_func (G_OBJECT (button),
                                         gimp_prop_check_button_callback,
                                         config);
    }
}


/******************/
/*  option menus  */
/******************/

static GtkWidget * gimp_prop_enum_option_menu_new_full (GObject     *config,
                                                        const gchar *property_name,
                                                        gint         minimum,
                                                        gint         maximum,
                                                        gint         n_values,
                                                        va_list      args);
static void        gimp_prop_enum_option_menu_callback (GtkWidget   *widget,
                                                        GObject     *config);
static void        gimp_prop_enum_option_menu_notify   (GObject     *config,
                                                        GParamSpec  *param_spec,
                                                        GtkWidget   *menu);

GtkWidget *
gimp_prop_enum_option_menu_new (GObject     *config,
                                const gchar *property_name,
                                gint         minimum,
                                gint         maximum)
{
  return gimp_prop_enum_option_menu_new_full (config, property_name,
                                              minimum, maximum, 
                                              0, 0);
}

GtkWidget *
gimp_prop_enum_option_menu_new_with_values (GObject     *config,
                                            const gchar *property_name,
                                            gint         n_values,
                                            ...)
{
  GtkWidget  *optionmenu;
  va_list     args;

  va_start (args, n_values);

  optionmenu = gimp_prop_enum_option_menu_new_full (config, property_name,
                                                    0, 0,
                                                    n_values, args);

  va_end (args);

  return optionmenu;
}

GtkWidget *
gimp_prop_enum_option_menu_new_valist (GObject     *config,
                                       const gchar *property_name,
                                       gint         n_values,
                                       va_list      args)
{
  return gimp_prop_enum_option_menu_new_full (config, property_name,
                                              0, 0,
                                              n_values, args);
}

GtkWidget *
gimp_prop_boolean_option_menu_new (GObject     *config,
                                   const gchar *property_name,
                                   const gchar *true_text,
                                   const gchar *false_text)
{
  GParamSpec *param_spec;
  GtkWidget  *optionmenu;
  gchar      *notify_name;
  gboolean    value;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! G_IS_PARAM_SPEC_BOOLEAN (param_spec))
    {
      g_warning (G_STRLOC ": property '%s' of %s is not boolean",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_get (config,
                property_name, &value,
                NULL);

  optionmenu =
    gimp_option_menu_new2 (FALSE,
                           G_CALLBACK (gimp_prop_enum_option_menu_callback),
			   config,
			   GINT_TO_POINTER (value),

			   true_text,  GINT_TO_POINTER (TRUE),  NULL,
			   false_text, GINT_TO_POINTER (FALSE), NULL,

			   NULL);

  g_object_set_data (G_OBJECT (optionmenu), "gimp-config-param-spec",
                     param_spec);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_enum_option_menu_notify),
                           optionmenu, 0);

  g_free (notify_name);

  return optionmenu;
}

static GtkWidget *
gimp_prop_enum_option_menu_new_full (GObject     *config,
                                     const gchar *property_name,
                                     gint         minimum,
                                     gint         maximum,
                                     gint         n_values,
                                     va_list      args)
{
  GParamSpec *param_spec;
  GtkWidget  *optionmenu;
  gchar      *notify_name;
  gint        value;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! G_IS_PARAM_SPEC_ENUM (param_spec))
    {
      g_warning (G_STRLOC ": property '%s' of %s is not an enum",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_get (config,
                property_name, &value,
                NULL);

  if (n_values > 0)
    {
      optionmenu =
        gimp_enum_option_menu_new_with_values_valist (param_spec->value_type,
                                                      G_CALLBACK (gimp_prop_enum_option_menu_callback),
                                                      config,
                                                      n_values, args);
    }
  else
    {
      if (minimum != maximum)
        {
          optionmenu =
            gimp_enum_option_menu_new_with_range (param_spec->value_type,
                                                  minimum, maximum,
                                                  G_CALLBACK (gimp_prop_enum_option_menu_callback),
                                                  config);
        }
      else
        {
          optionmenu =
            gimp_enum_option_menu_new (param_spec->value_type,
                                       G_CALLBACK (gimp_prop_enum_option_menu_callback),
                                       config);
        }
    }

  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (value));

  g_object_set_data (G_OBJECT (optionmenu), "gimp-config-param-spec",
                     param_spec);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_enum_option_menu_notify),
                           optionmenu, 0);

  g_free (notify_name);

  return optionmenu;
}

static void
gimp_prop_enum_option_menu_callback (GtkWidget *widget,
                                     GObject   *config)
{
  if (GTK_IS_MENU (widget->parent))
    {
      GtkWidget *optionmenu;

      optionmenu = gtk_menu_get_attach_widget (GTK_MENU (widget->parent));

      if (optionmenu)
        {
          GParamSpec *param_spec;

          param_spec = g_object_get_data (G_OBJECT (optionmenu),
                                          "gimp-config-param-spec");

          if (param_spec)
            {
              gint value;

              value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                          "gimp-item-data"));

              g_object_set (config,
                            param_spec->name, value,
                            NULL);
            }
        }
    }
}

static void
gimp_prop_enum_option_menu_notify (GObject    *config,
                                   GParamSpec *param_spec,
                                   GtkWidget  *menu)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  gimp_option_menu_set_history (GTK_OPTION_MENU (menu),
                                GINT_TO_POINTER (value));
}


/*****************/
/*  adjustments  */
/*****************/

static void   gimp_prop_adjustment_callback (GtkAdjustment *adjustment,
                                             GObject       *config);
static void   gimp_prop_adjustment_notify   (GObject       *config,
                                             GParamSpec    *param_spec,
                                             GtkAdjustment *adjustment);

GtkWidget *
gimp_prop_spin_button_new (GObject     *config,
                           const gchar *property_name,
                           gdouble      step_increment,
                           gdouble      page_increment,
                           gint         digits)
{
  GParamSpec *param_spec;
  GtkWidget  *spinbutton;
  GtkObject  *adjustment;
  gchar      *notify_name;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      GParamSpecInt *int_spec;
      gint           value;

      int_spec = G_PARAM_SPEC_INT (param_spec);

      g_object_get (config, property_name, &value, NULL);

      spinbutton = gimp_spin_button_new (&adjustment, value,
                                         int_spec->minimum,
                                         int_spec->maximum,
                                         step_increment,
                                         page_increment,
                                         0.0, 1.0, 0);
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      GParamSpecUInt *uint_spec;
      guint           value;

      uint_spec = G_PARAM_SPEC_UINT (param_spec);

      g_object_get (config, property_name, &value, NULL);

      spinbutton = gimp_spin_button_new (&adjustment, value,
                                         uint_spec->minimum,
                                         uint_spec->maximum,
                                         step_increment,
                                         page_increment,
                                         0.0, 1.0, 0);
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      GParamSpecDouble *double_spec;
      gdouble           value;

      double_spec = G_PARAM_SPEC_DOUBLE (param_spec);

      g_object_get (config, property_name, &value, NULL);

      spinbutton = gimp_spin_button_new (&adjustment, value,
                                         double_spec->minimum,
                                         double_spec->maximum,
                                         step_increment,
                                         page_increment,
                                         0.0, 1.0, digits);
    }
  else
    {
      g_warning (G_STRLOC ": property '%s' of %s is not numeric",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_set_data (G_OBJECT (adjustment), "gimp-config-param-spec",
                     param_spec);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_prop_adjustment_callback),
		    config);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_adjustment_notify),
                           adjustment, 0);

  g_free (notify_name);

  return spinbutton;
}

GtkWidget *
gimp_prop_memsize_entry_new (GObject     *config,
                             const gchar *property_name)
{
  GParamSpec      *param_spec;
  GParamSpecULong *ulong_spec;
  GtkObject       *adjustment;
  GtkWidget       *entry;
  gchar           *notify_name;
  guint            value;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! GIMP_IS_PARAM_SPEC_MEMSIZE (param_spec))
    {
      g_warning (G_STRLOC ": property '%s' of %s is not a memsize",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_get (config,
                property_name, &value,
                NULL);

  ulong_spec = G_PARAM_SPEC_ULONG (param_spec);

  adjustment = gtk_adjustment_new (value,
				   ulong_spec->minimum,
				   ulong_spec->maximum,
                                   1, 64, 0);
  entry = gimp_memsize_entry_new (GTK_ADJUSTMENT (adjustment));

  g_object_set_data (G_OBJECT (adjustment), "gimp-config-param-spec",
                     param_spec);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_prop_adjustment_callback),
		    config);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_adjustment_notify),
                           adjustment, 0);

  g_free (notify_name);

  return entry;
}

static void
gimp_prop_adjustment_callback (GtkAdjustment *adjustment,
                               GObject       *config)
{
  GParamSpec *param_spec;

  param_spec = g_object_get_data (G_OBJECT (adjustment),
                                  "gimp-config-param-spec");

  if (! param_spec)
    return;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      g_object_set (config,
                    param_spec->name, (gint) adjustment->value,
                    NULL);
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      g_object_set (config,
                    param_spec->name, (guint) adjustment->value,
                    NULL);
    }
  else if (G_IS_PARAM_SPEC_LONG (param_spec))
    {
      g_object_set (config,
                    param_spec->name, (glong) adjustment->value,
                    NULL);
    }
  else if (G_IS_PARAM_SPEC_ULONG (param_spec))
    {
      g_object_set (config,
                    param_spec->name, (gulong) adjustment->value,
                    NULL);
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      g_object_set (config,
                    param_spec->name, adjustment->value,
                    NULL);
    }
}

static void
gimp_prop_adjustment_notify (GObject       *config,
                             GParamSpec    *param_spec,
                             GtkAdjustment *adjustment)
{
  gdouble value;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config,
                    param_spec->name, &int_value,
                    NULL);

      value = int_value;
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      guint uint_value;

      g_object_get (config,
                    param_spec->name, &uint_value,
                    NULL);

      value = uint_value;
    }
  else if (G_IS_PARAM_SPEC_LONG (param_spec))
    {
      glong long_value;

      g_object_get (config,
                    param_spec->name, &long_value,
                    NULL);

      value = long_value;
    }
  else if (G_IS_PARAM_SPEC_ULONG (param_spec))
    {
      gulong ulong_value;

      g_object_get (config,
                    param_spec->name, &ulong_value,
                    NULL);

      value = ulong_value;
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      g_object_get (config,
                    param_spec->name, &value,
                    NULL);
    }
  else
    {
      return;
    }

  if (adjustment->value != value)
    {
      g_signal_handlers_block_by_func (G_OBJECT (adjustment),
                                       gimp_prop_adjustment_callback,
                                       config);

      gtk_adjustment_set_value (adjustment, value);

      g_signal_handlers_unblock_by_func (G_OBJECT (adjustment),
                                         gimp_prop_adjustment_callback,
                                         config);
    }
}


/*****************/
/*  text buffer  */
/*****************/

static void   gimp_prop_text_buffer_callback (GtkTextBuffer *text_buffer,
                                              GObject       *config);
static void   gimp_prop_text_buffer_notify   (GObject       *config,
                                              GParamSpec    *param_spec,
                                              GtkTextBuffer *text_buffer);

GtkTextBuffer *
gimp_prop_text_buffer_new (GObject     *config,
                           const gchar *property_name,
                           gint         max_len)
{
  GParamSpec    *param_spec;
  GtkTextBuffer *text_buffer;
  gchar         *notify_name;
  gchar         *value;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! G_IS_PARAM_SPEC_STRING (param_spec))
    {
      g_warning (G_STRLOC ": property '%s' of %s is not a string",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_get (config,
                property_name, &value,
                NULL);

  text_buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (text_buffer, value, -1);

  g_free (value);

  if (max_len > 0)
    g_object_set_data (G_OBJECT (text_buffer), "max-len",
                       GINT_TO_POINTER (max_len));

  g_object_set_data (G_OBJECT (text_buffer), "gimp-config-param-spec",
                     param_spec);

  g_signal_connect (G_OBJECT (text_buffer), "changed",
		    G_CALLBACK (gimp_prop_text_buffer_callback),
		    config);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_text_buffer_notify),
                           text_buffer, 0);

  g_free (notify_name);

  return text_buffer;
}

static void
gimp_prop_text_buffer_callback (GtkTextBuffer *text_buffer,
                                GObject       *config)
{
  GParamSpec *param_spec;

  param_spec = g_object_get_data (G_OBJECT (text_buffer),
                                  "gimp-config-param-spec");

  if (param_spec)
    {
      GtkTextIter  start_iter;
      GtkTextIter  end_iter;
      gchar       *text;
      gint         max_len;

      gtk_text_buffer_get_bounds (text_buffer, &start_iter, &end_iter);
      text = gtk_text_buffer_get_text (text_buffer,
                                       &start_iter, &end_iter, FALSE);

      max_len = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (text_buffer),
                                                    "max-length"));

      if (max_len > 0 && strlen (text) > max_len)
        {
          g_message (_("This text input field is limited to %d characters."), 
                     max_len);

          gtk_text_buffer_get_iter_at_offset (text_buffer, &start_iter,
                                              max_len - 1);
          gtk_text_buffer_get_end_iter (text_buffer, &end_iter);

          /*  this calls us recursivaly, but in the else branch  */
          gtk_text_buffer_delete (text_buffer, &start_iter, &end_iter);
        }
      else
        {
          g_signal_handlers_block_by_func (G_OBJECT (config),
                                           gimp_prop_text_buffer_notify,
                                           text_buffer);

          g_object_set (config,
                        param_spec->name, text,
                        NULL);

          g_signal_handlers_unblock_by_func (G_OBJECT (config),
                                             gimp_prop_text_buffer_notify,
                                             text_buffer);
        }

      g_free (text);
    }
}

static void
gimp_prop_text_buffer_notify (GObject       *config,
                              GParamSpec    *param_spec,
                              GtkTextBuffer *text_buffer)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (G_OBJECT (text_buffer),
                                   gimp_prop_text_buffer_callback,
                                   config);

  gtk_text_buffer_set_text (text_buffer, value, -1);

  g_signal_handlers_unblock_by_func (G_OBJECT (text_buffer),
                                     gimp_prop_text_buffer_callback,
                                     config);

  g_free (value);
}


/****************/
/*  file entry  */
/****************/


static void   gimp_prop_file_entry_callback (GimpFileSelection *entry,
                                             GObject           *config);
static void   gimp_prop_file_entry_notify   (GObject           *config,
                                             GParamSpec        *param_spec,
                                             GimpFileSelection *entry);

GtkWidget *
gimp_prop_file_entry_new (GObject     *config,
                          const gchar *property_name,
                          const gchar *filesel_title,
                          gboolean     dir_only,
                          gboolean     check_valid)
{
  GParamSpec *param_spec;
  GtkWidget  *entry;
  gchar      *notify_name;
  gchar      *value;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! GIMP_IS_PARAM_SPEC_PATH (param_spec))
    {
      g_warning (G_STRLOC ": property '%s' of %s is not a path",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_get (config,
                property_name, &value,
                NULL);

  entry = gimp_file_selection_new (filesel_title, value, dir_only, check_valid);

  g_free (value);

  g_object_set_data (G_OBJECT (entry), "gimp-config-param-spec", param_spec);

  g_signal_connect (G_OBJECT (entry), "filename_changed",
		    G_CALLBACK (gimp_prop_file_entry_callback),
		    config);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_file_entry_notify),
                           entry, 0);

  g_free (notify_name);

  return entry;
}

static void
gimp_prop_file_entry_callback (GimpFileSelection *entry,
                               GObject           *config)
{
  GParamSpec *param_spec;
  gchar      *value;

  param_spec = g_object_get_data (G_OBJECT (entry), "gimp-config-param-spec");

  if (! param_spec)
    return;

  value = gimp_file_selection_get_filename (entry);

  g_object_set (config,
                param_spec->name, value,
                NULL);

  g_free (value);
}

static void
gimp_prop_file_entry_notify (GObject           *config,
                             GParamSpec        *param_spec,
                             GimpFileSelection *entry)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (G_OBJECT (entry),
                                   gimp_prop_file_entry_callback,
                                   config);

  gimp_file_selection_set_filename (entry, value);

  g_signal_handlers_unblock_by_func (G_OBJECT (entry),
                                     gimp_prop_file_entry_callback,
                                     config);

  g_free (value);
}


/*****************/
/*  path editor  */
/*****************/

static void   gimp_prop_path_editor_callback (GimpPathEditor *editor,
                                              GObject        *config);
static void   gimp_prop_path_editor_notify   (GObject        *config,
                                              GParamSpec     *param_spec,
                                              GimpPathEditor *editor);

GtkWidget *
gimp_prop_path_editor_new (GObject     *config,
                           const gchar *property_name,
                           const gchar *filesel_title)
{
  GParamSpec *param_spec;
  GtkWidget  *editor;
  gchar      *notify_name;
  gchar      *value;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! GIMP_IS_PARAM_SPEC_PATH (param_spec))
    {
      g_warning (G_STRLOC ": property '%s' of %s is not a path",
                 property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  g_object_get (config,
                property_name, &value,
                NULL);

  editor = gimp_path_editor_new (filesel_title, value);

  g_free (value);

  g_object_set_data (G_OBJECT (editor), "gimp-config-param-spec", param_spec);

  g_signal_connect (G_OBJECT (editor), "path_changed",
		    G_CALLBACK (gimp_prop_path_editor_callback),
		    config);

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_path_editor_notify),
                           editor, 0);

  g_free (notify_name);

  return editor;
}

static void
gimp_prop_path_editor_callback (GimpPathEditor *editor,
                                GObject        *config)
{
  GParamSpec *param_spec;
  gchar      *value;

  param_spec = g_object_get_data (G_OBJECT (editor), "gimp-config-param-spec");

  if (! param_spec)
    return;

  value = gimp_path_editor_get_path (editor);

  g_object_set (config,
                param_spec->name, value,
                NULL);

  g_free (value);
}

static void
gimp_prop_path_editor_notify (GObject        *config,
                              GParamSpec     *param_spec,
                              GimpPathEditor *editor)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (G_OBJECT (editor),
                                   gimp_prop_path_editor_callback,
                                   config);

  gimp_path_editor_set_path (editor, value);

  g_signal_handlers_unblock_by_func (G_OBJECT (editor),
                                     gimp_prop_path_editor_callback,
                                     config);

  g_free (value);
}


/*****************/
/*  coordinates  */
/*****************/

static void   gimp_prop_coordinates_callback    (GimpSizeEntry *sizeentry,
                                                 GObject       *config);
static void   gimp_prop_coordinates_notify_x    (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 GimpSizeEntry *sizeentry);
static void   gimp_prop_coordinates_notify_y    (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 GimpSizeEntry *sizeentry);
static void   gimp_prop_coordinates_notify_unit (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 GimpSizeEntry *sizeentry);

GtkWidget *
gimp_prop_coordinates_new (GObject                   *config,
                           const gchar               *x_property_name,
                           const gchar               *y_property_name,
                           const gchar               *unit_property_name,
                           const gchar               *unit_format,
                           GimpSizeEntryUpdatePolicy  update_policy,
                           gdouble                    xresolution,
                           gdouble                    yresolution,
                           gboolean                   has_chainbutton)
{
  GParamSpec *x_param_spec;
  GParamSpec *y_param_spec;
  GParamSpec *unit_param_spec;
  GtkWidget  *sizeentry;
  gdouble     x_value;
  gdouble     y_value;
  GimpUnit    unit_value;
  gchar      *notify_name;
  gboolean    chain_checked = FALSE;

  x_param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               x_property_name);

  if (! x_param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 x_property_name);
      return NULL;
    }

  y_param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               y_property_name);

  if (! y_param_spec)
    {
      g_warning (G_STRLOC ": %s has no property named '%s'",
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 y_property_name);
      return NULL;
    }

  if (unit_property_name)
    {
      unit_param_spec =
        g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                      unit_property_name);

      if (! unit_param_spec)
        {
          g_warning (G_STRLOC ": %s has no property named '%s'",
                     g_type_name (G_TYPE_FROM_INSTANCE (config)),
                     unit_property_name);
          return NULL;
        }

      if (! GIMP_IS_PARAM_SPEC_UNIT (unit_param_spec))
        {
          g_warning (G_STRLOC ": property '%s' of %s is not a unit",
                     unit_property_name,
                     g_type_name (G_TYPE_FROM_INSTANCE (config)));
          return NULL;
        }

      g_object_get (config,
                    unit_property_name, &unit_value,
                    NULL);
    }
  else
    {
      unit_param_spec = NULL;

      unit_value = GIMP_UNIT_INCH;
    }

  if (G_IS_PARAM_SPEC_INT (x_param_spec) &&
      G_IS_PARAM_SPEC_INT (y_param_spec))
    {
      gint int_x;
      gint int_y;

      g_object_get (config,
                    x_property_name, &int_x,
                    y_property_name, &int_y,
                    NULL);

      x_value = int_x;
      y_value = int_y;
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (x_param_spec) &&
           G_IS_PARAM_SPEC_DOUBLE (y_param_spec))
    {
      g_object_get (config,
                    x_property_name, &x_value,
                    y_property_name, &y_value,
                    NULL);
    }
  else
    {
      g_warning (G_STRLOC ": properties '%s' and '%s' of %s are not either "
                 " both int or both double",
                 x_property_name,
                 y_property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  sizeentry = gimp_size_entry_new (2, unit_value, unit_format,
                                   FALSE, FALSE, TRUE, 10,
                                   update_policy);

  switch (update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
                                      xresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 1,
                                      yresolution, FALSE);

      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
                                             GIMP_MIN_IMAGE_SIZE,
                                             GIMP_MAX_IMAGE_SIZE);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
                                             GIMP_MIN_IMAGE_SIZE,
                                             GIMP_MAX_IMAGE_SIZE);

      if (ABS (x_value - y_value) < 1)
        chain_checked = TRUE;
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
                                             GIMP_MIN_RESOLUTION,
                                             GIMP_MAX_RESOLUTION);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
                                             GIMP_MIN_RESOLUTION,
                                             GIMP_MAX_RESOLUTION);

      if (ABS (x_value - y_value) < GIMP_MIN_RESOLUTION)
        chain_checked = TRUE;
      break;

    default:
      break;
    }

  gimp_size_entry_set_unit   (GIMP_SIZE_ENTRY (sizeentry), unit_value);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, x_value);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1, y_value);

  g_object_set_data (G_OBJECT (sizeentry), "gimp-config-param-spec-x",
                     x_param_spec);
  g_object_set_data (G_OBJECT (sizeentry), "gimp-config-param-spec-y",
                     y_param_spec);
  g_object_set_data (G_OBJECT (sizeentry), "gimp-config-param-spec-unit",
                     unit_param_spec);

  if (has_chainbutton)
    {
      GtkWidget *button;

      button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);

      if (chain_checked)
        gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (button), TRUE);

      gtk_table_attach_defaults (GTK_TABLE (sizeentry), button, 1, 3, 3, 4);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (sizeentry), "chainbutton", button);
    }

  g_signal_connect (G_OBJECT (sizeentry), "value_changed",
		    G_CALLBACK (gimp_prop_coordinates_callback),
		    config);
  g_signal_connect (G_OBJECT (sizeentry), "refval_changed",
		    G_CALLBACK (gimp_prop_coordinates_callback),
		    config);
  g_signal_connect (G_OBJECT (sizeentry), "unit_changed",
		    G_CALLBACK (gimp_prop_coordinates_callback),
		    config);

  notify_name = g_strconcat ("notify::", x_property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_coordinates_notify_x),
                           sizeentry, 0);

  g_free (notify_name);

  notify_name = g_strconcat ("notify::", y_property_name, NULL);

  g_signal_connect_object (config, notify_name,
                           G_CALLBACK (gimp_prop_coordinates_notify_y),
                           sizeentry, 0);

  g_free (notify_name);

  if (unit_property_name)
    {
      notify_name = g_strconcat ("notify::", unit_property_name, NULL);

      g_signal_connect_object (config, notify_name,
                               G_CALLBACK (gimp_prop_coordinates_notify_unit),
                               sizeentry, 0);

      g_free (notify_name);
    }

  return sizeentry;
}

static void
gimp_prop_coordinates_callback (GimpSizeEntry *sizeentry,
                                GObject       *config)
{
  GParamSpec *x_param_spec;
  GParamSpec *y_param_spec;
  GParamSpec *unit_param_spec;
  gdouble     x_value;
  gdouble     y_value;
  GimpUnit    unit_value;
  GtkWidget  *chainbutton;

  x_param_spec = g_object_get_data (G_OBJECT (sizeentry),
                                    "gimp-config-param-spec-x");
  y_param_spec = g_object_get_data (G_OBJECT (sizeentry),
                                    "gimp-config-param-spec-y");
  unit_param_spec = g_object_get_data (G_OBJECT (sizeentry),
                                       "gimp-config-param-spec-unit");

  if (! x_param_spec || ! y_param_spec)
    return;

  x_value    = gimp_size_entry_get_refval (sizeentry, 0);
  y_value    = gimp_size_entry_get_refval (sizeentry, 1);
  unit_value = gimp_size_entry_get_unit (sizeentry);

  chainbutton = g_object_get_data (G_OBJECT (sizeentry), "chainbutton");

  if (chainbutton)
    {
      gdouble *old_x_value;
      gdouble *old_y_value;

      old_x_value = g_object_get_data (G_OBJECT (sizeentry), "old-x-value");

      if (! old_x_value)
        {
          old_x_value = g_new0 (gdouble, 1);
          *old_x_value = 0.0;
          g_object_set_data_full (G_OBJECT (sizeentry), "old-x-value",
                                  old_x_value,
                                  (GDestroyNotify) g_free);
        }

      old_y_value = g_object_get_data (G_OBJECT (sizeentry), "old-y-value");

      if (! old_y_value)
        {
          old_y_value = g_new0 (gdouble, 1);
          *old_y_value = 0.0;
          g_object_set_data_full (G_OBJECT (sizeentry), "old-y-value",
                                  old_y_value,
                                  (GDestroyNotify) g_free);
        }

      if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chainbutton)))
        {
          if (x_value != *old_x_value)
            {
              *old_y_value = y_value = *old_x_value = x_value;
              gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1,
                                          y_value);
              return;
            }

          if (y_value != *old_y_value)
            {
              *old_x_value = x_value = *old_y_value = y_value;
              gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0,
                                          x_value);
              return;
            }
        }
      else
        {
          *old_x_value = x_value;
          *old_y_value = y_value;
        }
    }

  if (G_IS_PARAM_SPEC_INT (x_param_spec) &&
      G_IS_PARAM_SPEC_INT (y_param_spec))
    {
      g_object_set (config,
                    x_param_spec->name,    (gint) x_value,
                    y_param_spec->name,    (gint) y_value,

                    unit_param_spec ? unit_param_spec->name : NULL, unit_value,

                    NULL);
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (x_param_spec) &&
           G_IS_PARAM_SPEC_DOUBLE (y_param_spec))
    {
      g_object_set (config,
                    x_param_spec->name,    x_value,
                    y_param_spec->name,    y_value,

                    unit_param_spec ? unit_param_spec->name : NULL, unit_value,

                    NULL);
    }
}

static void
gimp_prop_coordinates_notify_x (GObject       *config,
                                GParamSpec    *param_spec,
                                GimpSizeEntry *sizeentry)
{
  gdouble value;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config,
                    param_spec->name, &int_value,
                    NULL);

      value = int_value;
    }
  else
    {
      g_object_get (config,
                    param_spec->name, &value,
                    NULL);
    }

  if (value != gimp_size_entry_get_refval (sizeentry, 0))
    {
      g_signal_handlers_block_by_func (G_OBJECT (sizeentry),
                                       gimp_prop_coordinates_callback,
                                       config);

      gimp_size_entry_set_refval (sizeentry, 0, value);

      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry),
                                         gimp_prop_coordinates_callback,
                                         config);
    }
}

static void
gimp_prop_coordinates_notify_y (GObject       *config,
                                GParamSpec    *param_spec,
                                GimpSizeEntry *sizeentry)
{
  gdouble value;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config,
                    param_spec->name, &int_value,
                    NULL);

      value = int_value;
    }
  else
    {
      g_object_get (config,
                    param_spec->name, &value,
                    NULL);
    }

  if (value != gimp_size_entry_get_refval (sizeentry, 1))
    {
      g_signal_handlers_block_by_func (G_OBJECT (sizeentry),
                                       gimp_prop_coordinates_callback,
                                       config);

      gimp_size_entry_set_refval (sizeentry, 1, value);

      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry),
                                         gimp_prop_coordinates_callback,
                                         config);
    }
}

static void
gimp_prop_coordinates_notify_unit (GObject       *config,
                                   GParamSpec    *param_spec,
                                   GimpSizeEntry *sizeentry)
{
  GimpUnit value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (value != gimp_size_entry_get_unit (sizeentry))
    {
      g_signal_handlers_block_by_func (G_OBJECT (sizeentry),
                                       gimp_prop_coordinates_callback,
                                       config);

      gimp_size_entry_set_unit (sizeentry, value);

      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry),
                                         gimp_prop_coordinates_callback,
                                         config);
    }
}
