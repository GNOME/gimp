#ifndef __PREVIEW_H
#define __PREVIEW_H

GtkWidget* create_preview (void);
void       updatepreview  (GtkWidget *wg, gpointer d);
void preview_free_resources(void);
void preview_set_button_label(const gchar * text);

#endif /* #ifndef __PREVIEW_H */
