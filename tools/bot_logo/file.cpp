//
// bot_logo - botman@planethalflife.com
//
// file.cpp - misc. file I/O routines
//

#include "windows.h"
#include "resource.h"

#include <math.h>

#include "bot_logo.h"


int num_mips = 0;
miptex_t *pMips[MAX_MIPS];

BYTE pixdata[256];
float linearpalette[256][3];
BYTE lbmpalette[256 * 3];
float d_red, d_green, d_blue;
int colors_used;
int color_used[256];
float	maxdistortion;

extern PBITMAPINFO pbmi;

extern RGBQUAD OriginalPalette[256];
extern RGBQUAD Palette[256];

extern BYTE *pOriginalData;
extern BOOL g_monochrome;


UINT GetPathFromFullPathName( LPCTSTR lpFullPathName, LPTSTR lpPathBuffer,
                              int nPathBufferLength )
{
   int nLength;
   int i;

   if ((nLength = lstrlen( lpFullPathName )) > nPathBufferLength)
      return( nLength );

   lstrcpy( lpPathBuffer, lpFullPathName );

   for( i = lstrlen( lpPathBuffer );
        (lpPathBuffer[i] != '\\') && (lpPathBuffer[i] != ':'); i-- )
      ;
   if (':' == lpPathBuffer[i])
      lpPathBuffer[i+1] = '\0';
   else
      lpPathBuffer[i] = '\0';

   nLength = i;

   for (i = 0; i < nLength; i++)
      lpPathBuffer[i] = tolower(lpPathBuffer[i]);

   return( (UINT) nLength );
}

HANDLE FindFile(HANDLE hFile, CHAR *file, CHAR *filename)
{
   WIN32_FIND_DATA pFindFileData;

   if (hFile == NULL)
   {
      hFile = FindFirstFile(filename, &pFindFileData);

      if (hFile == INVALID_HANDLE_VALUE)
         hFile = NULL;
   }
   else
   {
      if (FindNextFile(hFile, &pFindFileData) == 0)
      {
         FindClose(hFile);
         hFile = NULL;
      }
   }

   if (hFile != NULL)
      strcpy(file, pFindFileData.cFileName);
   
   return hFile;
}


