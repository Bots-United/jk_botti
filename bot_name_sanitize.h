//
// JK_Botti - be more human!
//
// bot_name_sanitize.h
//

#ifndef BOT_NAME_SANITIZE_H
#define BOT_NAME_SANITIZE_H

#include <string.h>

// Remove control chars (< 0x20), DEL (0x7F), and double-quotes from name in-place.
static inline void bot_name_sanitize(char *name)
{
   int length = strlen(name);
   int i;

   for (i = 0; i < length; i++)
   {
      if ((name[i] < ' ') || (name[i] > '~') || (name[i] == '"'))
      {
         int j;
         for (j = i; j < length; j++)
            name[j] = name[j+1];
         length--;
         i--;  // re-check this position (now holds the next char)
      }
   }
}

// Remove "(1)".."(9)" prefix added by HLDS for duplicate names.
static inline void bot_name_strip_hlds_tag(char *name)
{
   if (name[0] == '(' && (name[1] >= '1' && name[1] <= '9') && name[2] == ')')
   {
      int length = strlen(&name[3]) + 1; // str+null
      int i;
      for (i = 0; i < length; i++)
         name[i] = (&name[3])[i];
   }
}

// Remove "[lvl1]".."[lvl5]" prefix. If skill_out is non-NULL, store the
// extracted skill level there; otherwise just strip the tag.
static inline void bot_name_strip_skill_tag(char *name, int *skill_out)
{
   if (!strncmp(name, "[lvl", 4) && (name[4] >= '1' && name[4] <= '5') && name[5] == ']')
   {
      if (skill_out)
         *skill_out = name[4] - '0';

      int length = strlen(&name[6]) + 1; // str+null
      int i;
      for (i = 0; i < length; i++)
         name[i] = (&name[6])[i];
   }
}

#endif // BOT_NAME_SANITIZE_H
