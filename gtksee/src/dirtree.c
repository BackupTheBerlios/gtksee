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
#include <unistd.h>
#include <sys/types.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include <sys/stat.h>
#include <gtk/gtk.h>
#include "rc.h"
#include "dirtree.h"

#include "pixmaps/folder_collapsed.xpm"
#include "pixmaps/folder_expanded.xpm"

typedef struct dir_node_struct
{
	Dirtree *tree;
	guchar dir_name[256];
	GtkWidget *subtree, *parent;
} dir_node;

typedef struct sel_node_struct
{
	Dirtree *tree;
	guchar new_dir[256];
	GtkWidget *parent;
} sel_node;

#ifdef GTK_HAVE_FEATURES_1_1_4
static GtkTargetEntry dirtree_target_table[] = {
	{ "STRING", 0, 0 },
};
#endif

static void	dirtree_class_init	(DirtreeClass *class);
static void	dirtree_init		(Dirtree *dt);
static gboolean	dirtree_if_hide		(Dirtree *dt, guchar *dir, guchar *item);
int		dir_can_expand		(Dirtree *dt, char *dir);
void		item_expanded		(GtkTreeItem *treeitem, dir_node *node);
void		item_collapsed		(GtkTreeItem *treeitem, dir_node *node);
void		item_selected		(GtkTreeItem *treeitem, sel_node *node);
void		item_deselected		(GtkTreeItem *treeitem, sel_node *node);
void		dirtree_expand_subtree	(Dirtree *dt, GtkTree *tree, char *dir, char *e_target);
void		dirtree_item_set_pixmap	(GtkWidget *parent, GtkTreeItem *treeitem, gint expanded);
GtkWidget*	dirtree_item_new	(GtkWidget *parent, guchar *dir);
guchar*		dirtree_item_get_label	(GtkTreeItem *treeitem);
gint		dirtree_item_position	(GtkTree *tree, GtkWidget *treeitem, gint level);
gint		dirtree_get_height	(Dirtree *dt);
void		dirtree_create_root_item(Dirtree *dt, guchar *dir);
gint		dirtree_select_dir_internal
					(GtkTree *tree, guchar *dir);

#ifdef GTK_HAVE_FEATURES_1_1_4
void		dirtree_drag_data_received
					(GtkWidget *widget,
					 GdkDragContext *context,
					 gint x,
					 gint y,
					 GtkSelectionData *data,
					 guint info,
					 guint time,
					 gpointer node);
#endif

guint
dirtree_get_type ()
{
	static guint dt_type = 0;

	if (!dt_type)
	{
		GtkTypeInfo dt_info =
		{
			"Dirtree",
			sizeof (Dirtree),
			sizeof (DirtreeClass),
			(GtkClassInitFunc) dirtree_class_init,
			(GtkObjectInitFunc) dirtree_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};
		dt_type = gtk_type_unique (gtk_tree_get_type (), &dt_info);
	}
	return dt_type;
}

static void
dirtree_class_init (DirtreeClass *class)
{
}

static void
dirtree_init (Dirtree *dt)
{
	dt -> root[0] = '/';
	dt -> root[1] = '\0';
	dt -> selected_dir [0] = '\0';
	dt -> item_height = 1;
	dt -> parent = NULL;
}

static gboolean
dirtree_if_hide(Dirtree *dt, guchar *dir, guchar *name)
{
	guchar *hidden_dir, *ptr, buf[256], c;

	if (strcmp(name, ".") == 0) return TRUE;
	if (strcmp(name, "..") == 0) return TRUE;

	if (rc_get_boolean("show_hidden")) return FALSE;
	if (name[0] == '.') return TRUE;

	hidden_dir = rc_get("hidden_directory");
	strcpy(buf, dir);
	if (buf[strlen(buf) - 1] != '/') strcat(buf, "/");
	strcat(buf, name);
	ptr = strstr(hidden_dir, buf);
	if (ptr == NULL) return FALSE;
	c = *(ptr + strlen(buf));
	if (ptr != NULL &&
		(ptr == hidden_dir || *(ptr - 1) == ':') &&
		(c == '\0' || c == ':'))
	{
		return TRUE;
	}
	return FALSE;
}

