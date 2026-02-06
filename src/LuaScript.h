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
        L = luaL_newstate();
        luaL_openlibs(L);
    }

    ~LuaScript() {
        if (L) {
            lua_close(L);
        }
    }

    // Execute a Lua script file
    bool executeFile(const std::string& filename) {
        if (luaL_dofile(L, filename.c_str()) != LUA_OK) {
            std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    // Execute Lua code string
    bool executeString(const std::string& code) {
        if (luaL_dostring(L, code.c_str()) != LUA_OK) {
            std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    // Get global number value
    double getNumber(const std::string& varName) {
        lua_getglobal(L, varName.c_str());
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            return 0.0;
        }
        double value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return value;
    }

    // Get global string value
    std::string getString(const std::string& varName) {
        lua_getglobal(L, varName.c_str());
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return "";
        }
        std::string value = lua_tostring(L, -1);
        lua_pop(L, 1);
        return value;
    }

    // Set global number value
    void setNumber(const std::string& varName, double value) {
        lua_pushnumber(L, value);
        lua_setglobal(L, varName.c_str());
    }

    // Set global string value
    void setString(const std::string& varName, const std::string& value) {
        lua_pushstring(L, value.c_str());
        lua_setglobal(L, varName.c_str());
    }

    // Call a Lua function with no arguments
    bool callFunction(const std::string& funcName) {
        lua_getglobal(L, funcName.c_str());
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    lua_State* getState() { return L; }

private:
    lua_State* L;
};
