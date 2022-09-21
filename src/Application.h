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

private:
	static Application* instance;
	Engine* eng = nullptr;
	HWND windowHandle;
	std::mutex handle_mutex;
};
