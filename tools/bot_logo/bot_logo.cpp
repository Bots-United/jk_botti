//
// bot_logo - botman@planethalflife.com
//
// bot_logo.cpp - misc. file I/O routines
//

#include "windows.h"
#include "resource.h"
#include "bot_logo.h"

#include <stdio.h>
#include <math.h>
#include <sys/stat.h>


BYTE colors[8][3]=
{
   {255,180,24},  // orange
   {0,60,255},    // blue
   {0,167,255},   // light blue
   {0,167,0},     // green
   {255,73,0},    // red
   {123,73,0},    // brown
   {100,100,100}, // grey
   {36,36,36}     // black
};


PBITMAPINFO pbmi = NULL;
HBITMAP hBitmap = NULL;
HBRUSH h_background = NULL;

HPALETTE hPalette = NULL;
RGBQUAD OriginalPalette[256];
RGBQUAD Palette[256];
BOOL g_monochrome;

BYTE *pOriginalData = NULL;
LPBYTE pBitmapData = NULL;

CHAR g_szBitmapName[MAX_PATH];

BYTE g_palette_color = 0;  // currently selected palette color
int  offset_x = 0;
int  offset_y = 0;
int  size_x, size_y;

CHAR g_szFileName[MAX_PATH];
UINT g_ReturnStatus;
CHAR g_szMODdir[MAX_PATH];

TCHAR szExePathName[MAX_PATH];  // full pathname of the application
TCHAR szPath[MAX_PATH];  // path where the running application resides
TCHAR szParentPath[MAX_PATH];  // path to parent directory


//extern lumpinfo_t *pLumpInfo;
extern miptex_t *pMips[MAX_MIPS];
extern int num_mips;

extern BYTE pixdata[256];
extern float d_red, d_green, d_blue;
extern float linearpalette[256][3];
extern int colors_used;
extern int color_used[256];
extern BYTE lbmpalette[256 * 3];
extern float	maxdistortion;

BOOL changes = FALSE;

BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID NEAR GoModalDialogBoxParam( HINSTANCE hInstance, LPCSTR lpszTemplate,
                                 HWND hWnd, DLGPROC lpDlgProc, LPARAM lParam );
BOOL CALLBACK MODDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
   HWND hWnd;
   MSG msg;
   int length;

   GetModuleFileName( hInstance, szExePathName,
                      sizeof(szExePathName)/sizeof(TCHAR) );
   length = GetPathFromFullPathName( szExePathName, szPath,
                                     sizeof(szPath)/sizeof(TCHAR) );

   if ((strcmp(&szPath[length-8], "\\hpb_bot") != 0) &&
       (strcmp(&szPath[length-8], "\\release") != 0) &&
       (strcmp(&szPath[length-6], "\\debug") != 0))
   {
      MessageBox (NULL, "HPB bot is not installed properly", NULL, MB_OK);
      return 1;
   }

   GetPathFromFullPathName( szPath, szParentPath,
                            sizeof(szParentPath)/sizeof(TCHAR) );

   GoModalDialogBoxParam( hInstance, MAKEINTRESOURCE( MODDLGBOX ),
                          NULL, (DLGPROC) MODDlgProc, 0L );

   if (g_ReturnStatus == IDCANCEL)
      return 1;

   hWnd = CreateDialog(hInstance, MAKEINTRESOURCE( IDD_DIALOG1 ), 0, DialogProc);

   if (!hWnd)
   {
      MessageBox (NULL, "Can't create Dialog", NULL, MB_OK);
      return 1;
   }

   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return msg.wParam;
}


void GetBitmapPosition(HWND hWnd)
{
   HWND    hDlgItem;
   RECT    listRect;
   RECT    sRect;
   int     listbox_height;
   int     listbox_offset;
   int     dialog_height;
   int     dialog_offset;
   int     scrn_x, scrn_y;

   hDlgItem = GetDlgItem(hWnd, IDC_LIST1);

   GetWindowRect(hDlgItem, &listRect);  // get the listbox screen position
   GetWindowRect(hWnd, &sRect);  // get the dialog box screen position

   listbox_offset = listRect.top - sRect.top;
   listbox_height = listRect.bottom - listRect.top;

   dialog_height = sRect.bottom - sRect.top;

   GetClientRect(hWnd,  &sRect);

   if (pbmi->bmiHeader.biHeight <= listbox_height)
   {
      dialog_offset = dialog_height - (sRect.bottom - sRect.top);

      scrn_x = ((sRect.right - sRect.left) / 2) - pbmi->bmiHeader.biWidth/2;
      scrn_y = listbox_offset + (listbox_height/2) -
               dialog_offset - (pbmi->bmiHeader.biHeight/2);
   }
   else
   {
      scrn_x = ((sRect.right - sRect.left) / 2) - pbmi->bmiHeader.biWidth/2;
      scrn_y = ((sRect.bottom - sRect.top) / 2) - pbmi->bmiHeader.biHeight / 2;
   }

   offset_x = scrn_x;
   offset_y = scrn_y;

   size_x = pbmi->bmiHeader.biWidth;
   size_y = pbmi->bmiHeader.biHeight;
}


