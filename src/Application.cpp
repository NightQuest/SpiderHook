#include "preCompiled.h"

Application* Application::instance;

enum DetourID
{
	DETOUR_WEB_ZIP = 0,
};

struct WebZip
{
	uint8_t unk[0x47];
	uint32_t webZipCount;
	uint64_t unk2;
	float cooldown;
};

void WebZipCooldown(WebZip* self)
{
	self->webZipCount = 0;
	self->cooldown = 0.f;

	// Since we're changing the entire function, we don't need these
	auto info = Engine::getInstance()->getTrampoline(DETOUR_WEB_ZIP);
	reinterpret_cast<void(__fastcall*)(WebZip*)>(info->returnTrampoline)(self);
}

Application::Application() : eng(Engine::getInstance())
{
	if( !instance )
		instance = this;
}

Application::~Application()
{
}

Application* Application::getInstance()
{
	if( instance != nullptr )
		return instance;

	instance = new Application();

	return instance;
}

void Application::OnAttach()
{
	std::thread(
		[&]() {
			do
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				HWND hwnd = nullptr;
				hwnd = ::FindWindow("GameNxApp", nullptr);
				if( !hwnd )
					continue;

				auto ptr = std::make_unique<char*>(new char[MAX_PATH+1]);
				::GetWindowText(hwnd, *ptr, MAX_PATH);

				if( !std::regex_match(*ptr, std::regex(R"(^Marvel's Spider-Man Remastered v\d+\.\d+\.\d+\.\d+$)")) )
					continue;

				std::lock_guard lock(this->handle_mutex);
				this->windowHandle = hwnd;

				HKEY hKey;
				if( ::RegOpenKey(HKEY_CURRENT_USER, R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\Personalize)", &hKey) == ERROR_SUCCESS )
				{
					DWORD dwType = REG_DWORD, dwSize = sizeof(DWORD);
					DWORD dwAttrib = FALSE;

					if( ::RegQueryValueEx(hKey, "AppsUseLightTheme", nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwAttrib), &dwSize) == ERROR_SUCCESS )
					{
						BOOL darkMode = !dwAttrib;
						::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(BOOL));
					}
					::RegCloseKey(hKey);
				}

			} while( !this->windowHandle );
		}).detach();

	eng->detourFunction(DETOUR_WEB_ZIP, eng->RVAToPtr(0x09a8d50), WebZipCooldown, false);
}

void Application::OnDetach()
{

}

void Application::OnTick()
{
}
