/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenu_model.c
 * Copyright (C) 2023 Jehan
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/gimp.h"

#include "gimpaction.h"
#include "gimpmenumodel.h"
#include "gimpmenushell.h"
#include "gimpuimanager.h"


/**
 * GimpMenuModel:
 *
 * GimpMenuModel implements GMenuModel. We initialize an object from another
 * GMenuModel (usually a GMenu), auto-fill with various data from the
 * GimpAction, when they are not in GAction API, e.g. labels, but action
 * visibility.
 *
 * The object will also synchronize automatically with changes from the actions,
 * but also GimpUIManager for dynamic contents and will trigger an
 * "items-changed" when necessary. This allows for such variant to be used in
 * GTK API which has no knowledge of GimpAction or GimpUIManager enhancements.
 */


enum
{
  PROP_0,
  PROP_MANAGER,
  PROP_MODEL,
  PROP_PATH,
  PROP_IS_SECTION,
};

struct _GimpMenuModelPrivate
{
  GimpUIManager *manager;
  GMenuModel    *model;

  gchar         *path;
  gboolean       is_section;

  GList         *items;
  gint           last_n_items;
};


static void         gimp_menu_model_finalize                 (GObject             *object);
static void         gimp_menu_model_get_property             (GObject             *object,
                                                              guint                property_id,
                                                              GValue              *value,
                                                              GParamSpec          *pspec);
static void         gimp_menu_model_set_property             (GObject             *object,
                                                              guint                property_id,
                                                              const GValue        *value,
                                                              GParamSpec          *pspec);

static GVariant   * gimp_menu_model_get_item_attribute_value (GMenuModel          *model,
                                                              gint                 item_index,
                                                              const gchar         *attribute,
                                                              const GVariantType  *expected_type);
static void         gimp_menu_model_get_item_attributes      (GMenuModel          *model,
                                                              gint                 item_index,
                                                              GHashTable         **attributes);
static GMenuModel * gimp_menu_model_get_item_link            (GMenuModel          *model,
                                                              gint                 item_index,
                                                              const gchar         *link);
static void         gimp_menu_model_get_item_links           (GMenuModel          *model,
                                                              gint                 item_index,
                                                              GHashTable         **links);
static gint         gimp_menu_model_get_n_items              (GMenuModel          *model);
static gboolean     gimp_menu_model_is_mutable               (GMenuModel          *model);

static void         gimp_menu_model_initialize               (GimpMenuModel       *model,
                                                              GMenuModel          *gmodel);
static void         gimp_menu_model_update                   (GimpMenuModel       *model);
static gboolean     gimp_menu_model_handles_subpath          (GimpMenuModel       *model,
                                                              const gchar         *path);

static GMenuItem  * gimp_menu_model_get_item                 (GimpMenuModel       *model,
                                                              gint                 idx);

static void         gimp_menu_model_action_notify_visible    (GimpAction          *action,
                                                              GParamSpec          *pspec,
                                                              GimpMenuModel       *model);
static void         gimp_menu_model_action_notify_label      (GimpAction          *action,
                                                              GParamSpec          *pspec,
                                                              GMenuItem           *item);

static gboolean     gimp_menu_model_ui_added                 (GimpUIManager       *manager,
                                                              const gchar         *path,
                                                              const gchar         *action_name,
                                                              const gchar         *placeholder_key,
                                                              gboolean             top,
                                                              GimpMenuModel       *model);
static gboolean     gimp_menu_model_ui_removed               (GimpUIManager       *manager,
                                                              const gchar         *path,
                                                              const gchar         *action_name,
                                                              GimpMenuModel       *model);


G_DEFINE_TYPE_WITH_CODE (GimpMenuModel, gimp_menu_model, G_TYPE_MENU_MODEL, G_ADD_PRIVATE (GimpMenuModel))

#define parent_class gimp_menu_model_parent_class


/*  Class functions  */

