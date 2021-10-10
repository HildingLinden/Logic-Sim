#include <chrono>
#include <string>
#include <vector>
#include <format>

#include "component.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

enum class State {PLACING_GATE, DRAGGING_CONNECTION, DRAGGING_GATE};
class CircuitGUI : public olc::PixelGameEngine {
	std::vector<std::shared_ptr<Component>> gates;
	int size = 50;

	std::weak_ptr<Component> clickedGate;
	State state = State::PLACING_GATE;
	int gateIndex = 0;
	int inputIndex = 1;

	 bool checkCollision(std::weak_ptr<Component> &outGate, int32_t mouseX, int32_t mouseY, int32_t size) {
		for (std::shared_ptr<Component> &gate : gates) {
			if (mouseX > gate->x && mouseX < gate->x + size && mouseY > gate->y && mouseY < gate->y + size) {
				outGate = gate;
				return true;
			}
		}

		return false;
	}

	void handleUserInput() {
		// Add gate to the pixel x,y when clicking on an empty spot
		if (GetMouse(0).bPressed) {
			if (checkCollision(clickedGate, GetMouseX(), GetMouseY(), size)) {
				if (GetKey(olc::SHIFT).bHeld) {
					state = State::DRAGGING_GATE;
				}
				else {
					state = State::DRAGGING_CONNECTION;
				}
			} else {
				int x = GetMouseX() - size / 2;
				int y = GetMouseY() - size / 2;

				switch (gateIndex) {
				case 0:
					gates.push_back(std::make_shared<Output>("Output test", x, y));
					std::cout << "Add Output at " << x << " " << y << std::endl;
					break;
				case 1:
					gates.push_back(std::make_shared<AND>("AND test", x, y));
					std::cout << "Add AND at " << x << " " << y << std::endl;
					break;
				case 2:
					gates.push_back(std::make_shared<OR>("OR test", x, y));
					std::cout << "Add OR at " << x << " " << y << std::endl;
					break;
				case 3:
					gates.push_back(std::make_shared<XOR>("XOR test", x, y));
					std::cout << "Add XOR at " << x << " " << y << std::endl;
					break;
				case 4:
					gates.push_back(std::make_shared<NOT>("NOT test", x, y));
					std::cout << "Add NOT at " << x << " " << y << std::endl;
					break;
				case 5:
					gates.push_back(std::make_shared<Input>("Input test", x, y));
					std::cout << "Add Input at " << x << " " << y << std::endl;
					break;
				}
			}
		}

		// Connect gate where mouse is
		if (GetMouse(0).bReleased) {
			if (state == State::DRAGGING_CONNECTION) {
				// See if released on a gate
				std::weak_ptr<Component> gate;
				if (checkCollision(gate, GetMouseX(), GetMouseY(), size)) {
					auto ptr = gate.lock();
					auto clickedPtr = clickedGate.lock();
					if (clickedPtr != ptr) {
						std::cout << "Connected " << clickedPtr->name << " to input " << inputIndex - 1 << " of " << ptr->name << std::endl;
						ptr->connectInput(clickedPtr, inputIndex - 1);
					}
				}
			}

			state = State::PLACING_GATE;
		}

		if (GetMouse(1).bPressed && state == State::PLACING_GATE) {
			std::weak_ptr<Component> gate;
			if (checkCollision(gate, GetMouseX(), GetMouseY(), size)) {
				auto ptr = gate.lock();
				ptr->output = !ptr->output;
			}
		}

		// Remove gate when clicked on with middle mouse button
		if (GetMouse(2).bPressed && state == State::PLACING_GATE) {
			std::weak_ptr<Component> gate;
			if (checkCollision(gate, GetMouseX(), GetMouseY(), size)) {
				gates.erase(std::find(gates.begin(), gates.end(), gate.lock()));
			}
		}

		// Change the chosen gate or chosen input when a number is pressed
		if (state == State::PLACING_GATE) {
			if (GetKey(olc::K1).bPressed) gateIndex = 0;
			if (GetKey(olc::K2).bPressed) gateIndex = 1;
			if (GetKey(olc::K3).bPressed) gateIndex = 2;
			if (GetKey(olc::K4).bPressed) gateIndex = 3;
			if (GetKey(olc::K5).bPressed) gateIndex = 4;
			if (GetKey(olc::K6).bPressed) gateIndex = 5;
		}
		else if (state == State::DRAGGING_CONNECTION) {
			if (GetKey(olc::K1).bPressed) inputIndex = 1;
			if (GetKey(olc::K2).bPressed) inputIndex = 2;
		}

		if (state == State::DRAGGING_GATE) {
			auto ptr = clickedGate.lock();
			ptr->x = GetMouseX() - size / 2;
			ptr->y = GetMouseY() - size / 2;
		}
	}

