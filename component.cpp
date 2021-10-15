#include "component.h"

uint64_t Component::GUID = 1;

Component::Component(std::string name, int x, int y, int numInputs, uint64_t id) : name(std::move(name)), x(x), y(y), id(id) {
	inputs.resize(numInputs);
}

void Component::connectInput(std::shared_ptr<Component> &component, int index, std::vector<Point> connectionPoints) {
	if (index+1 > inputs.size()) {
		std::cout << name << " only has " << inputs.size() << " number of inputs" << std::endl;
	}
	else {
		inputs[index].src = component;
		inputs[index].points = connectionPoints;
	}
}

// Derived class constructors and methods
AND::AND(std::string name, int32_t x, int32_t y, uint64_t id) : Component(std::move(name), x, y, 2, id) {}
XOR::XOR(std::string name, int32_t x, int32_t y, uint64_t id) : Component(std::move(name), x, y, 2, id) {}
OR::OR(std::string name, int32_t x, int32_t y, uint64_t id) : Component(std::move(name), x, y, 2, id) {}
WIRE::WIRE(std::string name, int32_t x, int32_t y, uint64_t id) : Component(std::move(name), x, y, 1, id) {}
NOT::NOT(std::string name, int32_t x, int32_t y, uint64_t id) : Component(std::move(name), x, y, 1, id) { output = true; newOutput = true; }
Input::Input(std::string name, int32_t x, int32_t y, uint64_t id) : Component(std::move(name), x, y, 0, id) { output = true; newOutput = true; }
TIMER::TIMER(std::string name, int32_t x, int32_t y, uint64_t id) : Component(std::move(name), x, y, 0, id) {}

GateType AND::getType() { return GateType::AND; }
GateType XOR::getType() { return GateType::XOR; }
GateType OR::getType() { return GateType::OR; }
GateType WIRE::getType() { return GateType::WIRE; }
GateType NOT::getType() { return GateType::NOT; }
GateType Input::getType() { return GateType::INPUT; }
GateType TIMER::getType() { return GateType::TIMER; }

void AND::update() {
	bool input1 = false;
	bool input2 = false;

	if (auto ptr = inputs[0].src.lock()) {
		input1 = ptr->output;
	}
	if (auto ptr = inputs[1].src.lock()) {
		input2 = ptr->output;
	}

	newOutput = input1 && input2;
}

void XOR::update() {
	bool input1 = false;
	bool input2 = false;

	if (auto ptr = inputs[0].src.lock()) {
		input1 = ptr->output;
	}
	if (auto ptr = inputs[1].src.lock()) {
		input2 = ptr->output;
	}

	newOutput = input1 != input2;
}

void OR::update() {
	bool input1 = false;
	bool input2 = false;

	if (auto ptr = inputs[0].src.lock()) {
		input1 = ptr->output;
	}
	if (auto ptr = inputs[1].src.lock()) {
		input2 = ptr->output;
	}

	newOutput = input1 | input2;
}

void WIRE::update() {
	bool input1 = false;

	if (auto ptr = inputs[0].src.lock()) {
		input1 = ptr->output;
	}

	newOutput = input1;
}

void NOT::update() {
	bool input1 = false;

	if (auto ptr = inputs[0].src.lock()) {
		input1 = ptr->output;
	}

	newOutput = !input1;
}

void Input::update() {
	newOutput = output;
}

void TIMER::update() {
	if (counter < 30) {
		newOutput = false;
		counter++;
	}
	else if (counter < 60) {
		newOutput = true;
		counter++;
	}
	else {
		counter = 0;
	}
}