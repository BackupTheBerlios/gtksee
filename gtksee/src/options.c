/*
 * GTK See -- a image viewer based on GTK+
 * Copyright (C) 1998 Hotaru Lee <jkhotaru@mail.sti.com.cn> <hotaru@163.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "pixmaps/folder_mini.xpm"
#include "rc.h"
#include "gtksee.h"
#include "options.h"

static guchar *rootdir_rc = "root_directory";
static guchar *hiddendir_rc = "hidden_directory";
static guchar *interval_rc = "slideshow_delay";

static GtkWidget *root_directory_entry;
static GtkWidget *hidden_directory_entry;
static GtkWidget *slideshow_interval_scale;
static GtkWidget *slideshow_interval_entry;
static GtkWidget *slideshow_interval_label;

static guchar old_root_directory[40];
static guchar old_hidden_directory[40];

static const gint intervals[] =
{
	1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000,
	15000, 20000, 30000, 45000, 60000, 2*60000, 3*60000, 4*60000,
	5*60000, 6*60000, 7*60000, 8*60000, 9*60000, 10*60000, 20*60000,
	30*60000, 40*60000, 60*60000, 2*60*60000, 4*60*60000, 8*60*60000,
	24*60*60000
};
static const gint nintervals = 32;

static void	options_cancel			(GtkWidget *window);
static void	options_ok			(GtkWidget *window);
static void	options_root_browse		(GtkWidget *widget,
						 gpointer data);
static void	options_root_browse_ok		(GtkWidget *widget,
						 gpointer data);
static void	options_root_browse_cancel	(GtkWidget *widget,
						 gpointer data);
static gint	interval_get_index		(gint interval);
static void	interval_update			(gint interval);
static void	interval_entry_changed		(GtkWidget *widget,
						 gpointer data);
static void	interval_scale_changed		(GtkAdjustment *adjustment,
						 gpointer data);

void
options_show()
{
	GtkWidget *window, *notebook, *label, *button;
	GtkWidget *frame, *vbox, *vbox2, *hbox, *gtkpixmap;
	GtkObject *adjustment;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkStyle *style;
	guchar *buf;
	gint interval;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
		GTK_SIGNAL_FUNC(options_cancel), GTK_OBJECT(window));
	gtk_container_border_width(GTK_CONTAINER(window), 5);
	gtk_window_set_title(GTK_WINDOW(window), _("Options"));
	gtk_widget_realize(window);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);

	/* Adding tab: ---File System--- */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	frame = gtk_frame_new(_("Root directory"));
	gtk_container_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 4);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	root_directory_entry = gtk_entry_new();
	buf = rc_get(rootdir_rc);
	if (buf == NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(root_directory_entry), "/");
		old_root_directory[0] = '/';
		old_root_directory[1] = '\0';
	} else
	{
		gtk_entry_set_text(GTK_ENTRY(root_directory_entry), buf);
		strcpy(old_root_directory, buf);
	}
	gtk_box_pack_start(GTK_BOX(hbox), root_directory_entry,
		TRUE, TRUE, 2);
	gtk_widget_show(root_directory_entry);

	style = gtk_widget_get_style(window);
	pixmap = gdk_pixmap_create_from_xpm_d(window->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar**)folder_mini_xpm);
	gtkpixmap = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(gtkpixmap);
	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), gtkpixmap);
	gtk_signal_connect(GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(options_root_browse),
		NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button,
		FALSE, FALSE, 2);
	gtk_widget_show(button);

	frame = gtk_frame_new(_("Hidden directories"));
	gtk_container_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 4);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_widget_show(hbox);

	hidden_directory_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), hidden_directory_entry,
		TRUE, TRUE, 2);
	buf = rc_get(hiddendir_rc);
	if (buf == NULL)
	{
		old_hidden_directory[0] = '\0';
	} else
	{
		strcpy(old_hidden_directory, buf);
		gtk_entry_set_text(GTK_ENTRY(hidden_directory_entry), buf);
	}
	gtk_widget_show(hidden_directory_entry);

	label = gtk_label_new(_("File System"));
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

	/* Adding tab: ---Slide Show--- */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);

	frame = gtk_frame_new(_("Interval"));
	gtk_container_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 4);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox2), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_widget_show(vbox2);

	adjustment = gtk_adjustment_new(5.0, 0.0, (gfloat)nintervals,
		1.0, 1.0, 1.0);
	gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		GTK_SIGNAL_FUNC(interval_scale_changed), NULL);
	interval = rc_get_int(interval_rc);
	if (interval > RC_NOT_EXISTS)
	{
		GTK_ADJUSTMENT(adjustment)->value =
			(gfloat)interval_get_index(interval);
	}
	slideshow_interval_scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
	gtk_range_set_update_policy(GTK_RANGE(slideshow_interval_scale),
		GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_digits(GTK_SCALE(slideshow_interval_scale), 1);
	gtk_scale_set_draw_value(GTK_SCALE(slideshow_interval_scale), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox2), slideshow_interval_scale,
		TRUE, TRUE, 2);
	gtk_widget_show(slideshow_interval_scale);

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, TRUE, 2);
	gtk_widget_show(hbox);

	slideshow_interval_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), slideshow_interval_entry,
		TRUE, TRUE, 2);
	gtk_signal_connect(GTK_OBJECT(slideshow_interval_entry),
		"changed", GTK_SIGNAL_FUNC(interval_entry_changed),
		NULL);
	gtk_widget_show(slideshow_interval_entry);

	slideshow_interval_label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), slideshow_interval_label,
		TRUE, TRUE, 2);
	gtk_label_set_justify(GTK_LABEL(slideshow_interval_label),
		GTK_JUSTIFY_LEFT);
	gtk_widget_show(slideshow_interval_label);

	interval_update(interval);

	label = gtk_label_new(_("Slide Show"));
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

	/* Creating buttons */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	button = gtk_button_new_with_label(_(" Cancel "));
	gtk_signal_connect_object(GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(options_cancel),
		GTK_OBJECT(window));
	gtk_widget_show(button);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 4);
	button = gtk_button_new_with_label(_(" O K "));
	gtk_signal_connect_object(GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(options_ok),
		GTK_OBJECT(window));
	gtk_widget_show(button);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 4);

	/* finally, add notebook and buttons to window... */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_grab_add(window);
	gtk_widget_set_usize(window, 400, 300);

	gtk_widget_show(window);
}

