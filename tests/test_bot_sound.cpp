//
// JK_Botti - unit tests for bot_sound.cpp
//
// test_bot_sound.cpp
//

#include <stdlib.h>
#include <math.h>

#include "test_common.h"

#include "engine_mock.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "bot_sound.h"
#include "player.h"

// ============================================================
// Test helpers
// ============================================================

static void setup_bot(int slot, edict_t *pEdict, const Vector &origin)
{
   memset(&bots[slot], 0, sizeof(bot_t));
   bots[slot].is_used = TRUE;
   bots[slot].pEdict = pEdict;
   pEdict->v.origin = origin;
   pEdict->v.flags = FL_CLIENT | FL_FAKECLIENT;
}

// ============================================================
// 1. CSound::Clear
// ============================================================

static int test_clear_zeros_fields(void)
{
   TEST("CSound::Clear: zeroes all fields");
   mock_reset();

   CSound *pSound = CSoundEnt::SoundPointerForIndex(0);
   ASSERT_PTR_NOT_NULL(pSound);

   // Set non-zero values
   pSound->m_vecOrigin = Vector(1, 2, 3);
   pSound->m_iVolume = 100;
   pSound->m_flExpireTime = 99.0f;
   pSound->m_iNextAudible = 5;
   pSound->m_iChannel = 7;

   pSound->Clear();

   ASSERT_TRUE(pSound->m_vecOrigin.x == 0 && pSound->m_vecOrigin.y == 0 && pSound->m_vecOrigin.z == 0);
   ASSERT_INT(pSound->m_iVolume, 0);
   ASSERT_INT(pSound->m_iNext, SOUNDLIST_EMPTY);
   ASSERT_INT(pSound->m_iNextAudible, 0);
   ASSERT_INT(pSound->m_iChannel, 0);
   PASS();
   return 0;
}

static int test_clear_nulls_edict_and_owner(void)
{
   TEST("CSound::Clear: pEdict=NULL, botOwner=-1");
   mock_reset();

   CSound *pSound = CSoundEnt::SoundPointerForIndex(0);
   ASSERT_PTR_NOT_NULL(pSound);

   pSound->m_pEdict = (edict_t *)0xDEAD;
   pSound->m_iBotOwner = 5;

   pSound->Clear();

   ASSERT_PTR_NULL(pSound->m_pEdict);
   ASSERT_INT(pSound->m_iBotOwner, -1);
   PASS();
   return 0;
}

// ============================================================
// 2. CSound::Reset
// ============================================================

static int test_reset_zeros_origin_volume_next(void)
{
   TEST("CSound::Reset: zeroes origin, volume, next");
   mock_reset();

   CSound *pSound = CSoundEnt::SoundPointerForIndex(0);
   ASSERT_PTR_NOT_NULL(pSound);

   pSound->m_vecOrigin = Vector(10, 20, 30);
   pSound->m_iVolume = 50;
   pSound->m_iNext = 42;

   pSound->Reset();

   ASSERT_TRUE(pSound->m_vecOrigin.x == 0 && pSound->m_vecOrigin.y == 0 && pSound->m_vecOrigin.z == 0);
   ASSERT_INT(pSound->m_iVolume, 0);
   ASSERT_INT(pSound->m_iNext, SOUNDLIST_EMPTY);
   PASS();
   return 0;
}

static int test_reset_preserves_other_fields(void)
{
   TEST("CSound::Reset: preserves expireTime, pEdict, channel, botOwner");
   mock_reset();

   CSound *pSound = CSoundEnt::SoundPointerForIndex(0);
   ASSERT_PTR_NOT_NULL(pSound);

   edict_t *e = mock_alloc_edict();
   pSound->m_flExpireTime = 77.0f;
   pSound->m_pEdict = e;
   pSound->m_iChannel = 3;
   pSound->m_iBotOwner = 2;

   pSound->Reset();

   ASSERT_TRUE(pSound->m_flExpireTime == 77.0f);
   ASSERT_PTR_EQ(pSound->m_pEdict, e);
   ASSERT_INT(pSound->m_iChannel, 3);
   ASSERT_INT(pSound->m_iBotOwner, 2);
   PASS();
   return 0;
}

// ============================================================
// 3. CSoundEnt::Initialize
// ============================================================

static int test_init_all_in_free_list(void)
{
   TEST("Initialize: 1024 free, 0 active");
   mock_reset();

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_FREE), MAX_WORLD_SOUNDS);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 0);
   PASS();
   return 0;
}