HANDLE FindDirectory(HANDLE hFile, CHAR *dirname, CHAR *filename)
{
   WIN32_FIND_DATA pFindFileData;

   dirname[0] = 0;

   if (hFile == NULL)
   {
      hFile = FindFirstFile(filename, &pFindFileData);

      if (hFile == INVALID_HANDLE_VALUE)
         hFile = NULL;

      while (pFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
      {
         if (FindNextFile(hFile, &pFindFileData) == 0)
         {
            FindClose(hFile);
            hFile = NULL;
            return hFile;
         }
      }

      strcpy(dirname, pFindFileData.cFileName);

      return hFile;
   }
   else
   {
      if (FindNextFile(hFile, &pFindFileData) == 0)
      {
         FindClose(hFile);
         hFile = NULL;
         return hFile;
      }

      while (pFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
      {
         if (FindNextFile(hFile, &pFindFileData) == 0)
         {
            FindClose(hFile);
            hFile = NULL;
            return hFile;
         }
      }

      strcpy(dirname, pFindFileData.cFileName);

      return hFile;
   }
}


BOOL CheckBitmapFormat(LPTSTR lpszFileName)
{
   HANDLE              hFile;          // Bitmap filename
   BITMAPINFOHEADER    sBIH;           // Used if this is a Windows bitmap
   BITMAPFILEHEADER    sBFH;           // This is always filled in
   DWORD               dwDummy;

   hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

   if(hFile == INVALID_HANDLE_VALUE)
      return FALSE;

   if (!ReadFile(hFile, &sBFH, sizeof(BITMAPFILEHEADER), &dwDummy, NULL))
      return FALSE;

   if(sBFH.bfType != (UINT)0x4D42)
      return FALSE;

   SetFilePointer(hFile, sizeof(BITMAPFILEHEADER), NULL, FILE_BEGIN);
   
   if (!ReadFile(hFile, &sBIH, sizeof(BITMAPINFOHEADER), &dwDummy, NULL))
      return FALSE;

   if ((sBIH.biBitCount != 8) || (sBIH.biCompression != BI_RGB))
      return FALSE;
      
   if (((sBIH.biWidth % 16) != 0) || ((sBIH.biHeight % 16) != 0))
      return FALSE;
      
   if ((sBIH.biWidth > 256) || (sBIH.biHeight > 256))
      return FALSE;

   CloseHandle(hFile);
   
   return TRUE;
}


BOOL LoadBitmapFromFile(HDC hDC, LPTSTR lpszFileName)
{
   HANDLE              hFile;          // Bitmap filename
   DWORD               dwBitmapType,   // The type of the bitmap
                       dwSize;         // A size of memory to allocate
   int                 nPalEntries;    // # of entries in the palette
   BITMAPINFOHEADER    sBIH;           // Used if this is a Windows bitmap
   BITMAPFILEHEADER    sBFH;           // This is always filled in
   DWORD               dwDummy;
   BOOL                status;
   int                 index;

   status = FALSE;

   hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

   if(hFile != INVALID_HANDLE_VALUE)
   {
      // Read the BITMAPFILEHEADER from the file
      if (ReadFile(hFile, &sBFH, sizeof(BITMAPFILEHEADER), &dwDummy, NULL))
      {
         // Any valid bitmap file should have filetype = 'BM'
         if(sBFH.bfType == (UINT)0x4D42)
         {
            ReadFile(hFile, &dwBitmapType, sizeof(DWORD), &dwDummy, NULL);

            SetFilePointer(hFile, sizeof(BITMAPFILEHEADER), NULL, FILE_BEGIN);
   
            // Read in the INFOHEADER
            ReadFile(hFile, &sBIH, sizeof(BITMAPINFOHEADER), &dwDummy, NULL);

            // Figure out the number of entries in the palette
            // i.e. if the number of Bits per pixel (biBitCount) is 8,
            // the number of palette entries will be 1 << 8 = 256

            nPalEntries = ((UINT)1 << sBIH.biBitCount);

            // If the bitmap is not compressed, set the BI_RGB flag.
            sBIH.biCompression = BI_RGB;

            // Compute the number of bytes in the array of color
            // indices and store the result in biSizeImage.
            // For Windows NT/2000, the width must be DWORD aligned unless
            // the bitmap is RLE compressed. This example shows this.
            // For Windows 95/98, the width must be WORD aligned unless the
            // bitmap is RLE compressed.
            sBIH.biSizeImage = ((sBIH.biWidth * sBIH.biBitCount +31) & ~31) /8 * sBIH.biHeight;

            // Set biClrImportant to 0, indicating that all of the
            // device colors are important.
            sBIH.biClrImportant = 0;

            // The INFOHEADER version of a bitmap file uses
            // RGB Quads to store the color map
            dwSize = (nPalEntries * sizeof(RGBQUAD));

            // Read the palette data...
            ReadFile(hFile, OriginalPalette, dwSize, &dwDummy, NULL);

            memcpy(Palette, OriginalPalette, dwSize);

            g_monochrome = TRUE;

            for (index = 0; index < nPalEntries; index++)
            {
               if ((Palette[index].rgbRed != Palette[index].rgbGreen) ||
                   (Palette[index].rgbGreen != Palette[index].rgbBlue))
                  g_monochrome = FALSE;
            }

            if (pbmi)
               LocalFree(pbmi);

            pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) +
                                            sizeof(RGBQUAD) * (nPalEntries));

            memcpy(pbmi, &sBIH, sizeof(BITMAPINFOHEADER));

            memcpy((BYTE *)pbmi + sizeof(BITMAPINFOHEADER), OriginalPalette,
                   sizeof(RGBQUAD) * (nPalEntries));

            if (pOriginalData)
               LocalFree(pOriginalData);

            pOriginalData = (BYTE *)LocalAlloc(LPTR, sBIH.biSizeImage);

            // Read the bitmap data into memory
            ReadFile(hFile, pOriginalData, sBIH.biSizeImage, &dwDummy, NULL);

            status = TRUE;
         }
      }

      CloseHandle(hFile);
   }

   return(status);
}


