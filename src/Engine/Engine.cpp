#include "preCompiled.h"

Engine* Engine::instance;

Engine::Engine() : baseAddress(nullptr)
{
	// Grab the DOS header of Spider-Man.exe and make sure its signature is valid
	const auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(::GetModuleHandle(nullptr));
	if( dosHeader->e_magic != IMAGE_DOS_SIGNATURE )
		return;

	// Grab the NT header of Spider-Man.exe and make sure its signature is valid
	const auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint64_t>(dosHeader) + dosHeader->e_lfanew);
	if( ntHeaders->Signature != IMAGE_NT_SIGNATURE )
		return;

	// Set the base address, based on the header information.
	baseAddress = reinterpret_cast<LPVOID>(ntHeaders->OptionalHeader.ImageBase);// + ntHeaders->OptionalHeader.BaseOfCode);
}

Engine::~Engine()
{
}

Engine* Engine::getInstance()
{
	if( instance != nullptr )
		return instance;

	instance = new Engine();

	return instance;
}

LPVOID Engine::RVAToPtr(LPVOID address)
{
	return RVAToPtr(reinterpret_cast<uintptr_t>(address));
}

LPVOID Engine::RVAToPtr(uintptr_t address)
{
	return reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(baseAddress) + address);
}

bool Engine::patchBytes(LPVOID dest, const LPVOID src, size_t size)
{
	DWORD oldProtect;
	if( ::VirtualProtect(dest, size, PAGE_READWRITE, &oldProtect) )
	{
		::memcpy(dest, src, size);
		::VirtualProtect(dest, size, oldProtect, &oldProtect);
		return (::memcmp(dest, src, size) == 0);
	}
	return false;
}


// Caller is responsible for calling original - this is a destructive detour
bool Engine::detourFunction(LPVOID originalFunc, LPVOID newFunc)
{
	// Find a suitable place to create a code cave
	auto search = reinterpret_cast<uintptr_t>(originalFunc) - 0x2000;
	LPVOID trampoline = nullptr, originalTrampoline = nullptr;
	while( !trampoline )
	{
		trampoline = ::VirtualAlloc(reinterpret_cast<LPVOID>(search), sizeof(JMP_ABS), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		search += 0x200;
	}
	originalTrampoline = trampoline;

	// copy the bytes we're going to overwrite into our trampoline
	::memcpy(trampoline, originalFunc, sizeof(JMP_REL));

	// move past written bytes
	trampoline = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(trampoline) + sizeof(JMP_REL));

	// Write our trampoline to newFunc
	auto tramp = static_cast<JMP_ABS*>(trampoline);
	tramp->opcode = 0x25'FF; // reversed
	tramp->address = reinterpret_cast<uintptr_t>(newFunc);

	// Write our jmp to the trampoline
	JMP_REL jmp;
	jmp.opcode = 0xE9;
	jmp.address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(originalTrampoline) - reinterpret_cast<uintptr_t>(originalFunc));
	return patchBytes(originalFunc, &jmp, sizeof(JMP_REL));
}
