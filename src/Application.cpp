#include "preCompiled.h"

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
	std::thread tmp(
		[&]() {
			do
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				HWND hwnd = nullptr;
				hwnd = ::FindWindow("GameNxApp", nullptr);
				if( !hwnd )
					continue;

				char* ctmp = new char[MAX_PATH];
				::GetWindowText(hwnd, ctmp, MAX_PATH);
				std::string tmp(ctmp);
				delete[] ctmp;

				if( !std::regex_match(tmp, std::regex(R"(^Marvel's Spider-Man Remastered v\d+\.\d+\.\d+\.\d+$)")) )
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
						::DwmSetWindowAttribute(windowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(BOOL));
					}
					::RegCloseKey(hKey);
				}

				::SetWindowSubclass(this->windowHandle, staticWindowProc, ID_SUBCLASS, 0);

			} while( !this->windowHandle );
		});

	tmp.detach();

}

void Application::OnDetach()
{

}

void Application::OnTick()
{
}

LRESULT Application::staticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR ulSubclass, DWORD_PTR dwRefData)
{
	Application* app = getInstance();
	app->OnTick();

	return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}
