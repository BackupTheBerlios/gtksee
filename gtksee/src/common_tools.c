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
 * 2003-09-14: Modifications in:
 *             remove_file function
 *             remove_it   function
 *             rename_it   function
 * 2004-05-13: Added code for rename multiple files
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "common_tools.h"
#include "imagelist.h"
#include "gtksee.h"
#include "rc.h"
#include "rename_seq.h"

void     close_dialog   (GtkWidget *widget);
void     remove_it      (GtkWidget *widget, tool_parameters *param);
void     rename_it      (tool_parameters *param, GtkWidget *widget);

static tool_parameters   param;

void
close_gtksee()
{
   rc_save_gtkseerc();
   gtk_main_quit();
}

void
alert_dialog(gchar *myline)
{
   GtkWidget   *dialog, *label, *button;

   dialog = gtk_dialog_new();
   gtk_container_border_width(GTK_CONTAINER(dialog), 5);
   gtk_window_set_title(GTK_WINDOW(dialog), ("Ups!"));
   gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
   gtk_widget_set_usize(dialog, 200, 100);
   gtk_grab_add(dialog);

   label = gtk_label_new(myline);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 5);
   gtk_widget_show(label);

   button = gtk_button_new_with_label(_("ok"));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
   gtk_signal_connect_object(GTK_OBJECT(button),
      "clicked",
      GTK_SIGNAL_FUNC(close_dialog),
      (gpointer)dialog);
   GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default(button);
   gtk_widget_show(button);

   gtk_widget_show(dialog);
}

void
close_dialog(GtkWidget *widget)
{
   gtk_grab_remove(widget);
   gtk_widget_destroy(widget);
}

/* Function for remove files */
void
remove_it(GtkWidget *widget, tool_parameters *param)
{
   gchar       *buffer;
   ImageInfo   *info;
   GList       *buf;
   guint       focus_row, row_list, row = 0;
   gint        status;

#ifdef GTK_HAVE_FEATURES_1_1_0
   focus_row = GTK_CLIST(IMAGE_LIST(param->il)->child_list)->focus_row;
#endif
   buf       = param->selection;
   status    = 0;

   /* Delete the files */
   buffer = g_malloc(sizeof(gchar) * 1024);
   while (param->selection != NULL)
   {
      info = image_list_get_by_serial(IMAGE_LIST(param->il), (guint)(param->selection)->data);
      sprintf(buffer, "%s/%s", IMAGE_LIST(param->il)->dir,info->name);
      status |= unlink(buffer);

      if (info->serial < focus_row)
         row ++;

      param->selection = g_list_next(param->selection);
   }

   g_free(buffer);
   refresh_all();
   row_list = GTK_CLIST(IMAGE_LIST(param->il)->child_list)->rows;

#ifdef GTK_HAVE_FEATURES_1_1_0
   gtk_clist_unselect_row(GTK_CLIST(IMAGE_LIST(param->il)->child_list), focus_row, 0);
   focus_row = focus_row - row;

   if (row_list >= 0)
   {
      if (focus_row >= row_list - 1)
         focus_row = row_list - 1;
      GTK_CLIST(IMAGE_LIST(param->il)->child_list)->focus_row = focus_row;
      gtk_clist_select_row(GTK_CLIST(IMAGE_LIST(param->il)->child_list), focus_row, 0);
      gtk_clist_moveto(GTK_CLIST(IMAGE_LIST(param->il)->child_list), focus_row, 0, 0, 0);
   }
#else
   if (row_list >= 0)
   {
      gtk_clist_select_row(GTK_CLIST(IMAGE_LIST(param->il)->child_list), 0, 0);
      gtk_clist_moveto(GTK_CLIST(IMAGE_LIST(param->il)->child_list), focus_row, 0, 0, 0);
   }
#endif

   close_dialog(param->dialog);

   if (status == -1)
   {
      alert_dialog(_("Problem deleting file!"));
   }
}

void
remove_file(GtkWidget *il, GList *selection)
{
   gchar             *buf;
   GtkWidget         *dialog, *label, *button;
   tool_parameters   *selec;

   selec           = (gpointer) &param;
   selec->selection = (GList *) g_list_first(selection);

   dialog = gtk_dialog_new();
   gtk_container_border_width(GTK_CONTAINER(dialog), 5);
   gtk_window_set_title(GTK_WINDOW(dialog), _("Remove"));
   gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
   gtk_grab_add(dialog);
   selec->dialog = GTK_WIDGET(dialog);
   selec->il     = il;

   buf = g_malloc(sizeof(gchar) * 128);
   sprintf(buf, _("Do you really want to remove %i file(s) ?"), g_list_length(selection));
   label = gtk_label_new(buf);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 5);
   gtk_widget_show(label);
   g_free(buf);

   button = gtk_button_new_with_label(_("yes"));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
      TRUE, TRUE, 0);
   gtk_signal_connect(GTK_OBJECT(button),
      "clicked",
      GTK_SIGNAL_FUNC(remove_it),
      (gpointer)selec);
   GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default(button);
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

