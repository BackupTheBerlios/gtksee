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
#include "rc.h"
#include "viewertoolbar.h"

#include "pixmaps/browse.xpm"
#include "pixmaps/fullscreen.xpm"
#include "pixmaps/prev_image.xpm"
#include "pixmaps/next_image.xpm"
#include "pixmaps/slideshow.xpm"
#include "pixmaps/screen.xpm"
#include "pixmaps/rotate_left.xpm"
#include "pixmaps/rotate_right.xpm"
#include "pixmaps/refresh.xpm"

GtkWidget *prev_button, *next_button;
GtkWidget *slideshow_button;

static GdkColor*	get_viewer_tooltips_bgcolor	();

static GdkColor*
get_viewer_tooltips_bgcolor(GdkWindow *window, GdkColormap *colormap)
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

void
viewer_next_enable(gboolean e)
{
	gtk_widget_set_sensitive(next_button, e);
}

void
viewer_prev_enable(gboolean e)
{
	gtk_widget_set_sensitive(prev_button, e);
}

void
viewer_slideshow_set_state(gboolean e)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(slideshow_button), e);
}

GtkWidget*
get_viewer_toolbar(GtkWidget *parent)
{
	GtkWidget *toolbar, *pixmap_wid, *button;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkStyle *style;
	gboolean fit_screen;
	
	style = gtk_widget_get_style(parent);
	toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
	gtk_tooltips_set_colors(
		GTK_TOOLBAR(toolbar)->tooltips,
		get_viewer_tooltips_bgcolor(parent->window, gdk_window_get_colormap(parent->window)),
		&style->fg[GTK_STATE_NORMAL]);
	gtk_tooltips_set_delay(GTK_TOOLBAR(toolbar)->tooltips, 100);
	
	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)browse_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	gtk_toolbar_append_item(
		GTK_TOOLBAR(toolbar),
		NULL, _("Browse"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_browse),
		NULL);
	
	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)fullscreen_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	gtk_toolbar_append_item(
		GTK_TOOLBAR(toolbar),
		NULL, _("Full screen"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_full_screen),
		NULL);
	
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)prev_image_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	prev_button = gtk_toolbar_append_item(
		GTK_TOOLBAR(toolbar),
		NULL, _("Previous Image"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_prev_image),
		NULL);

	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)next_image_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	next_button = gtk_toolbar_append_item(
		GTK_TOOLBAR(toolbar),
		NULL, _("Next Image"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_next_image),
		NULL);
		
	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)slideshow_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	slideshow_button = gtk_toolbar_append_element(
		GTK_TOOLBAR(toolbar),
		GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
		NULL,
		NULL,
		_("Stop/Resume Slideshow"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_slideshow_toggled),
		NULL);
	
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
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
		GTK_SIGNAL_FUNC(viewer_toolbar_refresh),
		NULL);
	
	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)screen_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	button = gtk_toolbar_append_element(
		GTK_TOOLBAR(toolbar),
		GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
		NULL,
		NULL,
		_("Fit Screen"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_fitscreen_toggled),
		NULL);
	fit_screen = rc_get_boolean("fit_screen");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), fit_screen);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)rotate_left_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	gtk_toolbar_append_item(
		GTK_TOOLBAR(toolbar),
		NULL,
		_("Rotate -90"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_rotate_left),
		NULL);

	pixmap = gdk_pixmap_create_from_xpm_d(
		parent->window,
		&mask, &style->bg[GTK_STATE_NORMAL],
		(gchar **)rotate_right_xpm);
	pixmap_wid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmap_wid);
	gtk_toolbar_append_item(
		GTK_TOOLBAR(toolbar),
		NULL,
		_("Rotate +90"), NULL,
		pixmap_wid,
		GTK_SIGNAL_FUNC(viewer_toolbar_rotate_right),
		NULL);

	return toolbar;
}
