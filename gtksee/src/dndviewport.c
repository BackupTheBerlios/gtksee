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

#include "dndviewport.h"

#define min(a,b) (((a)<(b))?(a):(b))

static DndViewportClass *this_class = NULL;
static GdkCursor *hand_cursor = NULL;

enum
{
	POPUP_SIGNAL,
	DOUBLE_CLICKED_SIGNAL,
	MAX_SIGNALS
};

static gint dnd_viewport_signals[MAX_SIGNALS];

static void	dnd_viewport_class_init		(DndViewportClass *class);
static void	dnd_viewport_init		(DndViewport *viewport);
static void	dnd_viewport_realize		(GtkWidget *widget);

static void	dnd_viewport_press		(GtkWidget *widget,
						 GdkEvent *event,
						 gpointer data);
static void	dnd_viewport_motion		(GtkWidget *widget,
						 GdkEvent *event,
						 gpointer data);
static void	dnd_viewport_release		(GtkWidget *widget,
						 GdkEvent *event,
						 gpointer data);
static void	dnd_viewport_do_move		(DndViewport *viewport);

static void
dnd_viewport_class_init(DndViewportClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;
	
	widget_class = (GtkWidgetClass*)klass;
	object_class = (GtkObjectClass*)klass;
	this_class = klass;
	hand_cursor = gdk_cursor_new(GDK_HAND2);
	
	dnd_viewport_signals[POPUP_SIGNAL] = gtk_signal_new ("popup",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (DndViewportClass, popup),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	dnd_viewport_signals[DOUBLE_CLICKED_SIGNAL] = gtk_signal_new ("double_clicked",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (DndViewportClass, popup),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, dnd_viewport_signals, MAX_SIGNALS);
	
	klass->parent_realize = widget_class->realize;
	klass->popup = NULL;
	klass->double_clicked = NULL;
	widget_class->realize = dnd_viewport_realize;
}

static void
dnd_viewport_init(DndViewport *viewport)
{
	viewport->dragging = FALSE;
	viewport->old_x = 0;
	viewport->old_y = 0;
	viewport->move_x = 0;
	viewport->move_y = 0;
}

guint
dnd_viewport_get_type()
{
	static guint v_type = 0;

	if (!v_type)
	{
		GtkTypeInfo v_info =
		{
			"DndViewport",
			sizeof (DndViewport),
			sizeof (DndViewportClass),
			(GtkClassInitFunc) dnd_viewport_class_init,
			(GtkObjectInitFunc) dnd_viewport_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};
		v_type = gtk_type_unique (gtk_viewport_get_type (), &v_info);
	}
	return v_type;
}

GtkWidget*
dnd_viewport_new()
{
	GtkWidget *widget;
	GtkAdjustment *hadj, *vadj;
	GdkEventMask event_mask;
	
	widget = gtk_type_new(dnd_viewport_get_type());
	hadj = (GtkAdjustment*) gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	vadj = (GtkAdjustment*) gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	gtk_viewport_set_hadjustment(GTK_VIEWPORT(widget), hadj);
	gtk_viewport_set_vadjustment(GTK_VIEWPORT(widget), vadj);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(widget), GTK_SHADOW_NONE);
	event_mask = gtk_widget_get_events(widget);
	event_mask |=
		GDK_BUTTON_PRESS_MASK |
		GDK_BUTTON_RELEASE_MASK |
		GDK_BUTTON1_MOTION_MASK;
	gtk_widget_set_events(widget, event_mask);
	gtk_signal_connect(
		GTK_OBJECT(widget),
		"button_press_event",
		GTK_SIGNAL_FUNC(dnd_viewport_press),
		NULL);
	gtk_signal_connect(
		GTK_OBJECT(widget),
		"button_release_event",
		GTK_SIGNAL_FUNC(dnd_viewport_release),
		NULL);
	gtk_signal_connect(
		GTK_OBJECT(widget),
		"motion_notify_event",
		GTK_SIGNAL_FUNC(dnd_viewport_motion),
		NULL);
#ifdef GTK_HAVE_FEATURES_1_1_5
	gtk_object_default_construct(GTK_OBJECT(widget));
#endif
	return widget;
}

void
dnd_viewport_realize(GtkWidget *widget)
{
	this_class->parent_realize (widget);
	gdk_window_set_cursor(widget->window, hand_cursor);
}

static void
dnd_viewport_press(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	DndViewport *viewport = DND_VIEWPORT(widget);
	
	if (event->button.button == 3)
	{
		gtk_signal_emit(GTK_OBJECT(widget), dnd_viewport_signals[POPUP_SIGNAL]);
	} else
	if (event->type == GDK_2BUTTON_PRESS)
	{
		gtk_signal_emit(GTK_OBJECT(widget), dnd_viewport_signals[DOUBLE_CLICKED_SIGNAL]);
	}else
	{
		viewport->dragging = TRUE;
		viewport->old_x = event->button.x;
		viewport->old_y = event->button.y;
		viewport->move_x = 0;
		viewport->move_y = 0;
	}
}

static void
dnd_viewport_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	DndViewport *viewport;
	
	if (DND_VIEWPORT(widget)->dragging)
	{
		viewport = DND_VIEWPORT(widget);
		viewport->move_x += (gint)event->motion.x - viewport->old_x;
		viewport->move_y += (gint)event->motion.y - viewport->old_y;
		viewport->old_x = (gint)event->motion.x;
		viewport->old_y = (gint)event->motion.y;
	}
}

static void
dnd_viewport_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	DND_VIEWPORT(widget)->dragging = FALSE;
	dnd_viewport_do_move(DND_VIEWPORT(widget));
}

static void
dnd_viewport_do_move(DndViewport *viewport)
{
	GtkAdjustment *hadj, *vadj;

	if (viewport->move_x != 0)
	{
		hadj = gtk_viewport_get_hadjustment(GTK_VIEWPORT(viewport));
		gtk_adjustment_set_value(hadj,
			min(hadj->value-viewport->move_x, hadj->upper-hadj->page_size));
		viewport->move_x = 0;
	}
	if (viewport->move_y != 0)
	{
		vadj = gtk_viewport_get_vadjustment(GTK_VIEWPORT(viewport));
		gtk_adjustment_set_value(vadj,
			min(vadj->value-viewport->move_y, vadj->upper-vadj->page_size));
		viewport->move_y = 0;
	}
}

void
dnd_viewport_move(DndViewport *viewport, gint x, gint y)
{
	viewport->move_x -= x;
	viewport->move_y -= y;
	dnd_viewport_do_move(viewport);
}

void
dnd_viewport_reset(DndViewport *viewport)
{
	GtkAdjustment *hadj, *vadj;

	hadj = gtk_viewport_get_hadjustment(GTK_VIEWPORT(viewport));
	gtk_adjustment_set_value(hadj, 0);
	viewport->move_x = 0;
	vadj = gtk_viewport_get_vadjustment(GTK_VIEWPORT(viewport));
	gtk_adjustment_set_value(vadj, 0);
	viewport->move_y = 0;
}
