#include "omgps.h"
#include "xpm_image.h"
#include "sound.h"
#include "track.h"

#define NAV_DA_H_OFF	0
#define NAV_DA_SPD_OFF	7
#define NAV_DA_SV_OFF	15

#define NUM_NAV_DA		3
#define FONT_IMG_WIDTH	18
#define FONT_IMG_HEIGHT	36

static GtkWidget *nav_da;
#define NAV_DA_FONT_NUM 	20
static char nav_text[NAV_DA_FONT_NUM + 1];
static char last_nav_text[NAV_DA_FONT_NUM + 1];

#define HEADING_AREA_WIDTH	40
#define HEADING_AREA_HEIGHT	40
static GtkWidget *heading_da;
static GtkWidget *poll_engine_image, *track_image;

#define OUTER_WIDTH		4
#define INTER_WIDTH		2
/*(360 * 64)*/
#define POS_ARC			23040
/* position image: xpm image size */
#define XPM_SIZE		32
#define XPM_SIZE_HALF	16

static int x = 0, y = 0, r = 0;
static GdkRectangle last_rect;
static gboolean last_rect_valid = FALSE;

static int last_heading = -1;
static int lastx = 0, lasty = 0;

#define RESET_HEADING() \
	lastx = lasty = 0;	\
	last_heading = -1;	\

/**
 * Heading == -1 means just clear previous
 */
static void draw_speed_heading(int heading)
{
	#define DEG_TO_RAD 		(M_PI / 180)
	#define HEADING_W_HALF	(HEADING_AREA_WIDTH >> 1)
	#define HEADING_H_HALF	(HEADING_AREA_HEIGHT >> 1)
	#define HEADING_R 		(MIN(HEADING_W_HALF, HEADING_H_HALF) - 6)

	if (heading == last_heading)
		return;

	if (lastx > 0) {
		/* clear previous */
		gdk_draw_line(heading_da->window, g_context.heading_gc,
			HEADING_W_HALF, HEADING_H_HALF, lastx, lasty);
		RESET_HEADING();
	}

	/* degree 0 points to true north. headings respect to north, clockwise */
	if (heading >= 0) {
		float rad = heading * DEG_TO_RAD;
		lastx = (int)(HEADING_W_HALF + HEADING_R * sin(rad));
		lasty = (int)(HEADING_H_HALF - HEADING_R * cos(rad));
		gdk_draw_line(heading_da->window, g_context.heading_gc,
			HEADING_W_HALF, HEADING_H_HALF, lastx, lasty);

		last_heading = heading;
	}
}

static gboolean heading_da_expose_event (GtkWidget *widget, GdkEventExpose *evt, gpointer data)
{
	if (! GTK_WIDGET_VISIBLE(heading_da))
		return TRUE;

	gdk_draw_pixbuf (heading_da->window, g_context.drawingarea_bggc,
		g_xpm_images[XPM_ID_POSITION_HEADING].pixbuf,
		0, 0, 0, 0, HEADING_AREA_WIDTH, HEADING_AREA_HEIGHT,
		GDK_RGB_DITHER_NONE, -1, -1);

	RESET_HEADING();

	return TRUE;
}

static inline int get_hacc_r()
{
	int r = MIN(g_view.width, g_view.height) >> 1;
	int hacc_r;

	if (isnan(g_gpsdata.hacc)) {
		hacc_r = 0;
	} else {
		int zoom = g_view.fglayer.repo->zoom;
		hacc_r = g_gpsdata.hacc / g_pixel_meters[zoom];
		if (hacc_r > r)
			hacc_r = r;
	}
	return hacc_r;
}

/**
 * call this on: zoom, move
 */
