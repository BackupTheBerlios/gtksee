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

 /*
  * Code based in touch.c from fileutils package
  * Copyright (C) 1989, 1990, 1991, 1998, 2000 Free Software Foundation Inc.
  */
#include "config.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <utime.h>

#include "intl.h"
#include "gtksee.h"
#include "common_tools.h"
#include "imagelist.h"

GtkWidget      *dialog;
GtkWidget      *combo;
GtkWidget      *spinnerday;
GtkWidget      *spinnermonth;
GtkWidget      *spinneryear;
GtkWidget      *spinnerhour;
GtkWidget      *spinnermin;
GtkWidget      *hboxdate, *hboxtime;
GtkAdjustment  *adj;
gboolean       current_datetime;

/* Handle 12 digits worth of `MMDDhhmmYYYY'.  */
time_t
posixtime (const char *s)
{
   int         len;
   int         pair[6];
   int         *p;
   int         i;
   struct tm   t;

   len = 6;
   for (i = 0; i < len; i++)
      pair[i] = 10 * (s[2*i] - '0') + s[2*i + 1] - '0';

   p = pair;

   t.tm_mon = *p; ++p; --len;

   t.tm_mday= *p; ++p; --len;

   t.tm_hour= *p; ++p; --len;

   t.tm_min = *p; ++p; --len;

   t.tm_year= (p[0] * 100 + p[1]) - 1900;

   t.tm_sec  = 0;

   t.tm_isdst= -1;

   return mktime (&t);
}

/*
   value : the initial value.
   lower : the minimum value.
   upper : the maximum value.
   step_increment : the step increment.
   page_increment : the page increment.
   page_size : the page size.
 */
GtkWidget *
generate_spin(gfloat value, gfloat lower, gfloat upper,
               gfloat step_increment, gfloat page_increment,
               gfloat page_size)
{
   GtkWidget      *spinner;
   GtkAdjustment  *adj;

   adj = (GtkAdjustment *) gtk_adjustment_new (value, lower, upper,
                                                step_increment,
                                                page_increment,
                                                page_size);
   spinner = gtk_spin_button_new (adj, 0, 0);
   gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON (spinner),
                                       GTK_UPDATE_IF_VALID);
   gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
   gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
   gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinner), GTK_SHADOW_OUT);

   return spinner;
}

void
enable_spin_datetime(gboolean e)
{
   gtk_widget_set_sensitive(hboxdate, e);
   gtk_widget_set_sensitive(hboxtime, e);
}

void
current_date_time()
{
   enable_spin_datetime(FALSE);
   current_datetime = TRUE;
}

void
select_date_time()
{
   enable_spin_datetime(TRUE);
   current_datetime = FALSE;
}

void
change_datetime(gpointer il, gpointer button)
{
   ImageInfo      *info;
   gchar          *buf;
   gint           status;
   GList          *selection;
   time_t         t;
   struct utimbuf utb;

   selection = image_list_get_selection (IMAGE_LIST(il));
   selection = g_list_first(selection);
   
   status = 0;
   buf = g_malloc(sizeof(gchar) * 2048);

   if (current_datetime)
   {
      t = time (0);
   } else
   {
      /* Handle 12 digits worth of `MMDDhhmmYYYY'.  */
      sprintf(buf, "%02i%02i%02i%02i%04i",
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinnermonth))-1,
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinnerday))  ,
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinnerhour)) ,
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinnermin))  ,
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinneryear))
            );
      t = posixtime(buf);
   }

   utb.actime  =
   utb.modtime = t;

   while (selection != NULL)
   {
      info = image_list_get_by_serial(IMAGE_LIST(il), (guint)selection->data);
      sprintf(buf, "%s/%s", IMAGE_LIST(il)->dir, info->name);
      status |= utime (buf, &utb);
      selection = g_list_next(selection);
   }

   close_dialog(dialog);
   
   if (status)
   {
      alert_dialog(_("Problem changing date and time of files!"));
   }

   g_free(buf);
   refresh_all();
}

