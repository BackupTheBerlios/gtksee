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
 * 2004-05-13: Added code for rename multiple files
 */
#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "common_tools.h"
#include "rename_seq.h"
#include "imagelist.h"
#include "gtksee.h"
#include "rc.h"

#define NORMAL    1
#define UPPER     2
#define LOWER     3

gchar use_old_ext;
gchar upperlower;
gint  control;

GtkWidget      *spinner;
GtkAdjustment  *adj;
GtkWidget      *entry_ren;
GtkWidget      *renameclist;
GtkWidget      *dialog;

void rename_all(gpointer il, gpointer button);
void add_filenames(GtkWidget *il, GList *selection);
void gtk_entry_changed(GtkEditable *entry_ch);

void
rename_all(gpointer il, gpointer button)
{
   ImageInfo   *info;
   gchar       *buf = NULL;
   gchar       *fromfile, *tofile;
   gint        i;
   gint        status;

   fromfile = g_malloc(sizeof(gchar) * 2048);
   tofile   = g_malloc(sizeof(gchar) * 2048);

   status = 0;
   for (i=0; gtk_clist_get_text(GTK_CLIST(renameclist), i, 0, &buf) == 1; i++)
   {
      sprintf(fromfile, "%s/%s", IMAGE_LIST(il)->dir, buf);
      gtk_clist_get_text(GTK_CLIST(renameclist), i, 1, &buf);
      sprintf(tofile,   "%s/%s", IMAGE_LIST(il)->dir, buf);
      info = image_list_get_by_serial(IMAGE_LIST(il), i);
      //strncpy(info->name, tofile, 256);
      status |= rename(fromfile, tofile);
   }

   close_dialog(dialog);

   if (status != 0)
   {
      alert_dialog(_("Problem renaming files!"));
   }

   g_free(fromfile);
   g_free(tofile);

   refresh_all();
}
void
add_filenames(GtkWidget *il, GList *selection)
{
   ImageInfo *info;
   gint      i;
   gchar     *cols[2];

   for (i=0; selection != NULL; i++)
   {
      info = image_list_get_by_serial(IMAGE_LIST(il), (guint)(selection)->data);
      cols[0] = g_strdup(info->name);
      cols[1] = NULL;
      gtk_clist_append(GTK_CLIST(renameclist), cols);
      g_free(cols[0]);
      selection = g_list_next(selection);
   }
}

void
remove_extension(gchar *buffer, gchar *str)
{
   gint   i, j;

   i = 0;
   for (j=0; str[j]; j++)
   {
      buffer[j] = str[j];
      if (str[j] == '.')
         i = j;
   }

   if (i == 0)
   {
      i = j;
   }

   buffer[i] = '\0';
}

void
get_extension(gchar *buffer, gchar *str)
{
   gint   i, j;

   i = 0;
   for (j=0; str[j]; j++)
      if (str[j] == '.')
         i = j;

   for (; str[i] && i; i++)
      *buffer++ = str[i];

   *buffer = str[j];
}

void
add_name(gchar *buffer, gint *j)
{
   buffer[(*j)++] = '%';
   buffer[*j  ] = 's';
}

void
add_number(gchar *buffer, gchar *str, gint *i, gint *j)
{
   gint  size, c, k, n;

   for (k=*i; str[k]=='#'; k++)
      ;

   n  = k - *i;
   *i = k - 1;

   buffer[(*j)++] = '%';
   buffer[(*j)++] = '0';

   size = 0;
   do
   {
      buffer[(*j)++] = n % 10 + '0';
      size++;
   } while((n /= 10) > 0);

   for(k = *j-size, n = *j-1; k < n; k++,n--)
   {
      c        = buffer[k];
      buffer[k]= buffer[n];
      buffer[n]= c;
   }

   buffer[*j] = 'i';
}

