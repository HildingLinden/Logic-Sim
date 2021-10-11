#include "component.h"

uint64_t Component::GUID = 0;

Component::Component(std::string name, int x, int y, int numInputs, uint64_t id) : name(name), x(x), y(y), id(id) {
	inputs.resize(numInputs);
}

void Component::connectInput(std::shared_ptr<Component> &component, int index) {
	if (index+1 > inputs.size()) {
		std::cout << name << " only has " << inputs.size() << " number of inputs" << std::endl;
	}
	else {
		inputs[index] = component;
	}
}

// Derived class constructors and methods
AND::AND(std::string name, int32_t x, int32_t y, uint64_t id) : Component(name, x, y, 2, id) {}
XOR::XOR(std::string name, int32_t x, int32_t y, uint64_t id) : Component(name, x, y, 2, id) {}
OR::OR(std::string name, int32_t x, int32_t y, uint64_t id) : Component(name, x, y, 2, id) {}
Output::Output(std::string name, int32_t x, int32_t y, uint64_t id) : Component(name, x, y, 1, id) {}
NOT::NOT(std::string name, int32_t x, int32_t y, uint64_t id) : Component(name, x, y, 1, id) { output = true; newOutput = true; }
Input::Input(std::string name, int32_t x, int32_t y, uint64_t id) : Component(name, x, y, 0, id) { output = true; newOutput = true; }

GateType AND::getType() { return GateType::AND; }
GateType XOR::getType() { return GateType::XOR; }
GateType OR::getType() { return GateType::OR; }
GateType Output::getType() { return GateType::OUTPUT; }
GateType NOT::getType() { return GateType::NOT; }
GateType Input::getType() { return GateType::INPUT; }

void AND::update() {
	bool input1 = false;
	bool input2 = false;

	if (auto ptr = inputs[0].lock()) {
		input1 = ptr->output;
	}
	if (auto ptr = inputs[1].lock()) {
		input2 = ptr->output;
	}

	newOutput = input1 && input2;
}

void XOR::update() {
	bool input1 = false;
	bool input2 = false;

	if (auto ptr = inputs[0].lock()) {
		input1 = ptr->output;
	}
	if (auto ptr = inputs[1].lock()) {
		input2 = ptr->output;
	}

	newOutput = input1 != input2;
}

void OR::update() {
	bool input1 = false;
	bool input2 = false;

	if (auto ptr = inputs[0].lock()) {
		input1 = ptr->output;
	}
	if (auto ptr = inputs[1].lock()) {
		input2 = ptr->output;
	}

	newOutput = input1 | input2;
}

void Output::update() {
	bool input1 = false;

	if (auto ptr = inputs[0].lock()) {
		input1 = ptr->output;
	}

	newOutput = input1;
}

void NOT::update() {
	bool input1 = false;

	if (auto ptr = inputs[0].lock()) {
		input1 = ptr->output;
	}

	newOutput = !input1;
}

void Input::update() {
	newOutput = output;
}