int
dir_can_expand(Dirtree *dt, char *dir)
{
	DIR *dp;
	struct dirent *item;
	char fullname[256];
	struct stat st;
	int guess;
	
	if ((dp = opendir(dir)) == NULL) return FALSE;
	guess = FALSE;
	while ((item = readdir(dp)) != NULL)
	{
		if (item -> d_ino == 0) continue;
		if (dirtree_if_hide(dt, dir, item->d_name)) continue;
		strcpy(fullname, dir);
		if (fullname[strlen(fullname) - 1] != '/') strcat(fullname,"/");
		strcat(fullname, item -> d_name);
		stat(fullname, &st);
		if ((st.st_mode & S_IFMT) == S_IFDIR)
		{
			guess = TRUE;
			break;
		}
	}
	closedir(dp);
	return guess;
}

void
dirtree_expand_subtree(Dirtree *dt, GtkTree *tree, char *dir, char *e_target)
{
	DIR *dp;
	struct dirent *item;
	GtkWidget *treeitem;
	GtkWidget *subtree;
	char fullname[256], *idx;
	struct stat st;
	dir_node *node;
	sel_node *snode;
	GList *list, *t_pos;
	
	if ((dp = opendir(dir)) == NULL) return;
	list = NULL;
	/*
	 * Read dir items and sort them.
	 */
	while ((item = readdir(dp)) != NULL)
	{
		if (item -> d_ino == 0) continue;
		if (dirtree_if_hide(dt, dir, item->d_name)) continue;
		strcpy(fullname, dir);
		if (fullname[strlen(fullname) - 1] != '/') strcat(fullname,"/");
		strcat(fullname, item -> d_name);
		stat(fullname, &st);
		if ((st.st_mode & S_IFMT) != S_IFDIR) continue;
		list = g_list_insert_sorted(list, g_strdup(item -> d_name), (GCompareFunc)strcmp);
	}
	/*
	 * Now, we can add all items to the sub-tree.
	 */
	t_pos = g_list_first(list);
	while (t_pos != NULL)
	{
		treeitem = dirtree_item_new(dt -> parent, t_pos -> data);
		gtk_tree_append(GTK_TREE(tree), treeitem);
		strcpy(fullname, dir);
		if (fullname[strlen(fullname) - 1] != '/') strcat(fullname,"/");
		strcat(fullname, t_pos -> data);
		snode = g_malloc(sizeof(sel_node));
		snode -> tree = dt;
		strcpy(snode -> new_dir, fullname);
		snode -> parent = dt -> parent;
		gtk_signal_connect(GTK_OBJECT(treeitem), "select",
			GTK_SIGNAL_FUNC(item_selected), snode);
		gtk_signal_connect(GTK_OBJECT(treeitem), "deselect",
			GTK_SIGNAL_FUNC(item_deselected), snode);
#ifdef GTK_HAVE_FEATURES_1_1_4
		gtk_drag_dest_set(treeitem, GTK_DEST_DEFAULT_ALL,
			dirtree_target_table, 1,
			GDK_ACTION_COPY | GDK_ACTION_MOVE);
		gtk_signal_connect(GTK_OBJECT(treeitem), "drag_data_received",
			GTK_SIGNAL_FUNC(dirtree_drag_data_received), snode);
#endif

		if (dir_can_expand(dt, fullname))
		{
			subtree = gtk_tree_new();
			gtk_tree_item_set_subtree(GTK_TREE_ITEM(treeitem), subtree);
			node = g_malloc(sizeof(dir_node));
			strcpy(node -> dir_name, fullname);
			node -> subtree = subtree;
			node -> tree = dt;
			node -> parent = dt -> parent;
			gtk_signal_connect(GTK_OBJECT(treeitem), "expand",
				GTK_SIGNAL_FUNC(item_expanded), node);
			gtk_signal_connect(GTK_OBJECT(treeitem), "collapse",
				GTK_SIGNAL_FUNC(item_collapsed), node);
		}
		if (e_target != NULL && strlen(e_target) > 0)
		{
			idx = strchr(e_target, '/');
			if (idx != NULL &&
				strncmp(t_pos -> data, e_target, idx - e_target) == 0)
			{
				(*idx) = '\0';
				dirtree_expand_subtree(dt, GTK_TREE(subtree), fullname, idx + 1);
				gtk_tree_item_expand(GTK_TREE_ITEM(treeitem));
			}
		}
		gtk_widget_show(treeitem);
		t_pos = t_pos -> next;
	}
	g_list_free(list);
	closedir(dp);
}