BOOL LoadWADFile(LPTSTR lpszFileName)
{
   HANDLE  hFile;
   DWORD   dwDummy;
   DWORD   dwSize;
   int     index;
   wadinfo_t  WadHeader;
   lumpinfo_t *pLumpInfo = NULL;
   lumpinfo_t *pLump;

   for (index = 0; index < MAX_MIPS; index++)
      pMips[index] = NULL;

   hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

   if (hFile == INVALID_HANDLE_VALUE)
      return FALSE;

   if (!ReadFile(hFile, &WadHeader, sizeof(WadHeader), &dwDummy, NULL))
   {
      CloseHandle(hFile);  // can't read WAD header
      return FALSE;
   }

   if (strncmp(WadHeader.identification,"WAD3",4))
   {
      CloseHandle(hFile);
      return FALSE;  // not a WAD3 file
   }

   if (WadHeader.numlumps >= MAX_MIPS)
   {
      CloseHandle(hFile);
      return FALSE;  // too many lumps
   }

   if (SetFilePointer(hFile, WadHeader.infotableofs, NULL, FILE_BEGIN) == 0)
   {
      CloseHandle(hFile);
      return FALSE;  // can't seek to lump info table
   }

   dwSize = WadHeader.numlumps * sizeof(lumpinfo_t);

   pLumpInfo = (lumpinfo_t *)LocalAlloc(LPTR, dwSize);

   if (!ReadFile(hFile, pLumpInfo, dwSize, &dwDummy, NULL))
   {
      CloseHandle(hFile);  // can read lump info table
      return FALSE;
   }

   pLump = pLumpInfo;
   num_mips = 0;

   for (index = 0; index < WadHeader.numlumps; index++)
   {
      if (SetFilePointer(hFile, pLump->filepos, NULL, FILE_BEGIN) == 0)
      {
         CloseHandle(hFile);
         return FALSE;  // can't seek to lump table
      }

      pMips[index] = (miptex_t *)LocalAlloc(LPTR, pLump->size);

      if (!ReadFile(hFile, pMips[index], pLump->size, &dwDummy, NULL))
      {
         CloseHandle(hFile);
         return FALSE;
      }

      pLump++;
      num_mips++;
   }

   LocalFree(pLumpInfo);

   CloseHandle(hFile);

   return TRUE;
}


BOOL LoadBitmapFromMip(int index)
{
   BYTE *pMipData;
   palette_t *pMipPalette;
   int idx;
   unsigned int row;
   int mip1_size, mip2_size, mip3_size, mip4_size;

   mip1_size = pMips[index]->width * pMips[index]->height;
   mip2_size = (pMips[index]->width * pMips[index]->height) / 4;
   mip3_size = (pMips[index]->width * pMips[index]->height) / 16;
   mip4_size = (pMips[index]->width * pMips[index]->height) / 64;

   pMipPalette = (palette_t *)((BYTE *)pMips[index] + sizeof(miptex_t) +
                               mip1_size + mip2_size + mip3_size + mip4_size);

   if (pbmi)
      LocalFree(pbmi);

   pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) +
                                   256 * sizeof(RGBQUAD));

   memset(pbmi, 0, sizeof(BITMAPINFOHEADER));

   pbmi->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
   pbmi->bmiHeader.biWidth = pMips[index]->width;
   pbmi->bmiHeader.biHeight = pMips[index]->height;
   pbmi->bmiHeader.biPlanes = 1;
   pbmi->bmiHeader.biBitCount = 8;
   pbmi->bmiHeader.biCompression = BI_RGB;
   pbmi->bmiHeader.biSizeImage = pMips[index]->offsets[1] - pMips[index]->offsets[0];
   pbmi->bmiHeader.biXPelsPerMeter = 0;
   pbmi->bmiHeader.biYPelsPerMeter = 0;
   pbmi->bmiHeader.biClrUsed = 256;
   pbmi->bmiHeader.biClrImportant = 0;

   for (idx = 0; idx < 256; idx++)
   {
      pbmi->bmiColors[idx].rgbRed = pMipPalette->palette[idx * 3 + 0];
      pbmi->bmiColors[idx].rgbGreen = pMipPalette->palette[idx * 3 + 1];
      pbmi->bmiColors[idx].rgbBlue = pMipPalette->palette[idx * 3 + 2];
      pbmi->bmiColors[idx].rgbReserved = 0;
   }

   memcpy(OriginalPalette, pbmi->bmiColors, 256 * sizeof(RGBQUAD));

   if (pOriginalData)
      LocalFree(pOriginalData);

   pOriginalData = (BYTE *)LocalAlloc(LPTR, pbmi->bmiHeader.biSizeImage);

   pMipData = (BYTE *)pMips[index] + sizeof(miptex_t);

   idx = pMips[index]->height - 1;

   for (row = 0; row < pMips[index]->height; row++)
   {
      memcpy(pOriginalData+(pMips[index]->width * row),
             pMipData+(pMips[index]->width * idx),
             pMips[index]->width);

      idx--;
   }

   return TRUE;
}