static int test_init_free_chain(void)
{
   TEST("Initialize: free list chain 0->1->...->1023->EMPTY");
   mock_reset();

   int idx = CSoundEnt::FreeList();
   ASSERT_INT(idx, 0);

   // Walk first few and last
   for (int i = 0; i < MAX_WORLD_SOUNDS; i++)
   {
      CSound *p = CSoundEnt::SoundPointerForIndex(idx);
      ASSERT_PTR_NOT_NULL(p);
      if (i < MAX_WORLD_SOUNDS - 1)
      {
         ASSERT_INT(p->m_iNext, i + 1);
         idx = p->m_iNext;
      }
      else
      {
         ASSERT_INT(p->m_iNext, SOUNDLIST_EMPTY);
      }
   }
   PASS();
   return 0;
}

static int test_init_active_empty(void)
{
   TEST("Initialize: active list is EMPTY, IsEmpty() TRUE");
   mock_reset();

   ASSERT_INT(CSoundEnt::ActiveList(), SOUNDLIST_EMPTY);
   ASSERT_TRUE(pSoundEnt->IsEmpty());
   PASS();
   return 0;
}

// ============================================================
// 4. CSoundEnt::IAllocSound
// ============================================================

static int test_alloc_one(void)
{
   TEST("IAllocSound: alloc 1 -> free=1023, active=1");
   mock_reset();

   int idx = pSoundEnt->IAllocSound();
   ASSERT_TRUE(idx != SOUNDLIST_EMPTY);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_FREE), MAX_WORLD_SOUNDS - 1);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_alloc_returns_head_of_active(void)
{
   TEST("IAllocSound: returned index is head of active list");
   mock_reset();

   int idx = pSoundEnt->IAllocSound();
   ASSERT_INT(CSoundEnt::ActiveList(), idx);
   PASS();
   return 0;
}

static int test_alloc_when_full(void)
{
   TEST("IAllocSound: returns SOUNDLIST_EMPTY when free list empty");
   mock_reset();

   // Exhaust all sounds
   for (int i = 0; i < MAX_WORLD_SOUNDS; i++)
      pSoundEnt->IAllocSound();

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_FREE), 0);
   ASSERT_INT(pSoundEnt->IAllocSound(), SOUNDLIST_EMPTY);
   PASS();
   return 0;
}

// ============================================================
// 5. CSoundEnt::FreeSound
// ============================================================

static int test_free_head(void)
{
   TEST("FreeSound: free head of active list");
   mock_reset();

   int idx1 = pSoundEnt->IAllocSound();
   int idx2 = pSoundEnt->IAllocSound();
   // Active list: idx2 -> idx1
   ASSERT_INT(CSoundEnt::ActiveList(), idx2);

   // Free head (idx2), prev=SOUNDLIST_EMPTY
   CSoundEnt::FreeSound(idx2, SOUNDLIST_EMPTY);
   ASSERT_INT(CSoundEnt::ActiveList(), idx1);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_free_middle(void)
{
   TEST("FreeSound: free non-head (middle of active list)");
   mock_reset();

   int idx1 = pSoundEnt->IAllocSound();
   int idx2 = pSoundEnt->IAllocSound();
   int idx3 = pSoundEnt->IAllocSound();
   // Active list: idx3 -> idx2 -> idx1

   // Free idx2, previous is idx3
   CSoundEnt::FreeSound(idx2, idx3);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 2);

   // Verify idx3 -> idx1 (skips idx2)
   CSound *p3 = CSoundEnt::SoundPointerForIndex(idx3);
   ASSERT_INT(p3->m_iNext, idx1);
   PASS();
   return 0;
}

static int test_free_goes_to_free_list_head(void)
{
   TEST("FreeSound: freed sound becomes head of free list");
   mock_reset();

   int idx = pSoundEnt->IAllocSound();
   // Record free head AFTER alloc (it advanced past idx)
   int free_head_after_alloc = CSoundEnt::FreeList();

   // Now free it
   CSoundEnt::FreeSound(idx, SOUNDLIST_EMPTY);
   ASSERT_INT(CSoundEnt::FreeList(), idx);

   // And it points to what was the free head before freeing
   CSound *p = CSoundEnt::SoundPointerForIndex(idx);
   ASSERT_INT(p->m_iNext, free_head_after_alloc);
   PASS();
   return 0;
}

// ============================================================
// 6. CSoundEnt::InsertSound
// ============================================================

