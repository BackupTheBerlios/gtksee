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

#include "intl.h"
#include "gtypes.h"
#include "gtkseetoolbar.h"
#include "gtksee.h"
#include "common_tools.h"
#include "rc.h"

#include "pixmaps/parent_folder.xpm"
#include "pixmaps/refresh.xpm"
#include "pixmaps/view.xpm"
#include "pixmaps/remove.xpm"
#include "pixmaps/rename.xpm"
#include "pixmaps/about.xpm"
#include "pixmaps/thumbnails.xpm"
#include "pixmaps/timestamp.xpm"
#include "pixmaps/details.xpm"
#include "pixmaps/small_icons.xpm"

static GdkColor*  get_main_tooltips_bgcolor  ();

static tool_parameters  params;
static GtkWidget        *remove_button, *rename_button, *view_button;
static GtkWidget        *thumbnails_button, *small_icons_button, *details_button;
static GtkWidget        *timestamp_button;

void
toolbar_remove_enable(gboolean e)
{
   gtk_widget_set_sensitive(remove_button, e);
}

void
toolbar_rename_enable(gboolean e)
{
   gtk_widget_set_sensitive(rename_button, e);
}

void
toolbar_timestamp_enable(gboolean e)
{
   gtk_widget_set_sensitive(timestamp_button, e);
}

void
toolbar_view_enable(gboolean e)
{
   gtk_widget_set_sensitive(view_button, e);
}

static GdkColor*
get_main_tooltips_bgcolor(GdkWindow *window, GdkColormap *colormap)
{
   static GdkColor color;
   static gboolean alloced = FALSE;
   if (!alloced)
   {
      color.red = 61669;
      color.green = 59113;
      color.blue = 35979;
      color.pixel = 0;
      gdk_color_alloc(colormap, &color);
      alloced = TRUE;
   }
   return &color;
}

GtkWidget*
get_main_toolbar(GtkWidget *parent)
{
   GtkWidget *toolbar, *pixmap_wid;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkStyle *style;

   style    = gtk_widget_get_style(parent);
   toolbar  = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
   gtk_tooltips_set_colors(GTK_TOOLBAR(toolbar)->tooltips,
                     get_main_tooltips_bgcolor(parent->window, gdk_window_get_colormap(parent->window)),
                     &style->fg[GTK_STATE_NORMAL]);

   gtk_tooltips_set_delay(GTK_TOOLBAR(toolbar)->tooltips, 100);

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)parent_folder_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);

   gtk_toolbar_append_item(
                     GTK_TOOLBAR(toolbar),
                     NULL, _("Up One Level"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(toolbar_cdup),
                     NULL);
   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)refresh_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);

   gtk_toolbar_append_item(
                     GTK_TOOLBAR(toolbar),
                     NULL, _("Refresh"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(toolbar_refresh),
                     NULL);

   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)view_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);

   view_button = gtk_toolbar_append_item(
                     GTK_TOOLBAR(toolbar),
                     NULL, _("View"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(toolbar_view),
                     NULL);

   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)remove_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);

   remove_button = gtk_toolbar_append_item(
                     GTK_TOOLBAR(toolbar),
                     NULL, _("Remove"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(menu_edit_delete),
                     (gpointer)&params);

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)rename_xpm);

   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);

   rename_button = gtk_toolbar_append_item(
                     GTK_TOOLBAR(toolbar),
                     NULL, _("Rename"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(menu_edit_rename),
                     (gpointer)&params);

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)timestamp_xpm);

   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);

   timestamp_button = gtk_toolbar_append_item(
                        GTK_TOOLBAR(toolbar),
                        NULL, _("Timestamp"), NULL,
                        pixmap_wid,
                        GTK_SIGNAL_FUNC(menu_edit_timestamp),
                        (gpointer)&params);

   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)thumbnails_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);
   thumbnails_button = gtk_toolbar_append_element(
                     GTK_TOOLBAR(toolbar),
                     GTK_TOOLBAR_CHILD_RADIOBUTTON,
                     NULL,
                     NULL,
                     _("Thumbnails"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(toolbar_thumbnails),
                     NULL);

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)small_icons_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);
   small_icons_button = gtk_toolbar_append_element(
                     GTK_TOOLBAR(toolbar),
                     GTK_TOOLBAR_CHILD_RADIOBUTTON,
                     thumbnails_button,
                     NULL,
                     _("Small icons"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(toolbar_small_icons),
                     NULL);

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)details_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);
   details_button = gtk_toolbar_append_element(
                     GTK_TOOLBAR(toolbar),
                     GTK_TOOLBAR_CHILD_RADIOBUTTON,
                     thumbnails_button,
                     NULL,
                     _("Details"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(toolbar_details),
                     NULL);

   toolbar_set_list_type(rc_get_int("image_list_type"));

   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

   pixmap = gdk_pixmap_create_from_xpm_d(
                     parent->window,
                     &mask, &style->bg[GTK_STATE_NORMAL],
                     (gchar **)about_xpm);
   pixmap_wid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmap_wid);
   gtk_toolbar_append_item(
                     GTK_TOOLBAR(toolbar),
                     NULL, _("About"), NULL,
                     pixmap_wid,
                     GTK_SIGNAL_FUNC(toolbar_about),
                     NULL);

   return toolbar;
}

void
toolbar_set_list_type(ImageListType type)
{
   switch (type)
   {
      case IMAGE_LIST_THUMBNAILS:
         gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(thumbnails_button), TRUE);
         break;
      case IMAGE_LIST_SMALL_ICONS:
         gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(small_icons_button), TRUE);
         break;
      case IMAGE_LIST_DETAILS:
      default:
         gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(details_button), TRUE);
         break;
   }
}

