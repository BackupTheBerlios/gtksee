/*
 * GTK See -- an image viewer based on GTK+
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

#include "gtkseestatus.h"

typedef struct _PData {
      GtkWidget   *pbar;
      GtkWidget   *label;
      guchar      counter;
} ProgressData;

GtkWidget      *status_main; /* GtkLabel     */
GtkWidget      *status_file; /* GtkLabel     */
GtkWidget      *status_type; /* GtkPixmap    */
GtkWidget      *status_name; /* GtkLabel     */
GtkWidget      *status_prop; /* GtkLabel     */
ProgressData   *progress;    /* Progress Bar */

void
start_progress_data()
{
   gtk_widget_hide(GTK_WIDGET(progress->label));
   gtk_widget_show(GTK_WIDGET(progress->pbar));
   progress->counter = 0;
}

void
stop_progress_data()
{
   gtk_widget_hide(GTK_WIDGET(progress->pbar));
   gtk_widget_show(GTK_WIDGET(progress->label));
}

void
update_progress_data(gulong count, gulong counter_end)
{
   gfloat   value;
   gint     pct;

   if (counter_end)
   {
      value = (gfloat) count / (gfloat) counter_end;
      pct   = 100 * value;
      if (progress->counter != pct)
      {
         gtk_progress_set_percentage(GTK_PROGRESS(progress->pbar), value);
         while (gtk_events_pending()) gtk_main_iteration();
         progress->counter = pct;
      }
   }
}

void
set_status_main(gchar *text)
{
   gtk_label_set(GTK_LABEL(status_main), text);
}

void
set_status_file(gchar *text)
{
   gtk_label_set(GTK_LABEL(status_file), text);
}

void
set_status_type(GdkPixmap *pixmap, GdkBitmap *mask)
{
   gtk_widget_hide(status_type);
   if (pixmap != NULL)
   {
      gtk_widget_hide(status_type);
      gtk_pixmap_set(GTK_PIXMAP(status_type), pixmap, mask);
      gtk_widget_show(status_type);
   }
}

void
set_status_name(gchar *text)
{
   gtk_label_set(GTK_LABEL(status_name), text);
}

void
set_status_prop(gchar *text)
{
   gtk_label_set(GTK_LABEL(status_prop), text);
}

GtkWidget*
get_main_status_bar(GdkPixmap *pixmap, GdkBitmap *mask)
{
   GtkWidget *main_hbox, *hbox;
   GtkWidget *frame;
   GtkStyle  *style;

   /* Create container base */
   main_hbox = gtk_hbox_new(FALSE, 2);

   /* creating status_main */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
   gtk_widget_show(frame);

   /* Allocate memory for the progress data */
   progress = g_malloc(sizeof(ProgressData));

   /* Create the progress bar */
   progress->pbar = gtk_progress_bar_new();
   gtk_widget_set_usize(progress->pbar, 0, 8);
   style = gtk_style_copy(gtk_widget_get_style(GTK_WIDGET(progress->pbar)));
   style->bg[0].pixel = 1;
   style->bg[0].blue  = 0x00;
   style->bg[0].green = 0x00;
   style->bg[0].red   = 0xD000;
   gtk_widget_set_style(GTK_WIDGET(progress->pbar), style);

   /* Create the status label */
   status_main = gtk_label_new("");
   gtk_label_set_justify(GTK_LABEL(status_main), GTK_JUSTIFY_LEFT);
   gtk_misc_set_alignment(GTK_MISC(status_main), 0, 0);
   gtk_widget_set_usize(status_main, 150, 16);

   /* Save the address of status_main */
   progress->label = status_main;

   /* Widget container of progress bar and status main */
   hbox = gtk_vbox_new (FALSE, 0);

   gtk_container_add (GTK_CONTAINER (hbox), progress->pbar);
   gtk_widget_hide(progress->pbar);
   gtk_container_add(GTK_CONTAINER (hbox), status_main);
   gtk_widget_show (status_main);

   gtk_container_add(GTK_CONTAINER (frame), hbox);
   gtk_widget_show (hbox);

   /* Pack the frame */
   gtk_box_pack_start(GTK_BOX(main_hbox), frame, FALSE, FALSE, 0);

   /* creating status_file */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
   gtk_widget_show(frame);

   status_file = gtk_label_new("");
   gtk_label_set_justify(GTK_LABEL(status_file), GTK_JUSTIFY_LEFT);
   gtk_misc_set_alignment(GTK_MISC(status_file), 0, 0);
   gtk_widget_set_usize(status_file, 180, 16);
   gtk_container_add(GTK_CONTAINER (frame), status_file);
   gtk_widget_show (status_file);

   gtk_box_pack_start(GTK_BOX(main_hbox), frame, FALSE, FALSE, 0);

   /* creating status_type & status_name */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
   gtk_widget_show(frame);

   hbox = gtk_hbox_new(FALSE, 1);
   gtk_widget_show(hbox);

   status_type = gtk_pixmap_new(pixmap, mask);
   gtk_box_pack_start(GTK_BOX(hbox), status_type, FALSE, FALSE, 0);
   gtk_widget_show(status_type);

   status_name = gtk_label_new("");
   gtk_label_set_justify(GTK_LABEL(status_name), GTK_JUSTIFY_LEFT);
   gtk_misc_set_alignment(GTK_MISC(status_name), 0, 0);
   gtk_widget_set_usize(status_name, 150, 16);
   gtk_box_pack_start(GTK_BOX(hbox), status_name, TRUE, TRUE, 0);
   gtk_widget_show (status_name);

   gtk_container_add(GTK_CONTAINER(frame), hbox);

   gtk_box_pack_start(GTK_BOX(main_hbox), frame, TRUE, TRUE, 0);

   /* creating status_prop */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
   gtk_widget_show(frame);

   status_prop = gtk_label_new("");
   gtk_label_set_justify(GTK_LABEL(status_prop), GTK_JUSTIFY_LEFT);
   gtk_misc_set_alignment(GTK_MISC(status_prop), 0, 0);
   gtk_widget_set_usize(status_prop, 120, 16);
   gtk_container_add(GTK_CONTAINER (frame), status_prop);
   gtk_widget_show (status_prop);

   gtk_box_pack_start(GTK_BOX(main_hbox), frame, FALSE, FALSE, 0);

   return main_hbox;
}