/* Function for rename files */
void
rename_it(tool_parameters *param, GtkWidget *widget)
{
   gchar        *buf, *fromfile, *tofile;
   ImageInfo    *info;
   gint         res;
   struct stat stbuf;

   fromfile = g_malloc(sizeof(gchar) * 1024);
   tofile   = g_malloc(sizeof(gchar) * 1024);
   buf      = g_malloc(sizeof(gchar) * 256);
   info     = image_list_get_by_serial(IMAGE_LIST(param->il), (guint)(param->selection)->data);

   buf = (gchar *) gtk_editable_get_chars(GTK_EDITABLE(param->entry), 0, -1);

   if ((res = (strncmp(buf, info->name, 256)!=0) ? 1:-1) != -1 &&
         (res = (strlen(buf)!=0) ? 2:-2) != -2)
   {
      sprintf(fromfile, "%s/%s", IMAGE_LIST(param->il)->dir, info->name);
      sprintf(tofile,   "%s/%s", IMAGE_LIST(param->il)->dir, buf);
      if (stat(tofile, &stbuf) == -1)
      {
         if (rename(fromfile, tofile) == 0)
         {
            strncpy(info->name, buf, 256);
            refresh_all();
         } else
         {
            alert_dialog(_("Problem in rename file!"));
         }
      } else
      {
         alert_dialog(_("File exist !"));
      }
   } else
   {
      switch (res)
      {
         case -1 :  alert_dialog(_("Same filename !"));
                     break;
         case -2 :  alert_dialog(_("No filename !"));
                     break;
         default :  break;
      }
   }

   g_free(fromfile);
   g_free(tofile);
   g_free(buf);

   close_dialog(param->dialog);
}

void
rename_file(GtkWidget *il, GList *selection)
{
   ImageInfo         *info;
   tool_parameters   *selec;
   GtkWidget         *dialog, *label, *button, *entry;
   gint              numberfiles;

   numberfiles = g_list_length(selection);
   if (numberfiles==1)
   {
      dialog = gtk_dialog_new();
      gtk_container_border_width(GTK_CONTAINER(dialog), 5);
      gtk_window_set_title(GTK_WINDOW(dialog), _("Rename"));
      gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
      gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
      gtk_grab_add(dialog);

      selec             = (gpointer) &param;
      selec->dialog     = GTK_WIDGET(dialog);
      selec->il         = il;
      selec->selection  = (GList *) selection;

      label = gtk_label_new(_("Rename file:"));
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 5);
      gtk_widget_show(label);

      info = image_list_get_by_serial(IMAGE_LIST(il), (guint)(selection)->data);
      entry = gtk_entry_new();
      gtk_entry_set_max_length(GTK_ENTRY(entry), 250);
      gtk_entry_set_text(GTK_ENTRY(entry), info->name);
      gtk_entry_set_editable(GTK_ENTRY(entry), TRUE);
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, TRUE, TRUE, 5);
      selec->entry = entry;
      gtk_signal_connect_object(GTK_OBJECT(entry),
                        "activate",
                        GTK_SIGNAL_FUNC(rename_it),
                        (gpointer)selec);
      gtk_widget_grab_focus(entry);
      gtk_widget_show(entry);

      button = gtk_button_new_with_label(_("ok"));
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
      gtk_signal_connect_object(GTK_OBJECT(button),
            "clicked",
            GTK_SIGNAL_FUNC(rename_it),
            (gpointer)selec);

      GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
      gtk_widget_grab_default(button);
      gtk_widget_show(button);

      button = gtk_button_new_with_label(_("cancel"));
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
      gtk_signal_connect_object(GTK_OBJECT(button),
         "clicked",
         GTK_SIGNAL_FUNC(close_dialog),
         (gpointer)dialog);
      gtk_widget_show(button);
      gtk_widget_show(dialog);

   } else
   {
      rename_serie(il, selection);
   }
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

gulong
buf2long(guchar *buf, gint n)
{
   gulong l;
   gulong byte_order = 1;

   if ((char) byte_order)
   {
      l  = (gulong) (buf[n+0] << 24);
      l |= (gulong) (buf[n+1] << 16);
      l |= (gulong) (buf[n+2] <<  8);
      l |= (gulong)  buf[n+3];
   } else
   {
      l  = (gulong)  buf[n+0] ;
      l |= (gulong) (buf[n+1] <<  8);
      l |= (gulong) (buf[n+2] << 16);
      l |= (gulong) (buf[n+3] << 24);
   }

   return (l);
}

gushort
buf2short(guchar *buf, gint n)
{
   gushort l;
   gulong  byte_order = 1;

   if ((char) byte_order)
   {
      l  = (gushort) (buf[n+0] << 8);
      l |= (gushort)  buf[n+1];
   } else
   {
      l  = (gushort)  buf[n+0];
      l |= (gushort) (buf[n+1] << 8);
   }

   return (l);
}