	double simulate() {
		auto start = std::chrono::high_resolution_clock::now();

		for (auto &gate : gates) {
			gate->update();
		}

		for (auto &gate : gates) {
			gate->output = gate->newOutput;
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<double, std::milli>(end - start).count();

		return time;
	}

	double draw() {
		auto start = std::chrono::high_resolution_clock::now();

		Clear(olc::WHITE);

		// Draw all connections first
		for (auto &gate : gates) {
			for (auto &input : gate->inputs) {
				if (auto input_ptr = input.lock()) {
					olc::Pixel color = input_ptr->output ? olc::RED : olc::BLACK;
					DrawLine(gate->x + size / 2, gate->y + size / 2, input_ptr->x + size / 2, input_ptr->y + size / 2, color);
				}
			}
		}

		// Then draw all gates with the first part of their name on them
		for (auto &c : gates) {
			olc::Pixel color = c->output ? olc::RED : olc::GREY;
			FillRect(c->x, c->y, size, size, color);

			const std::string displayName = c->name.substr(0, c->name.find(" "));
			int xOffset = (displayName.size() / 2.0) * 8;
			DrawString(c->x + size / 2 - xOffset, c->y + size / 2 - 4, displayName, olc::BLACK);
		}

		// Draw current action string
		std::string stateString;
		switch (state) {
			case State::PLACING_GATE: {
				stateString = "Placing ";
				switch (gateIndex) {
				case 0:
					stateString += "Output";
					break;
				case 1:
					stateString += "AND";
					break;
				case 2:
					stateString += "OR";
					break;
				case 3:
					stateString += "XOR";
					break;
				case 4:
					stateString += "NOT";
					break;
				case 5:
					stateString += "Input";
					break;
				}
				break;
			}
			case State::DRAGGING_CONNECTION: {
				auto ptr = clickedGate.lock();
				DrawLine(ptr->x + size / 2, ptr->y + size / 2, GetMouseX(), GetMouseY(), olc::BLACK);
				stateString = "Connecting " + ptr->name + " and input " + std::to_string(inputIndex);
				break;
			}
			case State::DRAGGING_GATE: {
				auto ptr = clickedGate.lock();
				stateString = "Moving " + ptr->name;
				break;
			}
			default: {
				stateString = "";
				break;
			}
		}
		DrawString(5, 5, stateString, olc::BLACK, 2);

		auto end = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<double, std::milli>(end - start).count();

		return time;
	}

public:
	CircuitGUI() {
		sAppName = "Circuit Sim";
	}

	bool OnUserCreate() override {
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override {
		// User input
		handleUserInput();

		// Simulation update
		double simulationTime = simulate();

		// Drawing
		double drawTime = draw();

		sAppName = "Simulation: " + std::to_string(simulationTime) + "ms, Drawing : " + std::to_string(drawTime) + "ms\n";

		return true;
	}
};

// TODO: Create copy and move assignment and constructor for component that updates the parent ptr to avoid having to store all components as ptrs
int main() {
	CircuitGUI gui;
	if (gui.Construct(1300, 1300, 1, 1)) {
		gui.Start();
	}
	return 0;
}