static int test_insert_null_edict(void)
{
   TEST("InsertSound: NULL pEdict -> allocates, populates fields");
   mock_reset();

   Vector org(100, 200, 300);
   CSoundEnt::InsertSound(NULL, 5, org, 80, 2.0f, 3);

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_TRUE(pSound->m_vecOrigin.x == 100 && pSound->m_vecOrigin.y == 200 && pSound->m_vecOrigin.z == 300);
   ASSERT_INT(pSound->m_iVolume, 80);
   ASSERT_INT(pSound->m_iChannel, 5);
   ASSERT_INT(pSound->m_iBotOwner, 3);
   ASSERT_PTR_NULL(pSound->m_pEdict);
   ASSERT_TRUE(pSound->m_flExpireTime > gpGlobals->time);
   PASS();
   return 0;
}

static int test_insert_edict_new_channel(void)
{
   TEST("InsertSound: non-NULL pEdict, new channel -> alloc via GetEdictChannelSound");
   mock_reset();

   edict_t *e = mock_alloc_edict();
   Vector org(50, 60, 70);
   CSoundEnt::InsertSound(e, 3, org, 40, 1.5f, 0);

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, e);
   ASSERT_INT(pSound->m_iChannel, 3);
   ASSERT_INT(pSound->m_iVolume, 40);
   PASS();
   return 0;
}

static int test_insert_edict_existing_channel(void)
{
   TEST("InsertSound: existing edict+channel -> updates, no new alloc");
   mock_reset();

   edict_t *e = mock_alloc_edict();
   Vector org1(1, 2, 3);
   CSoundEnt::InsertSound(e, 7, org1, 10, 1.0f, -1);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);

   // Insert again with same edict+channel
   Vector org2(4, 5, 6);
   CSoundEnt::InsertSound(e, 7, org2, 20, 2.0f, 1);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1); // no new allocation

   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_TRUE(pSound->m_vecOrigin.x == 4 && pSound->m_vecOrigin.y == 5 && pSound->m_vecOrigin.z == 6);
   ASSERT_INT(pSound->m_iVolume, 20);
   ASSERT_INT(pSound->m_iBotOwner, 1);
   PASS();
   return 0;
}

static int test_insert_null_soundent(void)
{
   TEST("InsertSound: pSoundEnt NULL -> no crash");
   mock_reset();

   CSoundEnt *saved = pSoundEnt;
   pSoundEnt = NULL;

   // Should not crash
   CSoundEnt::InsertSound(NULL, 1, Vector(0, 0, 0), 10, 1.0f, -1);

   pSoundEnt = saved;
   PASS();
   return 0;
}

// ============================================================
// 7. CSoundEnt::GetEdictChannelSound
// ============================================================

static int test_get_edict_channel_new_nonzero(void)
{
   TEST("GetEdictChannelSound: no match, nonzero channel -> alloc new");
   mock_reset();

   edict_t *e = mock_alloc_edict();
   CSound *pSound = CSoundEnt::GetEdictChannelSound(e, 5);
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, e);
   ASSERT_INT(pSound->m_iChannel, 5);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_get_edict_channel_zero_no_match(void)
{
   TEST("GetEdictChannelSound: no match, channel 0 -> returns NULL");
   mock_reset();

   edict_t *e = mock_alloc_edict();
   CSound *pSound = CSoundEnt::GetEdictChannelSound(e, 0);
   ASSERT_PTR_NULL(pSound);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 0);
   PASS();
   return 0;
}

static int test_get_edict_channel_exact_match(void)
{
   TEST("GetEdictChannelSound: exact edict+channel match -> returns existing");
   mock_reset();

   edict_t *e = mock_alloc_edict();
   CSound *p1 = CSoundEnt::GetEdictChannelSound(e, 3);
   ASSERT_PTR_NOT_NULL(p1);

   CSound *p2 = CSoundEnt::GetEdictChannelSound(e, 3);
   ASSERT_PTR_EQ(p2, p1); // same sound
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_get_edict_channel_zero_wildcard(void)
{
   TEST("GetEdictChannelSound: channel 0 query matches any channel for edict");
   mock_reset();

   edict_t *e = mock_alloc_edict();
   CSound *p1 = CSoundEnt::GetEdictChannelSound(e, 9);
   ASSERT_PTR_NOT_NULL(p1);

   // Channel 0 should match any existing channel for this edict
   CSound *p2 = CSoundEnt::GetEdictChannelSound(e, 0);
   ASSERT_PTR_EQ(p2, p1);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_get_edict_channel_different_channels(void)
{
   TEST("GetEdictChannelSound: same edict, different channels -> correct one");
   mock_reset();

   edict_t *e = mock_alloc_edict();
   CSound *p1 = CSoundEnt::GetEdictChannelSound(e, 1);
   p1->m_iVolume = 111;
   CSound *p2 = CSoundEnt::GetEdictChannelSound(e, 2);
   p2->m_iVolume = 222;
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 2);

   CSound *found = CSoundEnt::GetEdictChannelSound(e, 1);
   ASSERT_INT(found->m_iVolume, 111);

   found = CSoundEnt::GetEdictChannelSound(e, 2);
   ASSERT_INT(found->m_iVolume, 222);
   PASS();
   return 0;
}

// ============================================================
// 8. CSoundEnt::ISoundsInList
// ============================================================

static int test_sounds_in_list_after_init(void)
{
   TEST("ISoundsInList: after init free=1024, active=0");
   mock_reset();

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_FREE), 1024);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 0);
   PASS();
   return 0;
}

