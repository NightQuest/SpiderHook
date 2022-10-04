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
	for( auto& [idx, val] : trampolines )
	{
		if( val.trampoline )
		{
			patchBytesCpy(val.originalFunction, *val.overwrittenBytes, val.overwrittenBytesSize);
			::VirtualFree(val.trampoline, 1024, MEM_RELEASE);
		}
	}
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

bool Engine::patchBytesCpy(LPVOID dest, const LPVOID src, size_t size)
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

bool Engine::patchBytesSet(LPVOID dest, const uint8_t src, size_t size)
{
	DWORD oldProtect;
	if( ::VirtualProtect(dest, size, PAGE_READWRITE, &oldProtect) )
	{
		::memset(dest, src, size);
		::VirtualProtect(dest, size, oldProtect, &oldProtect);
		for( uint32_t x = 0; x < size; x++ )
			if( static_cast<uint8_t*>(dest)[x] != src )
				return false;
		return true;
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
bool Engine::detourFunction(uint64_t id, LPVOID originalFunc, LPVOID newFunc, bool preserveBytes)
{
	trampolineInfo info;
	info.originalFunction = originalFunc;
	info.newFunction = newFunc;

	// No duplicates
	if( trampolines.contains(id) )
		return false;

	// Use capstone to extract our asm's byte size, so it's complete instructions
	::csh handle;
	if( ::cs_open(::CS_ARCH_X86, ::CS_MODE_64, &handle) == CS_ERR_OK )
	{
		::cs_insn* instructions;
		auto count = ::cs_disasm(handle, static_cast<const uint8_t*>(originalFunc), 20, reinterpret_cast<uint64_t>(originalFunc), 20, &instructions);
		if( count )
		{
			uint32_t totalSize = 0;
			for( uint32_t x = 0; x < count; x++ )
			{
				totalSize += instructions[x].size;
				if( totalSize >= sizeof(JMP_REL) )
				{
					info.overwrittenBytesSize = totalSize;
					break;
				}
			}
			::cs_free(instructions, count);
		}
		::cs_close(&handle);
	}
	
	// Make sure we have something to detour
	if( !info.overwrittenBytesSize )
		return false;

	// copy the bytes we're going to overwrite, and then overwrite with NOPs
	info.overwrittenBytes = std::make_unique<uint8_t*>(new uint8_t[info.overwrittenBytesSize]);
	::memcpy(*info.overwrittenBytes, originalFunc, info.overwrittenBytesSize);
	patchBytesSet(originalFunc, 0x90, info.overwrittenBytesSize);

	// Find a suitable place to create a code cave
	auto search = reinterpret_cast<uintptr_t>(originalFunc) - 0x2000;
	while( !info.trampoline )
	{
		info.trampoline = ::VirtualAlloc(reinterpret_cast<LPVOID>(search), 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		search += 0x200;
	}
	auto trampoline = info.trampoline;

	// Write our trampoline to newFunc
	auto tramp = static_cast<JMP_ABS*>(trampoline);
	tramp->opcode = 0x25'FF; // reversed
	tramp->address = reinterpret_cast<uintptr_t>(newFunc);

	// move past written bytes
	trampoline = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(trampoline) + sizeof(JMP_ABS));

	// Save our return trampoline
	info.returnTrampoline = reinterpret_cast<fnFunc>(trampoline);

	if( preserveBytes )
	{
		// Store our overwritten bytes
		::memcpy(trampoline, *info.overwrittenBytes, info.overwrittenBytesSize);

		// move past written bytes
		trampoline = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(trampoline) + info.overwrittenBytesSize);
	}

	// Write our trampoline to originalFunc (for returning)
	auto tramp2 = static_cast<JMP_REL*>(trampoline);
	tramp2->opcode = 0xE9; // relative jmp
	tramp2->address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(originalFunc) - reinterpret_cast<uintptr_t>(trampoline));

	// Write our jmp to the trampoline
	JMP_REL jmp;
	jmp.opcode = 0xE9; // relative jmp
	jmp.address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(info.trampoline) - reinterpret_cast<uintptr_t>(originalFunc) - sizeof(JMP_REL));
	const auto ret = patchBytesCpy(originalFunc, &jmp, sizeof(JMP_REL));

	// Store our trampoline data
	trampolines[id] = std::move(info);
	return ret;
}
