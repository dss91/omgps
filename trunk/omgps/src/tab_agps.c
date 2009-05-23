#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ubx.h"
#include "network.h"
#include "omgps.h"
#include "util.h"
#include "usart.h"
#include "customized.h"

static GtkWidget *user_entry, *pwd_entry, *pwd_entry_again;
static GtkWidget *save_account_button, *reset_account_button;
static GtkWidget *online_button, *reset_button;
static char *reset_type = "warm";

static void online_button_clicked(GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive(online_button, FALSE);
	switch_to_main_view(CTX_ID_AGPS_ONLINE);
}

static void reset_account()
{
	char *user = g_cfg->agps_user == NULL ? "" : g_cfg->agps_user;
	char *pass = g_cfg->agps_pwd == NULL ? "" : g_cfg->agps_pwd;
	gtk_entry_set_text(GTK_ENTRY(user_entry), user);
	gtk_entry_set_text(GTK_ENTRY(pwd_entry), pass);
	gtk_entry_set_text(GTK_ENTRY(pwd_entry_again), pass);
}

static void reset_account_button_clicked(GtkWidget *widget, gpointer data)
{
	reset_account();
}

static void save_account_button_clicked(GtkWidget *widget, gpointer data)
{
	char *user = (char *) gtk_entry_get_text(GTK_ENTRY(user_entry));
	char *pwd = (char *) gtk_entry_get_text(GTK_ENTRY(pwd_entry));
	char *pwd_again = (char *) gtk_entry_get_text(GTK_ENTRY(pwd_entry_again));

	char *error = NULL;

	#define SAVE_PREFIX "Save agps online account: "

	user = trim(user);
	if (user == NULL || strlen(user) == 0) {
		error = SAVE_PREFIX"user is empty";
		goto END;
	}

	if (!validate_email(user)) {
		error = SAVE_PREFIX"invalid user, should be email address";
		goto END;
	}

	pwd = trim(pwd);
	pwd_again = trim(pwd_again);

	if (pwd == NULL || pwd_again == NULL || strlen(pwd) == 0 || strlen(pwd_again) == 0) {
		error = SAVE_PREFIX"password is empty";
		goto END;
	}

	/* The password is generated by u-blox, 6-chars, but we can't guarantee that
	 * will be changed to 5 or less in future, or it let use to modify the password.
	 * Normally it is stupid to set it as that short! We don't validate length here. */

	if (strcmp(pwd, pwd_again) != 0) {
		error = SAVE_PREFIX"passwords not match";
		goto END;
	}

	g_cfg->agps_user = user;
	g_cfg->agps_pwd = pwd;

	settings_save();

END:

	if (error != NULL)
		warn_dialog(error);
	else
		info_dialog(SAVE_PREFIX"ok");
}

static void reset_radio_button_toggled(GtkWidget *widget, gpointer data)
{
	reset_type = (char *) data;
}

static gboolean reset_gps_cmd(void *_reset_type)
{
	char *reset_type = (char *) _reset_type;

	int gps_dev_fd = 0;

	/* non ubx means: we need open serial port
	 * ubx means: it is called by non-main thread, need UI lock */
	gboolean is_ubx = POLL_ENGINE_TEST(UBX);
	if (! is_ubx) {
		if ((gps_dev_fd = usart_open((U4)BAUD_RATE, FALSE)) <= 0) {
			warn_dialog("Open USART failed");
			return FALSE;
		}
		ubx_init(gps_dev_fd);
	}

	gboolean ok = ubx_reset_gps(reset_type);
	sleep(1);

	char buf[30];
	if (is_ubx)
		LOCK_UI();

	if (ok) {
		sprintf(buf, "%s reset GPS: done.", reset_type);
		info_dialog(buf);
	} else {
		sprintf(buf, "%s reset GPS: failed", reset_type);
		warn_dialog(buf);
	}
	gtk_widget_set_sensitive(reset_button, TRUE);

	if (is_ubx)
		UNLOCK_UI();

	if (gps_dev_fd > 0)
		close(gps_dev_fd);

	return ok;
}

static void reset_gps_button_clicked(GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive(widget, FALSE);

	if (POLL_ENGINE_TEST(UBX)) {
		if (! issue_ctrl_cmd(reset_gps_cmd, reset_type))
			warn_dialog("Another command is running, please wait a while");
	} else {
		reset_gps_cmd(reset_type);
	}
}

static void add_password_widget(GtkWidget *box, char *label, GtkWidget *pwd_entry)
{
	GtkWidget *pwd_label = gtk_label_new(label);
	gtk_entry_set_max_length(GTK_ENTRY(pwd_entry), 10);
	gtk_entry_set_visibility(GTK_ENTRY(pwd_entry), FALSE);
	gtk_box_pack_start(GTK_BOX (box), pwd_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (box), pwd_entry, TRUE, TRUE, 5);
}

