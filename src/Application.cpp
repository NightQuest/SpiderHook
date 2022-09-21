#include "preCompiled.h"

Application* Application::instance;
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

}

void Application::OnDetach()
{

}

void Application::OnTick()
{
}