static int test_sounds_in_list_after_alloc(void)
{
   TEST("ISoundsInList: after 1 alloc free=1023, active=1");
   mock_reset();

   pSoundEnt->IAllocSound();
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_FREE), 1023);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_sounds_in_list_invalid_type(void)
{
   TEST("ISoundsInList: invalid list type returns 0");
   mock_reset();

   ASSERT_INT(pSoundEnt->ISoundsInList(999), 0);
   PASS();
   return 0;
}

// ============================================================
// 9. CSoundEnt accessors
// ============================================================

static int test_active_list_returns_head(void)
{
   TEST("ActiveList: returns head of active list");
   mock_reset();

   ASSERT_INT(CSoundEnt::ActiveList(), SOUNDLIST_EMPTY);
   int idx = pSoundEnt->IAllocSound();
   ASSERT_INT(CSoundEnt::ActiveList(), idx);
   PASS();
   return 0;
}

static int test_free_list_returns_head(void)
{
   TEST("FreeList: returns head of free list");
   mock_reset();

   ASSERT_INT(CSoundEnt::FreeList(), 0);
   pSoundEnt->IAllocSound(); // takes index 0
   ASSERT_INT(CSoundEnt::FreeList(), 1);
   PASS();
   return 0;
}

static int test_sound_pointer_valid(void)
{
   TEST("SoundPointerForIndex: valid index returns correct pointer");
   mock_reset();

   CSound *p0 = CSoundEnt::SoundPointerForIndex(0);
   CSound *p1 = CSoundEnt::SoundPointerForIndex(1);
   ASSERT_PTR_NOT_NULL(p0);
   ASSERT_PTR_NOT_NULL(p1);
   ASSERT_TRUE(p1 != p0);
   PASS();
   return 0;
}

static int test_sound_pointer_invalid(void)
{
   TEST("SoundPointerForIndex: negative/too-large -> NULL");
   mock_reset();

   ASSERT_PTR_NULL(CSoundEnt::SoundPointerForIndex(-1));
   ASSERT_PTR_NULL(CSoundEnt::SoundPointerForIndex(MAX_WORLD_SOUNDS));
   PASS();
   return 0;
}

static int test_client_sound_index(void)
{
   TEST("ClientSoundIndex: returns ENTINDEX(pClient) - 1");
   mock_reset();

   edict_t *e = mock_alloc_edict(); // index 1
   int idx = CSoundEnt::ClientSoundIndex(e);
   int expected = ENTINDEX(e) - 1;
   ASSERT_INT(idx, expected);
   PASS();
   return 0;
}

// ============================================================
// 10. CSoundEnt::Think
// ============================================================

static int test_think_expired_freed(void)
{
   TEST("Think: expired sound gets freed");
   mock_reset();

   CSoundEnt::InsertSound(NULL, 1, Vector(0, 0, 0), 50, 0.0f, -1);
   // Duration 0 means expireTime = gpGlobals->time + 0 = gpGlobals->time
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);

   pSoundEnt->Think();
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 0);
   PASS();
   return 0;
}

static int test_think_non_expired_stays(void)
{
   TEST("Think: non-expired sound stays in active list");
   mock_reset();

   CSoundEnt::InsertSound(NULL, 1, Vector(0, 0, 0), 50, 10.0f, -1);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);

   pSoundEnt->Think();
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_think_rpg_rocket_origin_update(void)
{
   TEST("Think: rpg_rocket sound origin updates from edict");
   mock_reset();

   edict_t *rocket = mock_alloc_edict();
   mock_set_classname(rocket, "rpg_rocket");
   rocket->v.origin = Vector(10, 20, 30);

   CSoundEnt::InsertSound(rocket, 1, Vector(0, 0, 0), 50, 10.0f, -1);
   int idx = CSoundEnt::ActiveList();
   CSound *pSound = CSoundEnt::SoundPointerForIndex(idx);
   ASSERT_PTR_NOT_NULL(pSound);

   // Move the rocket
   rocket->v.origin = Vector(100, 200, 300);
   pSoundEnt->Think();

   ASSERT_TRUE(pSound->m_vecOrigin.x == 100 && pSound->m_vecOrigin.y == 200 && pSound->m_vecOrigin.z == 300);
   PASS();
   return 0;
}