void agps_tab_on_show()
{
	gboolean enable = POLL_STATE_TEST(RUNNING);

	gtk_widget_set_sensitive(reset_button, enable);

	enable &= (g_cfg->agps_user != NULL && g_cfg->agps_pwd != NULL);

	gtk_widget_set_sensitive(save_account_button, enable);
	gtk_widget_set_sensitive(reset_account_button, enable);
	gtk_widget_set_sensitive(online_button,	enable);
}

GtkWidget * agps_tab_create()
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *agps_frame = gtk_frame_new(" AGPS ");

	GtkWidget *agps_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER (agps_frame), agps_vbox);

	GtkWidget *hbox_top = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER (agps_vbox), hbox_top);

	GtkWidget *hbox_user = gtk_hbox_new(FALSE, 0);
	GtkWidget *hbox_pwd = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX (agps_vbox), hbox_user, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (agps_vbox), hbox_pwd, FALSE, FALSE, 5);

	GtkWidget *user_label = gtk_label_new("user:");
	gtk_label_set_justify(GTK_LABEL(user_label), GTK_JUSTIFY_LEFT);
	user_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(user_entry), 32);
	gtk_box_pack_start(GTK_BOX (hbox_user), user_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (hbox_user), user_entry, TRUE, TRUE, 5);

	pwd_entry = gtk_entry_new();
	pwd_entry_again = gtk_entry_new();
	add_password_widget(hbox_pwd, "pass:", pwd_entry);
	add_password_widget(hbox_pwd, "again:", pwd_entry_again);

	GtkWidget *button_box = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX (agps_vbox), button_box, FALSE, FALSE, 5);

	save_account_button = gtk_button_new_with_label("Save");
	gtk_container_add(GTK_CONTAINER(button_box), save_account_button);
	g_signal_connect (G_OBJECT (save_account_button), "clicked",
			G_CALLBACK (save_account_button_clicked), NULL);

	reset_account_button = gtk_button_new_with_label("Reset");
	gtk_container_add(GTK_CONTAINER(button_box), reset_account_button);
	g_signal_connect (G_OBJECT (reset_account_button), "clicked",
			G_CALLBACK (reset_account_button_clicked), NULL);

	online_button = gtk_button_new_with_label("AGPS Online");
	gtk_container_add(GTK_CONTAINER(button_box), online_button);
	g_signal_connect (G_OBJECT (online_button), "clicked",
			G_CALLBACK (online_button_clicked), NULL);

	reset_account();

	GtkWidget *tip_label = gtk_label_new("To apply for a free u-blox Assist Now account,\n"
		"please send an e-mail to: agps-account@u-blox.com");
	gtk_misc_set_alignment(GTK_MISC(tip_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX (agps_vbox), tip_label, FALSE, FALSE, 5);

	/* reset */

	GtkWidget *reset_frame = gtk_frame_new(" Reset GPS receiver ");
	GtkWidget *reset_hbox = gtk_hbutton_box_new();
	gtk_container_add(GTK_CONTAINER (reset_frame), reset_hbox);

	GtkWidget *hot_radio = gtk_radio_button_new_with_label(NULL, "hot");
	GtkWidget *warm_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(hot_radio), "warm");
	GtkWidget *cold_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(hot_radio), "cold");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hot_radio), TRUE);

	g_signal_connect (G_OBJECT (hot_radio), "toggled", G_CALLBACK (reset_radio_button_toggled), "hot");
	g_signal_connect (G_OBJECT (warm_radio), "toggled",	G_CALLBACK (reset_radio_button_toggled), "warm");
	g_signal_connect (G_OBJECT (cold_radio), "toggled",	G_CALLBACK (reset_radio_button_toggled), "cold");

	reset_button = gtk_button_new_with_label("reset");
	g_signal_connect (G_OBJECT (reset_button), "clicked", G_CALLBACK (reset_gps_button_clicked), NULL);

	gtk_container_add(GTK_CONTAINER (reset_hbox), hot_radio);
	gtk_container_add(GTK_CONTAINER (reset_hbox), warm_radio);
	gtk_container_add(GTK_CONTAINER (reset_hbox), cold_radio);
	gtk_container_add(GTK_CONTAINER (reset_hbox), reset_button);

	gtk_box_pack_start(GTK_BOX (vbox), agps_frame, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (vbox), reset_frame, FALSE, FALSE, 5);

	return vbox;
}