static void
options_cancel(GtkWidget *window)
{
	gtk_grab_remove(window);
	gtk_widget_destroy(window);
}

static void
options_ok(GtkWidget *window)
{
	guchar *new_root, *new_hidden;
	gboolean need_refresh;
	gint interval;

	need_refresh = FALSE;
	new_root = gtk_entry_get_text(GTK_ENTRY(root_directory_entry));
	if (strcmp(new_root, old_root_directory) != 0)
	{
		rc_set(rootdir_rc, new_root);
		gtksee_set_root(new_root);
		need_refresh = TRUE;
	}
	new_hidden = gtk_entry_get_text(GTK_ENTRY(hidden_directory_entry));
	if (strcmp(new_hidden, old_hidden_directory) != 0)
	{
		rc_set(hiddendir_rc, new_hidden);
		need_refresh = TRUE;
	}
	interval = atoi(gtk_entry_get_text(
		GTK_ENTRY(slideshow_interval_entry)));
	rc_set_int(interval_rc, interval);
	rc_save_gtkseerc();
	if (need_refresh)
	{
		refresh_all();
	}
	gtk_grab_remove(window);
	gtk_widget_destroy(window);
}

static void
options_root_browse(GtkWidget *widget, gpointer data)
{
	GtkWidget *filesel;

	filesel = gtk_file_selection_new(_("Root directory"));
	gtk_signal_connect(GTK_OBJECT(filesel),
		"destroy",
		GTK_SIGNAL_FUNC(options_root_browse_cancel),
		filesel);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
		"clicked",
		GTK_SIGNAL_FUNC(options_root_browse_ok),
		filesel);
	gtk_signal_connect(
		GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
		"clicked",
		GTK_SIGNAL_FUNC(options_root_browse_cancel),
		filesel);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel),
		gtk_entry_get_text(GTK_ENTRY(root_directory_entry)));
	gtk_grab_add(filesel);
	gtk_widget_show(filesel);
}

static void
options_root_browse_ok(GtkWidget *widget, gpointer data)
{
	guchar new_root[80], *ptr;

	strcpy(new_root, gtk_file_selection_get_filename(GTK_FILE_SELECTION(data)));
	if (new_root[strlen(new_root) - 1] != '/')
	{
		/* remove file name. only path name left */
		if ((ptr = strrchr(new_root, '/')) != NULL)
		{
			*(ptr + 1) = '\0';
		}
	}
	ptr = new_root;
	if (new_root[1] == '/') ptr++;
	gtk_entry_set_text(GTK_ENTRY(root_directory_entry), ptr);
	gtk_grab_remove(GTK_WIDGET(data));
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void
options_root_browse_cancel(GtkWidget *widget, gpointer data)
{
	gtk_grab_remove(GTK_WIDGET(data));
	gtk_widget_destroy(GTK_WIDGET(data));
}

static gint
interval_get_index(gint interval)
{
	gint i;

	for (i = 0; i < nintervals; i++)
	{
		if (interval <= intervals[i]) return i;
	}
	return  nintervals - 1;
}

static void
interval_update(gint interval)
{
	guchar buf[80];

	sprintf(buf, "%u", interval);
	gtk_entry_set_text(GTK_ENTRY(slideshow_interval_entry), buf);
}

static void
interval_entry_changed(GtkWidget *widget, gpointer data)
{
	static const guchar *unit_name[] =
		{N_("second(s)"), N_("minute(s)"), N_("hour(s)"), N_("day(s)")};
	static const gint unit_count[] =
		{1000, 60, 60, 24};
	gint i;
	guchar buf[80];
	gint interval;

	interval = atoi(gtk_entry_get_text(
		GTK_ENTRY(slideshow_interval_entry)));
	if (interval == 0)
	{
		gtk_label_set(GTK_LABEL(slideshow_interval_label),
			_("milliseconds, no delay"));
	} else
	{
		i = 0;
		interval = (interval + 999) / 1000;
		while (i < 3 && interval >= unit_count[i + 1])
		{
			i++;
			interval /= unit_count[i];
		}
		sprintf(buf, _("milliseconds, %i %s"), interval,
			_(unit_name[i]));
		gtk_label_set(GTK_LABEL(slideshow_interval_label), buf);
	}
}

static void
interval_scale_changed(GtkAdjustment *adjustment, gpointer data)
{
	gint interval;

	interval = intervals[(gint)adjustment->value];
	interval_update(interval);
}