static void
gimp_menu_model_class_init (GimpMenuModelClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GMenuModelClass *model_class  = G_MENU_MODEL_CLASS (klass);

  object_class->finalize     = gimp_menu_model_finalize;
  object_class->get_property = gimp_menu_model_get_property;
  object_class->set_property = gimp_menu_model_set_property;

  model_class->get_item_attribute_value = gimp_menu_model_get_item_attribute_value;
  model_class->get_item_attributes      = gimp_menu_model_get_item_attributes;
  model_class->get_item_link            = gimp_menu_model_get_item_link;
  model_class->get_item_links           = gimp_menu_model_get_item_links;
  model_class->get_n_items              = gimp_menu_model_get_n_items;
  model_class->is_mutable               = gimp_menu_model_is_mutable;

  g_object_class_install_property (object_class, PROP_MANAGER,
                                   g_param_spec_object ("manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        NULL, NULL,
                                                        G_TYPE_MENU_MODEL,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        NULL, NULL, NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_IS_SECTION,
                                   g_param_spec_boolean ("section",
                                                        NULL, NULL, FALSE,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_menu_model_init (GimpMenuModel *model)
{
  model->priv = gimp_menu_model_get_instance_private (model);

  model->priv->items         = NULL;
  model->priv->last_n_items  = 0;
  model->priv->path          = 0;
  model->priv->is_section    = FALSE;
}

static void
gimp_menu_model_finalize (GObject *object)
{
  GimpMenuModel *model = GIMP_MENU_MODEL (object);

  g_clear_object (&model->priv->model);
  g_list_free_full (model->priv->items, g_object_unref);
  g_free (model->priv->path);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gimp_menu_model_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpMenuModel *model = GIMP_MENU_MODEL (object);

  switch (property_id)
    {
    case PROP_MANAGER:
      g_value_set_object (value, model->priv->manager);
      break;
    case PROP_MODEL:
      g_value_set_object (value, model->priv->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_menu_model_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpMenuModel *model = GIMP_MENU_MODEL (object);

  switch (property_id)
    {
    case PROP_MANAGER:
      g_set_weak_pointer (&model->priv->manager, g_value_get_object (value));
      break;
    case PROP_MODEL:
      model->priv->model = g_value_dup_object (value);
      gimp_menu_model_initialize (model, model->priv->model);
      break;
    case PROP_PATH:
      model->priv->path = g_value_dup_string (value);
      break;
    case PROP_IS_SECTION:
      model->priv->is_section = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GVariant*
gimp_menu_model_get_item_attribute_value (GMenuModel         *model,
                                          gint                item_index,
                                          const gchar        *attribute,
                                          const GVariantType *expected_type)
{
  GimpMenuModel *m = GIMP_MENU_MODEL (model);
  GMenuItem     *item;

  item = gimp_menu_model_get_item (m, item_index);

  return g_menu_item_get_attribute_value (item, attribute, expected_type);
}

static void
gimp_menu_model_get_item_attributes (GMenuModel  *model,
                                     gint         item_index,
                                     GHashTable **attributes)
{
  GimpMenuModel *m = GIMP_MENU_MODEL (model);
  GMenuItem     *item;
  GVariant      *value;

  item = gimp_menu_model_get_item (m, item_index);

  *attributes = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                       (GDestroyNotify) g_variant_unref);

  value = g_menu_item_get_attribute_value (item, G_MENU_ATTRIBUTE_LABEL, NULL);
  if (value)
    g_hash_table_insert (*attributes, G_MENU_ATTRIBUTE_LABEL, value);

  value = g_menu_item_get_attribute_value (item, G_MENU_ATTRIBUTE_ACTION, NULL);
  if (value)
    g_hash_table_insert (*attributes, G_MENU_ATTRIBUTE_ACTION, value);

  value = g_menu_item_get_attribute_value (item, G_MENU_ATTRIBUTE_ICON, NULL);
  if (value)
    g_hash_table_insert (*attributes, G_MENU_ATTRIBUTE_ICON, value);

  value = g_menu_item_get_attribute_value (item, G_MENU_LINK_SUBMENU, NULL);
  if (value)
    g_hash_table_insert (*attributes, G_MENU_LINK_SUBMENU, value);

  value = g_menu_item_get_attribute_value (item, G_MENU_LINK_SECTION, NULL);
  if (value)
    g_hash_table_insert (*attributes, G_MENU_LINK_SECTION, value);
}

static GMenuModel*
gimp_menu_model_get_item_link (GMenuModel  *model,
                               gint         item_index,
                               const gchar *link)
{
  GimpMenuModel *m = GIMP_MENU_MODEL (model);
  GMenuItem     *item;

  item = gimp_menu_model_get_item (m, item_index);

  return g_menu_item_get_link (item, link);
}


static void
gimp_menu_model_get_item_links (GMenuModel  *model,
                                gint         item_index,
                                GHashTable **links)
{
  GimpMenuModel *m = GIMP_MENU_MODEL (model);
  GMenuModel    *subsection;
  GMenuModel    *submenu;
  GMenuItem     *item;

  *links = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                  (GDestroyNotify) g_object_unref);

  item       = gimp_menu_model_get_item (m, item_index);
  subsection = g_menu_item_get_link (item, G_MENU_LINK_SECTION);
  submenu    = g_menu_item_get_link (item, G_MENU_LINK_SUBMENU);

  if (subsection)
    g_hash_table_insert (*links, G_MENU_LINK_SECTION, g_object_ref (subsection));
  if (submenu)
    g_hash_table_insert (*links, G_MENU_LINK_SUBMENU, g_object_ref (submenu));

  g_clear_object (&subsection);
  g_clear_object (&submenu);
}

static gint
gimp_menu_model_get_n_items (GMenuModel *model)
{
  GimpMenuModel *m   = GIMP_MENU_MODEL (model);
  GApplication  *app = m->priv->manager->gimp->app;
  gint           len = 0;

  for (GList *iter = m->priv->items; iter; iter = iter->next)
    {
      const gchar *action_name = NULL;
      GMenuModel  *subsection;
      GMenuModel  *submenu;

      subsection = g_menu_item_get_link (iter->data, G_MENU_LINK_SECTION);
      submenu    = g_menu_item_get_link (iter->data, G_MENU_LINK_SUBMENU);

      if (subsection || submenu)
        {
          len++;
        }
      /* Count neither placeholders (items with no action_name), nor invisible
       * actions.
       */
      else if (g_menu_item_get_attribute (iter->data,
                                          G_MENU_ATTRIBUTE_ACTION,
                                          "&s", &action_name))
        {
          GAction *action;

          action = g_action_map_lookup_action (G_ACTION_MAP (app),
                                               action_name + 4);
          if (gimp_action_is_visible (GIMP_ACTION (action)))
            len++;
        }
      g_clear_object (&subsection);
      g_clear_object (&submenu);
    }

  m->priv->last_n_items = len;

  return len;
}

static gboolean
gimp_menu_model_is_mutable (GMenuModel* model)
{
  return TRUE;
}


/* Public functions. */

GimpMenuModel *
gimp_menu_model_new (GimpUIManager *manager,
                     GMenuModel    *model)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);

  return g_object_new (GIMP_TYPE_MENU_MODEL,
                       "manager", manager,
                       "model",   model,
                       NULL);
}

GimpMenuModel *
gimp_menu_model_get_submodel (GimpMenuModel *model,
                              const gchar   *path)
{
  GimpMenuModel *submodel;
  gchar         *submenus;
  gchar         *submenu;
  gchar         *subsubmenus;

  submodel = g_object_ref (model);

  if (path == NULL)
    return submodel;

  submenus    = g_strdup (path);
  subsubmenus = submenus;

  while (subsubmenus && strlen (subsubmenus) > 0)
    {
      gint n_items;
      gint i;

      submenu = subsubmenus;
      while (*submenu == '/')
        submenu++;

      subsubmenus = strstr (submenu, "/");
      if (subsubmenus)
        *(subsubmenus++) = '\0';

      if (strlen (submenu) == 0)
        break;

      n_items = g_menu_model_get_n_items (G_MENU_MODEL (submodel));
      for (i = 0; i < n_items; i++)
        {
          GMenuModel *subsubmodel;
          gchar      *label = NULL;
          gchar      *canon_label = NULL;

          subsubmodel = g_menu_model_get_item_link (G_MENU_MODEL (submodel), i, G_MENU_LINK_SUBMENU);
          g_menu_model_get_item_attribute (G_MENU_MODEL (submodel), i, G_MENU_ATTRIBUTE_LABEL, "s", &label);
          if (label)
            canon_label = gimp_menu_shell_make_canonical_path (label);

          if (subsubmodel && g_strcmp0 (canon_label, submenu) == 0)
            {
              g_object_unref (submodel);
              submodel = GIMP_MENU_MODEL (subsubmodel);

              g_free (label);
              g_free (canon_label);
              break;
            }
          g_clear_object (&subsubmodel);
          g_free (label);
          g_free (canon_label);
        }
      g_return_val_if_fail (i < n_items, NULL);
    }

  g_free (submenus);

  return submodel;
}

const gchar *
gimp_menu_model_get_path (GimpMenuModel *model)
{
  return model->priv->path;
}


/* Private functions. */

static GimpMenuModel *
gimp_menu_model_new_section (GimpUIManager *manager,
                             GMenuModel    *model,
                             const gchar   *path)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);

  return g_object_new (GIMP_TYPE_MENU_MODEL,
                       "manager", manager,
                       "model",   model,
                       "path",    path,
                       "section", TRUE,
                       NULL);
}

static GimpMenuModel *
gimp_menu_model_new_submenu (GimpUIManager *manager,
                             GMenuModel    *model,
                             const gchar   *path)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);

  return g_object_new (GIMP_TYPE_MENU_MODEL,
                       "manager", manager,
                       "model",   model,
                       "path",    path,
                       NULL);
}

