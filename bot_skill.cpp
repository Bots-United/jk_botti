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

void ResetSkillsToDefault(void)
{
   memcpy(skill_settings, default_skill_settings, sizeof(skill_settings));
}
