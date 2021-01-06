#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include "Detours/detours.h"

unsigned int lua_State;

typedef int(__cdecl* gluaL_newstate_def)();
typedef int(__cdecl* gluaL_loadstring_def)(int Client_LuaState, const char* Buffer);
typedef int(__cdecl* glua_pcall_def)(unsigned int Client_LuaState, int a1, int a2, int a3);
gluaL_newstate_def g_luaL_newstate;
gluaL_loadstring_def g_luaL_loadstring;
glua_pcall_def g_lua_pcall;

int hooked_luaL_newstate()
{
	unsigned int State = g_luaL_newstate();
	printf("Caught State: %p\n", State);
	lua_State = State;
	return State;
}

unsigned long __stdcall Thread(void*)
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
	SetConsoleTitleA("VIVA GMOD | Lua Executor");

	HMODULE lua_shared = GetModuleHandleA("lua_shared.dll");
	printf("Got Handle: %p\n", lua_shared);
	g_luaL_newstate = reinterpret_cast<gluaL_newstate_def>(reinterpret_cast<unsigned int>(GetProcAddress(lua_shared, "luaL_newstate")));
	printf("Got luaL_newstate: %p\n", g_luaL_newstate);
	g_luaL_loadstring = reinterpret_cast<gluaL_loadstring_def>(reinterpret_cast<unsigned int>(GetProcAddress(lua_shared, "luaL_loadstring")));
	printf("Got luaL_loadstring: %p\n", g_luaL_loadstring);
	g_lua_pcall = reinterpret_cast<glua_pcall_def>(reinterpret_cast<unsigned int>(GetProcAddress(lua_shared, "lua_pcall")));
	printf("Got lua_pcall: %p\n", g_lua_pcall);

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&reinterpret_cast<void*&>(g_luaL_newstate), hooked_luaL_newstate);
	DetourTransactionCommit();
	std::cout << "Hooked luaL_newstate.\n";

	std::string script;

	while (true)
	{
		std::getline(std::cin, script);
		if (!g_luaL_loadstring(lua_State, script.c_str()))
		{
			g_lua_pcall(lua_State, 0, (-1), 0);
		}
		script = "";
	}

	return 1;
}

int __stdcall DllMain(int, unsigned int call, void*)
{
	if (call == 1)
		CreateThread(0, 0, Thread, 0, 0, 0);
}