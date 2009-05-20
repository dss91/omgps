#include <sys/time.h>
#include <dirent.h>

#include "util.h"
#include "omgps.h"
#include "customized.h"

static GtkWidget *color_buttons[BASE_COLOR_COUNT];
static GtkWidget *clear_button, *save_button, *lockview_button;
static int last_color_idx = -1;

static mouse_handler_t mouse_scratch_handler;

#define SCREENSHOT_FILE_EXT		".png"
#define SCREENSHOT_FILE_TYPE	"png"

#define MAX_POINTS	100
#define MAX_IDX		(MAX_POINTS - 1)
static int point_idx = -1;
static point_t points[MAX_POINTS];

static GtkWidget *filelist_treeview, *filelist_treeview_sw;
static GtkListStore *filelist_store = NULL;
static char *filelist_treeview_col_names[] = {"File name", "Last Modified Time"};
static GtkWidget *notebook, *view_button, *delete_button, *screenshot_label, *screenshot_image;
static char *selected_file = NULL;
static GtkTreeIter selected_iter;

static char *get_full_path(char *fullpath, int buf_len, char *file);
static void add_file_to_list(GtkTreeIter *iter, char *filepath, char *filename);

void do_screenshot()
{
	#define SS_PREFIX "Save screenshot: "
	struct stat st;
	char buf[256];

	int i = 1;
	do {
		snprintf(buf, sizeof(buf), "%s/%04d%s", g_context.screenshot_dir, i++, SCREENSHOT_FILE_EXT);
	} while (stat(buf, &st) == 0);

	GError *err = NULL;

	GdkPixbuf *pixbuf = gdk_pixbuf_get_from_drawable(NULL, g_view.da->window, NULL,
		g_view.fglayer.visible.x, g_view.fglayer.visible.y, 0, 0,
		g_view.fglayer.visible.width, g_view.fglayer.visible.height);

	if (pixbuf == NULL) {
		warn_dialog(SS_PREFIX"unable to capture from window");
		return;
	}

	gboolean ret = gdk_pixbuf_save (pixbuf, buf, SCREENSHOT_FILE_TYPE, &err, "tEXt::Software", "omgps", NULL);
	gdk_pixbuf_unref(pixbuf);

	if (ret) {
		char buf1[128];
		snprintf(buf1, sizeof(buf1), SS_PREFIX"saved as: %s", buf);

		GtkTreeIter iter;
		gtk_list_store_insert (filelist_store, &iter, 0);
		add_file_to_list(&iter, buf, &buf[strlen(g_context.screenshot_dir) + 1]);
		info_dialog(buf1);
	} else {
		snprintf(buf, sizeof(buf), SS_PREFIX"unable to save, error=%s", err->message);
		g_error_free (err);
		warn_dialog(buf);
	}
}

static void draw_to_pixmap()
{
	int i, j=0;

	for (i=1; i<=point_idx; j=i, i++) {
		gdk_draw_line (g_view.pixmap, g_context.scratch_gc,
			points[j].x, points[j].y, points[i].x, points[i].y);
	}
}

static void scratch_press(point_t point, guint time)
{
	points[++point_idx] = point;
}

/* the press event handler may not be called before this handler! */
static void scratch_motion(point_t point, guint time)
{
	if(point_idx == -1) /* reversed event */
		return;

	gdk_draw_line (g_view.da->window, g_context.scratch_gc,
		points[point_idx].x, points[point_idx].y, point.x, point.y);

	if (point_idx < MAX_IDX) {
		points[++point_idx] = point;
	} else {
		draw_to_pixmap();
		point_idx = 0;
		points[0] = point;
	}
}

static void scratch_release(point_t point, guint time)
{
	if(point_idx <= 0) {
		point_idx = -1;
		return;
	}

	gdk_draw_line (g_view.da->window, g_context.scratch_gc,
		points[point_idx].x, points[point_idx].y, point.x, point.y);
	draw_to_pixmap();
	point_idx = -1;
}

static void scratch_redraw_view()
{
	gdk_draw_drawable (g_view.da->window, g_context.scratch_gc, g_view.pixmap,
		g_view.fglayer.visible.x, g_view.fglayer.visible.y,
		g_view.fglayer.visible.x, g_view.fglayer.visible.y,
		g_view.fglayer.visible.width, g_view.fglayer.visible.height);
}

