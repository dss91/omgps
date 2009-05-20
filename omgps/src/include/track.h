#ifndef TRACK_H_
#define TRACK_H_

#define TRACK_HEAD_LABEL_1 "start time: "
#define TRACK_HEAD_LABEL_2 "end time: "
#define TRACK_HEAD_LABEL_3 "record count: "

extern GtkWidget* ctx_tab_track_replay_create();
extern void ctx_tab_track_replay_on_show();

extern GtkWidget * track_tab_create();
extern void track_tab_on_show();

extern char *get_cur_replay_filepath();
extern void track_replay_cleanup();

extern gboolean track_new();
extern void track_add(/*double lat, double lon, U4 gps_tow*/);
extern void track_cleanup();
extern gboolean track_saveall(gboolean free);
extern void track_draw(GdkPixmap *canvas, gboolean refresh, GdkRectangle *rect);
extern int track_get_count();
extern void track_replay_centralize();

#endif /* TRACK_H_ */