void
item_expanded(GtkTreeItem *treeitem, dir_node *node)
{
	if (g_list_length(GTK_TREE(node -> subtree) -> children) <= 0)
	{
		dirtree_expand_subtree(node -> tree, GTK_TREE(node -> subtree),
			node -> dir_name, NULL);
	}
	dirtree_item_set_pixmap(node->parent, GTK_TREE_ITEM(treeitem), TRUE);
}

void
item_collapsed(GtkTreeItem *treeitem, dir_node *node)
{
	gtk_tree_select_child(GTK_TREE(node->tree), GTK_WIDGET(treeitem));
}

void
item_selected(GtkTreeItem *treeitem, sel_node *node)
{
	strcpy(node->tree->selected_dir, node->new_dir);
	dirtree_item_set_pixmap(node->parent, GTK_TREE_ITEM(treeitem), TRUE);
}

void
item_deselected(GtkTreeItem *treeitem, sel_node *node)
{
	if (! (treeitem -> expanded))
	{
		dirtree_item_set_pixmap(node->parent, GTK_TREE_ITEM(treeitem), FALSE);
	}
}

guchar*
dirtree_get_dir(Dirtree *tree)
{
	return tree->selected_dir;
}

GtkWidget*
dirtree_new(GtkWidget *parent, guchar *root)
{
	guchar wd[256];
#ifdef HAVE_GETCWD
	getcwd(wd, 256);
#else
	if (root != NULL && strlen(root) > 0)
	{
		strcpy(wd, root);
	} else
	{
		strcpy(wd, "/");
	}
#endif
	return GTK_WIDGET(dirtree_new_by_dir(parent, root, wd));
}

GtkWidget*
dirtree_new_by_dir(GtkWidget *parent, guchar *root, guchar *dir)
{
	Dirtree *dt;
	gint len;
	
	dt = DIRTREE (gtk_type_new (dirtree_get_type ()));
	
	dt -> parent = parent;
	if (root != NULL && strlen(root) > 0)
	{
		strcpy(dt->root, root);
		/* add the last '/' charater */
		len = strlen(root);
		if (root[len-1] != '/')
		{
			strcat(dt->root, "/");
		}
		if (dir[0] == '/')
		{
			strcpy(dt->selected_dir, dir);
		} else
		{
			strcpy(dt->selected_dir, dt->root);
			strcat(dt->selected_dir, dir);
		}
	} else
	{
		strcpy(dt->selected_dir, dir);
	}
	gtk_tree_set_view_mode(GTK_TREE(dt), GTK_TREE_VIEW_ITEM);
	gtk_tree_set_selection_mode(GTK_TREE(dt), GTK_SELECTION_BROWSE);
	dirtree_create_root_item(dt, dir);
	
	return GTK_WIDGET(dt);
}

void
dirtree_create_root_item(Dirtree *dt, guchar *dir)
{
	GtkWidget *rootitem, *subtree;
	GtkRequisition requisition;
	sel_node *snode;
	dir_node *node;
	guchar *root;
	
	root = dt->root;
	rootitem = dirtree_item_new(dt->parent, root);
	gtk_tree_append(GTK_TREE(dt), rootitem);
	
	snode = g_malloc(sizeof(sel_node));
	snode -> tree = dt;
	strcpy(snode->new_dir, root);
	snode -> parent = dt -> parent;
	gtk_signal_connect(GTK_OBJECT(rootitem), "select",
		GTK_SIGNAL_FUNC(item_selected), snode);
	gtk_signal_connect(GTK_OBJECT(rootitem), "deselect",
		GTK_SIGNAL_FUNC(item_deselected), snode);
#ifdef GTK_HAVE_FEATURES_1_1_4
	gtk_drag_dest_set(rootitem, GTK_DEST_DEFAULT_ALL,
		dirtree_target_table, 1,
		GDK_ACTION_COPY | GDK_ACTION_MOVE);
	gtk_signal_connect(GTK_OBJECT(rootitem), "drag_data_received",
		GTK_SIGNAL_FUNC(dirtree_drag_data_received), snode);
#endif
		
	subtree = gtk_tree_new();
	gtk_tree_item_set_subtree(GTK_TREE_ITEM(rootitem), subtree);
	gtk_widget_show(rootitem);
	
	node = g_malloc(sizeof(dir_node));
	strcpy(node -> dir_name, root);
	node -> subtree = subtree;
	node -> tree = dt;
	node -> parent = dt -> parent;
	gtk_signal_connect(GTK_OBJECT(rootitem), "expand",
		GTK_SIGNAL_FUNC(item_expanded), node);
	gtk_signal_connect(GTK_OBJECT(rootitem), "collapse",
		GTK_SIGNAL_FUNC(item_collapsed), node);
		
	gtk_widget_size_request(GTK_WIDGET(rootitem), &requisition);
	dt -> item_height = requisition.height;
	if (dir[0] == '/')
	{
		dir += strlen(root);
	}
	dirtree_expand_subtree(dt, GTK_TREE(subtree), dt->root, (char *)dir);
	gtk_tree_item_expand(GTK_TREE_ITEM(rootitem));
}

