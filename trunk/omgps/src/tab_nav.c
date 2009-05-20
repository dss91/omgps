#include "omgps.h"
#include "gps.h"

static GtkWidget *ubx_svinfo_treeview, *ubx_svinfo_treeview_sw;
static GtkListStore *ubx_svinfo_store = NULL;
static char *ubx_svinfo_col_names[] = {"SVID", "USED", "DCD", "EPH", "ALM", "CNO(dbHZ)"};

static GtkWidget *basicinfo_table, *ogpsd_svinfo_treeview, *ogpsd_svinfo_treeview_sw;
static GtkListStore *ogpsd_svinfo_store = NULL;
static char *ogpsd_svinfo_col_names[] = {"SVID", "USED", "Elevation(°)", "Azimuth(°)", "CNO(dbHZ)"};

static GtkWidget *time_label;

/* It seems only first 4 are used actually */
static char *fixtype_text[] = { "No fix", "Dead reckoning only", "2D", "3D",
	"GPS + dead reckoning",	"Time only fix"};

typedef enum
{
	FIX_TYPE,
	LAT,
	LON,
	HACC,
	HEIGHT,
	SPEED2D,
	HEADING2D,
	VELDOWN,
	ROW_COUNT
} ROW_IDX;

static char *nav_basic_names[] = {
	 " Fix Type",
	 " Latitude",
	 " Longitude",
	 " HACC",
	 " Altitude",
	 " Ground Speed",
	 " Ground Heading",
	 " Down Velocity",
};

static char *nav_basic_units[] = {
	 "",
	 "°",
	 "°",
	 "m",
	 "m",
	 "m/s ",
	 "°",
	 "m/s ",
};

static GtkWidget *labels[ROW_COUNT];

static char labels_text[ROW_COUNT][64];

static U4 sv_states[SV_MAX_CHANNELS];

/**
 * Update UI waste too much CPU cycles -- up to 50% usage!
 * We compare cached signature with new calculated to avoid unnecessary redraw.
 */
static inline void update_svinfo()
{
	GtkTreeIter iter;
	int i, j, k;
	U1 flags;
	svinfo_channel_t *sv;
	int channel_ids[SV_MAX_CHANNELS];

	k = g_gpsdata.sv_channel_count - 1;
	j = 0;

	for (i=k; i>=0; i--) {
		sv = &g_gpsdata.sv_channels[i];
		if (sv->cno > 0)
			channel_ids[j++] = i;
		else
			channel_ids[k--] = i;
	}

	if (memcmp(sv_states, g_gpsdata.sv_states, sizeof(sv_states)) == 0)
		return;

	memcpy(sv_states, g_gpsdata.sv_states, sizeof(sv_states));

	if (! POLL_ENGINE_TEST(OGPSD)) {
		gtk_list_store_clear(ubx_svinfo_store);

		for (i=0; i<g_gpsdata.sv_channel_count; i++) {
			sv = &g_gpsdata.sv_channels[channel_ids[i]];
			flags = sv->flags;
			gtk_list_store_append (ubx_svinfo_store, &iter);
			gtk_list_store_set (ubx_svinfo_store, &iter,
				0, (int)sv->sv_id,
				1, (flags & 0x01)? "*" : " ",
				2, (flags & 0x02)? "*" : " ",
				3, (flags & 0x04 && (flags & 0x08))? "*" : " ",
				4, (flags & 0x04)? "*" : " ",
				5, sv->cno,
				-1);
		}
	} else {
		gtk_list_store_clear(ogpsd_svinfo_store);

		for (i=0; i<g_gpsdata.sv_channel_count; i++) {
			sv = &g_gpsdata.sv_channels[channel_ids[i]];
			gtk_list_store_append (ogpsd_svinfo_store, &iter);
			gtk_list_store_set (ogpsd_svinfo_store, &iter,
				0, (int)sv->sv_id,
				1, (sv->flags == 0x01)? "*" : " ",
				2, sv->elevation,
				3, sv->azimuth,
				4, sv->cno,
				-1);
		}
	}
}

