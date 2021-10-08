#include "component.h"

Component::Component(std::string name, int x, int y, int numInputs) : name(name), x(x), y(y) {
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
AND::AND(std::string name, int32_t x, int32_t y) : Component(name, x, y, 2) {}
XOR::XOR(std::string name, int32_t x, int32_t y) : Component(name, x, y, 2) {}
OR::OR(std::string name, int32_t x, int32_t y) : Component(name, x, y, 2) {}
Output::Output(std::string name, int32_t x, int32_t y) : Component(name, x, y, 1) {}
NOT::NOT(std::string name, int32_t x, int32_t y) : Component(name, x, y, 1) { output = true; newOutput = true; }
Input::Input(std::string name, int32_t x, int32_t y) : Component(name, x, y, 0) { output = true; newOutput = true;  }

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
