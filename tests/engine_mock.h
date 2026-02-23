//
// JK_Botti - engine mock for unit testing
//
// engine_mock.h
//

#ifndef ENGINE_MOCK_H
#define ENGINE_MOCK_H

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"

// Pool of mock edicts for tests
#define MOCK_MAX_EDICTS 64
extern edict_t mock_edicts[MOCK_MAX_EDICTS];

// Reset all mock state between tests
void mock_reset(void);

// Edict helpers
edict_t *mock_alloc_edict(void);
void mock_set_classname(edict_t *e, const char *name);

// Register a func_breakable in the breakable list
void mock_add_breakable(edict_t *pEdict, int material_breakable);

// Mock cvar values
extern float mock_cvar_bm_gluon_mod_val;

// Control what TRACE_HULL / TRACE_LINE return
typedef void (*mock_trace_fn)(const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr);
extern mock_trace_fn mock_trace_hull_fn;
extern mock_trace_fn mock_trace_line_fn;

#endif // ENGINE_MOCK_H