/* Waste CPU! */
static void svinfo_treeview_func_text(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *model,
	GtkTreeIter *iter, gpointer data)
{
	char *value;
	gtk_tree_model_get(model, iter, 1, &value, -1);
	if (value && strcmp(value, "*") == 0)
		g_object_set(cell, "foreground", "Blue", "foreground-set", TRUE, NULL);
	else
		g_object_set(cell, "foreground-set", FALSE, NULL);
}

static void add_treeview(GtkWidget *vbox, GtkWidget *treeview, GtkWidget *treeview_sw, char *col_names[],
		int col_names_count)
{
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (treeview_sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (treeview_sw),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW (treeview), FALSE);
	gtk_container_add (GTK_CONTAINER (treeview_sw), treeview);

	/* add columns to the tree view */
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	int i;
	for (i=0; i<col_names_count; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (col_names[i], renderer, "text", i, NULL);
		gtk_tree_view_column_set_clickable(column, FALSE);
		gtk_tree_view_column_set_cell_data_func (column, renderer, svinfo_treeview_func_text, NULL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	}

	gtk_box_pack_start (GTK_BOX (vbox), treeview_sw, TRUE, TRUE, 5);
}

/**
 * GtkTreeview is not used here, because it's poor performance according to frequently updates.
 */
static GtkWidget * nav_basic_create_table()
{
	int i;
	GtkWidget *hbox, *label, *event_box, *vbox_1, *vbox_2, *vbox_3;
	GdkColor *color_1 = &g_base_colors[ID_COLOR_White];
	GdkColor color;
	gdk_color_parse("#F0F0F0", &color);
	GdkColor *color_2 = &color;

	hbox = gtk_hbox_new(FALSE, 1);
	vbox_1 = gtk_vbox_new(FALSE, 1);
	vbox_2 = gtk_vbox_new(FALSE, 1);
	vbox_3 = gtk_vbox_new(FALSE, 1);

	gtk_box_pack_start(GTK_BOX(hbox), vbox_1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_2, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), vbox_3, FALSE, FALSE, 0);

	for (i = 0; i<ROW_COUNT; i++) {
		/* first column */
		label = gtk_label_new(nav_basic_names[i]);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_FILL);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_misc_set_padding(GTK_MISC(label), 3, 3);

		event_box = gtk_event_box_new();
		gtk_widget_modify_bg(event_box, GTK_STATE_NORMAL, (i % 2 == 0)? color_1 : color_2);

		gtk_container_add(GTK_CONTAINER(event_box), label);
		gtk_container_add(GTK_CONTAINER(vbox_1), event_box);

		/* second column */

		labels[i] = gtk_label_new("");
		gtk_label_set_use_markup(GTK_LABEL(labels[i]), FALSE);
		gtk_misc_set_alignment(GTK_MISC(labels[i]), 0.0, 0.5);
		gtk_misc_set_padding(GTK_MISC(labels[i]), 3, 3);

		event_box = gtk_event_box_new();
		gtk_widget_modify_bg(event_box, GTK_STATE_NORMAL, (i % 2 == 0)? color_1 : color_2);

		gtk_container_add(GTK_CONTAINER(event_box), labels[i]);
		gtk_container_add(GTK_CONTAINER(vbox_2), event_box);

		/* third column */

		label = gtk_label_new(nav_basic_units[i]);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_misc_set_padding(GTK_MISC(label), 3, 3);

		event_box = gtk_event_box_new();
		gtk_widget_modify_bg(event_box, GTK_STATE_NORMAL, (i % 2 == 0)? color_1 : color_2);

		gtk_container_add(GTK_CONTAINER(event_box), label);
		gtk_container_add(GTK_CONTAINER(vbox_3), event_box);
	}

	return hbox;
}