static int test_think_null_edict_cleared(void)
{
   TEST("Think: FNullEnt edict -> m_pEdict set to NULL");
   mock_reset();

   // Insert a sound with a valid edict
   edict_t *e = mock_alloc_edict();
   CSoundEnt::InsertSound(e, 1, Vector(0, 0, 0), 50, 10.0f, -1);

   int idx = CSoundEnt::ActiveList();
   CSound *pSound = CSoundEnt::SoundPointerForIndex(idx);
   ASSERT_PTR_NOT_NULL(pSound);

   // Now make the edict "null" by pointing to worldspawn (edict 0, EntOffset=0 -> FNullEnt)
   pSound->m_pEdict = &mock_edicts[0];

   pSoundEnt->Think();
   ASSERT_PTR_NULL(pSound->m_pEdict);
   PASS();
   return 0;
}

static int test_think_mixed(void)
{
   TEST("Think: mixed expired/non-expired -> only expired freed");
   mock_reset();

   // Insert 3 sounds: 1 expired, 2 non-expired
   CSoundEnt::InsertSound(NULL, 1, Vector(1, 0, 0), 50, 10.0f, -1);  // non-expired
   CSoundEnt::InsertSound(NULL, 2, Vector(2, 0, 0), 50, 0.0f, -1);   // expired (duration 0)
   CSoundEnt::InsertSound(NULL, 3, Vector(3, 0, 0), 50, 10.0f, -1);  // non-expired

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 3);

   pSoundEnt->Think();
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 2);
   PASS();
   return 0;
}

// ============================================================
// 11. SaveSound
// ============================================================

static int test_save_sound_player_edict(void)
{
   TEST("SaveSound: player edict (ENTINDEX 1-32) -> direct InsertSound");
   mock_reset();

   // Edict at index 1 is a player (ENTINDEX 1, which is idx 0 < maxClients)
   edict_t *player = &mock_edicts[1];
   memset(player, 0, sizeof(*player));
   player->v.pContainingEntity = player;

   // Set up bot at slot 0 matching this player
   setup_bot(0, player, Vector(0, 0, 0));

   Vector org(10, 20, 30);
   SaveSound(player, org, 80, 3, 2.0f);

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, player);
   ASSERT_INT(pSound->m_iChannel, 3); // no 0x1000 flag for direct player
   PASS();
   return 0;
}

static int test_save_sound_no_nearby_bot(void)
{
   TEST("SaveSound: non-player, no nearby bot -> bot_index=-1");
   mock_reset();

   // Edict at high index (not a player)
   edict_t *entity = mock_alloc_edict();
   // Make sure it's beyond maxClients
   // mock_alloc_edict starts from index 1; we need index > 32
   // Allocate enough to get past player range
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();

   Vector org(500, 500, 500);
   SaveSound(entity, org, 50, 1, 1.0f);

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_INT(pSound->m_iBotOwner, -1);
   ASSERT_INT(pSound->m_iChannel, 1); // no 0x1000 flag
   PASS();
   return 0;
}

static int test_save_sound_bot_within_64(void)
{
   TEST("SaveSound: non-player, bot within 64 units -> override + 0x1000");
   mock_reset();

   // Set up a bot
   edict_t *bot_edict = mock_alloc_edict();
   setup_bot(0, bot_edict, Vector(100, 100, 100));

   // Non-player entity far from player indices
   edict_t *entity = mock_alloc_edict();
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();

   // Sound origin within 64 units of bot (distance ~17.3)
   Vector org(110, 110, 110);
   SaveSound(entity, org, 50, 2, 1.0f);

   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, bot_edict); // overridden to bot
   ASSERT_INT(pSound->m_iChannel, 2 | 0x1000); // channel has 0x1000 flag
   ASSERT_INT(pSound->m_iBotOwner, 0);
   PASS();
   return 0;
}

