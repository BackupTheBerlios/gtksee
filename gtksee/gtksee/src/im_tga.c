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

/*
 * 2003-10-13: This code was taken of xli version 1.17 by Graeme Gill
 * Created for xli by Graeme Gill,
 * partially based on tgatoppm Copyright by Jef Poskanzer,
 * which was partially based on tga2rast, version 1.0, by Ian MacPhedran.
 */

#include <stdio.h>
#include "im_tga.h"

/* Definitions for image types. */
#define TGA_Null      0       /* Not used */
#define TGA_Map       1
#define TGA_RGB       2
#define TGA_Mono      3
#define TGA_RLEMap    9
#define TGA_RLERGB   10
#define TGA_RLEMono  11
#define TGA_CompMap  32       /* Not used */
#define TGA_CompMap4 33       /* Not used */

/* Definitions for interleave flag. */
#define TGA_IL_None  0
#define TGA_IL_Two   1
#define TGA_IL_Four  2

#define ReadOK(file,buffer,len)  (fread(buffer, 1, len, file))
#define ToS(buffer, off)         ((buffer[(off)+1]<<8) | buffer[(off)])

/* Read the header of the file, and                         */
/* Return TRUE if this looks like a tga file                */
/* Note that since Targa files don't have a magic number,   */
/* we have to be pickey about this.                         */

