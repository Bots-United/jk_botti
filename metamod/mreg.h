// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// mreg.h - description of registered items (classes MRegFunc,
//          MRegFuncList)

/*
 * Copyright (c) 2001 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#ifndef MREG_H
#define MREG_H

#include "support_meta.h"		// mBOOL, etc


// Max number of registered commands we can manage.
#define MAX_REG_FUNCS	512
// Width required to printf above MAX, for show() functions.
#define WIDTH_MAX_REG_FUNCS	3

// Max number of registered cvars we can manage.
#define MAX_REG_CVARS	512
// Width required to printf above MAX, for show() functions.
#define WIDTH_MAX_REG_CVARS	3

// Max number of registered user msgs we can manage.
#define MAX_REG_MSGS	256

// Flags to indicate if given cvar or func is part of a loaded plugin.
typedef enum {
	RG_INVALID,
	RG_VALID,
} REG_STATUS;

// Pointer to function registered by AddServerCommand.
typedef void (*REG_CMD_FN) (void);


// An individual registered function/command.
class MRegFunc {
	friend class MRegFuncList;
	private:
	// data:
		int index;				// 1-based
	public:
		char *name;				// space is malloc'd
		REG_CMD_FN pfnFunc;		// pointer to the function
		int plugid;				// index id of corresponding plugin
		REG_STATUS status;		// whether corresponding plugin is loaded
	// functions:
		mBOOL call(void);		// try to call the function
};


// A list of registered functions.
class MRegFuncList {
	private:
	// data:
		MRegFunc flist[MAX_REG_FUNCS];	// array of registered functions
		int size;				// size of list, ie MAX_REG_FUNCS
		int endlist;			// index of last used entry

	public:
	// constructor:
		MRegFuncList(void);

	// functions:
		MRegFunc *find(const char *findname);	// find by MRegFunc->name
		MRegFunc *add(const char *addname);
		void disable(int plugin_id);			// change status to Invalid
		void show(void);						// list all funcs to console
		void show(int plugin_id);				// list given plugin's funcs to console
};



// An individual registered cvar.
class MRegCvar {
	friend class MRegCvarList;
	private:
	// data:
		int index;				// 1-based
	public:
		cvar_t data;			// actual cvar structure
		int plugid;				// index id of corresponding plugin
		REG_STATUS status;		// whether corresponding plugin is loaded
	// functions:
		mBOOL set(cvar_t *src);
};


// A list of registered cvars.
class MRegCvarList {
	private:
	// data:
		MRegCvar clist[MAX_REG_CVARS];	// array of registered cvars
		int size;				// size of list, ie MAX_REG_CVARS
		int endlist;			// index of last used entry

	public:
	// constructor:
		MRegCvarList(void);

	// functions:
		MRegCvar *add(const char *addname);
		MRegCvar *find(const char *findname);	// find by MRegCvar->data.name
		void disable(int plugin_id);			// change status to Invalid
		void show(void);						// list all cvars to console
		void show(int plugin_id);				// list given plugin's cvars to console
};



// An individual registered user msg, from gamedll.
class MRegMsg {
	friend class MRegMsgList;
	private:
	// data:
		int index;				// 1-based
	public:
		const char *name;		// name, assumed constant string in gamedll
		int msgid;				// msgid, assigned by engine
		int size;				// size, if given by gamedll
};


// A list of registered user msgs.
class MRegMsgList {
	private:
	// data:
		MRegMsg mlist[MAX_REG_MSGS];	// array of registered msgs
		int size;						// size of list, ie MAX_REG_MSGS
		int endlist;					// index of last used entry

	public:
	// constructor:
		MRegMsgList(void);

	// functions:
		MRegMsg *add(const char *addname, int addmsgid, int addsize);
		MRegMsg *find(const char *findname);
		MRegMsg *find(int findmsgid);
		void show(void);						// list all msgs to console
};

#endif /* MREG_H */