static inline void update_nav_basic()
{
	int i;

	for (i = 0; i<ROW_COUNT; i++)
		labels_text[i][0] = '\0';

	sprintf(labels_text[FIX_TYPE], "%s", fixtype_text[g_gpsdata.nav_status_fixtype]);

	if (g_gpsdata.latlon_valid) {
		sprintf(labels_text[LAT], "%lf (%c)", fabs(g_gpsdata.lat), (g_gpsdata.lat > 0? 'N' : 'S'));
		sprintf(labels_text[LON], "%lf (%c)", fabs(g_gpsdata.lon), (g_gpsdata.lon > 0? 'E' : 'W'));
		sprintf(labels_text[HACC], "%.2f", g_gpsdata.hacc);
	}

	if (g_gpsdata.height_valid) {
		sprintf(labels_text[HEIGHT], "%.2f ± %.2f", g_gpsdata.height, g_gpsdata.vacc);
	}

	if (g_gpsdata.vel_valid) {
		sprintf(labels_text[SPEED2D], "%.2f", g_gpsdata.speed_2d);
		sprintf(labels_text[HEADING2D], "%.1f", g_gpsdata.heading_2d);
		sprintf(labels_text[VELDOWN], "%.2f", g_gpsdata.vel_down);
	}

	for (i = 0; i<ROW_COUNT; i++) {
		gtk_label_set_text(GTK_LABEL(labels[i]), labels_text[i]);
	}

	char buf[64];
	time_t tm;
	time(&tm);
	struct tm *t = localtime(&tm);
	strftime(buf, sizeof(buf), " (Last update time=%H:%M:%S)", t);
	gtk_label_set_text(GTK_LABEL(time_label), buf);
}

void update_nav_tab()
{
	update_nav_basic();
	update_svinfo();
}

void nav_tab_on_show()
{
	if (POLL_STATE_TEST(SUSPENDING)) {
		gtk_widget_hide(time_label);
		gtk_widget_hide(basicinfo_table);
		gtk_widget_hide(ubx_svinfo_treeview_sw);
		gtk_widget_hide(ogpsd_svinfo_treeview_sw);
	} else if (POLL_STATE_TEST(RUNNING)) {
		gtk_widget_show(basicinfo_table);
		gtk_widget_show(time_label);
		if (POLL_ENGINE_TEST(OGPSD)) {
			gtk_widget_hide(ubx_svinfo_treeview_sw);
			gtk_widget_show(ogpsd_svinfo_treeview_sw);
		} else {
			gtk_widget_show(ubx_svinfo_treeview_sw);
			gtk_widget_hide(ogpsd_svinfo_treeview_sw);
		}
	}
}

GtkWidget * nav_tab_create()
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

	/* basic info label */
	time_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(time_label), 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), time_label, FALSE, FALSE, 1);

	/* basic table */
	basicinfo_table = nav_basic_create_table();

	gtk_box_pack_start (GTK_BOX (vbox), basicinfo_table, FALSE, FALSE, 5);

	GtkTreeSelection *sel;

	/* svinfo treeviews */
	ubx_svinfo_treeview = gtk_tree_view_new ();
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ubx_svinfo_treeview));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_NONE);
	ubx_svinfo_treeview_sw = gtk_scrolled_window_new (NULL, NULL);
	add_treeview(vbox, ubx_svinfo_treeview, ubx_svinfo_treeview_sw, ubx_svinfo_col_names,
			sizeof(ubx_svinfo_col_names)/sizeof(char *));

	ubx_svinfo_store = gtk_list_store_new (sizeof(ubx_svinfo_col_names)/sizeof(char*),
		G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_UINT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ubx_svinfo_treeview), GTK_TREE_MODEL(ubx_svinfo_store));

	ogpsd_svinfo_treeview = gtk_tree_view_new ();
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ogpsd_svinfo_treeview));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_NONE);
	ogpsd_svinfo_treeview_sw = gtk_scrolled_window_new (NULL, NULL);
	add_treeview(vbox, ogpsd_svinfo_treeview, ogpsd_svinfo_treeview_sw, ogpsd_svinfo_col_names,
			sizeof(ogpsd_svinfo_col_names)/sizeof(char *));

	ogpsd_svinfo_store = gtk_list_store_new (sizeof(ogpsd_svinfo_col_names)/sizeof(char*),
		G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ogpsd_svinfo_treeview), GTK_TREE_MODEL(ogpsd_svinfo_store));

	memset(sv_states, 0, sizeof(sv_states));

	return vbox;
}
