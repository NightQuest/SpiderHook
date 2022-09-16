#pragma once

class Application
{
public:
	Application();
	~Application();

	static Application* getInstance();

	void OnAttach();
	void OnDetach();
	void OnTick();

	static constexpr uint32_t ID_SUBCLASS = 907;
	static LRESULT staticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR ulSubclass, DWORD_PTR dwRefData);

private:
	static Application* instance;
	Engine* eng = nullptr;
	HWND windowHandle;
	std::mutex handle_mutex;
};