static void color_button_toggled(GtkWidget *widget, gpointer data)
{
	if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	if (last_color_idx >= 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(color_buttons[last_color_idx]), FALSE);

	last_color_idx = (int)data;
	/* this color is not allocated, so we call gdk_gc_set_rgb_fg_color() */
	gdk_gc_set_rgb_fg_color(g_context.scratch_gc, &g_base_colors[last_color_idx]);
}

static void clear_button_clicked(GtkWidget *widget, gpointer data)
{
	map_draw_back_layers(g_view.pixmap);
	scratch_redraw_view();
}

static void save_button_clicked(GtkWidget *widget, gpointer data)
{
	do_screenshot();
}

static void lockview_button_toggled(GtkWidget *widget, gpointer data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lockview_button))) {
		map_config_main_view(&mouse_scratch_handler, 0x2|0x4|0x8, TRUE, TRUE);
		map_draw_back_layers(g_view.pixmap);
		map_set_redraw_func(&scratch_redraw_view);
	} else {
		map_set_redraw_func(NULL);
		map_config_main_view(NULL, 0x2|0x4|0x8, FALSE, FALSE);
	}
}

static void close_button_clicked(GtkWidget *widget, gpointer data)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lockview_button), FALSE);

	point_idx = -1;
	switch_to_main_view(CTX_ID_GPS_FIX);
}

/**
 * The GtkColorButton is good, but the pop-up dialog does not shown properly,
 * and arbitrary color makes no sense, so just show several colored buttons instead.
 * It takes no effect to set a button color when it is created and added to container!
 * Reference:
 * (1) http://library.gnome.org/devel/gtk/stable/
 * (2) http://ometer.com/gtk-colors.html
 */
static void render_color_buttons()
{
	int i;
	GtkWidget *button;
	GdkColor *color;

	for (i=0; i<BASE_COLOR_COUNT; i++) {
		color = &g_base_colors[i];
		button = color_buttons[i];
		gtk_widget_show(button);
		gtk_widget_modify_bg(button, GTK_STATE_NORMAL, color);
		gtk_widget_modify_bg(button, GTK_STATE_ACTIVE, color);
		gtk_widget_modify_bg(button, GTK_STATE_SELECTED, color);
		gtk_widget_modify_bg(button, GTK_STATE_PRELIGHT, color);

		/* Help recognize colored buttons when selected/active */
		color = (i==ID_COLOR_Aqua || i==ID_COLOR_Silver||i==ID_COLOR_Lime ||
				i==ID_COLOR_White || i==ID_COLOR_Yellow)?
				&g_base_colors[ID_COLOR_Black] : &g_base_colors[ID_COLOR_White];

		gtk_widget_modify_fg(button, GTK_STATE_NORMAL, color);
		gtk_widget_modify_fg(button, GTK_STATE_ACTIVE, color);
		gtk_widget_modify_fg(button, GTK_STATE_SELECTED, color);
		gtk_widget_modify_fg(button, GTK_STATE_PRELIGHT, color);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(color_buttons[ID_COLOR_Black]), TRUE);
}

void ctx_tab_scratch_on_show()
{
	if (last_color_idx == -1)
		render_color_buttons();

	point_idx = -1;
}