static int test_save_sound_bot_owner_match(void)
{
   TEST("SaveSound: bot is owner -> override + 0x1000 [BUG TEST]");
   mock_reset();

   // Set up a bot far away
   edict_t *bot_edict = mock_alloc_edict();
   setup_bot(0, bot_edict, Vector(9999, 9999, 9999));

   // Non-player entity owned by the bot
   edict_t *entity = mock_alloc_edict();
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();
   entity->v.owner = bot_edict; // bot is the owner

   Vector org(0, 0, 0);
   SaveSound(entity, org, 50, 4, 1.0f);

   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, bot_edict); // should be overridden to bot
   ASSERT_INT(pSound->m_iChannel, 4 | 0x1000); // channel should have 0x1000 flag
   ASSERT_INT(pSound->m_iBotOwner, 0);
   PASS();
   return 0;
}

static int test_save_sound_nearest_bot(void)
{
   TEST("SaveSound: multiple bots -> nearest selected");
   mock_reset();

   edict_t *bot1_edict = mock_alloc_edict();
   setup_bot(0, bot1_edict, Vector(50, 0, 0)); // distance 50

   edict_t *bot2_edict = mock_alloc_edict();
   setup_bot(1, bot2_edict, Vector(20, 0, 0)); // distance 20 (closer)

   edict_t *entity = mock_alloc_edict();
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();

   Vector org(0, 0, 0);
   SaveSound(entity, org, 50, 1, 1.0f);

   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, bot2_edict); // nearest bot
   ASSERT_INT(pSound->m_iBotOwner, 1); // slot 1
   PASS();
   return 0;
}

static int test_save_sound_bot_at_exactly_64(void)
{
   TEST("SaveSound: bot at exactly 64.0 distance -> NOT selected");
   mock_reset();

   edict_t *bot_edict = mock_alloc_edict();
   setup_bot(0, bot_edict, Vector(64, 0, 0)); // exactly 64 units away

   edict_t *entity = mock_alloc_edict();
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();

   Vector org(0, 0, 0);
   SaveSound(entity, org, 50, 1, 1.0f);

   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   // Bot at exactly 64 is NOT < 64, so no override
   ASSERT_INT(pSound->m_iChannel, 1); // no 0x1000 flag
   PASS();
   return 0;
}

static int test_save_sound_bot_at_63_9(void)
{
   TEST("SaveSound: bot at 63.9 distance -> selected");
   mock_reset();

   edict_t *bot_edict = mock_alloc_edict();
   setup_bot(0, bot_edict, Vector(63.9f, 0, 0)); // just under 64

   edict_t *entity = mock_alloc_edict();
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();

   Vector org(0, 0, 0);
   SaveSound(entity, org, 50, 1, 1.0f);

   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, bot_edict);
   ASSERT_INT(pSound->m_iChannel, 1 | 0x1000);
   PASS();
   return 0;
}

static int test_save_sound_owner_match_priority(void)
{
   TEST("SaveSound: owner match takes priority over closer non-owner");
   mock_reset();

   edict_t *bot1_edict = mock_alloc_edict();
   setup_bot(0, bot1_edict, Vector(1, 0, 0)); // very close, not owner

   edict_t *bot2_edict = mock_alloc_edict();
   setup_bot(1, bot2_edict, Vector(9999, 9999, 9999)); // far away, but owner

   edict_t *entity = mock_alloc_edict();
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();
   entity->v.owner = bot2_edict; // bot2 is owner

   Vector org(0, 0, 0);
   SaveSound(entity, org, 50, 1, 1.0f);

   CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ActiveList());
   ASSERT_PTR_NOT_NULL(pSound);
   ASSERT_PTR_EQ(pSound->m_pEdict, bot2_edict); // owner wins
   ASSERT_INT(pSound->m_iBotOwner, 1); // bot2's slot
   ASSERT_INT(pSound->m_iChannel, 1 | 0x1000);
   PASS();
   return 0;
}

// ============================================================
// 12. CSoundEnt::Spawn
// ============================================================

static int test_spawn(void)
{
   TEST("Spawn: sets m_nextthink to time+1");
   mock_reset();

   float time_before = gpGlobals->time;
   pSoundEnt->Spawn();
   ASSERT_TRUE(pSoundEnt->m_nextthink == time_before + 1.0f);
   // Spawn calls Initialize which resets the lists
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_FREE), MAX_WORLD_SOUNDS);
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 0);
   PASS();
   return 0;
}

// ============================================================
// 13. Debug paths
// ============================================================

