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

#include "util.h"

char*
fnumber(long num)
{
	char *result,n[30];
	int len, pre, i, suf, pointer;

	result = (char *)malloc(30 * sizeof(char));
	sprintf(n, "%li", num);
	len = strlen(n);
	pre = len % 3;
	i = pre = (pre>0?pre:3);
	pointer = 0;
	while (i > 0)
	{
		result[pointer++] = n[pointer];
		i--;
	}
	suf = len - pre;
	while (suf > 0)
	{
		result[pointer++] = ',';
		result[pointer++] = n[len - suf];
		result[pointer++] = n[len - suf + 1];
		result[pointer++] = n[len - suf + 2];
		suf -= 3;
	}
	result[pointer] = '\0';
	return result;
}

char*
fsize(long size)
{	
	static const char *units[] = {"", "KB", "MB", "GB"};
	static const int num_units = 4;

	int t = 0;
	char *result;
	double num;
	
	result = malloc(20 * sizeof(char));
	if (size < 1024)
	{
		sprintf(result, "%li %s", size, size<=1?"byte":"bytes");
		return result;
	}
	
	num = size * 1.0;
	while (t < num_units - 1 && num >=1024)
	{
		num /= 1024;
		t++;
	}

	sprintf(result, "%.1f %s", num, units[t]);
	return result;
}

unsigned int
strtohash(unsigned char *str)
{
	unsigned char *s = str;
	unsigned int result = 0;
	while ((*s) != '\0')
	{
		result += (unsigned int)(*s++);
	}
	return result;
}
