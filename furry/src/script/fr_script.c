#include "fr_script.h"
#include "../core/fr_log.h"
#include "../platform/fr_input.h"
#include "../platform/fr_platform.h"
#include "../render/fr_renderer.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

struct FrScript {
  lua_State *L;
  FrWindow *window;
  FrRenderer *renderer;
};

// --- Window API ---
static int lua_window_should_close(lua_State *L) {
  FrScript *script = (FrScript *)lua_touserdata(L, lua_upvalueindex(1));
  if (!script || !script->window)
    return 0;
  bool close = fr_window_should_close(script->window);
  lua_pushboolean(L, close);
  return 1;
}

static int lua_window_poll_events(lua_State *L) {
  FrScript *script = (FrScript *)lua_touserdata(L, lua_upvalueindex(1));
  if (!script || !script->window)
    return 0;
  fr_window_poll_events(script->window);
  return 0;
}

// --- Input API ---
static int lua_input_is_key_pressed(lua_State *L) {
  if (lua_gettop(L) < 1)
    return 0;
  int key = luaL_checkinteger(L, 1);
  bool pressed = fr_input_is_key_pressed((FrKey)key);
  lua_pushboolean(L, pressed);
  return 1;
}

static int lua_input_is_key_down(lua_State *L) {
  if (lua_gettop(L) < 1)
    return 0;
  int key = luaL_checkinteger(L, 1);
  bool down = fr_input_is_key_down((FrKey)key);
  lua_pushboolean(L, down);
  return 1;
}

// --- Renderer API ---
static int lua_renderer_begin_frame(lua_State *L) {
  FrScript *script = (FrScript *)lua_touserdata(L, lua_upvalueindex(1));
  if (!script || !script->renderer)
    return 0;
  bool res = fr_renderer_begin_frame(script->renderer);
  lua_pushboolean(L, res);
  return 1;
}

static int lua_renderer_end_frame(lua_State *L) {
  FrScript *script = (FrScript *)lua_touserdata(L, lua_upvalueindex(1));
  if (!script || !script->renderer)
    return 0;
  fr_renderer_end_frame(script->renderer);
  return 0;
}

static int lua_draw_sprite(lua_State *L) {
  FrScript *script = (FrScript *)lua_touserdata(L, lua_upvalueindex(1));
  if (!script || !script->renderer)
    return 0;

  // Arguments: texture_ptr (lightuserdata), x, y, w, h, r, g, b, a
  if (lua_gettop(L) < 9)
    return 0;

  FrTexture *tex = (FrTexture *)lua_touserdata(L, 1);
  float px = (float)luaL_checknumber(L, 2);
  float py = (float)luaL_checknumber(L, 3);
  float w = (float)luaL_checknumber(L, 4);
  float h = (float)luaL_checknumber(L, 5);
  float cr = (float)luaL_checknumber(L, 6);
  float cg = (float)luaL_checknumber(L, 7);
  float cb = (float)luaL_checknumber(L, 8);
  float ca = (float)luaL_checknumber(L, 9);

  FrVec2 pos = {px, py};
  FrVec2 size = {w, h};
  FrVec4 color = {cr, cg, cb, ca};

  fr_draw_sprite(script->renderer, tex, pos, size, color);
  return 0;
}

static void register_engine_api(FrScript *script) {
  lua_State *L = script->L;

  // Pass script instance as upvalue
  lua_pushlightuserdata(L, script);
  lua_pushcclosure(L, lua_window_should_close, 1);
  lua_setglobal(L, "WindowShouldClose");

  lua_pushlightuserdata(L, script);
  lua_pushcclosure(L, lua_window_poll_events, 1);
  lua_setglobal(L, "WindowPollEvents");

  lua_pushcfunction(L, lua_input_is_key_pressed);
  lua_setglobal(L, "InputIsKeyPressed");

  lua_pushcfunction(L, lua_input_is_key_down);
  lua_setglobal(L, "InputIsKeyDown");

  lua_pushlightuserdata(L, script);
  lua_pushcclosure(L, lua_renderer_begin_frame, 1);
  lua_setglobal(L, "RendererBeginFrame");

  lua_pushlightuserdata(L, script);
  lua_pushcclosure(L, lua_renderer_end_frame, 1);
  lua_setglobal(L, "RendererEndFrame");

  lua_pushlightuserdata(L, script);
  lua_pushcclosure(L, lua_draw_sprite, 1);
  lua_setglobal(L, "DrawSprite");
}

FrScript *fr_script_init(FrWindow *window, FrRenderer *renderer) {
  FrScript *script = (FrScript *)malloc(sizeof(FrScript));
  if (!script)
    return NULL;

  script->window = window;
  script->renderer = renderer;

  script->L = luaL_newstate();
  if (!script->L) {
    FR_ERROR("Failed to initialize Lua state");
    free(script);
    return NULL;
  }

  luaL_openlibs(script->L);
  register_engine_api(script);

  FR_INFO("Lua scripting initialized");
  return script;
}

void fr_script_shutdown(FrScript *script) {
  if (!script)
    return;
  if (script->L) {
    lua_close(script->L);
  }
  free(script);
  FR_INFO("Lua scripting shutdown");
}

bool fr_script_execute_file(FrScript *script, const char *path) {
  if (!script || !script->L)
    return false;
  if (luaL_dofile(script->L, path) != LUA_OK) {
    FR_ERROR("Lua File Error: %s", lua_tostring(script->L, -1));
    lua_pop(script->L, 1);
    return false;
  }
  return true;
}

bool fr_script_execute_string(FrScript *script, const char *code) {
  if (!script || !script->L)
    return false;
  if (luaL_dostring(script->L, code) != LUA_OK) {
    FR_ERROR("Lua String Error: %s", lua_tostring(script->L, -1));
    lua_pop(script->L, 1);
    return false;
  }
  return true;
}
