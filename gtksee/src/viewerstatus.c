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

#include "util.h"
#include "viewerstatus.h"

GtkWidget *viewer_status_type; /* GtkPixmap */
GtkWidget *viewer_status_name; /* GtkLabel */
GtkWidget *viewer_status_size; /* GtkLabel */
GtkWidget *viewer_status_prop; /* GtkLabel */
GtkWidget *viewer_status_zoom; /* GtkLabel */

void
set_viewer_status_size(glong size)
{
	gtk_label_set(GTK_LABEL(viewer_status_size), fsize(size));
}

void
set_viewer_status_type(GdkPixmap *pixmap, GdkBitmap *mask)
{
	gtk_pixmap_set(GTK_PIXMAP(viewer_status_type), pixmap, mask);
	gtk_widget_show(viewer_status_type);
}

void
set_viewer_status_name(gchar *text)
{
	gtk_label_set(GTK_LABEL(viewer_status_name), text);
}

void
set_viewer_status_prop(gchar *text)
{
	gtk_label_set(GTK_LABEL(viewer_status_prop), text);
}

void
set_viewer_status_zoom(gint n)
{
	gchar buffer[6];
	sprintf(buffer, "%i%%", n);
	gtk_label_set(GTK_LABEL(viewer_status_zoom), buffer);
}

GtkWidget*
get_viewer_status_bar(GdkPixmap *pixmap, GdkBitmap *mask)
{
	GtkWidget *main_hbox, *hbox;
	GtkWidget *frame;

	main_hbox = gtk_hbox_new(FALSE, 2);

	/* creating viewer_status_type & viewer_status_name */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);
	
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	
	viewer_status_type = gtk_pixmap_new(pixmap, mask);
	gtk_box_pack_start(GTK_BOX(hbox), viewer_status_type, FALSE, FALSE, 0);
	gtk_widget_show(viewer_status_type);
	
	viewer_status_name = gtk_label_new("");
	gtk_label_set_justify(GTK_LABEL(viewer_status_name), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(viewer_status_name), 0, 0);
	gtk_widget_set_usize(viewer_status_name, 140, 16);
	gtk_box_pack_start(GTK_BOX(hbox), viewer_status_name, TRUE, TRUE, 0);
	gtk_widget_show (viewer_status_name);
	
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	
	gtk_box_pack_start(GTK_BOX(main_hbox), frame, TRUE, TRUE, 0);

	/* creating viewer_status_size */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);

	viewer_status_size = gtk_label_new("");
	gtk_label_set_justify(GTK_LABEL(viewer_status_size), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(viewer_status_size), 0, 0);
	gtk_widget_set_usize(viewer_status_size, 60, 16);
	gtk_container_add(GTK_CONTAINER (frame), viewer_status_size);
	gtk_widget_show (viewer_status_size);
	
	gtk_box_pack_start(GTK_BOX(main_hbox), frame, FALSE, FALSE, 0);

	/* creating viewer_status_prop */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);

	viewer_status_prop = gtk_label_new("");
	gtk_label_set_justify(GTK_LABEL(viewer_status_prop), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(viewer_status_prop), 0, 0);
	gtk_widget_set_usize(viewer_status_prop, 120, 16);
	gtk_container_add(GTK_CONTAINER (frame), viewer_status_prop);
	gtk_widget_show (viewer_status_prop);
	
	gtk_box_pack_start(GTK_BOX(main_hbox), frame, FALSE, FALSE, 0);

	/* creating viewer_status_zoom */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);

	viewer_status_zoom = gtk_label_new("");
	gtk_label_set_justify(GTK_LABEL(viewer_status_zoom), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(viewer_status_zoom), 0, 0);
	gtk_widget_set_usize(viewer_status_zoom, 36, 16);
	gtk_container_add(GTK_CONTAINER (frame), viewer_status_zoom);
	gtk_widget_show (viewer_status_zoom);
	
	gtk_box_pack_start(GTK_BOX(main_hbox), frame, FALSE, FALSE, 0);

	return main_hbox;
}