GtkWidget * ctx_tab_scratch_create()
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *title_label = gtk_label_new("");

    gtk_label_set_markup(GTK_LABEL(title_label), "<span weight='bold'>Scratch on map</span>");
    GtkWidget *sep = gtk_hseparator_new();

	gtk_container_add(GTK_CONTAINER (vbox), title_label);
	gtk_container_add(GTK_CONTAINER (vbox), sep);

	/* color buttons */

	GtkWidget *color_buttons_hbox = gtk_hbox_new(TRUE, 2);
	gtk_container_add (GTK_CONTAINER (vbox), color_buttons_hbox);

	int i;
	for (i=0; i<BASE_COLOR_COUNT; i++) {
		color_buttons[i] = (GtkWidget *)gtk_toggle_button_new_with_label("  ");
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(color_buttons[i]), TRUE);
		gtk_container_add (GTK_CONTAINER (color_buttons_hbox), color_buttons[i]);
		g_signal_connect (G_OBJECT (color_buttons[i]), "toggled",
			G_CALLBACK (color_button_toggled), (gpointer)i);
	}

	/* function buttons */

	GtkWidget *hbox = gtk_hbox_new(TRUE, 1);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);

	lockview_button = gtk_toggle_button_new_with_label("lock view");
	g_signal_connect (G_OBJECT (lockview_button), "toggled",
		G_CALLBACK (lockview_button_toggled), NULL);

	save_button = gtk_button_new_with_label("save");
	g_signal_connect (G_OBJECT (save_button), "clicked",
		G_CALLBACK (save_button_clicked), NULL);

	clear_button = gtk_button_new_with_label("clear");
	g_signal_connect (G_OBJECT (clear_button), "clicked",
		G_CALLBACK (clear_button_clicked), NULL);

	GtkWidget *close_button = gtk_button_new_with_label("close");
	g_signal_connect (G_OBJECT (close_button), "clicked",
		G_CALLBACK (close_button_clicked), NULL);

	gtk_container_add (GTK_CONTAINER (hbox), lockview_button);
	gtk_container_add (GTK_CONTAINER (hbox), save_button);
	gtk_container_add (GTK_CONTAINER (hbox), clear_button);
	gtk_container_add (GTK_CONTAINER (hbox), close_button);

	mouse_scratch_handler.press_handler = scratch_press;
	mouse_scratch_handler.release_handler = scratch_release;
	mouse_scratch_handler.motion_handler = scratch_motion;

	return vbox;
}

static void scratch_button_clicked(GtkWidget *widget, gpointer data)
{
	switch_to_main_view(CTX_ID_SCRATCH);
}

void scratch_tab_on_show()
{
	gtk_widget_set_sensitive(view_button, FALSE);
	gtk_widget_set_sensitive(delete_button, FALSE);
}

static void filelist_treeview_row_selected (GtkTreeView *tree_view, gpointer user_data)
{
	selected_file = NULL;

	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(filelist_treeview));
	int count = gtk_tree_selection_count_selected_rows (sel);
	if (count == 0)
		return;

	GtkTreeModel *treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(filelist_treeview));
	GList *rows = gtk_tree_selection_get_selected_rows(sel, &treemodel);
	GtkTreePath *path = (GtkTreePath *)(g_list_first(rows)->data);

	gtk_tree_model_get_iter (treemodel, &selected_iter, path);
	gtk_tree_model_get (treemodel, &selected_iter, 0, &selected_file, -1);
	gtk_tree_path_free(path);

	if (! GTK_WIDGET_SENSITIVE(view_button))
		gtk_widget_set_sensitive(view_button, TRUE);

	if (! GTK_WIDGET_SENSITIVE(delete_button))
		gtk_widget_set_sensitive(delete_button, TRUE);
}

static void view_button_clicked(GtkWidget *widget, gpointer data)
{
	char buf[256];

	if (! selected_file) {
		info_dialog("no file was selected!");
		return;
	}
	get_full_path(buf, sizeof(buf), selected_file);

	gtk_image_set_from_file(GTK_IMAGE(screenshot_image), buf);
	gtk_label_set_text(GTK_LABEL(screenshot_label), buf);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 1);
}

static void delete_button_clicked(GtkWidget *widget, gpointer data)
{
	char buf[256];

	if (! selected_file) {
		info_dialog("no file was selected!");
		return;
	}

	snprintf(buf, sizeof(buf), "Delete file:\n\n%s", selected_file);

	if (confirm_dialog(buf)) {
		gtk_list_store_remove (filelist_store, &selected_iter);
		get_full_path(buf, sizeof(buf), selected_file);
		unlink(buf);

		gtk_widget_set_sensitive(view_button, FALSE);
		gtk_widget_set_sensitive(delete_button, FALSE);
	}
}

static char *get_full_path(char *fullpath, int buf_len, char *file)
{
	if (fullpath == NULL)
		fullpath = (char *)malloc(buf_len);
	snprintf(fullpath, buf_len, "%s/%s", g_context.screenshot_dir, file);
	return fullpath;
}

