#pragma once

#pragma data_seg(".spiderhook_shared")
static HINSTANCE hInstance;
static HMODULE hFaultRep = nullptr;
static unsigned int instances = 0;
static Application app;
#pragma data_seg()
