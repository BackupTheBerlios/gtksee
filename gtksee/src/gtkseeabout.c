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
#include "../icons/gtksee.xpm"
#include "pixmaps/hotaru.xpm"

void		destroy_about_dialog	();

void
destroy_about_dialog(GtkWidget *dialog)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
}

GtkWidget*
get_about_dialog()
{
	static GtkStyle *style;
	static GdkPixmap *gtksee_pixmap, *hotaru_pixmap;
	static GdkBitmap *gtksee_mask, *hotaru_mask;
	
	GtkWidget *dialog;
	GtkWidget *frame, *box, *table, *hbox, *bbox;
	GtkWidget *widget, *label, *pix;
	GtkRequisition requisition;
	
	/* creating about dialog itself */
	dialog = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_signal_connect_object(GTK_OBJECT(dialog),
		"delete_event",
		GTK_SIGNAL_FUNC(destroy_about_dialog),
		GTK_OBJECT(dialog));
	gtk_grab_add(dialog);

	if (gtksee_pixmap == NULL || hotaru_pixmap == NULL)
	{
		gtk_widget_realize(dialog);
		style = gtk_widget_get_style(dialog);
		gtksee_pixmap = gdk_pixmap_create_from_xpm_d(
			dialog->window,
			&gtksee_mask, &style->bg[GTK_STATE_NORMAL],
			(gchar **)gtksee_xpm);
		style = gtk_widget_get_style(dialog);
		hotaru_pixmap = gdk_pixmap_create_from_xpm_d(
			dialog->window,
			&hotaru_mask, &style->bg[GTK_STATE_NORMAL],
			(gchar **)hotaru_xpm);
		gtk_widget_unrealize(dialog);
	}
	
	/* creating labels & pixmap and add them into a box */
	box = gtk_vbox_new(FALSE, 2);
	gtk_container_border_width(GTK_CONTAINER(box), 5);
	gtk_widget_show(box);
	
	widget = gtk_label_new("GTK See");
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	widget = gtk_pixmap_new(gtksee_pixmap, gtksee_mask);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	widget = gtk_label_new(VERSION);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	widget = gtk_label_new("");
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	widget = gtk_label_new(_("(c)1998-? Lee Luyang(Hotaru), China"));
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	widget = gtk_label_new("http://hotaru.clinuxworld.com/gtksee/");
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	/* adding box into frame */
	frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	gtk_container_add(GTK_CONTAINER(frame), box);
	
	/* creating the main box */
	box = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(box);
	
	/* adding frame to main box */
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
	
	/* creating license */
	widget = gtk_label_new(_("This software is distributed under"));
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	widget = gtk_label_new(_("GPL Version 2"));
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
	
	widget = gtk_label_new("");
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);

	/* creating the table and add main box */
	table = gtk_table_new(2, 1, FALSE);
	gtk_widget_show(table);
	gtk_table_attach(
		GTK_TABLE(table),
		box,
		0, 1, 0, 1,
		0, 0, 3, 3);

	/* creating button */
	bbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(bbox);
	
	pix = gtk_pixmap_new(hotaru_pixmap, hotaru_mask);
	gtk_box_pack_start(GTK_BOX(bbox), pix, FALSE, FALSE, 0);
	gtk_widget_show(pix);
	
	label = gtk_label_new(_("O K"));
	gtk_box_pack_start(GTK_BOX(bbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	
	widget = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(widget), bbox);
	gtk_widget_show(widget);
	
	gtk_signal_connect_object(GTK_OBJECT(widget),
		"clicked",
		GTK_SIGNAL_FUNC(destroy_about_dialog),
		GTK_OBJECT(dialog));
	
	/* creating hbox and adding button */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, FALSE, 0);
	
	/* adding hbox into the table */
	gtk_table_attach(
		GTK_TABLE(table),
		hbox,
		0, 1, 1, 2,
		0, 0, 3, 3);
	
	/* creating the main frame, and add frame into it */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_widget_show(frame);
	
	/* Finally, don't forget to add the main frame into the dialog :) */
	gtk_container_add(GTK_CONTAINER(dialog), frame);
	
	gtk_widget_size_request(dialog, &requisition);
	gtk_widget_set_uposition(
		dialog,
		(gdk_screen_width() - requisition.width) / 2,
		(gdk_screen_height() - requisition.height) / 2);
	
	return dialog;
}
