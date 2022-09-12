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
	static bool patchBytes(LPVOID dest, LPVOID src, size_t size);
	bool detourFunction(LPVOID originalFunc, LPVOID newFunc);

private:
	LPVOID baseAddress = nullptr;

	// Singleton
	static Engine* instance;
};