static int test_think_show_report(void)
{
   TEST("Think: m_fShowReport=TRUE exercises report path");
   mock_reset();

   CSoundEnt::InsertSound(NULL, 1, Vector(10, 20, 30), 50, 10.0f, -1);
   pSoundEnt->m_fShowReport = TRUE;
   pSoundEnt->Think();
   // No crash, report path exercised
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

static int test_think_debug_2(void)
{
   TEST("Think: m_bDebug=2 exercises particle effect path");
   mock_reset();

   // Insert a sound with a valid player edict (exercises is_player branch)
   // Use mock_alloc_edict so we don't conflict with later allocs
   edict_t *player = mock_alloc_edict(); // index 1, which is <= maxClients
   mock_set_classname(player, "player");
   CSoundEnt::InsertSound(player, 1, Vector(10, 20, 30), 50, 10.0f, -1);

   // Insert a sound with a non-player edict (index > maxClients)
   edict_t *entity = mock_alloc_edict();
   while (ENTINDEX(entity) <= (int)gpGlobals->maxClients)
      entity = mock_alloc_edict();
   mock_set_classname(entity, "monster_zombie");
   CSoundEnt::InsertSound(entity, 2, Vector(50, 60, 70), 80, 10.0f, -1);

   // Insert a sound with zero volume (should be skipped)
   CSoundEnt::InsertSound(NULL, 3, Vector(0, 0, 0), 0, 10.0f, -1);

   pSoundEnt->m_bDebug = 2;
   pSoundEnt->Think();
   // No crash, particle effect path exercised for player and non-player
   PASS();
   return 0;
}

static int test_alloc_when_full_debug(void)
{
   TEST("IAllocSound: m_bDebug=1 on overflow prints message");
   mock_reset();

   pSoundEnt->m_bDebug = TRUE;
   // Exhaust all sounds
   for (int i = 0; i < MAX_WORLD_SOUNDS; i++)
      pSoundEnt->IAllocSound();

   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_FREE), 0);
   // This should hit the debug printf path
   ASSERT_INT(pSoundEnt->IAllocSound(), SOUNDLIST_EMPTY);
   PASS();
   return 0;
}

static int test_insert_null_alloc_failure_debug(void)
{
   TEST("InsertSound: alloc failure with m_bDebug=1 prints message");
   mock_reset();

   pSoundEnt->m_bDebug = TRUE;
   // Exhaust all sounds
   for (int i = 0; i < MAX_WORLD_SOUNDS; i++)
      pSoundEnt->IAllocSound();

   // InsertSound with NULL edict -> tries IAllocSound -> fails -> debug msg
   CSoundEnt::InsertSound(NULL, 1, Vector(0, 0, 0), 10, 1.0f, -1);
   // Should not crash, debug message path exercised
   PASS();
   return 0;
}

static int test_get_edict_channel_alloc_failure_debug(void)
{
   TEST("GetEdictChannelSound: alloc failure with m_bDebug=1");
   mock_reset();

   pSoundEnt->m_bDebug = TRUE;
   // Exhaust all sounds
   for (int i = 0; i < MAX_WORLD_SOUNDS; i++)
      pSoundEnt->IAllocSound();

   edict_t *e = mock_alloc_edict();
   CSound *pSound = CSoundEnt::GetEdictChannelSound(e, 5);
   ASSERT_PTR_NULL(pSound);
   PASS();
   return 0;
}

static int test_init_displaysoundlist(void)
{
   TEST("Initialize: displaysoundlist cvar=1 -> m_fShowReport=TRUE");
   mock_reset();

   mock_cvar_displaysoundlist_val = 1.0f;
   pSoundEnt->Initialize();
   ASSERT_INT(pSoundEnt->m_fShowReport, TRUE);
   // Restore
   mock_cvar_displaysoundlist_val = 0.0f;
   PASS();
   return 0;
}

static int test_sounds_in_list_invalid_debug(void)
{
   TEST("ISoundsInList: invalid type with m_bDebug=1 prints message");
   mock_reset();

   pSoundEnt->m_bDebug = TRUE;
   ASSERT_INT(pSoundEnt->ISoundsInList(999), 0);
   PASS();
   return 0;
}

static int test_sound_pointer_invalid_debug(void)
{
   TEST("SoundPointerForIndex: too large with m_bDebug=1");
   mock_reset();

   pSoundEnt->m_bDebug = TRUE;
   ASSERT_PTR_NULL(CSoundEnt::SoundPointerForIndex(MAX_WORLD_SOUNDS));
   PASS();

   TEST("SoundPointerForIndex: negative with m_bDebug=1");
   mock_reset();

   pSoundEnt->m_bDebug = TRUE;
   ASSERT_PTR_NULL(CSoundEnt::SoundPointerForIndex(-1));
   PASS();
   return 0;
}

