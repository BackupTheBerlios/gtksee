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

#include "gtypes.h"
#include "scanline.h"
#include "imagesiitem.h"

static void	image_siitem_class_init		(ImageSiItemClass *klass);
static void	image_siitem_init		(ImageSiItem *ii);
static void	image_siitem_draw		(GtkWidget *widget,
						 GdkRectangle *area);
static gint	image_siitem_expose		(GtkWidget *widget,
						 GdkEventExpose *event);
static void	image_siitem_size_request	(GtkWidget *widget,
						 GtkRequisition *requisition);
static void	image_siitem_paint		(GtkWidget *widget,
						 GdkRectangle *area);
static void	image_siitem_realize		(GtkWidget *widget);

static void
image_siitem_class_init(ImageSiItemClass *klass)
{
	GtkWidgetClass *widget_class;
	
	widget_class = (GtkWidgetClass*) klass;
	
	widget_class->draw = image_siitem_draw;
	widget_class->size_request = image_siitem_size_request;
	widget_class->realize = image_siitem_realize;
	widget_class->expose_event = image_siitem_expose;
}

static void
image_siitem_init(ImageSiItem *ii)
{
	ii -> pixmap = NULL;
	ii -> mask = NULL;
	ii -> label[0] = '\0';
	ii -> color_set = FALSE;
	ii -> bg_gc = NULL;
}

guint
image_siitem_get_type()
{
	static guint ii_type = 0;

	if (!ii_type)
	{
		GtkTypeInfo ii_info =
		{
			"ImageSiItem",
			sizeof (ImageSiItem),
			sizeof (ImageSiItemClass),
			(GtkClassInitFunc) image_siitem_class_init,
			(GtkObjectInitFunc) image_siitem_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};
		ii_type = gtk_type_unique (gtk_widget_get_type(), &ii_info);
	}
	return ii_type;
}

GtkWidget*
image_siitem_new(GdkPixmap *pixmap, GdkBitmap *mask, guchar *label)
{
	ImageSiItem *ii;
	
	ii = IMAGE_SIITEM(gtk_type_new(image_siitem_get_type()));
	ii -> pixmap = pixmap;
	ii -> mask = mask;
	strcpy(ii->label, label);
	return GTK_WIDGET(ii);
}

void
image_siitem_set_color(ImageSiItem *ii, GdkColor *color)
{
	if (color == NULL)
	{
		ii->color_set = FALSE;
	} else
	{
		ii->color_set = TRUE;
		ii->color.pixel = color->pixel;
	}
	gtk_widget_queue_draw(GTK_WIDGET(ii));
}

void
image_siitem_destroy(ImageSiItem *ii)
{
	if (ii->bg_gc != NULL) gdk_gc_destroy(ii->bg_gc);
	gtk_widget_destroy(GTK_WIDGET(ii));
}

static void
image_siitem_draw(GtkWidget *widget, GdkRectangle *area)
{
	if (GTK_WIDGET_DRAWABLE(widget))
	{
		image_siitem_paint(widget, area);
		gtk_widget_draw_default(widget);
		gtk_widget_draw_focus(widget);
	}
}

static void
image_siitem_paint(GtkWidget *widget, GdkRectangle *area)
{
	ImageSiItem *item;
	
	if (GTK_WIDGET_DRAWABLE(widget))
	{
		item = IMAGE_SIITEM(widget);
		
		/* clear area */
		gtk_style_set_background(
			widget->style,
			widget->window,
			GTK_STATE_NORMAL);
		gdk_window_clear_area(widget->window,
			area->x, area->y, area->width, area->height);
		
		/* fill label with bg_gc if needed */
		if (GTK_WIDGET_STATE(widget) == GTK_STATE_SELECTED)
		{
			gdk_draw_rectangle(
				widget->window,
				widget->style->bg_gc[GTK_STATE_SELECTED],
				TRUE,
				17, 0,
				1+gdk_string_width(widget->style->font, item->label),
				16);
		} else if (item->color_set)
		{
			gdk_gc_set_foreground(item->bg_gc, &item->color);
			gdk_draw_rectangle(
				widget->window,
				item->bg_gc,
				TRUE,
				17, 0,
				1+gdk_string_width(widget->style->font, item->label),
				16);
		}
		
		/* draw pixmap */
		if (item->pixmap != NULL)
		{
			gdk_draw_pixmap(
				widget->window,
				widget->style->bg_gc[GTK_STATE_NORMAL],
				item->pixmap,
				0, 0,
				0, 0,
				-1, -1);
		}
		
		/* draw label */
		if (strlen(item->label) < 1) return;
		gdk_draw_string(
			widget->window,
			widget->style->font,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			17,
			widget->style->font->ascent,
			item->label);
	}
}

static void
image_siitem_realize(GtkWidget *widget)
{
	ImageSiItem *item;
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_IMAGE_SIITEM(widget));

	item = IMAGE_SIITEM(widget);
	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget);

	attributes.event_mask |= (
		GDK_EXPOSURE_MASK |
		GDK_BUTTON_PRESS_MASK |
		GDK_BUTTON_RELEASE_MASK |
		GDK_ENTER_NOTIFY_MASK |
		GDK_LEAVE_NOTIFY_MASK);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, item);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);

	item->bg_gc = gdk_gc_new(widget->window);
}

static gint
image_siitem_expose(GtkWidget *widget, GdkEventExpose *event)
{
	ImageSiItem *item;
	
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(IS_IMAGE_SIITEM(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);
	
	if (GTK_WIDGET_DRAWABLE(widget))
	{
		item = IMAGE_SIITEM(widget);
		image_siitem_paint(widget, &event->area);
		gtk_widget_draw_default(widget);
		gtk_widget_draw_focus(widget);
	}
	return FALSE;
}

static void
image_siitem_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	ImageSiItem *item = IMAGE_SIITEM(widget);
	
	requisition->width = 17
		+ gdk_string_width(widget->style->font, item->label);
	requisition->height = 16;
}

void
image_siitem_set_pixmap(ImageSiItem *ii, GdkPixmap *pixmap, GdkBitmap *mask)
{
	ii->pixmap = pixmap;
	ii->mask = mask;
	gtk_widget_queue_draw(GTK_WIDGET(ii));
}