static void add_file_to_list(GtkTreeIter *iter, char *filepath, char *filename)
{
	char buf[30];
	struct tm *tm;
	time_t tt;

	struct stat st;
	if (stat(filepath, &st) == 0) {
		tt = st.st_mtim.tv_sec;
		tm = localtime(&tt);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
	} else {
		sprintf(buf, "%s", "unkown");
	}

	gtk_list_store_set (filelist_store, iter,
		0, filename,
		1, buf,
		-1);
}

GtkWidget * scratch_tab_create()
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 5);

	/* file list treeview */

	filelist_treeview = gtk_tree_view_new ();
	filelist_treeview_sw = gtk_scrolled_window_new (NULL, NULL);

	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(filelist_treeview));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT(filelist_treeview), "cursor-changed",
		G_CALLBACK (filelist_treeview_row_selected), NULL);

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (filelist_treeview_sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (filelist_treeview_sw),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (filelist_treeview), TRUE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW (filelist_treeview), FALSE);
	gtk_container_add (GTK_CONTAINER (filelist_treeview_sw), filelist_treeview);

	/* add columns to the tree view */
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	int i;
	int col_count = sizeof (filelist_treeview_col_names) / sizeof (char *);

	for (i=0; i<col_count; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (filelist_treeview_col_names[i],
			renderer, "text", i, NULL);
		gtk_tree_view_column_set_clickable(column, FALSE);
		gtk_tree_view_append_column (GTK_TREE_VIEW(filelist_treeview), column);
	}

	filelist_store = gtk_list_store_new (col_count,	G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(filelist_treeview), GTK_TREE_MODEL(filelist_store));


	GtkWidget *hbox = gtk_hbox_new(TRUE, 5);

	GtkWidget *scratch_button = gtk_button_new_with_label("Scratch on map");
	g_signal_connect (G_OBJECT (scratch_button), "clicked",
		G_CALLBACK (scratch_button_clicked), NULL);

	view_button = gtk_button_new_with_label("View");
	g_signal_connect (G_OBJECT(view_button), "clicked",
		G_CALLBACK (view_button_clicked), NULL);

	delete_button = gtk_button_new_with_label("Delete");
	g_signal_connect (G_OBJECT(delete_button), "clicked",
		G_CALLBACK (delete_button_clicked), NULL);

	gtk_container_add(GTK_CONTAINER(hbox), view_button);
	gtk_container_add(GTK_CONTAINER(hbox), delete_button);
	gtk_container_add(GTK_CONTAINER(hbox), scratch_button);

	gtk_box_pack_start(GTK_BOX(vbox), filelist_treeview_sw, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	/* populate files into filelist treeview */

	GtkTreeIter iter;
	struct dirent *ep;
	char buf[256];
	char *file_path = NULL;
	int len, ext_len = strlen(SCREENSHOT_FILE_EXT);
	char *fname;
	struct stat st;

	DIR *dp = opendir (g_context.screenshot_dir);
	if (dp == NULL) {
		if (stat(buf, &st) == 0)
			warn_dialog("unable to list screenshot files");
	} else {
		while ((ep = readdir (dp))) {
			fname = ep->d_name;
			len = strlen(fname);
			if (ep->d_type == DT_REG && len > ext_len &&
				(strncmp(&fname[len-ext_len], SCREENSHOT_FILE_EXT, ext_len) == 0)) {

				gtk_list_store_prepend (filelist_store, &iter);

				file_path = get_full_path(buf, sizeof(buf), fname);
				add_file_to_list(&iter, file_path, fname);
			}
		}
		closedir (dp);
	}

	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

	GtkWidget *label;

	label = gtk_label_new("screen shot list");
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

	GtkWidget *image_box = gtk_vbox_new(FALSE, 0);
	screenshot_label = gtk_label_new("");
	screenshot_image = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(image_box), screenshot_label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(screenshot_label), 0.0, 0.5);

	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_NONE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), screenshot_image);

	/* stretch */
	gtk_box_pack_start(GTK_BOX(image_box), sw, TRUE, TRUE, 0);

	label = gtk_label_new("view screen shot");
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), image_box, label);

	return notebook;
}