static void
gimp_menu_model_initialize (GimpMenuModel *model,
                            GMenuModel    *gmodel)
{
  GApplication *app = model->priv->manager->gimp->app;
  gint          n_items;

  g_return_if_fail (GIMP_IS_MENU_MODEL (model));
  g_return_if_fail (gmodel == NULL || G_IS_MENU_MODEL (gmodel));

  n_items = gmodel != NULL ? g_menu_model_get_n_items (gmodel) : 0;
  for (int i = 0; i < n_items; i++)
    {
      GMenuModel *subsection;
      GMenuModel *submenu;
      gchar      *label       = NULL;
      gchar      *action_name = NULL;
      GMenuItem  *item        = NULL;

      subsection = g_menu_model_get_item_link (G_MENU_MODEL (gmodel), i, G_MENU_LINK_SECTION);
      submenu    = g_menu_model_get_item_link (G_MENU_MODEL (gmodel), i, G_MENU_LINK_SUBMENU);
      g_menu_model_get_item_attribute (G_MENU_MODEL (gmodel), i,
                                       G_MENU_ATTRIBUTE_LABEL, "s", &label);
      g_menu_model_get_item_attribute (G_MENU_MODEL (gmodel), i,
                                       G_MENU_ATTRIBUTE_ACTION, "s", &action_name);

      if (subsection != NULL)
        {
          GimpMenuModel *submodel;

          submodel = gimp_menu_model_new_section (model->priv->manager, subsection,
                                                  model->priv->path);
          item = g_menu_item_new_section (label, G_MENU_MODEL (submodel));
          g_object_unref (submodel);
        }
      else if (submenu != NULL)
        {
          GimpMenuModel *submodel;
          gchar         *canon_label;
          gchar         *path;

          g_return_if_fail (label != NULL);
          canon_label = gimp_menu_shell_make_canonical_path (label);
          path = g_strdup_printf ("%s/%s",
                                  model->priv->path ? model->priv->path : "",
                                  canon_label);
          g_free (canon_label);

          submodel = gimp_menu_model_new_submenu (model->priv->manager, submenu, path);
          item     = g_menu_item_new_submenu (label, G_MENU_MODEL (submodel));

          g_object_unref (submodel);
          g_free (path);
        }
      else
        {
          item = g_menu_item_new_from_model (G_MENU_MODEL (gmodel), i);

          if (action_name)
            {
              GAction *action;

              action = g_action_map_lookup_action (G_ACTION_MAP (app),
                                                   action_name + 4);
              g_signal_connect_object (action,
                                       "notify::visible",
                                       G_CALLBACK (gimp_menu_model_action_notify_visible),
                                       model, 0);

              g_menu_item_set_label (item, gimp_action_get_label (GIMP_ACTION (action)));
              g_signal_connect_object (action,
                                       "notify::label",
                                       G_CALLBACK (gimp_menu_model_action_notify_label),
                                       item, 0);
            }
            /* else we instal a placeholder (no-action and always invisible) item. */
        }

      if (item)
        model->priv->items = g_list_append (model->priv->items, item);

      g_free (label);
      g_free (action_name);
      g_clear_object (&subsection);
      g_clear_object (&submenu);
    }

  if (! model->priv->is_section)
    {
      g_signal_connect_object (model->priv->manager, "ui-added",
                               G_CALLBACK (gimp_menu_model_ui_added),
                               model, 0);
      g_signal_connect_object (model->priv->manager, "ui-removed",
                               G_CALLBACK (gimp_menu_model_ui_removed),
                               model, 0);
      gimp_ui_manager_foreach_ui (model->priv->manager,
                                  (GimpUIMenuCallback) gimp_menu_model_ui_added,
                                  model);
    }
}

