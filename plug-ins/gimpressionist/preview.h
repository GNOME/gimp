#ifndef __PREVIEW_H
#define __PREVIEW_H

GtkWidget* create_preview (void);
void       updatepreview  (GtkWidget *wg, gpointer d);
void preview_free_resources(void);

#endif /* #ifndef __PREVIEW_H */
