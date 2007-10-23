#ifndef __PRESETS_H
#define __PRESETS_H

enum SELECT_PRESET_RETURN_VALUES
{
    SELECT_PRESET_OK = 0,
    SELECT_PRESET_FILE_NOT_FOUND = -1,
    SELECT_PRESET_LOAD_FAILED = -2,
};

void create_presetpage (GtkNotebook *);
int select_preset (const gchar *preset);
void preset_free (void);
void preset_save_button_set_sensitive (gboolean s);

#endif