static void
gimp_menu_model_update (GimpMenuModel *model)
{
  gint last_n_items;
  gint n_items;

  g_return_if_fail (GIMP_IS_MENU_MODEL (model));

  last_n_items = model->priv->last_n_items;
  n_items      = gimp_menu_model_get_n_items (G_MENU_MODEL (model));

  /* A bit "lazy" but should make sure we don't mess up. */
  g_menu_model_items_changed (G_MENU_MODEL (model), 0, last_n_items, n_items);
}

static gboolean
gimp_menu_model_handles_subpath (GimpMenuModel *model,
                                 const gchar   *path)
{
  if (model->priv->path == NULL || ! g_str_has_prefix (path, model->priv->path))
    return FALSE;

  for (GList *iter = model->priv->items; iter; iter = iter->next)
    {
      GMenuModel  *submenu    = NULL;
      GMenuModel  *subsection = NULL;
      const gchar *label = NULL;
      GMenuItem   *item = iter->data;

      submenu    = g_menu_item_get_link (item, G_MENU_LINK_SUBMENU);
      subsection = g_menu_item_get_link (item, G_MENU_LINK_SECTION);
      g_menu_item_get_attribute (item, G_MENU_ATTRIBUTE_LABEL, "s", &label);

      if (subsection != NULL)
        {
          /* Checking if a subsection is a sub-path only gives a partial view of
           * the whole menu. So we only handle the negative result (which means
           * we found a submenu model which will handle this path instead).
           */
          if (! gimp_menu_model_handles_subpath (GIMP_MENU_MODEL (subsection), path))
            return FALSE;
        }
      else if (submenu != NULL)
        {
          gchar *subpath;

          g_return_val_if_fail (label != NULL, FALSE);
          subpath = g_strdup_printf ("%s/", GIMP_MENU_MODEL (submenu)->priv->path);
          if (g_strcmp0 (path, GIMP_MENU_MODEL (submenu)->priv->path) == 0 ||
              g_str_has_prefix (path, subpath))
            {
              /* A submodel will handle the new path. */
              g_free (subpath);
              g_clear_object (&submenu);

              return FALSE;
            }
          g_free (subpath);
        }
      g_clear_object (&submenu);
    }

  /* This is a subpath with submenus to be created! */
  return TRUE;
}

