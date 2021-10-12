#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <string_view>

enum class GateType { AND, XOR, OR, OUTPUT, NOT, INPUT };
// Base class
class Component {
public:
	std::string name;
	int32_t x;
	int32_t y;
	bool output = false;
	bool newOutput = false;
	bool selected = false;
	std::vector<std::weak_ptr<Component>> inputs;
	const uint64_t id;
	static uint64_t GUID;

	Component(std::string name, int x, int y, int numInputs, uint64_t id);
	virtual void update() = 0;
	virtual GateType getType() = 0;
	void connectInput(std::shared_ptr<Component> &component, int index);
};

// Derived classes
class AND : public Component {
	void update() override;
	GateType getType() override;
public:
	AND(std::string name_, int32_t x = 0, int32_t y = 0, uint64_t id = Component::GUID++);
};

class XOR : public Component {
	void update() override;
	GateType getType() override;
public:
	XOR(std::string name_, int32_t x = 0, int32_t y = 0, uint64_t id = Component::GUID++);
};

class OR : public Component {
	void update() override;
	GateType getType() override;
public:
	OR(std::string name_, int32_t x = 0, int32_t y = 0, uint64_t id = Component::GUID++);
};

class Output : public Component {
	void update() override;
	GateType getType() override;
public:
	Output(std::string name_, int32_t x = 0, int32_t y = 0, uint64_t id = Component::GUID++);
};

class NOT : public Component {
	void update() override;
	GateType getType() override;
public:
	NOT(std::string name_, int32_t x = 0, int32_t y = 0, uint64_t id = Component::GUID++);
};

class Input : public Component {
	void update() override;
	GateType getType() override;
public:
	Input(std::string name_, int32_t x = 0, int32_t y = 0, uint64_t id = Component::GUID++);
};