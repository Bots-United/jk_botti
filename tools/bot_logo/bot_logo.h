#ifndef FUNCS_H
#define FUNCS_H

/*

   WAD3 Header
   {
      char identification[4];		// should be WAD3 or 3DAW
      int  numlumps;
      int  infotableofs;
   }

   Mip section
   {
      First mip
      {
         char name[16];
         unsigned width, height;
         unsigned offsets[4];  // mip offsets relative to start of this mip
         byte first_mip[width*height];
         byte second_mip[width*height/4];
         byte third_mip[width*height/16];
         byte fourth_mip[width*height/64];
         short int palette_size;
         byte palette[palette_size*3];
         short int padding;
      }
      [...]
      Last mip
   }

   Lump table
   {
      First lump entry
      {
         int  filepos;
         int  disksize;
         int  size;  // uncompressed
         char type;  // 0x43 - WAD3 mip (Half-Life)
         char compression;
         char pad1, pad2;
         char name[16];  // must be null terminated
      }
      [...]
      Last lump entry
   }

*/

typedef struct
{
   char identification[4];		// should be WAD2 (or 2DAW) or WAD3 (or 3DAW)
   int  numlumps;
   int  infotableofs;
} wadinfo_t;

typedef struct
{
   int  filepos;
   int  disksize;
   int  size;					// uncompressed
   char type;
   char compression;
   char pad1, pad2;
   char name[16];				// must be null terminated
} lumpinfo_t;


#define MAX_MIPS   1000
#define MIPLEVELS  4

typedef struct miptex_s
{
   char     name[16];
   unsigned width, height;
   unsigned offsets[MIPLEVELS];		// four mip maps stored
} miptex_t;

#define PALETTE_SIZE 256

typedef struct palette_s
{
   short int palette_size;
   unsigned char palette[PALETTE_SIZE * 3];
   short int padding;
} palette_t;


UINT GetPathFromFullPathName( LPCTSTR lpFullPathName, LPTSTR lpPathBuffer,
                              int nPathBufferLength );
HANDLE FindFile(HANDLE hFile, CHAR *file, CHAR *filename);
HANDLE FindDirectory(HANDLE hFile, CHAR *dirname, CHAR *filename);
BOOL CheckBitmapFormat(LPTSTR lpszFileName);
BOOL LoadBitmapFromFile(HDC hDC, LPTSTR lpszFileName);
BOOL LoadWADFile(LPTSTR lpszFileName);
BOOL LoadBitmapFromMip(int index);
BOOL WriteWADFile(LPTSTR lpszFileName);
BYTE AveragePixels (int count);

#endif