static int test_client_sound_index_bogus_debug(void)
{
   TEST("ClientSoundIndex: bogus index returns -1 (world edict)");
   mock_reset();

   pSoundEnt->m_bDebug = TRUE;
   // edict at index 0 -> result = 0 - 1 = -1 -> bogus (< 0)
   int idx = CSoundEnt::ClientSoundIndex(&mock_edicts[0]);
   ASSERT_INT(idx, -1);
   PASS();

   TEST("ClientSoundIndex: bogus index returns -1 (beyond maxClients)");
   mock_reset();
   {
      // Allocate edicts until we get one past maxClients
      edict_t *e;
      do { e = mock_alloc_edict(); } while (ENTINDEX(e) <= (int)gpGlobals->maxClients);
      // ENTINDEX(e) - 1 >= maxClients -> bogus, should return -1
      idx = CSoundEnt::ClientSoundIndex(e);
      ASSERT_INT(idx, -1);
   }
   PASS();
   return 0;
}

static int test_insert_debug_2(void)
{
   TEST("InsertSound: m_bDebug=2 exercises particle path");
   mock_reset();

   pSoundEnt->m_bDebug = 2;
   CSoundEnt::InsertSound(NULL, 1, Vector(100, 200, 300), 50, 2.0f, -1);
   // No crash, particle effect path exercised
   ASSERT_INT(pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE), 1);
   PASS();
   return 0;
}

// ============================================================
// Main
// ============================================================

int main(void)
{
   int failures = 0;

   printf("=== CSound::Clear tests ===\n");
   failures += test_clear_zeros_fields();
   failures += test_clear_nulls_edict_and_owner();

   printf("=== CSound::Reset tests ===\n");
   failures += test_reset_zeros_origin_volume_next();
   failures += test_reset_preserves_other_fields();

   printf("=== CSoundEnt::Initialize tests ===\n");
   failures += test_init_all_in_free_list();
   failures += test_init_free_chain();
   failures += test_init_active_empty();

   printf("=== CSoundEnt::IAllocSound tests ===\n");
   failures += test_alloc_one();
   failures += test_alloc_returns_head_of_active();
   failures += test_alloc_when_full();

   printf("=== CSoundEnt::FreeSound tests ===\n");
   failures += test_free_head();
   failures += test_free_middle();
   failures += test_free_goes_to_free_list_head();

   printf("=== CSoundEnt::InsertSound tests ===\n");
   failures += test_insert_null_edict();
   failures += test_insert_edict_new_channel();
   failures += test_insert_edict_existing_channel();
   failures += test_insert_null_soundent();

   printf("=== CSoundEnt::GetEdictChannelSound tests ===\n");
   failures += test_get_edict_channel_new_nonzero();
   failures += test_get_edict_channel_zero_no_match();
   failures += test_get_edict_channel_exact_match();
   failures += test_get_edict_channel_zero_wildcard();
   failures += test_get_edict_channel_different_channels();

   printf("=== CSoundEnt::ISoundsInList tests ===\n");
   failures += test_sounds_in_list_after_init();
   failures += test_sounds_in_list_after_alloc();
   failures += test_sounds_in_list_invalid_type();

   printf("=== CSoundEnt accessors tests ===\n");
   failures += test_active_list_returns_head();
   failures += test_free_list_returns_head();
   failures += test_sound_pointer_valid();
   failures += test_sound_pointer_invalid();
   failures += test_client_sound_index();

   printf("=== CSoundEnt::Think tests ===\n");
   failures += test_think_expired_freed();
   failures += test_think_non_expired_stays();
   failures += test_think_rpg_rocket_origin_update();
   failures += test_think_null_edict_cleared();
   failures += test_think_mixed();

   printf("=== SaveSound tests ===\n");
   failures += test_save_sound_player_edict();
   failures += test_save_sound_no_nearby_bot();
   failures += test_save_sound_bot_within_64();
   failures += test_save_sound_bot_owner_match();
   failures += test_save_sound_nearest_bot();
   failures += test_save_sound_bot_at_exactly_64();
   failures += test_save_sound_bot_at_63_9();
   failures += test_save_sound_owner_match_priority();

   printf("=== CSoundEnt::Spawn tests ===\n");
   failures += test_spawn();

   printf("=== Debug path tests ===\n");
   failures += test_think_show_report();
   failures += test_think_debug_2();
   failures += test_alloc_when_full_debug();
   failures += test_insert_null_alloc_failure_debug();
   failures += test_get_edict_channel_alloc_failure_debug();
   failures += test_init_displaysoundlist();
   failures += test_sounds_in_list_invalid_debug();
   failures += test_sound_pointer_invalid_debug();
   failures += test_client_sound_index_bogus_debug();
   failures += test_insert_debug_2();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return failures ? 1 : 0;
}
