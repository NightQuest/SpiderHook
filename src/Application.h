#pragma once

class Application
{
public:
	Application();
	~Application();

	void OnAttach();
	void OnDetach();

private:
	Engine* eng = nullptr;
};