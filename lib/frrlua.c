/*
 * This file defines the lua interface into
 * FRRouting.
 *
 * Copyright (C) 2016-2019 Cumulus Networks, Inc.
 * Donald Sharp, Quentin Young
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; see the file COPYING; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <zebra.h>

#if defined(HAVE_LUA)
#include "prefix.h"
#include "frrlua.h"
#include "log.h"

/*
 * FRR convenience functions.
 *
 * This section has convenience functions used to make interacting with the Lua
 * stack easier.
 */

const char *frrlua_table_get_string(lua_State *L, const char *key)
{
	const char *str;

	lua_pushstring(L, key);
	lua_gettable(L, -2);

	str = (const char *)lua_tostring(L, -1);
	lua_pop(L, 1);

	return str;
}

int frrlua_table_get_integer(lua_State *L, const char *key)
{
	int result;

	lua_pushstring(L, key);
	lua_gettable(L, -2);

	result = lua_tointeger(L, -1);
	lua_pop(L, 1);

	return result;
}

/*
 * Encoders.
 *
 * This section has functions that convert internal FRR datatypes into Lua
 * datatypes.
 */

void frrlua_newtable_prefix(lua_State *L, const struct prefix *prefix)
{
	char buffer[100];

	zlog_debug("frrlua: pushing prefix table");

	lua_newtable(L);
	lua_pushstring(L, prefix2str(prefix, buffer, 100));
	lua_setfield(L, -2, "route");
	lua_pushinteger(L, prefix->family);
	lua_setfield(L, -2, "family");
	lua_setglobal(L, "prefix");
}

void frrlua_newtable_interface(lua_State *L, const struct interface *ifp)
{
	zlog_debug("frrlua: pushing interface table");

	lua_newtable(L);
	lua_pushstring(L, ifp->name);
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, ifp->ifindex);
	lua_setfield(L, -2, "ifindex");
	lua_pushinteger(L, ifp->status);
	lua_setfield(L, -2, "status");
	lua_pushinteger(L, ifp->flags);
	lua_setfield(L, -2, "flags");
	lua_pushinteger(L, ifp->metric);
	lua_setfield(L, -2, "metric");
	lua_pushinteger(L, ifp->speed);
	lua_setfield(L, -2, "speed");
	lua_pushinteger(L, ifp->mtu);
	lua_setfield(L, -2, "mtu");
	lua_pushinteger(L, ifp->mtu6);
	lua_setfield(L, -2, "mtu6");
	lua_pushinteger(L, ifp->bandwidth);
	lua_setfield(L, -2, "bandwidth");
	lua_pushinteger(L, ifp->link_ifindex);
	lua_setfield(L, -2, "link_ifindex");
	lua_pushinteger(L, ifp->ll_type);
	lua_setfield(L, -2, "linklayer_type");
}

/*
 * Logging.
 *
 * Lua-compatible wrappers for FRR logging functions.
 */
static const char *frrlua_log_thunk(lua_State *L)
{
	int nargs;

	nargs = lua_gettop(L);
	assert(nargs == 1);

	return lua_tostring(L, 1);
}

static int frrlua_log_debug(lua_State *L)
{
	zlog_debug("%s", frrlua_log_thunk(L));
	return 0;
}

static int frrlua_log_info(lua_State *L)
{
	zlog_info("%s", frrlua_log_thunk(L));
	return 0;
}

static int frrlua_log_notice(lua_State *L)
{
	zlog_notice("%s", frrlua_log_thunk(L));
	return 0;
}

static int frrlua_log_warn(lua_State *L)
{
	zlog_warn("%s", frrlua_log_thunk(L));
	return 0;
}

static int frrlua_log_error(lua_State *L)
{
	zlog_err("%s", frrlua_log_thunk(L));
	return 0;
}

static const luaL_Reg log_funcs[] = {
	{"debug", frrlua_log_debug},
	{"info", frrlua_log_info},
	{"notice", frrlua_log_notice},
	{"warn", frrlua_log_warn},
	{"error", frrlua_log_error},
	{},
};

void frrlua_export_logging(lua_State *L)
{
	lua_newtable(L);
	luaL_setfuncs(L, log_funcs, 0);
	lua_setfield(L, -2, "log");
}


/*
 * Experimental.
 *
 * This section has experimental Lua functionality that doesn't belong
 * elsewhere.
 */

enum frrlua_rm_status frrlua_run_rm_rule(lua_State *L, const char *rule)
{
	int status;

	lua_getglobal(L, rule);
	status = lua_pcall(L, 0, 1, 0);
	if (status) {
		zlog_debug("Executing Failure with function: %s: %d",
			   rule, status);
		return LUA_RM_FAILURE;
	}

	status = lua_tonumber(L, -1);
	return status;
}

/* Initialization */

static void *frrlua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	(void)ud;
	(void)osize; /* not used */

	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else
		return realloc(ptr, nsize);
}

lua_State *frrlua_initialize(const char *file)
{
	int status;
	lua_State *L = lua_newstate(frrlua_alloc, NULL);

	luaL_openlibs(L);
	if (file) {
		status = luaL_loadfile(L, file);
		if (status) {
			zlog_debug("Failure to open %s %d", file, status);
			lua_close(L);
			return NULL;
		}
		lua_pcall(L, 0, LUA_MULTRET, 0);
	}

	return L;
}


#endif
