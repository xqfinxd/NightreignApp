#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <string>
#include <iostream>

class LuaScript {
public:
    LuaScript() {
        m_state = luaL_newstate();
        luaL_openlibs(m_state);
    }

    ~LuaScript() {
        if (m_state) {
            lua_close(m_state);
        }
    }

    // Execute a Lua script file
    bool executeFile(const std::string& filename) {
        if (luaL_dofile(m_state, filename.c_str()) != LUA_OK) {
            std::cerr << "Lua Error: " << lua_tostring(m_state, -1) << std::endl;
            lua_pop(m_state, 1);
            return false;
        }
        return true;
    }

    lua_State* getState() { return m_state; }

private:
    lua_State* m_state;
};
