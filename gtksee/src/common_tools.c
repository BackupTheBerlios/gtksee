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
#include <unistd.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "common_tools.h"

void		close_dialog	(GtkWidget *dialog);
void		remove_it	(GtkWidget *widget, tool_parameters *param);
void		rename_it	(GtkWidget *widget, tool_parameters *param);

void
close_dialog(GtkWidget *dialog)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
}

void
remove_it(GtkWidget *widget, tool_parameters *param)
{
	gchar buffer[256];
	
	close_dialog(param->widget);
	if ((*(param->file_func)) (buffer) &&
	    unlink(buffer) == 0 &&
	    param->fin_func != NULL) (*(param->fin_func)) ();
}

void
rename_it(GtkWidget *widget, tool_parameters *param)
{
	gchar buffer[256], dest[256], *text, *pos;
	
	if (!(*(param->file_func)) (buffer))
	{
		close_dialog(param->widget);
		return;
	}
	text = gtk_entry_get_text(GTK_ENTRY(param->entry));
	close_dialog(param->widget);
	if (text == NULL || strlen(text) < 1)
	{
		return;
	}
	if (text[0] != '/') /* not from root tree */
	{
		strcpy(dest, buffer);
		if ((pos = strrchr(dest, '/')) == NULL)
		{
			return;
		}
		pos++;
		if (strcmp(pos, text) == 0)
		{
			return;
		}
		strcpy(pos, text);
	} else
	{
		if (strcmp(buffer, text) == 0)
		{
			return;
		}
		strcpy(dest, text);
	}
	
	if (rename(buffer, dest) == 0 &&
	    param->fin_func != NULL) (*(param->fin_func)) ();
}

void
remove_file(GtkWidget *widget, tool_parameters *param)
{
	gchar buffer[256];
	GtkWidget *dialog, *label, *button;
	
	if (!(*(param->file_func))(buffer)) return;
	if (strlen(buffer) < 1) return;
	
	dialog = gtk_dialog_new();
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Remove"));
	gtk_grab_add(dialog);
	param->widget = dialog;
	
	label = gtk_label_new(_("Do you really want to remove:"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE,
		TRUE, 5);
	gtk_widget_show(label);
       
	label = gtk_label_new(strcat(buffer," ?"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE,
		TRUE, 5);
	gtk_widget_show(label);
	
	button = gtk_button_new_with_label(_("yes"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
		TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(remove_it),
		(gpointer)param);
	gtk_widget_show(button);
	
	button = gtk_button_new_with_label(_("no"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
		TRUE, TRUE, 0);
	gtk_signal_connect_object(GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(close_dialog),
		(gpointer)dialog);
	gtk_widget_show(button);
	
	gtk_widget_show(dialog);
}

void
rename_file(GtkWidget *widget, tool_parameters *param)
{
	gchar buffer[256];
	GtkWidget *dialog, *label, *button, *entry;
	
	if (! (*(param->file_func)) (buffer)) return;
	if (strlen(buffer) < 1) return;
	
	dialog = gtk_dialog_new();
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Rename"));
	gtk_grab_add(dialog);
	param->widget = dialog;
	
	label = gtk_label_new(_("Rename file:"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE,
		TRUE, 5);
	gtk_widget_show(label);
	
	label = gtk_label_new(buffer);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE,
		TRUE, 5);
	gtk_widget_show(label);
	
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_entry_set_editable(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, TRUE,
		TRUE, 5);
	gtk_signal_connect(
		GTK_OBJECT(entry),
		"activate",
		(GtkSignalFunc)rename_it,
		param
	);
	param->entry = entry;
	gtk_widget_show(entry);
	
	button = gtk_button_new_with_label(_("ok"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
		TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(rename_it),
		(gpointer)param);
	gtk_widget_show(button);
	
	button = gtk_button_new_with_label(_("cancel"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
		TRUE, TRUE, 0);
	gtk_signal_connect_object(GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(close_dialog),
		(gpointer)dialog);
	gtk_widget_show(button);
	
	gtk_widget_show(dialog);
}

GtkWidget*
info_dialog_new(guchar *title, guint *sizes, guchar **text)
{
	static GdkFont *big_font = NULL, *small_font = NULL;
	GtkWidget *dialog, *hbox, *text_area, *button, *scrollbar;
	gint i;
	
	if (big_font == NULL)
	{
		big_font = gdk_font_load(
			_("-adobe-helvetica-bold-r-normal--*-180-*-*-*-*-*-*"));
	}
	
	if (small_font == NULL)
	{
		small_font = gdk_font_load(
			_("-adobe-times-medium-r-normal--*-140-*-*-*-*-*-*"));
	}
	
	dialog = gtk_dialog_new();
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	if (title != NULL)
	{
		gtk_window_set_title(GTK_WINDOW(dialog), title);
	}
	
	/* creating text area */
	text_area = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(text_area), FALSE);
	gtk_text_set_word_wrap(GTK_TEXT(text_area), TRUE);
	gtk_widget_set_usize(text_area, 300, 300);
	gtk_widget_show(text_area);
	
	/* creating vscrollbar */
	scrollbar = gtk_vscrollbar_new(GTK_TEXT(text_area)->vadj);
	gtk_widget_show(scrollbar);
	
	/* creating hbox, adding text and vscrollbar into it */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), text_area, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), scrollbar, FALSE, FALSE, 0);
	gtk_widget_show(hbox);
	
	/* then add the hbox into the dialog */
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 4);
	
	/* creating OK button */
	button = gtk_button_new_with_label(_("O K"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_signal_connect_object(
		GTK_OBJECT(button),
		"clicked",
		GTK_SIGNAL_FUNC(gtk_widget_destroy),
		GTK_OBJECT(dialog));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);
	
	/* adding texts */
	i = 0;
	gtk_text_freeze(GTK_TEXT(text_area));
	gtk_widget_realize(text_area);
	while (text[i] != NULL)
	{
		gtk_text_insert(
			GTK_TEXT(text_area),
			(sizes[i]%2)?small_font:big_font,
			NULL, NULL,
			_(text[i]), strlen(_(text[i])));
		i++;
	}
	gtk_text_thaw(GTK_TEXT(text_area));
	
	return dialog;
}
