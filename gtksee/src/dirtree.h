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

#ifndef __DIRTREE_H__
#define __DIRTREE_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define DIRTREE(obj)		GTK_CHECK_CAST (obj, dirtree_get_type (), Dirtree)
#define DIRTREE_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, dirtree_get_type (), DirtreeClass)
#define IS_DIRTREE(obj)		GTK_CHECK_TYPE (obj, dirtree_get_type ())

typedef struct _Dirtree		Dirtree;
typedef struct _DirtreeClass	DirtreeClass;

struct _DirtreeClass
{
	GtkTreeClass parent_class;
};

struct _Dirtree
{
	GtkTree tree;
	guchar root[512];
	guchar selected_dir[256];
	gint item_height;
	GtkWidget *parent;
};

guint		dirtree_get_type	(void);
GtkWidget*	dirtree_new		(GtkWidget *parent, guchar *root);
GtkWidget*	dirtree_new_by_dir	(GtkWidget *parent, guchar *root, guchar *dir);
guchar*		dirtree_get_dir		(Dirtree *tree);
gint		dirtree_select_dir	(Dirtree *tree, guchar *dir);
gint		dirtree_cdup		(Dirtree *tree);
gint		dirtree_refresh		(Dirtree *tree);
void		dirtree_set_root	(Dirtree *tree, guchar *root);

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif

