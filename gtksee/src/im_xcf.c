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

/* #define DEBUG */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* patch from Pyun YongHyeon: fixed compilation error for FreeBSD 3.0 */
#ifdef __FreeBSD__
#include <sys/types.h>
#endif

#ifdef HAVE_OS2_H
#include <os2.h>
#include <sys/types.h>
#endif

#include <netinet/in.h>
#include <gtk/gtk.h>

#include "gtypes.h"
#include "im_xcf.h"

#define MAX_LAYERS 256
#define MAX_CHANNELS 256

/* These two values are defined in GIMP, and they are very
 * IMPORTANT. Do NOT change them.
 */
#define TILE_WIDTH 64
#define TILE_HEIGHT 64

typedef enum
{
   PROP_END = 0,
   PROP_COLORMAP = 1,
   PROP_ACTIVE_LAYER = 2,
   PROP_ACTIVE_CHANNEL = 3,
   PROP_SELECTION = 4,
   PROP_FLOATING_SELECTION = 5,
   PROP_OPACITY = 6,
   PROP_MODE = 7,
   PROP_VISIBLE = 8,
   PROP_LINKED = 9,
   PROP_PRESERVE_TRANSPARENCY = 10,
   PROP_APPLY_MASK = 11,
   PROP_EDIT_MASK = 12,
   PROP_SHOW_MASK = 13,
   PROP_SHOW_MASKED = 14,
   PROP_OFFSETS = 15,
   PROP_COLOR = 16,
   PROP_COMPRESSION = 17,
   PROP_GUIDES = 18
} PropType;

typedef enum
{
   COMPRESS_NONE = 0,
   COMPRESS_RLE = 1,
   COMPRESS_ZLIB = 2,
   COMPRESS_FRACTAL = 3
} CompressionType;

typedef enum
{
   XCF_RGB_IMAGE     = 0,
   XCF_GRAY_IMAGE    = 1,
   XCF_INDEXED_IMAGE = 2
} GImageType;

typedef enum
{
   TILE_RGB = 0,
   TILE_GRAY = 1,
   TILE_INDEXED = 2,
   TILE_MASK = 3,
   TILE_CHANNEL = 4
} HierarchyType;

typedef struct
{
   guint32 width;
   guint32 height;
   guint32 bpp;
   HierarchyType type;

   /* level */
   gint curlevel;
   guint32 level_width;
   guint32 level_height;

   /* tile */
   gint curtile;

   /* data */
   guchar *data;
} XcfHierarchy;

typedef struct
{
   guint32 width;
   guint32 height;
   guint32 type;
   guint32 opacity;
   guint32 visible;
   guint32 linked;
   guint32 preserve_trans;
   guint32 apply_mask;
   guint32 edit_mask;
   guint32 show_mask;
   gint32 offset_x;
   gint32 offset_y;
   guint32 mode;
} XcfLayer;

typedef struct
{
   guint32 width;
   guint32 height;
   guint32 opacity;
   guint32 visible;
   guint32 show_mask;
   guint8 color[3];
} XcfChannel;

typedef struct
{
   gint version;
   guint32 width;
   guint32 height;
   guint32 type;
   guint8 compression;

   gint curlayer;
   gint curchannel;

   gint pass;
   XcfLoadFunc func;

   /* indexed */
   guint32 num_cols;
   guchar cmap[768];

   /* channel */
   guint8 color[3];

} XcfImage;

guint    xcf_read_int32    (FILE     *fp,
                guint32  *data,
                gint      count);
guint    xcf_read_int8     (FILE     *fp,
                guint8   *data,
                gint      count);
guint    xcf_read_string      (FILE     *fp,
                gchar    *data);
gboolean xcf_load_image    (FILE     *fp,
                XcfImage *image);
gboolean xcf_load_layer    (FILE     *fp,
                XcfImage *image);
gboolean xcf_load_channel  (FILE     *fp,
                XcfImage *image);
gboolean xcf_load_layer_mask  (FILE     *fp,
                XcfImage *image,
                XcfHierarchy *hierarchy);
gboolean xcf_load_hierarchy   (FILE     *fp,
                XcfImage *image,
                XcfHierarchy *hierarchy);
gboolean xcf_load_level    (FILE     *fp,
                XcfImage *image,
                XcfHierarchy *hierarchy);
gboolean xcf_load_tile     (FILE     *fp,
                XcfImage *image,
                XcfHierarchy *hierarchy);
gboolean xcf_load_image_properties
               (FILE     *fp,
                XcfImage *image);
gboolean xcf_load_layer_properties
               (FILE     *fp,
                XcfLayer *layer);
gboolean xcf_load_channel_properties
               (FILE     *fp,
                XcfChannel *channel);
void     xcf_put_pixel_element   (XcfImage *image,
                guchar *line,
                gint x,
                gint off,
                gint ic);

gboolean
xcf_get_header(gchar *filename, xcf_info *info)
{
   FILE *fp;
   guchar buf[10];
   gint version;
   guint32 type;

   if ((fp = fopen(filename, "rb")) == NULL) return FALSE;

   /* signature */
   if ((fread(buf, 9, 1, fp) < 1) || strncmp(buf, "gimp xcf ", 9) != 0)
   {
      fclose(fp);
      return FALSE;
   }

   /* version */
   if (fread(buf, 5, 1, fp) < 1 || buf[4] != '\0')
   {
      fclose(fp);
      return FALSE;
   }
   if (strncmp(buf, "file", 4) == 0)
   {
      version = 0;
   } else
   if (buf[0] == 'v')
   {
      version = atoi(&buf[1]);
   } else
   {
      fclose(fp);
      return FALSE;
   }
   if (version < 0 || version > 1)
   {
      fclose(fp);
      return FALSE;
   }

   /* width, height, type */
   if (!xcf_read_int32(fp, &info->width, 1) ||
       !xcf_read_int32(fp, &info->height, 1) ||
       !xcf_read_int32(fp, &type, 1))
   {
      fclose(fp);
      return FALSE;
   }

   switch (type)
   {
     case XCF_RGB_IMAGE:
      info->ncolors = 24;
      info->alpha = 0;
      break;
     case XCF_GRAY_IMAGE:
      info->ncolors = 8;
      info->alpha = 0;
      break;
     case XCF_INDEXED_IMAGE:
      info->ncolors = 8;
      info->alpha = 0;
      break;
     default: /* unknown image type */
      fclose(fp);
      return FALSE;
   }

   fclose(fp);
   return TRUE;
}

gboolean
xcf_load(gchar *filename, XcfLoadFunc func)
{
   FILE *fp;
   XcfImage xcf_image;
   gboolean success;

#ifdef DEBUG
   g_print("\n=========================================================\n");
   g_print("  %s\n", filename);
   g_print("=========================================================\n");
#endif

   if ((fp = fopen(filename, "rb")) == NULL) return FALSE;

   xcf_image.func = func;
   xcf_image.compression = COMPRESS_NONE;
   success = xcf_load_image(fp, &xcf_image);

   fclose(fp);
#ifdef DEBUG
   if (success)
      g_print("XCF: Finished loading %s\n\n", filename);
   else
      g_print("XCF: Error occurred.\n\n");
#endif
   return success;
}

gboolean
xcf_load_image(FILE *fp, XcfImage *image)
{
   guchar buf[10];
   guint32 layer_offset[MAX_LAYERS];
   guint32 channel_offset[MAX_CHANNELS];
   gint i, num_layers, num_channels;
   guint32 offset;
   glong pos;

   /* signature */
   if ((fread(buf, 9, 1, fp) < 1) || strncmp(buf, "gimp xcf ", 9) != 0) return FALSE;

   /* version */
   if (fread(buf, 5, 1, fp) < 1 || buf[4] != '\0') return FALSE;
   if (strncmp(buf, "file", 4) == 0) image->version = 0;
   else if (buf[0] == 'v') image->version = atoi(&buf[1]);
   else return FALSE;
   if (image->version < 0 || image->version > 1) return FALSE;

   /* width, height, type */
   if (!xcf_read_int32(fp, &image->width, 1) ||
       !xcf_read_int32(fp, &image->height, 1) ||
       !xcf_read_int32(fp, &image->type, 1))
   {
      return FALSE;
   }

   switch (image->type)
   {
     case XCF_RGB_IMAGE:
     case XCF_GRAY_IMAGE:
     case XCF_INDEXED_IMAGE:
      break;
     default: /* unknown image type */
      return FALSE;
   }

   if (!xcf_load_image_properties(fp, image)) return FALSE;

#ifdef DEBUG
   g_print("version=%i width=%i height=%i type=%i compression=%i\n",
      image->version,
      image->width,
      image->height,
      image->type,
      image->compression);
#endif

   /* load layers */
   num_layers = 0;
   while (TRUE)
   {
      if (!xcf_read_int32(fp, &offset, 1)) return FALSE;
      if (offset == 0) break;
      if (num_layers < MAX_LAYERS)
         layer_offset[num_layers++] = offset;
   }
   pos = ftell(fp);
   image->curlayer = 0;
   image->pass = 0;
   for (i = num_layers - 1; i >= 0; i--)
   {
      fseek(fp, layer_offset[i], SEEK_SET);
      if (!xcf_load_layer(fp, image)) return FALSE;
      image->curlayer++;
   }

   fseek(fp, pos, SEEK_SET);

   /* load channels */
   num_channels = 0;
   while (TRUE)
   {
      if (!xcf_read_int32(fp, &offset, 1)) return FALSE;
      if (offset == 0) break;
      if (num_channels < MAX_CHANNELS)
         channel_offset[num_channels++] = offset;
   }
   image->curchannel = 0;
   for (i = 0; i < num_channels; i++)
   {
      fseek(fp, channel_offset[i], SEEK_SET);
      if (!xcf_load_channel(fp, image)) return FALSE;
      image->curchannel++;
   }

   /* That's all. */
   return TRUE;
}

gboolean
xcf_load_layer(FILE *fp, XcfImage *image)
{
   XcfHierarchy hierarchy;
   XcfLayer layer;
   guint32 offset;
   glong pos, len;
   gint i, left, right, width;
   guchar *pix;
#ifdef DEBUG
   guchar name[256];
#endif

   layer.offset_x = 0;
   layer.offset_y = 0;
   layer.opacity = 255;
   layer.visible = 1;
   layer.linked = 0;
   layer.preserve_trans = 0;
   layer.apply_mask = 0;
   layer.edit_mask = 0;
   layer.show_mask = 0;
   layer.mode = NORMAL_MODE;

   /* layer width, height, type */
   if (!xcf_read_int32(fp, &layer.width, 1) ||
       !xcf_read_int32(fp, &layer.height, 1) ||
       !xcf_read_int32(fp, &layer.type, 1))
   {
      return FALSE;
   }

#ifdef DEBUG
   xcf_read_string(fp, name);
#else
   xcf_read_string(fp, NULL);
#endif

   if (!xcf_load_layer_properties(fp, &layer)) return FALSE;

#ifdef DEBUG
   g_print("Loading layer %i: %s\n", image->curlayer, name);
   g_print("   width=%i height=%i type=%i offset_x=%i offset_y=%i\n",
      layer.width,
      layer.height,
      layer.type,
      layer.offset_x,
      layer.offset_y);
#endif

   if (!layer.visible) return TRUE;

   /* hierarchy */
   len = sizeof(guchar) * layer.width * layer.height * 4;
   hierarchy.data = g_malloc(len);
   memset(hierarchy.data, 255, len);

   if (!xcf_read_int32(fp, &offset, 1))
   {
      g_free(hierarchy.data);
      return FALSE;
   }
   pos = ftell(fp);
   fseek(fp, offset, SEEK_SET);

   hierarchy.type = image->type;
   if (!xcf_load_hierarchy(fp, image, &hierarchy))
   {
      g_free(hierarchy.data);
      return FALSE;
   }
   fseek(fp, pos, SEEK_SET);

   /* layer mask */
   if (!xcf_read_int32(fp, &offset, 1))
   {
      g_free(hierarchy.data);
      return FALSE;
   }
   if (offset > 0)
   {
      pos = ftell(fp);
      fseek(fp, offset, SEEK_SET);

      if (!xcf_load_layer_mask(fp, image, &hierarchy))
      {
         g_free(hierarchy.data);
         return FALSE;
      }
      fseek(fp, pos, SEEK_SET);
   }

   /* do layer opacity */
   if (layer.opacity < 255)
   {
      len = layer.width * layer.height;
      for (i = 0, pix = &hierarchy.data[3]; i < len; i++, pix += 4)
      {
         *pix = (*pix) * layer.opacity / 255;
      }
   }

   /* send pixels, then free them */
   left = max(layer.offset_x, 0);
   right = min(layer.offset_x + layer.width, image->width);
   width = right - left;
   for (i = max(layer.offset_y, 0);
        i < min(layer.offset_y + layer.height, image->height);
        i++)
   {
      if ((*image->func) (
         &hierarchy.data[((i-layer.offset_y)*layer.width+(left-layer.offset_x))<<2],
         width, left, i, 4, image->pass, layer.mode)) break;
   }

   image->pass++;

   g_free(hierarchy.data);
   return TRUE;
}

gboolean
xcf_load_channel(FILE *fp, XcfImage *image)
{
   XcfHierarchy hierarchy;
   XcfChannel channel;
   guint32 offset;
   gint i;
   glong pos, len;
   guchar *pix;
#ifdef DEBUG
   guchar name[256];
#endif

   channel.opacity = 255;
   channel.visible = 1;

   /* channel width, height */
   if (!xcf_read_int32(fp, &channel.width, 1) ||
       !xcf_read_int32(fp, &channel.height, 1))
   {
      return FALSE;
   }

#ifdef DEBUG
   xcf_read_string(fp, name);
   g_print("Loading channel %i: %s\n", image->curchannel, name);
   g_print("   width=%i height=%i\n",
      channel.width,
      channel.height);
#else
   xcf_read_string(fp, NULL);
#endif

   if (!xcf_load_channel_properties(fp, &channel)) return FALSE;

   if (!channel.visible) return TRUE;

   /* hierarchy */
   if (!xcf_read_int32(fp, &offset, 1)) return FALSE;
   pos = ftell(fp);
   fseek(fp, offset, SEEK_SET);

   hierarchy.type = TILE_CHANNEL;
   image->color[0] = channel.color[0];
   image->color[1] = channel.color[1];
   image->color[2] = channel.color[2];
   len = channel.width * channel.height;
   hierarchy.data = g_malloc(sizeof(guchar) * len * 4);
   if (!xcf_load_hierarchy(fp, image, &hierarchy))
   {
      g_free(hierarchy.data);
      return FALSE;
   }
   fseek(fp, pos, SEEK_SET);

   /* do opacity */
   if (channel.opacity < 255)
   {
      for (i = 0, pix = &hierarchy.data[3]; i < len; i++, pix += 4)
      {
         *pix = (*pix) * channel.opacity / 255;
      }
   }

   /* send channel pixels, then free them */
   for (i = 0; i < channel.height; i++)
   {
      if ((*image->func) (&hierarchy.data[i*channel.width*4],
         channel.width, 0, i, 4, image->pass, NORMAL_MODE)) break;
   }

   image->pass++;

   g_free(hierarchy.data);
   return TRUE;
}

gboolean
xcf_load_layer_mask(FILE *fp, XcfImage *image, XcfHierarchy *hierarchy)
{
   guint32 width, height;
   XcfChannel channel;
   guint32 offset;
   glong pos;
#ifdef DEBUG
   guchar name[256];
#endif

   if (!xcf_read_int32(fp, &width, 1) ||
       !xcf_read_int32(fp, &height, 1)) return FALSE;

#ifdef DEBUG
   if (!xcf_read_string(fp, name)) return FALSE;
   g_print("   Loading layer mask %s: width=%i height=%i\n",
      name, width, height);
#else
   if (!xcf_read_string(fp, NULL)) return FALSE;
#endif

   if (!xcf_load_channel_properties(fp, &channel)) return FALSE;
   if (!xcf_read_int32(fp, &offset, 1)) return FALSE;
   pos = ftell(fp);
   fseek(fp, offset, SEEK_SET);
   hierarchy->type = TILE_MASK;
   if (!xcf_load_hierarchy(fp, image, hierarchy)) return FALSE;
   fseek(fp, pos, SEEK_SET);

   return TRUE;
}

gboolean
xcf_load_hierarchy(FILE *fp, XcfImage *image, XcfHierarchy *hierarchy)
{
   guint32 offset;
   glong pos;

   /* hierarchy width, height, bpp(BYTEs Per Pixel) */
   if (!xcf_read_int32(fp, &hierarchy->width, 1) ||
       !xcf_read_int32(fp, &hierarchy->height, 1) ||
       !xcf_read_int32(fp, &hierarchy->bpp, 1)) return FALSE;

#ifdef DEBUG
   g_print("   Hierarchy: width=%i height=%i bpp=%i\n",
      hierarchy->width, hierarchy->height, hierarchy->bpp);
#endif

   hierarchy->curlevel = 0;
   while (TRUE)
   {
      if (!xcf_read_int32(fp, &offset, 1)) return FALSE;
      if (offset == 0) break;
      pos = ftell(fp);
      fseek(fp, offset, SEEK_SET);
      if (!xcf_load_level(fp, image, hierarchy)) return FALSE;
      fseek(fp, pos, SEEK_SET);
      hierarchy->curlevel++;
   }

   return TRUE;
}

gboolean
xcf_load_level(FILE *fp, XcfImage *image, XcfHierarchy *hierarchy)
{
   guint32 offset;
   glong pos;

   /* level width, height */
   if (!xcf_read_int32(fp, &hierarchy->level_width, 1) ||
       !xcf_read_int32(fp, &hierarchy->level_height, 1)) return FALSE;

#ifdef DEBUG
   g_print("      Level: width=%i height=%i\n",
      hierarchy->level_width, hierarchy->level_height);
#endif

   hierarchy->curtile = 0;
   while (TRUE)
   {
      if (!xcf_read_int32(fp, &offset, 1)) return FALSE;
      if (offset == 0) break;
      pos = ftell(fp);
      fseek(fp, offset, SEEK_SET);
      if (!xcf_load_tile(fp, image, hierarchy)) return FALSE;
      fseek(fp, pos, SEEK_SET);
      hierarchy->curtile++;
   }

   return TRUE;
}

gboolean
xcf_load_tile(FILE *fp, XcfImage *image, XcfHierarchy *hierarchy)
{
   guint32 current, size, bpp, tile_height, tile_width;
   gint i, j, n, c, t, off, ntile_rows, ntile_columns;
   gint curtile_row, curtile_column;
   gint lastrow_height, lastcolumn_width;
   gint rowi, columni;
   guchar *data;

   ntile_rows = (hierarchy->level_height + TILE_HEIGHT - 1) / TILE_HEIGHT;
   ntile_columns = (hierarchy->level_width + TILE_WIDTH - 1) / TILE_WIDTH;
   curtile_row = hierarchy->curtile / ntile_columns;
   curtile_column = hierarchy->curtile % ntile_columns;

   lastcolumn_width = hierarchy->level_width - ((ntile_columns - 1) * TILE_WIDTH);
   lastrow_height = hierarchy->level_height - ((ntile_rows - 1) * TILE_HEIGHT);

   tile_width = TILE_WIDTH;
   tile_height = TILE_HEIGHT;

   if (curtile_column == ntile_columns - 1)
   {
      tile_width = lastcolumn_width;
   }
   if (curtile_row == ntile_rows - 1)
   {
      tile_height = lastrow_height;
   }

   size = tile_width * tile_height;

#ifdef DEBUG
   g_print("         Tile %i: row=%i column=%i width=%i height=%i\n",
      hierarchy->curtile, curtile_row, curtile_column, tile_width, tile_height);
#endif

   bpp = hierarchy->bpp;

   switch (image->compression)
   {
     case COMPRESS_NONE:
      for (i = 0; i < bpp; i++)
      {
         switch (hierarchy->type)
         {
           case TILE_RGB:
            off = i;
            break;
           case TILE_GRAY:
            if (i == 0) off = -1;
            else off = 3;
            break;
           case TILE_INDEXED:
            if (i == 0) off = -2;
            else off = 3;
            break;
           case TILE_MASK:
            off = 4;
            break;
           case TILE_CHANNEL:
            off = 5;
            break;
           default:
            off = -3;
            break;
         }

         current = 0;
         while (current < size)
         {
            if ((c = fgetc(fp)) == EOF) return FALSE;
            rowi = current / tile_width;
            columni = current % tile_width;
            data = &hierarchy->data[((rowi+curtile_row*TILE_HEIGHT)*hierarchy->level_width
               +curtile_column*TILE_WIDTH)*4];
            xcf_put_pixel_element(image, data, columni, off, c);
            current++;
         }
      }
      break;
     case COMPRESS_RLE:
      for (i = 0; i < bpp; i++)
      {
         switch (hierarchy->type)
         {
           case TILE_RGB:
            off = i;
            break;
           case TILE_GRAY:
            if (i == 0) off = -1;
            else off = 3;
            break;
           case TILE_INDEXED:
            if (i == 0) off = -2;
            else off = 3;
            break;
           case TILE_MASK:
            off = 4;
            break;
           case TILE_CHANNEL:
            off = 5;
            break;
           default:
            off = -3;
            break;
         }

         current = 0;
         while (current < size)
         {
            if ((n = fgetc(fp)) == EOF) return FALSE;
            if (n >= 128)
            {
               if (n == 128)
               {
                  if ((n = fgetc(fp)) == EOF) return FALSE;
                  if ((t = fgetc(fp)) == EOF) return FALSE;
                  n = (n<<8) | t;
               } else
               {
                  n = 256 - n;
               }
               for (j = 0; j < n && current < size; j++)
               {
                  if ((c = fgetc(fp)) == EOF) return FALSE;
                  rowi = current / tile_width;
                  columni = current % tile_width;
                  data = &hierarchy->data[((rowi+curtile_row*TILE_HEIGHT)*hierarchy->level_width
                     +curtile_column*TILE_WIDTH)*4];
                  xcf_put_pixel_element(image, data, columni, off, c);
                  current++;
               }
            } else
            {
               n++;
               if (n == 128)
               {
                  if ((n = fgetc(fp)) == EOF) return FALSE;
                  if ((t = fgetc(fp)) == EOF) return FALSE;
                  n = (n<<8) | t;
               }
               if ((c = fgetc(fp)) == EOF) return FALSE;
               for (j = 0; j < n && current < size; j++)
               {
                  rowi = current / tile_width;
                  columni = current % tile_width;
                  data = &hierarchy->data[((rowi+curtile_row*TILE_HEIGHT)*hierarchy->level_width
                     +curtile_column*TILE_WIDTH)*4];
                  xcf_put_pixel_element(image, data, columni, off, c);
                  current++;
               }
            }
         }
      }
      break;
     case COMPRESS_ZLIB:
     case COMPRESS_FRACTAL:
     default:
      return FALSE;
   }

   return TRUE;
}

gboolean
xcf_load_image_properties(FILE *fp, XcfImage *image)
{
  PropType prop_type;
  guint32 prop_size;
  gint i, j;
  guint8 compression;

  while (TRUE)
    {
      if (!xcf_read_int32(fp, &prop_type, 1) ||
          !xcf_read_int32(fp, &prop_size, 1))
        return FALSE;

      switch (prop_type)
        {
        case PROP_END:
          return TRUE;
        case PROP_COLORMAP:
          if (!xcf_read_int32(fp, &image->num_cols, 1)) return FALSE;
          if (image->version == 0)
            {
              fseek(fp, image->num_cols, SEEK_SET);
              for (i = 0, j = 0; i<image->num_cols; i++)
                {
                  image->cmap[j++] = i;
                  image->cmap[j++] = i;
                  image->cmap[j++] = i;
                }
            }
          else
            {
              if (!xcf_read_int8(fp, image->cmap, image->num_cols * 3)) return FALSE;
            }
          break;
        case PROP_COMPRESSION:
          if (!xcf_read_int8(fp, &compression, 1)) return FALSE;
          if ((compression != COMPRESS_NONE) &&
              (compression != COMPRESS_RLE) &&
              (compression != COMPRESS_ZLIB) &&
              (compression != COMPRESS_FRACTAL))
            {
              return FALSE;
            }
          image->compression = compression;
          break;
        case PROP_GUIDES:
        default:
     fseek(fp, prop_size, SEEK_CUR);
          break;
        }
    }
}

gboolean
xcf_load_layer_properties(FILE *fp, XcfLayer *layer)
{
  PropType prop_type;
  guint32 prop_size, dummy;

  while (TRUE)
    {
      if (!xcf_read_int32(fp, &prop_type, 1) ||
          !xcf_read_int32(fp, &prop_size, 1))
        return FALSE;

      switch (prop_type)
        {
        case PROP_END:
          return TRUE;
        case PROP_ACTIVE_LAYER:
          break;
        case PROP_FLOATING_SELECTION:
          if (!xcf_read_int32(fp, &dummy, 1)) return FALSE;
          break;
        case PROP_OPACITY:
          if (!xcf_read_int32(fp, &layer->opacity, 1)) return FALSE;
          break;
        case PROP_VISIBLE:
          if (!xcf_read_int32(fp, &layer->visible, 1)) return FALSE;
          break;
        case PROP_LINKED:
          if (!xcf_read_int32(fp, &layer->linked, 1)) return FALSE;
          break;
        case PROP_PRESERVE_TRANSPARENCY:
          if (!xcf_read_int32(fp, &layer->preserve_trans, 1)) return FALSE;
          break;
        case PROP_APPLY_MASK:
          if (!xcf_read_int32(fp, &layer->apply_mask, 1)) return FALSE;
          break;
        case PROP_EDIT_MASK:
          if (!xcf_read_int32(fp, &layer->edit_mask, 1)) return FALSE;
          break;
        case PROP_SHOW_MASK:
          if (!xcf_read_int32(fp, &layer->show_mask, 1)) return FALSE;
          break;
        case PROP_OFFSETS:
          if (!xcf_read_int32(fp, &layer->offset_x, 1)) return FALSE;
          if (!xcf_read_int32(fp, &layer->offset_y, 1)) return FALSE;
          break;
        case PROP_MODE:
          if (!xcf_read_int32(fp, &layer->mode, 1)) return FALSE;
          break;
        default:
     fseek(fp, prop_size, SEEK_CUR);
          break;
        }
    }
}

gboolean
xcf_load_channel_properties(FILE *fp, XcfChannel *channel)
{
  PropType prop_type;
  guint32 prop_size;

  while (TRUE)
    {
      if (!xcf_read_int32(fp, &prop_type, 1) ||
          !xcf_read_int32(fp, &prop_size, 1))
        return FALSE;

      switch (prop_type)
        {
        case PROP_END:
          return TRUE;
        case PROP_ACTIVE_CHANNEL:
        case PROP_SELECTION:
          break;
        case PROP_OPACITY:
          if (!xcf_read_int32(fp, &channel->opacity, 1)) return FALSE;
          break;
        case PROP_VISIBLE:
          if (!xcf_read_int32(fp, &channel->visible, 1)) return FALSE;
          break;
        case PROP_SHOW_MASKED:
          if (!xcf_read_int32(fp, &channel->show_mask, 1)) return FALSE;
          break;
        case PROP_COLOR:
          if (!xcf_read_int8(fp, channel->color, 3)) return FALSE;
          break;
        default:
     fseek(fp, prop_size, SEEK_CUR);
     break;
        }
    }
}


void
xcf_put_pixel_element(XcfImage *image, guchar *line, gint x, gint off, gint ic)
{
   gint t = x<<2;
   guchar c = (guchar) ic;

   switch (off)
   {
     case -1: /* gray */
      line[t++] = c;
      line[t++] = c;
      line[t] = c;
      break;
     case -2: /* indexed */
      line[t++] = image->cmap[ic * 3];
      line[t++] = image->cmap[ic * 3 + 1];
      line[t] = image->cmap[ic * 3 + 2];
      break;
     case -3: /* do nothing */
      break;
     case 4: /* mask */       /* Fixed by J. P. Demailly */
      line[t] = line[t] * c / 255; t++;
      line[t] = line[t] * c / 255; t++;
      line[t] = line[t] * c / 255; t++;
      line[t] = line[t] * c / 255; t++;
      break;
     case 5: /* channel */
      line[t++] = image->color[0];
      line[t++] = image->color[1];
      line[t++] = image->color[2];
      line[t] = 255 - c;
     default:
      line[t+off] = c;
      break;
   }
}

guint
xcf_read_int32(FILE *fp, guint32 *data, gint count)
{
  guint total;

  total = count;
  if (count > 0)
    {
      xcf_read_int8 (fp, (guint8*) data, count * 4);

      while (count--)
        {
          *data = ntohl (*data);
          data++;
        }
    }

  return total * 4;
}

guint
xcf_read_int8(FILE *fp, guint8 *data, gint count)
{
  guint total;
  int bytes;

  total = count;
  while (count > 0)
    {
      bytes = fread ((char*) data, sizeof (char), count, fp);
      if (bytes <= 0) /* something bad happened */
        break;
      count -= bytes;
      data += bytes;
    }

  return total;
}

guint
xcf_read_string(FILE *fp, gchar *data)
{
  guint32 tmp;
  guint total;

  total = xcf_read_int32 (fp, &tmp, 1);
  if (data == NULL)
    {
      fseek(fp, tmp, SEEK_CUR);
      total += tmp;
    }
  else
    {
      if (tmp > 0)
        {
          total += xcf_read_int8 (fp, (guint8*) data, tmp);
        }
      data[tmp] = '\0';
    }

  return total;
}
