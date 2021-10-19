#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <string_view>

enum class GateType { AND, XOR, OR, WIRE, NOT, INPUT, TIMER };

class Component;

class Point {
public:
	int x;
	int y;
};

inline Point operator-(const Point &u, const Point &v) {
	return Point(u.x - v.x, u.y - v.y);
}

inline Point operator-(const Point &u, const int v) {
	return Point(u.x - v, u.y - v);
}

inline Point operator+(const Point &u, const Point &v) {
	return Point(u.x + v.x, u.y + v.y);
}

inline Point operator+(const Point &u, const int v) {
	return Point(u.x + v, u.y + v);
}

struct InputPath {
	std::weak_ptr<Component> src;
	std::vector<Point> points;
};

// Base class
class Component {
public:
	std::string name;
	Point position;
	bool output = false;
	bool newOutput = false;
	bool selected = false;
	std::vector<InputPath> inputs;
	const uint64_t id;
	static uint64_t GUID;

	Component(std::string name, Point point, int numInputs, uint64_t id);
	virtual void update() = 0;
	virtual GateType getType() = 0;
	void connectInput(std::shared_ptr<Component> &component, int index, std::vector<Point> connectionPoints);
};

// Derived classes
class AND : public Component {
	void update() override;
	GateType getType() override;
public:
	AND(std::string name_, Point point, uint64_t id = Component::GUID++);
};

class XOR : public Component {
	void update() override;
	GateType getType() override;
public:
	XOR(std::string name_, Point point, uint64_t id = Component::GUID++);
};

class OR : public Component {
	void update() override;
	GateType getType() override;
public:
	OR(std::string name_, Point point, uint64_t id = Component::GUID++);
};

class WIRE : public Component {
	void update() override;
	GateType getType() override;
public:
	WIRE(std::string name_, Point point, uint64_t id = Component::GUID++);
};

class NOT : public Component {
	void update() override;
	GateType getType() override;
public:
	NOT(std::string name_, Point point, uint64_t id = Component::GUID++);
};

class Input : public Component {
	void update() override;
	GateType getType() override;
public:
	Input(std::string name_, Point point, uint64_t id = Component::GUID++);
};

class TIMER : public Component {
	int counter = 0;
	void update() override;
	GateType getType() override;
public:
	TIMER(std::string name_, Point point, uint64_t id = Component::GUID++);
};