static GMenuItem *
gimp_menu_model_get_item (GimpMenuModel *model,
                          gint           idx)
{
  GimpMenuModel *m   = GIMP_MENU_MODEL (model);
  GApplication  *app = m->priv->manager->gimp->app;
  gint           cur = -1;

  for (GList *iter = m->priv->items; iter; iter = iter->next)
    {
      const gchar *action_name = NULL;
      GMenuModel  *subsection;
      GMenuModel  *submenu;

      subsection = g_menu_item_get_link (iter->data, G_MENU_LINK_SECTION);
      submenu    = g_menu_item_get_link (iter->data, G_MENU_LINK_SUBMENU);

      if (subsection || submenu)
        {
          cur++;
        }
      /* Count neither placeholders (items with no action_name), nor invisible
       * actions.
       */
      else if (g_menu_item_get_attribute (iter->data,
                                          G_MENU_ATTRIBUTE_ACTION,
                                          "&s", &action_name))
        {
          GAction *action;

          action = g_action_map_lookup_action (G_ACTION_MAP (app),
                                               action_name + 4);
          if (gimp_action_is_visible (GIMP_ACTION (action)))
            cur++;
        }

      g_clear_object (&subsection);
      g_clear_object (&submenu);

      if (cur == idx)
        return iter->data;
    }

  return NULL;
}