static gboolean map_draw_position()
{
	GdkDrawable *canvas = g_view.da->window;

	gboolean valid = (g_gpsdata.latlon_valid);

	x = g_view.pos_offset.x;
	y = g_view.pos_offset.y;
	r = get_hacc_r();

	/* Backup.
	 * Real range, including cross sign and circle.
	 * Take care about extra space due to round end */

	int rr = MAX(XPM_SIZE_HALF, r) + OUTER_WIDTH;
	GdkRectangle pos_rect = {x-rr, y-rr, rr << 1 , rr << 1};
	last_rect_valid = gdk_rectangle_intersect(&g_view.fglayer.visible, &pos_rect, &last_rect);

	/* Draw cross and circle */

	if (r >= 10) {
		int d = r << 1;
		gdk_gc_set_line_attributes(g_context.pos_hacc_circle_gc, OUTER_WIDTH,
			(valid? GDK_LINE_SOLID : GDK_LINE_ON_OFF_DASH), GDK_CAP_ROUND, GDK_JOIN_ROUND);
		gdk_gc_set_rgb_fg_color (g_context.pos_hacc_circle_gc, &g_base_colors[ID_COLOR_White]);
		gdk_draw_arc(canvas, g_context.pos_hacc_circle_gc, FALSE, x - r, y - r, d, d, 0, POS_ARC);

		gdk_gc_set_line_attributes(g_context.pos_hacc_circle_gc, INTER_WIDTH,
			(valid? GDK_LINE_SOLID : GDK_LINE_ON_OFF_DASH), GDK_CAP_ROUND, GDK_JOIN_ROUND);
		gdk_gc_set_rgb_fg_color (g_context.pos_hacc_circle_gc, &g_base_colors[ID_COLOR_Olive]);
		gdk_draw_arc(canvas, g_context.pos_hacc_circle_gc, FALSE, x - r, y - r, d, d, 0, POS_ARC);
		r = rr;
	} else {
		r = 0;
	}

	int id = valid? XPM_ID_POSITION_VALID : XPM_ID_POSITION_INVALID;
	gdk_draw_pixbuf (g_view.da->window, g_context.track_gc, g_xpm_images[id].pixbuf,
		0, 0, x - XPM_SIZE_HALF, y - XPM_SIZE_HALF, XPM_SIZE, XPM_SIZE,
		GDK_RGB_DITHER_NONE, -1, -1);

	return TRUE;
}

static void increment_draw()
{
	if (last_rect_valid) {
		gdk_draw_drawable (g_view.da->window, g_context.track_gc, g_view.pixmap,
			last_rect.x, last_rect.y, last_rect.x, last_rect.y,
			last_rect.width, last_rect.height);
	}

	GdkRectangle rect;
	track_draw(g_view.pixmap, FALSE, &rect);
	gdk_draw_drawable(g_view.da->window, g_context.track_gc, g_view.pixmap,
		rect.x, rect.y, rect.x, rect.y,	rect.width, rect.height);

	map_draw_position();
}

/**
 * Update current position's screen offset
 * return: TRUE: need redraw
 */
static gboolean update_position_offset()
{
	/* pixels */
	#define SENSITIVE_R 5
	static int lastx = -SENSITIVE_R;
	static int lasty = -SENSITIVE_R;
	static int last_pacc_r = 0;

	/* delta relative to view center */
	point_t pos_pixel = wgs84_to_tilepixel(g_view.pos_wgs84, g_view.fglayer.repo->zoom, g_view.fglayer.repo);
	g_view.pos_offset.x = pos_pixel.x - g_view.fglayer.tl_pixel.x;
	g_view.pos_offset.y = pos_pixel.y - g_view.fglayer.tl_pixel.y;

	int diff_x = abs(g_view.pos_offset.x - lastx);
	int diff_y = abs(g_view.pos_offset.y - lasty);

	if (sqrt((diff_x * diff_x) + (diff_y * diff_y)) >= SENSITIVE_R) {
		lastx = g_view.pos_offset.x;
		lasty = g_view.pos_offset.y;
		return TRUE;
	}

	int pacc_r = get_hacc_r();

	if (abs(pacc_r - last_pacc_r) >= SENSITIVE_R) {
		last_pacc_r = pacc_r;
		return TRUE;
	}

	return FALSE;
}

/**
 * The background map.
 * refresh: moved, zoomed, auto-center mode
 */