void
dirtree_item_set_pixmap(GtkWidget *parent, GtkTreeItem *treeitem, gint expanded)
{
	static GdkPixmap *folder_expanded, *folder_collapsed;
	static GdkBitmap *mask_e, *mask_c;
	static GtkStyle *style;
	
	GtkPixmap *hbox, *pixmap;
	
	if (folder_expanded == NULL || folder_collapsed == NULL)
	{
		style = gtk_widget_get_style(parent);
		folder_expanded = gdk_pixmap_create_from_xpm_d(
			parent->window,
			&mask_e, &style->bg[GTK_STATE_NORMAL],
			(gchar **)folder_expanded_xpm
		);
		folder_collapsed = gdk_pixmap_create_from_xpm_d(
			parent->window,
			&mask_c, &style->bg[GTK_STATE_NORMAL],
			(gchar **)folder_collapsed_xpm
		);
	}
	
	hbox = gtk_container_children(GTK_CONTAINER(treeitem))->data;
	pixmap = gtk_container_children(GTK_CONTAINER(hbox))->data;
	if (expanded)
	{
		gtk_pixmap_set(GTK_PIXMAP(pixmap), folder_expanded, mask_e);
	} else
	{
		gtk_pixmap_set(GTK_PIXMAP(pixmap), folder_collapsed, mask_c);
	}
}

GtkWidget*
dirtree_item_new(GtkWidget *parent, guchar *dir)
{
	static GdkPixmap *folder_pix;
	static GdkBitmap *mask;
	static GtkStyle *style;
	
	GtkWidget *hbox, *folder, *label, *treeitem;

	if (folder_pix == NULL)
	{
		style = gtk_widget_get_style(parent);
		folder_pix = gdk_pixmap_create_from_xpm_d(
			parent->window,
			&mask, &style->bg[GTK_STATE_NORMAL],
			(gchar **)folder_expanded_xpm);
	}	
	
	treeitem = gtk_tree_item_new();
	hbox = gtk_hbox_new(FALSE, 1);
	folder = gtk_pixmap_new(folder_pix, mask);
	gtk_box_pack_start(GTK_BOX(hbox), folder, FALSE, FALSE, 0);
	label = gtk_label_new(dir);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(treeitem), hbox);
	
	dirtree_item_set_pixmap(parent, GTK_TREE_ITEM(treeitem), FALSE);
	
	gtk_widget_show(folder);
	gtk_widget_show(label);
	gtk_widget_show(hbox);

	return treeitem;
}

guchar*
dirtree_item_get_label(GtkTreeItem *treeitem)
{
	GtkWidget *hbox, *label;
	gchar *text;
	
	hbox = gtk_container_children(GTK_CONTAINER(treeitem))->data;
	label = gtk_container_children(GTK_CONTAINER(hbox))->next->data;
	gtk_label_get(GTK_LABEL(label), &text);
	return (guchar*)text;
}

/* return value: the y location of the item which has been selected. */
gint
dirtree_select_dir(Dirtree *tree, guchar *dir)
{
	GList *children;
	guchar *root;
	
	root = tree->root;
	children = gtk_container_children (GTK_CONTAINER (tree));
	if (strcmp(root, dir) == 0 ||
	    (strlen(dir) == strlen(root)-1 &&
	    strncmp(root, dir, strlen(root)-1) == 0))
	{
		gtk_tree_select_child(GTK_TREE(tree), children->data);
		return 0;
	}
	if (dir[0] == '/')
	{
		if (strncmp(root, dir, strlen(root)) != 0) return 0;
		dir += strlen(root);
	}
	return dirtree_select_dir_internal(GTK_TREE(GTK_TREE_ITEM(children->data)->subtree), dir);
}