gboolean
tga_get_header(gchar *filename, tga_info *info)
{
   FILE     *zf;
   guchar   buf[18];

   if ((zf = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   }

   if(ReadOK(zf, buf, 18) != 18)
   {
      fclose(zf);
      return FALSE;
   }

   fclose(zf);

   info->IDLength      = (guchar)buf[0];
   info->ColorMapType  = (guchar)buf[1];
   info->ImageType     = (guchar)buf[2];
   info->CMapStart     = (guint )ToS(buf, 3);
   info->CMapLength    = (guint )ToS(buf, 5);
   info->CMapDepth     = (guchar)buf[7];
   info->XOffset       = (guint )ToS(buf, 8);
   info->YOffset       = (guint )ToS(buf, 10);
   info->Width         = (guint )ToS(buf, 12);
   info->Height        = (guint )ToS(buf, 14);
   info->PixelDepth    = (guchar)buf[16];
   info->AttBits       = (guchar)buf[17] & 0xf;
   info->Rsrvd         = ((guchar)buf[17] & 0x10) >> 4;
   info->OrgBit        = ((guchar)buf[17] & 0x20) >> 5;
   info->IntrLve       = ((guchar)buf[17] & 0xc0) >> 6;

   /* See if it is consistent with a tga files */

   if(   info->ColorMapType != 0
      && info->ColorMapType != 1)
      return FALSE;

   if(   info->ImageType != TGA_Map
      && info->ImageType != TGA_RGB
      && info->ImageType != TGA_Mono
      && info->ImageType != TGA_RLEMap
      && info->ImageType != TGA_RLERGB
      && info->ImageType != TGA_RLEMono)
      return FALSE;

   if(   info->CMapDepth != 0
      && info->CMapDepth != 15
      && info->CMapDepth != 16
      && info->CMapDepth != 24
      && info->CMapDepth != 32)
      return FALSE;

   if(   info->PixelDepth != 8
      && info->PixelDepth != 15
      && info->PixelDepth != 16
      && info->PixelDepth != 24
      && info->PixelDepth != 32)
      return FALSE;

   if(   info->IntrLve != TGA_IL_None
      && info->IntrLve != TGA_IL_Two
      && info->IntrLve != TGA_IL_Four)
      return FALSE;

   /* Do a few more consistency checks */
   if(info->ImageType == TGA_Map ||
      info->ImageType == TGA_RLEMap)
   {
      if(info->ColorMapType != 1 || info->CMapDepth == 0)
         return FALSE;  /* map types must have map */
   } else
   {
      if(info->ColorMapType != 0 || info->CMapDepth != 0)
         return FALSE;  /* non-map types must not have map */
   }

   /* can only handle 8 or 16 bit pseudo color images */
   if(info->ColorMapType != 0 && info->PixelDepth > 16)
      return FALSE;

   /* other numbers must not be silly */
   if(   (info->CMapStart + info->CMapLength) > 65535
      || (info->XOffset + info->Width)        > 65535
      || (info->YOffset + info->Height)       > 65535)
      return FALSE;

   /* setup run-length encoding flag */
   if(   info->ImageType == TGA_RLEMap
      || info->ImageType == TGA_RLERGB
      || info->ImageType == TGA_RLEMono)
      info->RLE = TRUE;
   else
      info->RLE = FALSE;

   return TRUE;
}

gboolean
tga_load(gchar *filename, TgaLoadFunc func)
{
   FILE     *zf;
   tga_info hdr;
   gint     span, hretrace, vretrace, nopix, x, y;
   guchar   *data, *eoi,*my_buffer, *row;
   guint    rd_count, wr_count, pixlen;
   guchar   tga_cmap[256][3];

   if (tga_get_header(filename, &hdr) == FALSE)
   {
      return FALSE;
   }

   if ((zf = fopen (filename, "rb")) == NULL)
   {
      return FALSE;
   }

   if (fseek(zf, 18, SEEK_CUR) != 0)
   {
      fclose(zf);
      return FALSE;
   }

   /* OK, we are in ... */

   /* Skip the identifier string */
   if (hdr.IDLength)
   {
      if (fseek(zf, hdr.IDLength, SEEK_CUR) != 0)
      {
         fclose(zf);
         return FALSE;
      }
   }

   pixlen = (hdr.PixelDepth + 7) >> 3;
   my_buffer = g_malloc(hdr.Width * hdr.Height * 3);

   /* Create the appropriate image and colormap */
   if(hdr.ColorMapType != 0)  /* must be 8 or 16 bit mapped type */
   {
      gint    i, n=0;
      guchar  buf[4];

      switch (hdr.CMapDepth)
      {
         case 8:     /* Gray scale color map */
            for(i=hdr.CMapStart; i < (hdr.CMapStart + hdr.CMapLength); i++)
            {
               if(ReadOK(zf, buf, 1) != 1)
                  goto data_short;

               tga_cmap[i][0] =
               tga_cmap[i][1] =
               tga_cmap[i][2] = buf[0];
            }
            break;

       case 15: /* 5 bits of RGB */
       case 16:
            for(i=hdr.CMapStart; i < ( hdr.CMapStart + hdr.CMapLength ); i++)
            {
               if(ReadOK(zf, buf, 2) != 2)
                  goto data_short;

               tga_cmap[i][0] = (buf[1]<<1) & 0xf8;
               tga_cmap[i][1] = ((buf[1]>>5)& 0x04) | ((buf[1]<<6)&0xc0)
                                 | ((buf[0]>>2)&0x38);
               tga_cmap[i][2] = (buf[0]<<3) & 0xf8;
            }
            break;

       case 32:   /* 8 bits of RGB + alpha */
            n = 1;
       case 24:   /* 8 bits of RGB */
            n += 3;
            for(i = hdr.CMapStart; i < ( hdr.CMapStart + hdr.CMapLength ); i++)
            {
               if(ReadOK(zf, buf, n) != n)
                  goto data_short;

               tga_cmap[i][0]= buf[2]; /* R */
               tga_cmap[i][1]= buf[1]; /* G */
               tga_cmap[i][2]= buf[0]; /* B */
            }
            break;
      }
   }

   /* work out virtical advance and retrace */
   if(hdr.OrgBit) /* if upper left */
      span = hdr.Width;
   else
      span = -hdr.Width;

   if (hdr.IntrLve == TGA_IL_Four )
   {
      hretrace = 4 * span - hdr.Width;
   } else if(hdr.IntrLve == TGA_IL_Two )
   {
      hretrace = 2 * span - hdr.Width;
   } else
   {
      hretrace = span - hdr.Width;
   }

   vretrace = (hdr.Height-1) * -span;

   nopix = (hdr.Width * hdr.Height * 3);  /* total number of pixels */

   hretrace *= 3;                         /* scale for bytes per pixel */
   vretrace *= 3;
   span     *= 3;

   if(hdr.OrgBit) /* if upper left */
   {
      data = my_buffer;
      eoi  = my_buffer + nopix;
   } else
   {
      eoi  = my_buffer + span;
      data = my_buffer + nopix + span;
   }

   /* Now read in the image data */
   rd_count = wr_count = 0;
   if (!hdr.RLE)           /* If not rle, all are literal */
   {
      wr_count =
      rd_count = nopix;
   }

   for(y = 0 ; y < hdr.Height; y++, data += hretrace)
   {
      guchar buf[6];       /* 0, 1 for pseudo, 2, 3, 4 for BGR */

      if (data == eoi)     /* adjust for possible retrace */
      {
         data += vretrace;
         eoi += span;
      }

      /* Do another line of pixels */
      for(x = 0 ; x < hdr.Width; x++)
      {
         if(wr_count == 0)
         { /* Deal with run length encoded. */
            if(ReadOK(zf, buf, 1) != 1)
               goto data_short;

            if(buf[0] & 0x80)
            {  /* repeat count */
               wr_count = (guint)buf[0] - 127;  /* no. */
               rd_count = 1;                    /* need to read pixel to repeat */
            } else
            {  /* number of literal pixels */
               wr_count =
               rd_count = buf[0] + 1;
            }
         }
         if(rd_count != 0)
         {  /* read a pixel and decode into RGB */
            switch (hdr.PixelDepth)
            {
               case 8:           /* Pseudo or Gray */
                  if(ReadOK(zf, buf, 1) != 1)
                     goto data_short;
                  break;

               case 15:          /* 5 bits of RGB or 16 pseudo color */
               case 16:
                  if(ReadOK(zf, buf, 2) != 2)
                     goto data_short;
                  break;

               case 24:          /* 8 bits of B G R */
                  if(ReadOK(zf, &buf[2], 3) != 3)
                     goto data_short;
                  break;

               case 32:          /* 8 bits of B G R + alpha */
                  if(ReadOK(zf, &buf[2], 4) != 4)
                     goto data_short;
               break;
            }
            rd_count--;
         }

         switch(pixlen)
         {
            case 1:              /* 8 bit pseudo or gray */
               if(hdr.ColorMapType != 0)
               {
                  *data++ = tga_cmap[buf[0]][0];      /* R */
                  *data++ = tga_cmap[buf[0]][1];      /* G */
                  *data++ = tga_cmap[buf[0]][2];      /* B */
               } else
               {
                  *data++ = buf[0];                   /* R */
                  *data++ = buf[0];                   /* G */
                  *data++ = buf[0];                   /* B */
               }
               break;

            case 2:              /* 16 bit pseudo */
               buf[4] = (buf[1]<<1) & 0xf8;                       /* R */
               buf[3] = ((buf[1]>>5)& 0x04) | ((buf[1]<<6)&0xc0)
                           | ((buf[0]>>2)&0x38);                  /* G */
               buf[2] = (buf[0]<<3) & 0xf8;                       /* B */
               if(hdr.ColorMapType != 0)
               {
                  *data++ = tga_cmap[buf[4]][0];
                  *data++ = tga_cmap[buf[3]][1];
                  *data++ = tga_cmap[buf[2]][2];
               } else
               {
                  *data++ = buf[4];
                  *data++ = buf[3];
                  *data++ = buf[2];
               }
               break;

            case 3:              /* True color */
            case 4:
               *data++ = buf[4]; /* R */
               *data++ = buf[3]; /* G */
               *data++ = buf[2]; /* B */
               break;
         }
         wr_count--;
      }
   }

   for(y = 0; y < hdr.Height; y++)
   {
      row = my_buffer + (y * hdr.Width * 3);
      if ((*func) (row, hdr.Width, 0, y, 3, -1, 0)) break;
   }

   fclose(zf);
   g_free(my_buffer);
   return TRUE;

data_short:
   fclose(zf);
   g_free(my_buffer);
   return FALSE;
}