static void
gimp_menu_model_action_notify_visible (GimpAction    *action,
                                       GParamSpec    *pspec,
                                       GimpMenuModel *model)
{
  gimp_menu_model_update (model);
}

static void
gimp_menu_model_action_notify_label (GimpAction *action,
                                     GParamSpec *pspec,
                                     GMenuItem  *item)
{
  g_return_if_fail (GIMP_IS_ACTION (action));
  g_return_if_fail (G_IS_MENU_ITEM (item));

  g_menu_item_set_label (item, gimp_action_get_label (action));
}

static gboolean
gimp_menu_model_ui_added (GimpUIManager *manager,
                          const gchar   *path,
                          const gchar   *action_name,
                          const gchar   *placeholder_key,
                          gboolean       top,
                          GimpMenuModel *model)
{
  gboolean added = FALSE;

  if (g_strcmp0 (path, model->priv->path) == 0)
    {
      GApplication *app = model->priv->manager->gimp->app;
      GAction      *action;
      gchar        *detailed_action_name;
      GMenuItem    *item;

      action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name);

      g_return_val_if_fail (action != NULL, FALSE);

      added = TRUE;

      g_signal_handlers_disconnect_by_func (action,
                                            G_CALLBACK (gimp_menu_model_action_notify_visible),
                                            model);
      detailed_action_name = g_strdup_printf ("app.%s", g_action_get_name (action));
      item = g_menu_item_new (gimp_action_get_label (GIMP_ACTION (action)), detailed_action_name);
      /* TODO: add also G_MENU_ATTRIBUTE_ICON attribute? */
      g_free (detailed_action_name);

      if (placeholder_key)
        {
          GList      *placeholder = NULL;
          GMenuModel *subsection  = NULL;

          for (GList *iter = model->priv->items; iter; iter = iter->next)
            {
              const gchar *label;

              subsection = g_menu_item_get_link (iter->data, G_MENU_LINK_SECTION);

              if (subsection != NULL)
                {
                  if (gimp_menu_model_ui_added (manager, path, action_name,
                                                placeholder_key, top,
                                                GIMP_MENU_MODEL (subsection)))
                    break;
                  else
                    g_clear_object (&subsection);
                }
              else if (g_menu_item_get_attribute (iter->data,
                                                  G_MENU_ATTRIBUTE_LABEL,
                                                  "&s", &label) &&
                       g_strcmp0 (label, placeholder_key) == 0)
                {
                  placeholder = iter;
                  break;
                }
            }

          if (placeholder)
            {
              if (top)
                model->priv->items = g_list_insert_before (model->priv->items,
                                                           placeholder, item);
              else
                model->priv->items = g_list_insert_before (model->priv->items,
                                                           placeholder->next, item);
            }
          else
            {
              added = FALSE;

              if (subsection == NULL && ! model->priv->is_section)
                g_warning ("%s: no placeholder item '%s'.", G_STRFUNC, placeholder_key);
            }
          /* else: added in a subsection. */

          g_clear_object (&subsection);
        }
      else if (top)
        {
          model->priv->items = g_list_prepend (model->priv->items, item);
        }
      else
        {
          model->priv->items = g_list_append (model->priv->items, item);
        }

      g_signal_connect_object (action,
                               "notify::visible",
                               G_CALLBACK (gimp_menu_model_action_notify_visible),
                               model, 0);
      g_signal_connect_object (action,
                               "notify::label",
                               G_CALLBACK (gimp_menu_model_action_notify_label),
                               item, 0);

      gimp_menu_model_update (model);
    }
  else if (gimp_menu_model_handles_subpath (model, path))
    {
      gchar *new_dirs;
      gchar *new_dir;

      new_dirs = g_strdup (path + strlen (model->priv->path) + 1);
      new_dir  = new_dirs;

      while (new_dir != NULL)
        {
          GimpMenuModel *submodel;
          GMenuItem     *item;
          gchar         *canon_label;
          gchar         *new_path;
          gchar         *next_dir;

          while (*new_dir == '/')
            new_dir++;

          if (*new_dir == '\0')
            break;

          next_dir = strstr (new_dir, "/");
          if (next_dir)
            *(next_dir++) = '\0';

          canon_label = gimp_menu_shell_make_canonical_path (new_dir);
          new_path = g_strdup_printf ("%s/%s",
                                      model->priv->path ? model->priv->path : "",
                                      canon_label);

          submodel = gimp_menu_model_new_submenu (model->priv->manager, NULL, new_path);
          item     = g_menu_item_new_submenu (new_dir, G_MENU_MODEL (submodel));

          model->priv->items = g_list_append (model->priv->items, item);

          g_free (canon_label);
          g_object_unref (submodel);
          g_free (new_path);

          new_dir = next_dir;
        }

      g_free (new_dirs);
    }

  return added;
}