void map_redraw_view_gps_running()
{
	map_draw_back_layers(g_view.pixmap);

	track_draw(g_view.pixmap, TRUE, NULL);

	update_position_offset();

	gdk_draw_drawable (g_view.da->window, g_context.track_gc, g_view.pixmap,
		g_view.fglayer.visible.x, g_view.fglayer.visible.y,
		g_view.fglayer.visible.x, g_view.fglayer.visible.y,
		g_view.fglayer.visible.width, g_view.fglayer.visible.height);

	map_draw_position();
}

static void draw_labels(gboolean force_redraw)
{
	if (! GTK_WIDGET_VISIBLE(nav_da))
		return;

	GdkGC *gc = nav_da->style->bg_gc[GTK_WIDGET_STATE(nav_da)];

	int i, offset = 0;
	XPM_ID_T id;

	for (i=0; i<NAV_DA_FONT_NUM; i++) {
		offset += FONT_IMG_WIDTH;

		if (! force_redraw && nav_text[i] == last_nav_text[i])
			continue;

		gdk_draw_rectangle (nav_da->window,	gc,	TRUE, offset, 2, FONT_IMG_WIDTH, FONT_IMG_HEIGHT);
		last_nav_text[i] = nav_text[i];

		switch(nav_text[i]) {
		case '0':
			id = XPM_ID_LETTER_0; break;
		case '1':
			id = XPM_ID_LETTER_1; break;
		case '2':
			id = XPM_ID_LETTER_2; break;
		case '3':
			id = XPM_ID_LETTER_3; break;
		case '4':
			id = XPM_ID_LETTER_4; break;
		case '5':
			id = XPM_ID_LETTER_5; break;
		case '6':
			id = XPM_ID_LETTER_6; break;
		case '7':
			id = XPM_ID_LETTER_7; break;
		case '8':
			id = XPM_ID_LETTER_8; break;
		case '9':
			id = XPM_ID_LETTER_9; break;
		case '.':
			id = XPM_ID_LETTER_dot; break;
		case '/':
			id = XPM_ID_LETTER_slash; break;
		case '-':
			id = XPM_ID_LETTER_minus; break;
		case 'm':
			id = XPM_ID_LETTER_m; break;
		case 'p':
			id = XPM_ID_LETTER_p; break;
		case 's':
			id = XPM_ID_LETTER_s; break;
		default:
			id = XPM_ID_NONE; break;
		}

		if (id != XPM_ID_NONE) {
			gdk_draw_pixbuf (nav_da->window, gc, g_xpm_images[id].pixbuf,
				0, 0, offset, 2, FONT_IMG_WIDTH, FONT_IMG_HEIGHT, GDK_RGB_DITHER_NORMAL, -1, -1);
		}
	}
}

static gboolean nav_da_expose_event (GtkWidget *widget, GdkEventExpose *evt, gpointer data)
{
	draw_labels(TRUE);
	return FALSE;
}

/**
 * Called by polling thread on each poll_suspending
 * Use global data <g_csd>
 */
void poll_update_ui()
{
	static gboolean last_valid = FALSE;
	gboolean offset_sensitive = FALSE;

	if (g_gpsdata.latlon_valid) {
		g_view.pos_wgs84.lat = g_gpsdata.lat;
		g_view.pos_wgs84.lon = g_gpsdata.lon;
		offset_sensitive = update_position_offset();
	}

	memset(nav_text, 0, NAV_DA_FONT_NUM);

	if (g_gpsdata.height_valid)
		sprintf(&nav_text[NAV_DA_H_OFF], "%dm", (int)g_gpsdata.height);

	if (g_gpsdata.vel_valid) {
		sprintf(&nav_text[NAV_DA_SPD_OFF], "%.1fmps", g_gpsdata.speed_2d);
		draw_speed_heading((g_gpsdata.speed_2d > 0.1? (int)g_gpsdata.heading_2d : -1));
	}

	if (g_gpsdata.svinfo_valid)
		sprintf(&nav_text[NAV_DA_SV_OFF], "%d/%d", g_gpsdata.sv_in_use, g_gpsdata.sv_get_signal);

	draw_labels(FALSE);

	gboolean out_of_view = g_view.pos_offset.x < g_view.fglayer.visible.x ||
		g_view.pos_offset.y < g_view.fglayer.visible.y ||
		g_view.pos_offset.x >= g_view.fglayer.visible.x + g_view.fglayer.visible.width ||
		g_view.pos_offset.y >= g_view.fglayer.visible.x + g_view.fglayer.visible.height;

	if (g_context.cursor_in_view && out_of_view) {
		map_centralize();
	} else if (offset_sensitive) {
		increment_draw();
	} else if (last_valid != g_gpsdata.latlon_valid) {
		update_position_offset();
		increment_draw();
	}

	last_valid = g_gpsdata.latlon_valid;
}

