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

#ifndef __RC_H__
#define __RC_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#define RC_NOT_EXISTS (-1)

#include <gtk/gtk.h>

void		rc_init			();
void		rc_set			(guchar *key, guchar *value);
void		rc_set_boolean		(guchar *key, gboolean value);
void		rc_set_int		(guchar *key, gint value);
guchar*		rc_get			(guchar *key);
gboolean	rc_get_boolean		(guchar *key);
gint		rc_get_int		(guchar *key);
void		rc_save_gtkseerc	();

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif
