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

trampolineInfo* Engine::getTrampoline(uint64_t id)
{
	if( !trampolines.contains(id) )
		return nullptr;

	return &trampolines[id];
}

// Caller is responsible for calling trampolineReturn and handling overwrittenBytes
bool Engine::detourFunction(uint64_t id, LPVOID originalFunc, LPVOID newFunc)
{
	trampolineInfo info;
	info.originalFunction = originalFunc;
	info.newFunction = newFunc;


	// Find a suitable place to create a code cave
	auto search = reinterpret_cast<uintptr_t>(originalFunc) - 0x2000;
	while( !info.trampoline )
	{
		info.trampoline = ::VirtualAlloc(reinterpret_cast<LPVOID>(search), 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		search += 0x200;
	}
	info.overwrittenBytes = std::make_unique<uint8_t*>(new uint8_t[sizeof(JMP_REL)]);


	// copy the bytes we're going to overwrite into our trampoline
	::memcpy(*info.overwrittenBytes, originalFunc, sizeof(JMP_REL));
	//::memcpy(info.trampoline, *info.overwrittenBytes, sizeof(JMP_REL));

	// move past written bytes
	auto trampoline = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(info.trampoline));

	// Write our trampoline to newFunc
	auto tramp = static_cast<JMP_ABS*>(trampoline);
	tramp->opcode = 0x25'FF; // reversed
	tramp->address = reinterpret_cast<uintptr_t>(newFunc);

	// move past written bytes
	trampoline = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(trampoline) + sizeof(JMP_ABS));

	// Store our trampoline data
	info.returnTrampoline = reinterpret_cast<fnFunc>(trampoline);
	trampolines[id] = std::move(info);

	// Write our trampoline to originalFunc (for returning)
	auto tramp2 = static_cast<JMP_REL*>(trampoline);
	tramp2->opcode = 0xE9; // relative jmp
	tramp2->address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(originalFunc) - reinterpret_cast<uintptr_t>(trampoline) + sizeof(JMP_REL) - 1);

	// Write our jmp to the trampoline
	JMP_REL jmp;
	jmp.opcode = 0xE9; // relative jmp
	jmp.address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(info.trampoline) - reinterpret_cast<uintptr_t>(originalFunc) - sizeof(JMP_REL));
	const auto ret = patchBytes(originalFunc, &jmp, sizeof(JMP_REL));

	trampolines[id] = std::move(info);
	return ret;
}
