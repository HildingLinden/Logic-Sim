#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <memory>

// Base class
class Component {
public:
	std::string name;
	int32_t x;
	int32_t y;
	bool output = false;
	bool newOutput = false;
	std::vector<std::weak_ptr<Component>> inputs;

	Component(std::string name, int x, int y, int numInputs);
	virtual void update() = 0;
	void connectInput(std::shared_ptr<Component> &component, int index);
};

// Derived classes
class AND : public Component {
	void update() override;
public:
	AND(std::string name_, int32_t x = 0, int32_t y = 0);
};

class XOR : public Component {
	void update() override;
public:
	XOR(std::string name_, int32_t x = 0, int32_t y = 0);
};

class OR : public Component {
	void update() override;
public:
	OR(std::string name_, int32_t x = 0, int32_t y = 0);
};

class Output : public Component {
	void update() override;
public:
	Output(std::string name_, int32_t x = 0, int32_t y = 0);
};

class NOT : public Component {
	void update() override;
public:
	NOT(std::string name_, int32_t x = 0, int32_t y = 0);
};

class Input : public Component {
	void update() override;
public:
	Input(std::string name_, int32_t x = 0, int32_t y = 0);
};