static gboolean
gimp_menu_model_ui_removed (GimpUIManager *manager,
                            const gchar   *path,
                            const gchar   *action_name,
                            GimpMenuModel *model)
{
  gboolean removed = FALSE;

  if (g_strcmp0 (path, model->priv->path) == 0)
    {
      GApplication *app         = model->priv->manager->gimp->app;
      GMenuItem    *item        = NULL;
      GMenuModel   *subsection  = NULL;
      GAction      *action;

      action = g_action_map_lookup_action (G_ACTION_MAP (app), action_name);

      g_return_val_if_fail (action != NULL, FALSE);

      removed = TRUE;

      for (GList *iter = model->priv->items; iter; iter = iter->next)
        {
          const gchar *action;

          subsection = g_menu_item_get_link (iter->data, G_MENU_LINK_SECTION);

          if (subsection != NULL)
            {
              if (gimp_menu_model_ui_removed (manager, path, action_name,
                                              GIMP_MENU_MODEL (subsection)))
                break;
              else
                g_clear_object (&subsection);
            }
          else if (g_menu_item_get_attribute (iter->data,
                                              G_MENU_ATTRIBUTE_ACTION,
                                              "&s", &action) &&
                   /* "action" attribute will start with "app." prefix. */
                   g_strcmp0 (action + 4, action_name) == 0)
            {
              item = iter->data;
              model->priv->items = g_list_delete_link (model->priv->items, iter);
              break;
            }
        }

      if (item)
        {
          g_signal_handlers_disconnect_by_func (action,
                                                G_CALLBACK (gimp_menu_model_action_notify_visible),
                                                model);
          g_signal_handlers_disconnect_by_func (action,
                                                G_CALLBACK (gimp_menu_model_action_notify_label),
                                                item);
          g_object_unref (item);
        }
      else
        {
          removed = FALSE;

          if (subsection == NULL && ! model->priv->is_section)
            g_warning ("%s: no item for action name '%s'.", G_STRFUNC, action_name);
        }
      /* else: removed in a subsection. */

      g_clear_object (&subsection);

      gimp_menu_model_update (model);
    }

  return removed;
}
