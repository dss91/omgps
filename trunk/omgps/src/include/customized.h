#ifndef CUSTOMIZED_
#define CUSTOMIZED_

#include <gtk/gtk.h>
#include <glib.h>

GtkCellRenderer * clickable_cell_renderer_pixbuf_new();

GtkWidget* hyperlink_label_new(char *text, PangoFontDescription *font_desc,
	gboolean (*on_click)(GtkWidget *widget, GdkEventButton *event, gpointer data),
	gpointer data);

extern void warn_dialog(char *msg);
extern void info_dialog(char *msg);
extern gboolean confirm_dialog(char *msg);

#endif /* CUSTOMIZED_ */