void RedrawClientArea(HWND hWnd, HDC hDC)
{
   HDC     hDCMem;

   if (hBitmap)
   {
      GetBitmapPosition(hWnd);

      hDCMem = CreateCompatibleDC(hDC);

      SelectObject (hDCMem, hBitmap);
      SetMapMode (hDCMem, GetMapMode(hDC));

      if (hPalette)
      {
         SelectPalette(hDC, hPalette, FALSE);
         RealizePalette(hDC);
         SelectPalette(hDCMem, hPalette, FALSE);
         RealizePalette(hDCMem);
      }
      
      BitBlt(hDC, offset_x, offset_y, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight,
             hDCMem, 0, 0, SRCCOPY);

      DeleteDC(hDCMem);
   }

   return;
}


void MakeBitmap(HDC hDC, RGBQUAD *pPalette, BYTE *pData)
{
   int           palette_size;
   int           dwSize;
   LPLOGPALETTE  lpMem;          // A mem pointer to the LOGPALETTE
   HANDLE        hMem;           // A handle to global memory
   int           index;

   palette_size = (1 << pbmi->bmiHeader.biBitCount);

   memcpy(pbmi->bmiColors, pPalette, palette_size * 4);

   dwSize = sizeof(LOGPALETTE) + (palette_size * sizeof(PALETTEENTRY));

   hMem = GlobalAlloc(GHND, dwSize);
   lpMem = (LPLOGPALETTE)GlobalLock(hMem);

   lpMem->palVersion    = 0x0300;
   lpMem->palNumEntries = palette_size;

   for (index=0; index < palette_size; index++)
   {
      lpMem->palPalEntry[index].peRed   = pPalette[index].rgbRed;
      lpMem->palPalEntry[index].peGreen = pPalette[index].rgbGreen;
      lpMem->palPalEntry[index].peBlue  = pPalette[index].rgbBlue;
      lpMem->palPalEntry[index].peFlags = 0;
   }

   if (hPalette)
      DeleteObject(hPalette);

   hPalette = CreatePalette(lpMem);

   GlobalUnlock(hMem);
   GlobalFree(hMem);

   if (hBitmap)
      DeleteObject(hBitmap);

   hBitmap = CreateDIBSection(hDC, pbmi, DIB_RGB_COLORS,
                              (VOID **)&pBitmapData, NULL, 0);

   memcpy(pBitmapData, pData, pbmi->bmiHeader.biSizeImage);
}