void
gtk_entry_changed(GtkEditable *entry_ch)
{
   gchar       *text;
   gchar       *textcell = NULL;
   gchar       *buffer;
   gchar       *nombre;
   gchar       *finalname;

   gchar       put_number, put_name;
   gint        inicio;
   gchar       pos_name = 0;
   gint        i, j, k;

   put_number = 0;
   put_name   = 0;

   text     = gtk_editable_get_chars(entry_ch, 0, -1);
   inicio   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinner));

   buffer   = (gchar *) g_malloc(sizeof(gchar) * 512);
   nombre   = (gchar *) g_malloc(sizeof(gchar) * 1024);
   finalname= (gchar *) g_malloc(sizeof(gchar) * 1024);

   for (i=0, j=0; text[i]; i++, j++)
   {
      switch(text[i])
      {
         case '@':      /* Template for old name */
            if (!put_name)
            {
               put_name = 1;
               if (put_number)
                  pos_name = 1;
               else
                  pos_name = 2;

               add_name(&buffer[0], &j);
            } else
            {
               buffer[j] = text[i];
            }
            break;

         case '#':      /* Template for number */
            if (!put_number)
            {
               put_number = 1;
               add_number(&buffer[0], text, &i, &j);
            } else
            {
               buffer[j] = text[i];
            }
            break;

         default:
            buffer[j] = text[i];
            break;
      }
   }

   if (!put_number)
   {
      i = 0;
      add_number(&buffer[0], "#\0", &i, &j);
      j ++;
   }
   buffer[j] = '\0';

   for (i=0; gtk_clist_get_text(GTK_CLIST(renameclist), i, 0, &textcell) == 1; i++)
   {
      if (use_old_ext)
      {
         get_extension(&buffer[j], textcell);
      }

      if (put_name)
      {
         remove_extension(finalname, textcell);
         switch (pos_name)
         {
            case 1:
               sprintf(nombre, buffer, inicio++, finalname);
               break;

            case 2:
               sprintf(nombre, buffer, finalname, inicio++);
               break;
         }
      } else
      {
         sprintf(nombre, buffer, inicio++);
      }

      switch (upperlower)
      {
         case UPPER:
            for (k=0; nombre[k]; k++)
               nombre[k] = toupper(nombre[k]);
            break;

         case LOWER:
            for (k=0; nombre[k]; k++)
               nombre[k] = tolower(nombre[k]);
            break;
      }

      gtk_clist_set_text(GTK_CLIST(renameclist), i, 1, nombre);
   }

   g_free(finalname);
   g_free(nombre);
   g_free(buffer);
   g_free(text);

   return;
}

void
gtk_spin_changed(GtkSpinButton *spin, gpointer data)
{
   int   i;

   i = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data));
   if (control != i)
   {
      control = i;
      gtk_entry_changed(GTK_EDITABLE(entry_ren));
   }
}

void
change_state()
{
   use_old_ext = !use_old_ext;
   gtk_entry_changed(GTK_EDITABLE(entry_ren));
}

void
name2normal(GtkWidget *widget, gpointer data)
{
   if (upperlower != NORMAL)
   {
      upperlower = NORMAL;
      gtk_entry_changed(GTK_EDITABLE(entry_ren));
   }
}

void
name2upper(GtkWidget *widget, gpointer data)
{
   if (upperlower != UPPER)
   {
      upperlower = UPPER;
      gtk_entry_changed(GTK_EDITABLE(entry_ren));
   }
}

void
name2lower(GtkWidget *widget, gpointer data)
{
   if (upperlower != LOWER)
   {
      upperlower = LOWER;
      gtk_entry_changed(GTK_EDITABLE(entry_ren));
   }
}

