#define BOTSKILL

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"
#include "bot_weapons.h"

#include "bot_skill.h"

bot_skill_settings_t default_skill_settings[5] = {
   // best skill (lvl1)
   { 5, {0.10, 0.25}, 10.0, 50.0, 80, 
     {0.01, 0.07, 0.10}, {0.04, 0.11, 0.15}, 
     {0.1, 0.3},
     150.0, 10.0, 0.100, 5.0, 0.025,
     4.0, TRUE, 50, 75, 50, 100,
     TRUE, 100, 100, 1000.0, 400.0, 20.0, 1000.0,
     TRUE, 99 },
   // lvl2
   { 15, {0.25, 0.5}, 8.0, 30.0, 70,
     {0.02, 0.09, 0.12}, {0.06, 0.14, 0.18},
     {0.2, 0.5},
     175.0, 20.0, 0.200, 3.0, 0.05,
     3.0, TRUE, 35, 60, 35, 90,
     TRUE, 50, 50, 1000.0, 400.0, 10.0, 1000.0,
     TRUE, 66 },
   // lvl3
   { 50, {0.50, 0.75}, 6.0, 15.0, 60,
     {0.03, 0.12, 0.15}, {0.08, 0.18, 0.22},
     {0.3, 0.7},
     200.0, 25.0, 0.300, 2.0, 0.075,
     2.0, TRUE, 20, 40, 20, 70,
     TRUE, 20, 20, 1000.0, 400.0, 10.0, 1000.0,
     TRUE, 33 },
   // lvl4
   { 100, {1.0, 1.5}, 4.0, 5.0, 50,
     {0.04, 0.14, 0.18}, {0.10, 0.21, 0.25},
     {0.6, 1.4},
     250.0, 30.0, 0.400, 1.5, 0.1,
     1.5, TRUE, 10, 25, 10, 40,
     TRUE, 0, 0, 0.0, 0.0, 0.0, 0.0,
     FALSE, 0 },
   // worst skill (lvl5)
   { 200, {1.5, 2.5}, 0.5, 1.0, 40,
     {0.05, 0.17, 0.21}, {0.12, 0.25, 0.30},
     {1.2, 2.8},
     300.0, 35.0, 0.500, 1.0, 0.15,
     1.0, FALSE, 5, 15, 5, 0,
     FALSE, 0, 0, 0.0, 0.0, 0.0, 0.0,
     FALSE, 0 },
};

bot_skill_settings_t skill_settings[5];

void ResetSkillsToDefault(void)
{
   memcpy(skill_settings, default_skill_settings, sizeof(skill_settings));
}