BOOL CenterWindow (HWND hWnd)
{
    RECT    rRect, rParentRect;
    HWND    hParentWnd;
    int     wParent, hParent, xNew, yNew;
    int     w, h;

    GetWindowRect (hWnd, &rRect);
    w = rRect.right - rRect.left;
    h = rRect.bottom - rRect.top;

    if (NULL == (hParentWnd = GetParent( hWnd )))
       hParentWnd = GetDesktopWindow();

    GetWindowRect( hParentWnd, &rParentRect );

    wParent = rParentRect.right - rParentRect.left;
    hParent = rParentRect.bottom - rParentRect.top;

    xNew = wParent/2 - w/2 + rParentRect.left;
    yNew = hParent/2 - h/2 + rParentRect.top;

    return SetWindowPos (hWnd, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


BOOL CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   HDC hDC;
   TEXTMETRIC tm;
   PAINTSTRUCT ps;
   RECT rect;
   COLORREF clrref;
   LPDRAWITEMSTRUCT pDIS;
   HBRUSH hbr;
   int lineHeight;
   HWND hWndList;
   int nItem;
   HANDLE hFile;
   char filename[MAX_PATH];
   char file[MAX_PATH];
   char full_filename[MAX_PATH];
   int length;
   int index;
   HWND hItemWnd;
   BYTE palette_color;
   BYTE red, green, blue;
   float r_scale, g_scale, b_scale;
   CHAR item_name[16];
   CHAR wad_name[16];
   int x,y;
   int width, height;
   int difference, min_difference, min_index;
   DWORD dwSize;
   int mip_index;
   CHAR mip_name[16];
   int value, max_value;
   palette_t *pMipPalette;
   char src_filename[MAX_PATH], dest_filename[MAX_PATH];
   int miplevel, mipstep;
   unsigned char *pLump, *pLump0;
   int mip_width, mip_height;
   int count, xx, yy;
   int testpixel;

   switch (message)
   {
      case WM_INITDIALOG:

         if (!CenterWindow( hWnd ))
            return( FALSE );

         clrref = GetSysColor(COLOR_BTNFACE);
         h_background = CreateSolidBrush(clrref);

         hItemWnd = GetDlgItem(hWnd, IDC_ADD);
         EnableWindow(hItemWnd, FALSE);

         hItemWnd = GetDlgItem(hWnd, IDC_REMOVE);
         EnableWindow(hItemWnd, FALSE);

         hItemWnd = GetDlgItem(hWnd, IDC_COMBO1);
         EnableWindow(hItemWnd, FALSE);

         g_palette_color = 0;

         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));
         SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)" "));

         hFile = NULL;
         strcpy(filename, szParentPath);
         strcat(filename, "\\logos\\*.bmp");

         // display the .bmp files in the logos folder...
         while ((hFile = FindFile(hFile, file, filename)) != NULL)
         {
            strcpy(full_filename, szParentPath);
            strcat(full_filename, "\\logos\\");
            strcat(full_filename, file);

            if (CheckBitmapFormat(full_filename))
            {
               length = strlen(file);

               file[length-4] = 0;

               SendDlgItemMessage(hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM)((LPCSTR)file));
            }
         }

         SendDlgItemMessage(hWnd, IDC_LIST1, LB_SETCURSEL, -1, 0L);

         strcpy(filename, szParentPath);
         strcat(filename, "\\");
         strcat(filename, g_szMODdir);
         strcat(filename, "\\decals.wad");

         if (!LoadWADFile(filename))
         {
            strcpy(filename, szParentPath);
            strcat(filename, "\\");
            strcat(filename, "valve");
            strcat(filename, "\\decals.wad");

            if (!LoadWADFile(filename))
            {
               MessageBox(hWnd, "Error loading WAD file!", "Error", MB_OK);
               SendMessage(hWnd, WM_COMMAND, IDCLOSE, 0L);

               return FALSE;
            }

            strcpy(g_szMODdir, "valve");

            MessageBox(hWnd, "This MOD uses the decals.wad file\nfrom the Half-Life valve directory", "Warning", MB_OK);
         }

         // display the lumps from the WAD file...
         for (index=0; index < MAX_MIPS; index++)
         {
            if (pMips[index])
            {
               if (strncmp(pMips[index]->name, "{__", 3) == 0)
               {
                  SendDlgItemMessage(hWnd, IDC_LIST2, LB_ADDSTRING, 0,
                                     (LPARAM)((LPCSTR)&(pMips[index]->name[3])));
               }

               if (strncmp(pMips[index]->name, "__", 2) == 0)
               {
                  SendDlgItemMessage(hWnd, IDC_LIST2, LB_ADDSTRING, 0,
                                     (LPARAM)((LPCSTR)&(pMips[index]->name[2])));
               }
            }
         }


         SendDlgItemMessage(hWnd, IDC_LIST2, LB_SETCURSEL, -1, 0L);
 
        break;

      case WM_MEASUREITEM:

         hDC = GetDC( hWnd );
         GetTextMetrics( hDC, &tm );
         ReleaseDC( hWnd, hDC );

         lineHeight = tm.tmExternalLeading +
                      tm.tmInternalLeading + tm.tmHeight;

         ((MEASUREITEMSTRUCT FAR *)(lParam))->itemHeight = lineHeight;

         return TRUE;

      case WM_DRAWITEM:

         pDIS = (LPDRAWITEMSTRUCT) lParam;

         // Draw the focus rectangle for an empty list box or an
         // empty combo box to indicate that the control has the focus

         if ((int)(pDIS->itemID) < 0)
         {
//            if ((pDIS->itemState) & (ODS_FOCUS))
//               DrawFocusRect (pDIS->hDC, &pDIS->rcItem);
            return TRUE;
         }

         if ((pDIS->itemAction == ODA_SELECT) ||
             (pDIS->itemAction == ODA_DRAWENTIRE))
         {
            hbr = NULL;

            palette_color = 0;

            if ((pDIS->itemID >= 1) && (pDIS->itemID <= 8))
            {
               palette_color = pDIS->itemID;

               red   = colors[palette_color-1][0];
               green = colors[palette_color-1][1];
               blue  = colors[palette_color-1][2];

//               hDC = CreateCompatibleDC(pDIS->hDC);
               hbr = CreateSolidBrush(RGB(red,green,blue));

               SelectObject(pDIS->hDC, hbr);
               Rectangle(pDIS->hDC, pDIS->rcItem.left, pDIS->rcItem.top,
                                    pDIS->rcItem.right,  pDIS->rcItem.bottom);
               DeleteObject(hbr);
//               DeleteDC(hDC);
            }

//            if (pDIS->itemState & ODS_FOCUS)
//               DrawFocusRect (pDIS->hDC, &pDIS->rcItem);

//            if (pDIS->itemState & ODS_SELECTED)
//            {
//               DrawFocusRect(pDIS->hDC, &pDIS->rcItem);
//            }
         }