void ctx_tab_gps_fix_on_show()
{
	last_rect_valid = FALSE;

	if (POLL_STATE_TEST(RUNNING)) {
		map_set_redraw_func(&map_redraw_view_gps_running);
	} else {
		map_set_redraw_func(NULL);
	}
}

void ctx_gpsfix_on_track_state_changed()
{
	gboolean enabled = g_context.track_enabled;

	gtk_image_set_from_pixbuf(GTK_IMAGE(track_image),
		g_xpm_images[enabled? XPM_ID_TRACK_ON : XPM_ID_TRACK_OFF].pixbuf);
}

void ctx_gpsfix_on_poll_state_changed()
{
	switch(g_context.poll_state) {
	case POLL_STATE_RUNNING:
		if (ctx_tab_get_current_id() == CTX_ID_NONE)
			switch_to_main_view(CTX_ID_GPS_FIX);
		break;
	case POLL_STATE_STARTING:
	case POLL_STATE_STOPPING:
		break;
	case POLL_STATE_SUSPENDING:
		last_rect_valid = FALSE;
		if (ctx_tab_get_current_id() == CTX_ID_NONE)
			switch_to_main_view(CTX_ID_NONE);
		break;
	}
}

void ctx_gpsfix_on_poll_engine_changed()
{
	XPM_ID_T id;

	switch(g_context.poll_engine) {
	case POLL_ENGINE_OGPSD:
		id = XPM_ID_POLLENGINE_FSO;
		break;
	case POLL_ENGINE_UBX:
		id = XPM_ID_POLLENGINE_UBX;
		break;
	default:
		return;
	}
	gtk_image_set_from_pixbuf(GTK_IMAGE(poll_engine_image), g_xpm_images[id].pixbuf);
}

GtkWidget * ctx_tab_gps_fix_create()
{
	memset(last_nav_text, 0, NAV_DA_FONT_NUM);

	GtkWidget *hbox = gtk_hbox_new(FALSE, 0);

	GtkWidget *nav_hbox = gtk_hbox_new(TRUE, 0);

	nav_da = gtk_drawing_area_new();
	gtk_widget_set_size_request(nav_da, FONT_IMG_WIDTH * NAV_DA_FONT_NUM, FONT_IMG_HEIGHT);
	gtk_container_add(GTK_CONTAINER(nav_hbox), nav_da);
	g_signal_connect (nav_da, "expose-event", G_CALLBACK(nav_da_expose_event), NULL);

	gtk_box_pack_start (GTK_BOX (hbox), nav_hbox, TRUE, TRUE, 0);

	heading_da = gtk_drawing_area_new();
	gtk_widget_set_size_request(heading_da, HEADING_AREA_WIDTH, HEADING_AREA_HEIGHT);
	gtk_box_pack_start(GTK_BOX (hbox), heading_da, FALSE, FALSE, 0);
	g_signal_connect (heading_da, "expose-event", G_CALLBACK(heading_da_expose_event), NULL);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	track_image = gtk_image_new_from_pixbuf(g_xpm_images[XPM_ID_TRACK_OFF].pixbuf);
	gtk_box_pack_start(GTK_BOX (vbox), track_image, FALSE, FALSE, 0);

	poll_engine_image = gtk_image_new_from_pixbuf(g_xpm_images[XPM_ID_POLLENGINE_UBX].pixbuf);
	gtk_box_pack_end(GTK_BOX (vbox), poll_engine_image, FALSE, FALSE, 0);

	return hbox;
}
