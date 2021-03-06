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

#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>
#include "imagelist.h"

#define FONT_BIG 0
#define FONT_SMALL 1

typedef  gboolean (*GetFileFunc)          (guchar *buffer);
typedef  void     (*FinishCallbackFunc)   ();

typedef struct
{
   /* additional parameters... */
   GtkWidget *entry;
   /* List of files  */
   GList *selection;
   /* a widget(dialog) which should be closed */
   GtkWidget *dialog;
   /* widget of files */
   GtkWidget *il;
} tool_parameters;

void        close_gtksee   ();
void        close_dialog   (GtkWidget *widget);
void        alert_dialog   (gchar     *myline);
void        remove_file    (GtkWidget *il, GList *selection);
void        rename_file    (GtkWidget *il, GList *selection);
GtkWidget*  info_dialog_new(guchar *title, guint *sizes, guchar **text);
gulong      buf2long       (guchar *buf, gint n);
gushort     buf2short      (guchar *buf, gint n);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
