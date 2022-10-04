#pragma once

#pragma pack(push, 1)
struct JMP_REL
{
	uint8_t opcode = 0;
	union
	{
		uint32_t address = 0;
		uint8_t addressShort;
	};
};
struct JMP_ABS
{
	uint16_t opcode = 0;
	uint32_t opcodeExcess = 0;
	uint64_t address = 0;
};
#pragma pack(pop)

using fnFunc = void(__fastcall *)(void);
struct trampolineInfo
{
	LPVOID originalFunction = nullptr;
	LPVOID newFunction = nullptr;
	LPVOID trampoline = nullptr;
	fnFunc returnTrampoline = nullptr; // Dangerous right now
	std::unique_ptr<uint8_t*> overwrittenBytes = nullptr;
	size_t overwrittenBytesSize = 0;
};

class Engine
{
public:
	Engine();
	~Engine();

	// Singleton
	static Engine* getInstance();

	// Utility functions
	LPVOID RVAToPtr(LPVOID address);
	LPVOID RVAToPtr(uintptr_t address);
	static bool patchBytesCpy(LPVOID dest, LPVOID src, size_t size);
	static bool patchBytesSet(LPVOID dest, uint8_t src, size_t size);

	// Detour stuff
	bool detourFunction(uint64_t id, LPVOID originalFunc, LPVOID newFunc, bool preserveBytes);
	trampolineInfo* getTrampoline(uint64_t id);

private:
	LPVOID baseAddress = nullptr;

	// Singleton
	static Engine* instance;

	// Trampoline cache
	std::unordered_map<uint64_t, trampolineInfo> trampolines;
};