gint
dirtree_select_dir_internal(GtkTree *tree, guchar *dir)
{
	GList *children;
	gint idx;
	guchar *label;
	GtkWidget *treeitem;
	
	children = gtk_container_children (GTK_CONTAINER (tree));
	while (children) {
		if (GTK_IS_TREE_ITEM(children->data))
		{
			treeitem = children->data;
			label = dirtree_item_get_label(GTK_TREE_ITEM(treeitem));
			idx = (guchar*)strchr(dir, '/') - dir;
			if (strcmp(label, dir) == 0)
			{
				gtk_tree_select_child(GTK_TREE(tree), treeitem);
				return dirtree_item_position(GTK_TREE_ROOT_TREE(tree), treeitem, 0);
			} else
			if (strncmp(label, dir, idx) == 0 &&
				GTK_TREE_ITEM(treeitem)->subtree != NULL)
			{
				if (!GTK_TREE_ITEM(treeitem)->expanded)
				{
					gtk_tree_item_expand(
						GTK_TREE_ITEM(treeitem));
				}
				return dirtree_select_dir_internal(
					GTK_TREE(GTK_TREE_ITEM(treeitem)->subtree),
					dir + idx + 1);
			}
		} else
		{
			treeitem = GTK_TREE(children->data) -> tree_owner;
			label = dirtree_item_get_label(GTK_TREE_ITEM(treeitem));
			idx = (guchar*)strchr(dir, '/') - dir;
			if (strncmp(label, dir, idx) == 0) {
				if (strlen(label) == strlen(dir))
				{
					gtk_tree_select_child(GTK_TREE(tree), treeitem);
					return dirtree_item_position(GTK_TREE_ROOT_TREE(tree), treeitem, 0);
				} else
				{
					return dirtree_select_dir_internal(GTK_TREE(children->data), dir + idx + 1);
				}
				break;
			}
		}
		children = g_list_remove_link (children, children);
	}
	return -1;
}

gint
dirtree_cdup(Dirtree *dt)
{
	gint n = strlen(dt->selected_dir) - 1;
	while (n > 1 && dt->selected_dir[n] != '/') n--;
	if (n > 0) dt->selected_dir[n] = '\0';
	return dirtree_select_dir(dt, dt->selected_dir);
}

gint
dirtree_refresh(Dirtree *tree)
{
	GtkWidget *rootitem;
	guchar buffer[256];
	
	rootitem = GTK_WIDGET(GTK_TREE(tree)->children->data);
	gtk_widget_destroy (rootitem);
	
	GTK_TREE(tree)->selection = NULL;
	
	strcpy(buffer, tree->selected_dir);
	dirtree_create_root_item(tree, tree->selected_dir);
	strcpy(tree->selected_dir, buffer);
	return 0;
}

gint
dirtree_item_position(GtkTree *tree, GtkWidget *treeitem, gint level)
{
	GList *children;
	gint pos;
	static gboolean found;
	
	children = gtk_container_children (GTK_CONTAINER (tree));
	pos = 0;
	found = FALSE;
	while (children) {
#ifdef GTK_HAVE_FEATURES_1_1_4
		if (children->data == treeitem)
		{
			found = TRUE;
			return pos;
		}
		pos ++;
		if (GTK_TREE_ITEM(children->data)->expanded)
		{
			pos += dirtree_item_position(
				GTK_TREE(
				GTK_TREE_ITEM_SUBTREE(
				GTK_TREE_ITEM(children->data))),
				treeitem, level+1);
			if (found) return pos;
		}
#else
		if (GTK_IS_TREE_ITEM(children->data))
		{
			if (children->data == treeitem)
			{
				found = TRUE;
				return pos;
			}
			pos ++;
		} else
		{
			pos += dirtree_item_position(GTK_TREE(children->data), treeitem, level+1);
			if (found) return pos;
		}
#endif
		children = g_list_remove_link (children, children);
	}
	if (level == 0 && !found) return -1;
	return pos;
}

#ifdef GTK_HAVE_FEATURES_1_1_4
void
dirtree_drag_data_received
	(GtkWidget *widget,
	 GdkDragContext *context,
	 gint x,
	 gint y,
	 GtkSelectionData *data,
	 guint info,
	 guint time,
	 gpointer node)
{
	g_print("Data received \"%s\" at %s\n", (gchar*)data->data,
		(gchar*)((sel_node*)node)->new_dir);
	gtk_drag_finish(context, TRUE, FALSE, time);
}
#endif

void
dirtree_set_root(Dirtree *tree, guchar *root)
{
	strcpy(tree->root, root);
	if (root[strlen(root) - 1] != '/')
	{
		strcat(tree->root, "/");
	}
}