void
timestamp_file(GtkWidget *il, GList *selection)
{
   GtkWidget *frame;
   GtkWidget *r_button;
   GtkWidget *label;
   GSList    *group = NULL;
   GtkWidget *vbox;
   GtkWidget *button;

   current_datetime = TRUE;

   dialog = gtk_dialog_new();
   gtk_container_border_width(GTK_CONTAINER(dialog), 5);
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
   gtk_window_set_title(GTK_WINDOW(dialog), _("Change timestamp..."));
   gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, TRUE);
   gtk_grab_add(dialog);

   /* First container */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 5);

   vbox = gtk_vbox_new(FALSE, 10);
   gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
   gtk_container_add(GTK_CONTAINER(frame), vbox);

   /* Add radio button for select timestamp type */
   r_button = gtk_radio_button_new_with_label (NULL, _("Current Date and Time"));
   gtk_box_pack_start (GTK_BOX (vbox), r_button, TRUE, TRUE, 0);
   gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(r_button), TRUE);
   gtk_signal_connect (GTK_OBJECT (r_button), "toggled",
            GTK_SIGNAL_FUNC (current_date_time),
            NULL);

   group = gtk_radio_button_group (GTK_RADIO_BUTTON (r_button));

   r_button = gtk_radio_button_new_with_label(group, _("Select Date and Time"));
   gtk_box_pack_start (GTK_BOX (vbox), r_button, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT (r_button), "toggled",
            GTK_SIGNAL_FUNC (select_date_time),
            NULL);

   /* Date options */
   hboxdate = gtk_hbox_new(FALSE, 10);
   gtk_container_set_border_width(GTK_CONTAINER(hboxdate), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hboxdate, TRUE, TRUE, 0);

   label = gtk_label_new(_("Date:"));
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
   gtk_box_pack_start(GTK_BOX(hboxdate), label, FALSE, FALSE, 0);

   /* Create the spin day button */
   spinnerday = generate_spin(1, 1, 31.0, 1.0, 5.0, 0);
   gtk_box_pack_start(GTK_BOX(hboxdate), spinnerday, FALSE, TRUE, 0);

   /* Create the spin month */
   spinnermonth = generate_spin(1, 1, 12.0, 1.0, 3.0, 0);
   gtk_box_pack_start(GTK_BOX(hboxdate), spinnermonth, FALSE, TRUE, 0);

   /* Create the spin year button */
   spinneryear = generate_spin(1970.0, 1970.0, 2030.0, 1.0, 50.0, 0);
   gtk_widget_set_usize(GTK_WIDGET(spinneryear), 60, 0);
   gtk_box_pack_start(GTK_BOX(hboxdate), spinneryear, TRUE, TRUE, 0);

   /* Time options */
   hboxtime = gtk_hbox_new(FALSE, 10);
   gtk_container_set_border_width(GTK_CONTAINER(hboxtime), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hboxtime, TRUE, TRUE, 0);

   label = gtk_label_new(_("Time:"));
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
   gtk_box_pack_start(GTK_BOX(hboxtime), label, FALSE, FALSE, 0);

   /* Create the spin hour button */
   spinnerhour = generate_spin(0, 0, 23.0, 1.0, 5.0, 0);
   gtk_box_pack_start(GTK_BOX(hboxtime), spinnerhour, FALSE, TRUE, 0);


   /* Create the spin min button */
   spinnermin = generate_spin(0, 0, 59.0, 1.0, 5.0, 0);
   gtk_box_pack_start(GTK_BOX(hboxtime), spinnermin, FALSE, TRUE, 0);

   /* Create the buttons */
   button = gtk_button_new_with_label(_("O K"));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
   gtk_signal_connect_object(GTK_OBJECT(button),
         "clicked",
         GTK_SIGNAL_FUNC(change_datetime),
         (gpointer)il);
   gtk_widget_show(button);

   button = gtk_button_new_with_label(_("Cancel"));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
   gtk_signal_connect_object(GTK_OBJECT(button),
         "clicked",
         GTK_SIGNAL_FUNC(gtk_widget_destroy),
         (gpointer)dialog);

   enable_spin_datetime(FALSE);

   gtk_widget_show_all(dialog);

   return;
}