//         else if (pDIS->itemAction == ODA_FOCUS)
//         {
//            DrawFocusRect (pDIS->hDC, &pDIS->rcItem);
//         }

         return TRUE; 

      case WM_PAINT:

         hDC = BeginPaint(hWnd, &ps);

         RedrawClientArea(hWnd, hDC);

         EndPaint(hWnd, &ps);

         return TRUE;

      case WM_COMMAND:

         switch (LOWORD(wParam))
         {
            case IDC_LIST1:
               if (HIWORD(wParam) == LBN_SELCHANGE)
               {
                  index = SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETCURSEL, 0, 0L);

                  SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETTEXT, index, (LPARAM)((LPSTR)filename));

                  strcpy(g_szBitmapName, filename);

                  strcpy(g_szFileName, szParentPath);
                  strcat(g_szFileName, "\\logos\\");
                  strcat(g_szFileName, filename);
                  strcat(g_szFileName, ".bmp");

                  hDC = GetDC(hWnd);

                  if (hBitmap)
                  {
                     if ((pbmi->bmiHeader.biHeight > 128) ||
                         (pbmi->bmiHeader.biWidth > 128))
                     {
                        GetClientRect(hWnd, &rect);
                        InvalidateRect(hWnd, &rect, TRUE);
                     }
                  }

                  LoadBitmapFromFile(hDC, g_szFileName);

                  if ((hBitmap) && (h_background))
                  {
                     rect.left = offset_x;
                     rect.top = offset_y;
                     rect.right = offset_x + size_x;
                     rect.bottom = offset_y + size_y;

                     FillRect(hDC, &rect, h_background);
                  }

                  MakeBitmap(hDC, OriginalPalette, pOriginalData);

                  ReleaseDC(hWnd, hDC);


                  SendDlgItemMessage(hWnd, IDC_LIST2, LB_SETCURSEL, -1, 0L);

                  SendDlgItemMessage(hWnd, IDC_COMBO1, CB_SETCURSEL, 0, 0L);

                  g_palette_color = 0;

                  hItemWnd = GetDlgItem(hWnd, IDC_ADD);
                  EnableWindow(hItemWnd, TRUE);

                  hItemWnd = GetDlgItem(hWnd, IDC_REMOVE);
                  EnableWindow(hItemWnd, FALSE);

                  hItemWnd = GetDlgItem(hWnd, IDC_COMBO1);
                  EnableWindow(hItemWnd, TRUE);

                  // Force an update of the screen
                  GetBitmapPosition(hWnd);

                  rect.left = offset_x;
                  rect.top = offset_y;
                  rect.right = offset_x + size_x;
                  rect.bottom = offset_y + size_y;

                  InvalidateRect(hWnd, &rect, TRUE);
                  UpdateWindow(hWnd);
               }

               return TRUE;

            case IDC_LIST2:
               if (HIWORD(wParam) == LBN_SELCHANGE)
               {
                  index = SendDlgItemMessage(hWnd, IDC_LIST2, LB_GETCURSEL, 0, 0L);

                  SendDlgItemMessage(hWnd, IDC_LIST2, LB_GETTEXT, index, (LPARAM)((LPSTR)item_name));

                  strcpy(wad_name, "{__");
                  strcat(wad_name, item_name);

                  // find the correct index based on name...

                  index = 0;
                  while (index < MAX_MIPS)
                  {
                     if (pMips[index])
                     {
                        if (strcmp(wad_name, pMips[index]->name) == 0)
                           break;
                     }

                     index++;
                  }

                  if (index == MAX_MIPS)
                  {
                     strcpy(wad_name, "__");
                     strcat(wad_name, item_name);

                     // find the correct index based on name...

                     index = 0;
                     while (index < MAX_MIPS)
                     {
                        if (pMips[index])
                        {
                           if (strcmp(wad_name, pMips[index]->name) == 0)
                              break;
                        }

                        index++;
                     }
                  }

                  if (index == MAX_MIPS)
                  {
                     MessageBox(hWnd, "Error finding mip!", "Error", MB_OK);
                     return TRUE;
                  }

                  hDC = GetDC(hWnd);

                  if (hBitmap)
                  {
                     if ((pbmi->bmiHeader.biHeight > 128) ||
                         (pbmi->bmiHeader.biWidth > 128))
                     {
                        GetClientRect(hWnd, &rect);
                        InvalidateRect(hWnd, &rect, TRUE);
                     }
                  }

                  LoadBitmapFromMip(index);

                  if ((hBitmap) && (h_background))
                  {
                     rect.left = offset_x;
                     rect.top = offset_y;
                     rect.right = offset_x + size_x;
                     rect.bottom = offset_y + size_y;

                     FillRect(hDC, &rect, h_background);
                  }

                  memcpy(Palette, OriginalPalette, (1 << pbmi->bmiHeader.biBitCount) * 4);

                  if ((Palette[255].rgbRed != 0) ||
                      (Palette[255].rgbGreen != 0) ||
                      (Palette[255].rgbBlue != 255))
                  {
                     r_scale = (float)Palette[255].rgbRed / (float)255.0;
                     g_scale = (float)Palette[255].rgbGreen / (float)255.0;
                     b_scale = (float)Palette[255].rgbBlue / (float)255.0;

                     // set palette 255 index back to pure white
                     Palette[255].rgbRed   = 255;
                     Palette[255].rgbGreen = 255;
                     Palette[255].rgbBlue  = 255;

                     for (index = 0; index < (1 << pbmi->bmiHeader.biBitCount); index++)
                     {
                        Palette[index].rgbRed   = (BYTE)((float)Palette[index].rgbRed * r_scale);
                        Palette[index].rgbGreen = (BYTE)((float)Palette[index].rgbGreen * g_scale);
                        Palette[index].rgbBlue  = (BYTE)((float)Palette[index].rgbBlue * b_scale);
                        Palette[index].rgbReserved = 0;
                     }
                  }

                  MakeBitmap(hDC, Palette, pOriginalData);

                  ReleaseDC(hWnd, hDC);

                  SendDlgItemMessage(hWnd, IDC_LIST1, LB_SETCURSEL, -1, 0L);

                  SendDlgItemMessage(hWnd, IDC_COMBO1, CB_SETCURSEL, 0, 0L);

                  g_palette_color = 0;

                  hItemWnd = GetDlgItem(hWnd, IDC_ADD);
                  EnableWindow(hItemWnd, FALSE);

                  hItemWnd = GetDlgItem(hWnd, IDC_REMOVE);
                  EnableWindow(hItemWnd, TRUE);

                  hItemWnd = GetDlgItem(hWnd, IDC_COMBO1);
                  EnableWindow(hItemWnd, FALSE);

                  // Force an update of the screen
                  GetBitmapPosition(hWnd);

                  rect.left = offset_x;
                  rect.top = offset_y;
                  rect.right = offset_x + size_x;
                  rect.bottom = offset_y + size_y;

                  InvalidateRect(hWnd, &rect, TRUE);
                  UpdateWindow(hWnd);
               }

               return TRUE;

            case IDC_COMBO1:
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                     hWndList = GetDlgItem(hWnd, IDC_COMBO1);
                     nItem = SendMessage(hWndList, CB_GETCURSEL, 0, 0);

                  if (nItem == 0)
                  {
                     // restore palette to original color...
                     hDC = GetDC(hWnd);
                     MakeBitmap(hDC, OriginalPalette, pOriginalData);
                     ReleaseDC(hWnd, hDC);
                  }
                  else
                  {
                     g_palette_color = nItem;

                     red   = colors[g_palette_color-1][0];
                     green = colors[g_palette_color-1][1];
                     blue  = colors[g_palette_color-1][2];

                     // set the palette color
                     hDC = GetDC(hWnd);

                     memcpy(Palette, OriginalPalette, (1 << pbmi->bmiHeader.biBitCount) * 4);

                     r_scale = (float)red / (float)255.0;
                     g_scale = (float)green / (float)255.0;
                     b_scale = (float)blue / (float)255.0;

                     for (index = 0; index < (1 << pbmi->bmiHeader.biBitCount); index++)
                     {
                        Palette[index].rgbRed   = (BYTE)((float)Palette[index].rgbRed * r_scale);
                        Palette[index].rgbGreen = (BYTE)((float)Palette[index].rgbGreen * g_scale);
                        Palette[index].rgbBlue  = (BYTE)((float)Palette[index].rgbBlue * b_scale);
                        Palette[index].rgbReserved = 0;
                     }

                     MakeBitmap(hDC, Palette, pOriginalData);

                     ReleaseDC(hWnd, hDC);
                  }

                  // Force an update of the screen
                  GetBitmapPosition(hWnd);

                  rect.left = offset_x;
                  rect.top = offset_y;
                  rect.right = offset_x + size_x;
                  rect.bottom = offset_y + size_y;

                  InvalidateRect(hWnd, &rect, TRUE);
                  UpdateWindow(hWnd);
               }

               return TRUE;

            case IDC_ADD:

               if (num_mips >= MAX_MIPS)
                  return TRUE;

               changes = TRUE;

               width = pbmi->bmiHeader.biWidth;
               height = pbmi->bmiHeader.biHeight;

               // assume all of the colors are used for mip mapping...
               colors_used = 256;
               for (x=0; x < 256; x++) color_used[x] = 1;

               // write the bitmap data and palette to the lump array...
               index = 0;
               while (pMips[index])
                  index++;

               mip_index = index;

               dwSize = sizeof(miptex_t) + (width * height) +
                        (width * height / 4) + (width * height / 16) +
                        (width * height / 64) + sizeof(palette_t);

               pMips[mip_index] = (miptex_t *)LocalAlloc(LPTR, dwSize);

               pMips[mip_index]->height = pbmi->bmiHeader.biHeight;
               pMips[mip_index]->width = pbmi->bmiHeader.biWidth;
               pMips[mip_index]->offsets[0] = sizeof(miptex_t);
               pMips[mip_index]->offsets[1] = pMips[mip_index]->offsets[0] + width * height;
               pMips[mip_index]->offsets[2] = pMips[mip_index]->offsets[1] + (width * height) / 4;
               pMips[mip_index]->offsets[3] = pMips[mip_index]->offsets[2] + (width * height) / 16;

               pLump = (BYTE *)pMips[mip_index] + pMips[mip_index]->offsets[0];

               index = height - 1;

               for (y=0; y < height; y++)
               {
                  memcpy(pLump+(width * y), pOriginalData+(width * index), width);
                  index--;
               }

               pMipPalette = (palette_t *)((char *)pMips[mip_index] +
                             pMips[mip_index]->offsets[3] + (width * height / 64));

               pMipPalette->palette_size = 256;

               for (index = 0; index < 256; index++)
               {
                  pMipPalette->palette[index * 3 + 0] = OriginalPalette[index].rgbRed;
                  pMipPalette->palette[index * 3 + 1] = OriginalPalette[index].rgbGreen;
                  pMipPalette->palette[index * 3 + 2] = OriginalPalette[index].rgbBlue;
               }

               pMipPalette->padding = 0;

               memcpy(lbmpalette, pMipPalette->palette, 256 * 3);

               // calculate gamma corrected linear palette
               for (x = 0; x < 256; x++)
               {
                  for (y = 0; y < 3; y++)
                  {
                     float f;
                     f = (float)(lbmpalette[x*3+y] / 255.0);
//                     linearpalette[x][y] = (float)pow(f, 2.2 ); // assume textures are done at 2.2, we want to remap them at 1.0
                     linearpalette[x][y] = (float)pow(f, 1.0);
                  }
               }

               pLump0 = (BYTE *)pMips[mip_index] + pMips[mip_index]->offsets[0];

               maxdistortion = 0;

               for (miplevel = 1; miplevel < 4; miplevel++)
      	      {
                  int pixTest;
                  d_red = d_green = d_blue = 0;	// no distortion yet

                  pLump = (BYTE *)pMips[mip_index] + pMips[mip_index]->offsets[miplevel];

                  mipstep = 1 << miplevel;
                  pixTest = (int)((float)(mipstep * mipstep) * 0.4);	// 40% of pixels

                  for (y = 0; y < height ; y += mipstep)
                  {
                     for (x = 0 ; x < width ; x += mipstep)
                     {
                        count = 0;

                        for (yy = 0; yy < mipstep; yy++)
                        {
                           for (xx = 0; xx < mipstep; xx++)
                           {
                              testpixel = pLump0[(y+yy)*width + x + xx ];
						
                              // If this isn't a transparent pixel, add it in to the image filter
                              if ( testpixel != 255 )
                              {
                                 pixdata[count] = testpixel;
                                 count++;
                              }
                           }
                        }

                        if ( count <= pixTest )	// Solid pixels account for < 40% of this pixel, make it transparent
                        {
                           *pLump++ = 255;
                        }
                        else
                        {
                           *pLump++ = AveragePixels(count);
                        }
                     }	
                  }
               }


               if (!g_monochrome)
               {
                  // assume none of the colors are used for palette 255 setting...
                  colors_used = 0;

                  for (x=0; x < 256; x++)
                     color_used[x] = 0;

                  for (x=0; x < width; x++)
                  {
                     for (y=0; y < height; y++)
                     {
                        if (!color_used[pOriginalData[x*height + y]])
                        {
                           color_used[pOriginalData[x*height + y]] = 1;
                           colors_used++;
                        }
                     }
                  }

                  // if all colors used and index 255 is NOT pure blue...
                  if ((colors_used == 256) &&
                      ((OriginalPalette[255].rgbRed !=0) ||
                       (OriginalPalette[255].rgbGreen != 0) ||
                       (OriginalPalette[255].rgbBlue != 255)))
                  {
                     // replace index 255 color with closest color...

                     red   = OriginalPalette[255].rgbRed;
                     green = OriginalPalette[255].rgbGreen;
                     blue  = OriginalPalette[255].rgbBlue;

                     min_difference = 256*3;
                     min_index = 0;

                     for (index=0; index < 255; index++)
                     {
                        difference = abs(OriginalPalette[index].rgbRed - red) +
                                     abs(OriginalPalette[index].rgbGreen - green) +
                                     abs(OriginalPalette[index].rgbBlue - blue);

                        if (difference < min_difference)
                        {
                           min_difference = difference;
                           min_index = index;
                        }
                     }

                     for (miplevel = 0; miplevel < 4; miplevel++)
                     {
                        mipstep = 1 << miplevel;
                        pLump = (BYTE *)pMips[mip_index] + pMips[mip_index]->offsets[miplevel];

                        mip_width = width/mipstep;
                        mip_height = height/mipstep;

                        for (x=0; x < mip_width; x++)
                        {
                           for (y=0; y < mip_height; y++)
                           {
                              if (pLump[x*mip_height + y] == 255)
                                 pLump[x*mip_height + y] = min_index;
                           }
                        }
                     }

                     colors_used--;
                     color_used[255] = 0;
                  }

                  if ((colors_used < 256) && (color_used[255]))
                  {
                     // move index 255 to first unused position...

                     index = 0;
                     while (color_used[index])
                        index++;

                     pMipPalette->palette[index * 3 + 0] = OriginalPalette[255].rgbRed;
                     pMipPalette->palette[index * 3 + 1] = OriginalPalette[255].rgbGreen;
                     pMipPalette->palette[index * 3 + 2] = OriginalPalette[255].rgbBlue;

                     for (miplevel = 0; miplevel < 4; miplevel++)
                     {
                        mipstep = 1 << miplevel;
                        pLump = (BYTE *)pMips[mip_index] + pMips[mip_index]->offsets[miplevel];

                        mip_width = width/mipstep;
                        mip_height = height/mipstep;

                        for (x=0; x < mip_width; x++)
                        {
                           for (y=0; y < mip_height; y++)
                           {
                              if (pLump[x*mip_height + y] == 255)
                                 pLump[x*mip_height + y] = index;
                           }
                        }
                     }
                  }

                  pMipPalette->palette[255 * 3 + 0] = 0;
                  pMipPalette->palette[255 * 3 + 1] = 0;
                  pMipPalette->palette[255 * 3 + 2] = 255;
               }

               if (g_palette_color != 0)
               {
                  pMipPalette->palette[255 * 3 + 0] = colors[g_palette_color-1][0];
                  pMipPalette->palette[255 * 3 + 1] = colors[g_palette_color-1][1];
                  pMipPalette->palette[255 * 3 + 2] = colors[g_palette_color-1][2];
               }


               if (strlen(g_szBitmapName) > 9)
                  g_szBitmapName[10] = 0;

               if (g_monochrome)
                  strcpy(mip_name, "__");
               else
                  strcpy(mip_name, "{__");

               strcat(mip_name, g_szBitmapName);

               length = strlen(mip_name);

               max_value = 0;

               // find the highest number for this name...
               for (index = 0; index < num_mips; index++)
               {
                  if (pMips[index])
                  {
                     if (strncmp(pMips[index]->name, mip_name, length) == 0)
                     {
                        sscanf(&(pMips[index]->name[length+1]), "%d", &value);

                        if (value > max_value)
                           max_value = value;
                     }
                  }
               }

               max_value++;

               if (max_value > 99)
               {
                  MessageBox(hWnd, "Too many mips for this file!", "Error", MB_OK);
                  LocalFree(pMips[mip_index]);
                  pMips[mip_index] = NULL;
                  return TRUE;
               }

               sprintf(pMips[mip_index]->name, "%s_%02d", mip_name, max_value);

               if (g_monochrome)
                  SendDlgItemMessage(hWnd, IDC_LIST2, LB_ADDSTRING, 0,
                                     (LPARAM)((LPCSTR)&(pMips[mip_index]->name[2])));
               else
                  SendDlgItemMessage(hWnd, IDC_LIST2, LB_ADDSTRING, 0,
                                     (LPARAM)((LPCSTR)&(pMips[mip_index]->name[3])));

               num_mips++;


               return TRUE;

            case IDC_REMOVE:

               changes = TRUE;

               index = SendDlgItemMessage(hWnd, IDC_LIST2, LB_GETCURSEL, 0, 0L);

               SendDlgItemMessage(hWnd, IDC_LIST2, LB_GETTEXT, index, (LPARAM)((LPSTR)item_name));

               SendDlgItemMessage(hWnd, IDC_LIST2, LB_DELETESTRING, index, 0L);

               strcpy(wad_name, "{__");
               strcat(wad_name, item_name);

               // find the correct index based on name...

               index = 0;
               while (index < MAX_MIPS)
               {
                  if (pMips[index])
                  {
                     if (strcmp(wad_name, pMips[index]->name) == 0)
                        break;
                  }

                  index++;
               }

               if (index == MAX_MIPS)
               {
                  strcpy(wad_name, "__");
                  strcat(wad_name, item_name);

                  // find the correct index based on name...

                  index = 0;
                  while (index < MAX_MIPS)
                  {
                     if (pMips[index])
                     {
                        if (strcmp(wad_name, pMips[index]->name) == 0)
                           break;
                     }

                     index++;
                  }
               }

               if (index == MAX_MIPS)
               {
                  MessageBox(hWnd, "Error finding mip!", "Error", MB_OK);
                  return FALSE;
               }

               LocalFree(pMips[index]);

               pMips[index] = NULL;


               hDC = GetDC(hWnd);

               if (hBitmap)
               {
                  if ((pbmi->bmiHeader.biHeight > 128) ||
                      (pbmi->bmiHeader.biWidth > 128))
                  {
                     GetClientRect(hWnd, &rect);
                     InvalidateRect(hWnd, &rect, TRUE);
                  }
               }

               if ((hBitmap) && (h_background))
               {
                  rect.left = offset_x;
                  rect.top = offset_y;
                  rect.right = offset_x + size_x;
                  rect.bottom = offset_y + size_y;

                  FillRect(hDC, &rect, h_background);
                  InvalidateRect(hWnd, &rect, TRUE);
               }

               ReleaseDC(hWnd, hDC);

               if (hBitmap)
                  DeleteObject(hBitmap);

               UpdateWindow(hWnd);


               SendDlgItemMessage(hWnd, IDC_LIST1, LB_SETCURSEL, -1, 0L);
               SendDlgItemMessage(hWnd, IDC_LIST2, LB_SETCURSEL, -1, 0L);
               SendDlgItemMessage(hWnd, IDC_COMBO1, CB_SETCURSEL, 0, 0L);

               g_palette_color = 0;

               hItemWnd = GetDlgItem(hWnd, IDC_ADD);
               EnableWindow(hItemWnd, FALSE);

               hItemWnd = GetDlgItem(hWnd, IDC_REMOVE);
               EnableWindow(hItemWnd, FALSE);

               hItemWnd = GetDlgItem(hWnd, IDC_COMBO1);
               EnableWindow(hItemWnd, FALSE);

               return TRUE;

            case IDCLOSE:

               if (changes)
               {
                  if (MessageBox(hWnd, "Do you wish to save your changes?", "Warning", MB_YESNO) == IDYES)
                  {
                     // check if backup file is needed

                     strcpy(src_filename, szParentPath);
                     strcat(src_filename, "\\");
                     strcat(src_filename, g_szMODdir);
                     strcat(src_filename, "\\decals.wad");

                     strcpy(dest_filename, szParentPath);
                     strcat(dest_filename, "\\");
                     strcat(dest_filename, g_szMODdir);
                     strcat(dest_filename, "\\decals_old.wad");

                     if (CopyFile(src_filename, dest_filename, TRUE))
                        MessageBox(hWnd, "A backup copy of decals.wad was\ncreated called decals_old.wad", "Warning", MB_OK);

                     if (!WriteWADFile(src_filename))
                        MessageBox(hWnd, "An error occured writing decals.wad", "Error", MB_OK);
                  }
               }

               for (index = 0; index < MAX_MIPS; index++)
               {
                  if (pMips[index])
                     LocalFree(pMips[index]);
               }

               if (pbmi)
                  LocalFree(pbmi);

               if (pOriginalData)
                  LocalFree(pOriginalData);

               if (hBitmap)
                  DeleteObject(hBitmap);

               if (hPalette)
                  DeleteObject(hPalette);

               if (h_background)
                  DeleteObject(h_background);

               PostQuitMessage(0);
               return TRUE;
         }

         break;

      case WM_CLOSE:

         SendMessage(hWnd, WM_COMMAND, IDCLOSE, 0L);

         break;
   }

   return FALSE;
}


