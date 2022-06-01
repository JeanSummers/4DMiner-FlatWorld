#include <stdio.h>
#include <string>
#include "MinHook.h"
#include <memory>

typedef struct {
	uint8_t blocks[8][128][8][8];
	int64_t pos[3];
	unsigned char stuffing[960];
} Chunk;

class World {};

typedef void(__thiscall* generateChunk)(World*, Chunk*);

generateChunk func_original = nullptr;

int generate(int64_t x, int64_t y, int64_t z, int64_t w) {
	int64_t elevation = 32;
	if (y > elevation) return 0;
	if (y == elevation) return 1;
	if (y > elevation - 5 && y < elevation) return 2;
	return 3;
}


void __fastcall func_detour(World* world, Chunk* chunk) {
	printf("Generating chunk | %4lld | %4lld | %4lld |\n", chunk->pos[0], chunk->pos[1], chunk->pos[2]);
	for (int64_t X = 0; X < 8; X++) {
		for (int64_t Y = 0; Y < 128; Y++) {
			for (int64_t Z = 0; Z < 8; Z++) {
				for (int64_t W = 0; W < 8; W++) {
					chunk->blocks[X][Y][Z][W] = generate(X, Y, Z, W);
				}
			}
		}
	}
}

template <typename TARGET, typename DETOUR, typename ORIGINAL>
BOOL createHook(TARGET* target, DETOUR* detour, ORIGINAL* original) {
	auto t = reinterpret_cast<LPVOID>(target);
	auto d = reinterpret_cast<LPVOID>(detour);
	auto o = reinterpret_cast<LPVOID*>(original);
	auto created = MH_CreateHook(t, d, o);
	if (created != MH_OK) {
		auto status_str = MH_StatusToString(created);
		printf("Error: Unable to create hook for %llX: %s", (uint64_t)t, status_str);
		return FALSE;
	}
	auto enabled = MH_EnableHook(MH_ALL_HOOKS);
	if (enabled != MH_OK) {
		auto status_str = MH_StatusToString(enabled);
		printf("Error: Unable to enable hook for %llX: %s", (uint64_t)t, status_str);
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI HookThread(LPVOID lpReserved) {
	HMODULE handle = GetModuleHandleA(NULL);

	printf("Base: %llX\n", (uint64_t)handle);

	auto target = reinterpret_cast<LPVOID>((uint64_t)handle + 0x87640);

	printf("Target: %llX\n", (uint64_t)target);

	createHook(target, func_detour, &func_original);

	return 0;
}

bool init = false;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lp_reserved) {
	if (init) return 0;
	if (ul_reason_for_call != DLL_PROCESS_ATTACH) return 1;

	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);

	printf("Mod started\n");

	auto initialized = MH_Initialize();
	if (initialized != MH_OK) {
		auto status_str = MH_StatusToString(initialized);
		printf("Error: Unable to initialize hooks: %s", status_str);
		return FALSE;
	}

	auto handle = CreateThread(nullptr, 0, HookThread, hModule, 0, nullptr);
	if (!handle) {
		printf("Error: Unable to spawn hook thread");
		return FALSE;
	}

	init = true;
	return TRUE;
}