BOOL WriteWADFile(LPTSTR lpszFileName)
{
   HANDLE  hFile;
   DWORD   dwDummy;
   wadinfo_t  WadHeader;
   int     index;
   int     num_lumps;
   int     offset;
   DWORD   dwSize;
   lumpinfo_t lump;

   hFile = CreateFile(lpszFileName, GENERIC_WRITE, FILE_SHARE_READ,
                      NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

   if (hFile == INVALID_HANDLE_VALUE)
      return FALSE;

   strcpy(WadHeader.identification,"WAD3");

   num_lumps = 0;

   offset = sizeof(wadinfo_t);

   for (index=0; index < MAX_MIPS; index++)
   {
      if (pMips[index])
      {
         offset = offset + pMips[index]->offsets[3] +
                  (pMips[index]->height * pMips[index]->width / 64) +
                  sizeof(palette_t);

         num_lumps++;
      }
   }

   WadHeader.numlumps = num_lumps;
   WadHeader.infotableofs = offset;

   if (!WriteFile(hFile, &WadHeader, sizeof(WadHeader), &dwDummy, NULL))
   {
      CloseHandle(hFile);  // can't write WAD header
      return FALSE;
   }

   for (index = 0; index < MAX_MIPS; index++)
   {
      if (pMips[index])
      {
         dwSize = pMips[index]->offsets[3] +
                  (pMips[index]->height * pMips[index]->width / 64) +
                  sizeof(palette_t);

         if (!WriteFile(hFile, pMips[index], dwSize, &dwDummy, NULL))
         {
            CloseHandle(hFile);  // can't write WAD header
            return FALSE;
         }
      }
   }

   offset = sizeof(wadinfo_t);

   for (index = 0; index < MAX_MIPS; index++)
   {
      if (pMips[index])
      {
         dwSize = pMips[index]->offsets[3] +
                  (pMips[index]->height * pMips[index]->width / 64) +
                  sizeof(palette_t);

         lump.filepos = offset;
         lump.disksize = dwSize;
         lump.size = dwSize;
         lump.type = 0x43;  //  Half-Life WAD3 mip
         lump.compression = 0;
         lump.pad1 = 0;
         lump.pad2 = 0;
         memset(lump.name, 0, sizeof(lump.name));
         strcpy(lump.name, pMips[index]->name);

         if (!WriteFile(hFile, &lump, sizeof(lumpinfo_t), &dwDummy, NULL))
         {
            CloseHandle(hFile);  // can't write WAD header
            return FALSE;
         }

         offset += dwSize;
      }
   }

   CloseHandle(hFile);

   return TRUE;
}


BYTE AddColor( float r, float g, float b )
{
   int i;

   for (i = 0; i < 255; i++)
   {
      if (!color_used[i])
      {
         linearpalette[i][0] = r;
         linearpalette[i][1] = g;
         linearpalette[i][2] = b;

         if (r < 0) r = 0.0;
         if (r > 1.0) r = 1.0;
         lbmpalette[i*3+0] = (BYTE)(pow( r, 1.0 / 2.2) * 255);

         if (g < 0) g = 0.0;
         if (g > 1.0) g = 1.0;
         lbmpalette[i*3+1] = (BYTE)(pow( g, 1.0 / 2.2) * 255);

         if (b < 0) b = 0.0;
         if (b > 1.0) b = 1.0;
         lbmpalette[i*3+2] = (BYTE)(pow( b, 1.0 / 2.2) * 255);

         color_used[i] = 1;
         colors_used++;

         return i;
      }
   }

   return 0;
}


BYTE AveragePixels (int count)
{
   float r,g,b;
   int   i;
   int   vis;
	int   pix;
	float dr, dg, db;
	float bestdistortion, distortion;
	int   bestcolor;

   vis = 0;
   r = g = b = 0;

   for (i = 0; i < count; i++)
   {
      pix = pixdata[i];
      r += linearpalette[pix][0];
      g += linearpalette[pix][1];
      b += linearpalette[pix][2];
   }

   r /= count;
   g /= count;
   b /= count;

   r += d_red;
   g += d_green;
   b += d_blue;
	
//
// find the best color
//
   bestdistortion = 3.0;
   bestcolor = -1;

   for (i = 0; i < 255; i++)
   {
      if (color_used[i])
      {
         pix = i;	//pixdata[i];

         dr = r - linearpalette[i][0];
         dg = g - linearpalette[i][1];
         db = b - linearpalette[i][2];

         distortion = dr*dr + dg*dg + db*db;
         if (distortion < bestdistortion)
         {
            if (!distortion)
            {
               d_red = d_green = d_blue = 0;	// no distortion yet
               return pix;		// perfect match
            }

            bestdistortion = distortion;
            bestcolor = pix;
         }
      }
   }

   if (bestdistortion > 0.001 && colors_used < 255)
   {
      bestcolor = AddColor( r, g, b );
      d_red = d_green = d_blue = 0;
      bestdistortion = 0;
   }
   else
   {
      // error diffusion
      d_red = r - linearpalette[bestcolor][0];
      d_green = g - linearpalette[bestcolor][1];
      d_blue = b - linearpalette[bestcolor][2];
   }

   if (bestdistortion > maxdistortion)
      maxdistortion = bestdistortion;

   return bestcolor;
}