void
rename_serie(GtkWidget *il, GList *selection)
{
   GtkWidget *frame, *label;
   GtkWidget *vbox, *hbox;
   GtkWidget *scrolled_window;
   GtkWidget *check_button;
   GtkWidget *button;
   GtkWidget *r_button;
   GSList    *group;
   gchar     *titles[2] = { _("Old Name"), _("New Name") };

   use_old_ext = FALSE;
   upperlower  = NORMAL;
   control     = -1;

   dialog = gtk_dialog_new();
   gtk_container_border_width(GTK_CONTAINER(dialog), 5);
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
   gtk_window_set_title(GTK_WINDOW(dialog), _("Rename Files..."));
   gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, TRUE);
   gtk_grab_add(dialog);

   /* First container */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 5);
   gtk_widget_show(frame);

   vbox = gtk_vbox_new(FALSE, 10);
   gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   gtk_widget_show(vbox);

   /* Create the label of template */
   label = gtk_label_new(_("Use # for insert number.\nUse @ for insert old name."));
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
   gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);

   /* Create the entry_ren for template */
   entry_ren = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY(entry_ren), 128);
   gtk_entry_append_text(GTK_ENTRY(entry_ren), "name_#");
   gtk_box_pack_start(GTK_BOX(vbox), entry_ren, TRUE, TRUE, 0);
   gtk_widget_grab_focus(entry_ren);
   gtk_widget_show(entry_ren);

   /* Other container */
   hbox = gtk_hbox_new(FALSE, 5);
   gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
   gtk_container_add(GTK_CONTAINER(vbox), hbox);
   gtk_widget_show(hbox);

   /* Create the label Start */
   label = gtk_label_new(_("Start"));
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
   gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
   gtk_widget_show(label);

   /* Create the spin button */
   adj = (GtkAdjustment *) gtk_adjustment_new (0, 0, 256.0, 1.0,
                     5.0, 0.0);
   spinner = gtk_spin_button_new (adj, 0, 0);
   gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON (spinner), GTK_UPDATE_IF_VALID);
   gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
   gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
   gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinner), GTK_SHADOW_OUT);
   gtk_signal_connect (GTK_OBJECT (spinner), "changed",
                        (GtkSignalFunc) gtk_spin_changed,
                        (gpointer) spinner);
   gtk_box_pack_start(GTK_BOX(hbox), spinner, TRUE, TRUE, 0);
   gtk_widget_show(spinner);

   /* Create the toggle button */
   check_button = gtk_toggle_button_new_with_label(_("Use extension of the origin file"));
   gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
                       GTK_SIGNAL_FUNC (change_state),
                       NULL);
   gtk_box_pack_end(GTK_BOX(hbox), check_button, TRUE, TRUE, 0);
   gtk_widget_show (check_button);

   /* Create uppercase, lowercase and normal buttons */
   hbox = gtk_hbox_new(FALSE, 5);
   gtk_container_add(GTK_CONTAINER(vbox), hbox);
   gtk_widget_show(hbox);

   r_button = gtk_radio_button_new_with_label (NULL, _("Normal"));
   gtk_box_pack_start (GTK_BOX (hbox), r_button, TRUE, TRUE, 0);
   gtk_widget_show (r_button);
   gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(r_button), TRUE);
   gtk_signal_connect (GTK_OBJECT (r_button), "toggled",
            GTK_SIGNAL_FUNC (name2normal),
            NULL);

   group = gtk_radio_button_group (GTK_RADIO_BUTTON (r_button));

   r_button = gtk_radio_button_new_with_label(group, _("Uppercase"));
   gtk_box_pack_start (GTK_BOX (hbox), r_button, TRUE, TRUE, 0);
   gtk_widget_show (r_button);
   gtk_signal_connect (GTK_OBJECT (r_button), "toggled",
            GTK_SIGNAL_FUNC (name2upper),
            NULL);

   group = gtk_radio_button_group (GTK_RADIO_BUTTON (r_button));

   r_button = gtk_radio_button_new_with_label(group, _("Lowercase"));
   gtk_box_pack_start (GTK_BOX (hbox), r_button, TRUE, TRUE, 0);
   gtk_widget_show (r_button);
   gtk_signal_connect (GTK_OBJECT (r_button), "toggled",
            GTK_SIGNAL_FUNC (name2lower),
            NULL);

   /* Create a scrolled window to pack the CList widget into */
   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, FALSE, FALSE, 0);
   gtk_widget_show (scrolled_window);

   /* Create the CList */
   renameclist = gtk_clist_new_with_titles( 2, titles);

   gtk_clist_set_shadow_type (GTK_CLIST(renameclist), GTK_SHADOW_ETCHED_OUT);
   gtk_clist_set_column_auto_resize (GTK_CLIST(renameclist), 0, TRUE);
   gtk_clist_set_column_auto_resize (GTK_CLIST(renameclist), 1, TRUE);
   gtk_widget_set_usize(GTK_WIDGET(renameclist), 350, 150);
   gtk_clist_column_titles_passive(GTK_CLIST(renameclist));

   /* Add the CList widget to the vertical box and show it. */
   gtk_container_add(GTK_CONTAINER(scrolled_window), renameclist);
   add_filenames(il, selection);
   gtk_widget_show(renameclist);

   /* If change the entry, change the clist */
   gtk_signal_connect (GTK_OBJECT (entry_ren), "changed",
                        (GtkSignalFunc) gtk_entry_changed,
                        NULL);

   /* Create the buttons */
   button = gtk_button_new_with_label(_("O K"));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
   gtk_signal_connect_object(GTK_OBJECT(button),
         "clicked",
         GTK_SIGNAL_FUNC(rename_all),
         (gpointer)il);
   gtk_widget_show(button);

   button = gtk_button_new_with_label(_("Cancel"));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
   gtk_signal_connect_object(GTK_OBJECT(button),
         "clicked",
         GTK_SIGNAL_FUNC(close_dialog),
         (gpointer)dialog);
   gtk_widget_show(button);

   gtk_spin_changed(GTK_SPIN_BUTTON(spinner), spinner);

   gtk_widget_show(dialog);

   return;
}