VOID NEAR GoModalDialogBoxParam( HINSTANCE hInstance, LPCSTR lpszTemplate,
                                 HWND hWnd, DLGPROC lpDlgProc, LPARAM lParam )
{
   DLGPROC  lpProcInstance ;

   lpProcInstance = (DLGPROC) MakeProcInstance( (FARPROC) lpDlgProc,
                                                hInstance ) ;

   DialogBoxParam( hInstance, lpszTemplate, hWnd, lpProcInstance, lParam ) ;

   FreeProcInstance( (FARPROC) lpProcInstance ) ;

}


BOOL CALLBACK MODDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   HANDLE hFile = NULL;
   char path[MAX_PATH];
   char dirname[MAX_PATH];
   char filename[MAX_PATH];
   int i;
   int count = 0;
   struct stat stat_str;

   switch (uMsg)
   {
      case WM_INITDIALOG:

         if (!CenterWindow( hDlg ))
            return( FALSE );

         strcpy(path, szParentPath);
         strcat(path, "\\*");

         while ((hFile = FindDirectory(hFile, dirname, path)) != NULL)
         {
            if ((strcmp(dirname, ".") == 0) || (strcmp(dirname, "..") == 0))
               continue;

            strcpy(filename, szParentPath);
            strcat(filename, "\\");
            strcat(filename, dirname);
            strcat(filename, "\\liblist.gam");

            if (stat(filename, &stat_str) == 0)
            {
               SendDlgItemMessage(hDlg, IDC_MOD, CB_ADDSTRING, 0, (LPARAM)((LPCSTR)dirname));
               count++;
            }
         }

         if (count == 0)
         {
            MessageBox (NULL, "Can't find any MOD directories\nAre you sure you have things installed right?", NULL, MB_OK);
            g_ReturnStatus = IDCANCEL;
            EndDialog(hDlg, TRUE);
            return ( TRUE );
         }

         SendDlgItemMessage(hDlg, IDC_MOD, CB_SETCURSEL, 0, 0L);

         return TRUE;

      case WM_COMMAND:
         switch ((WORD) wParam)
         {
            case IDOK:
               i = SendDlgItemMessage(hDlg, IDC_MOD, CB_GETCURSEL, 0, 0L);

               SendDlgItemMessage(hDlg, IDC_MOD, CB_GETLBTEXT, i, (LPARAM)((LPCSTR)g_szMODdir));

               g_ReturnStatus = IDOK;
               EndDialog(hDlg, TRUE);
               return ( TRUE );

            case IDCANCEL:
               g_ReturnStatus = IDCANCEL;
               EndDialog(hDlg, TRUE);
               return ( TRUE );
         }
         break;
   }

   return FALSE;
}
