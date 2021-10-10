#include <chrono>
#include <string>
#include <vector>

#include "component.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class CircuitGUI : public olc::PixelGameEngine {
	std::vector<std::shared_ptr<Component>> gates;
	std::weak_ptr<Component> connectionSrc;
	float totalTime = 0;
	int size = 50;

	bool isInGateMode = true;
	int gateIndex = 0;
	int inputIndex = 1;

public:
	CircuitGUI() {
		sAppName = "Circuit Sim";
	}

	bool OnUserCreate() override {
		return true;
	}

	std::weak_ptr<Component> checkCollision(int32_t mouseX, int32_t mouseY, int32_t size) {
		for (std::shared_ptr<Component> &gate : gates) {
			if (mouseX > gate->x && mouseX < gate->x + size && mouseY > gate->y && mouseY < gate->y + size) {
				return gate;
			}
		}

		return std::weak_ptr<Component>();
	}

	bool OnUserUpdate(float fElapsedTime) override {
		totalTime += fElapsedTime;

		// Add gate to the pixel x,y when clicked on empty or save reference to gate if clicked on gate
		if (GetMouse(0).bPressed) {
			auto gate = checkCollision(GetMouseX(), GetMouseY(), size);
			if (!gate.expired()) {
				connectionSrc = gate;
				isInGateMode = false;
			}
			else {
				connectionSrc.reset();

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
			if (auto ptr = connectionSrc.lock()) {
				// See if released on a gate
				if (auto gate = checkCollision(GetMouseX(), GetMouseY(), size).lock()) {
					if (gate != ptr) {
						std::cout << "Connected " << ptr->name << " to input " << inputIndex - 1 << " of " << gate->name << std::endl;
						gate->connectInput(ptr, inputIndex - 1);
					}
				}
				else {
					connectionSrc.reset();
				}
			}

			isInGateMode = true;
		}

		if (GetMouse(1).bPressed) {
			if (auto gate = checkCollision(GetMouseX(), GetMouseY(), size).lock()) {
				gate->output = !gate->output;
			}
		}

		// Remove gate when clicked on with middle mouse button
		if (GetMouse(2).bPressed) {
			connectionSrc.reset();
			if (auto gate = checkCollision(GetMouseX(), GetMouseY(), size).lock()) {
				gates.erase(std::find(gates.begin(), gates.end(), gate));
			}
		}

		// Change the chosen gate or chosen input when a number is pressed
		if (isInGateMode) {
			if (GetKey(olc::K1).bPressed) gateIndex = 0;
			if (GetKey(olc::K2).bPressed) gateIndex = 1;
			if (GetKey(olc::K3).bPressed) gateIndex = 2;
			if (GetKey(olc::K4).bPressed) gateIndex = 3;
			if (GetKey(olc::K5).bPressed) gateIndex = 4;
			if (GetKey(olc::K6).bPressed) gateIndex = 5;
		}
		else {
			if (GetKey(olc::K1).bPressed) inputIndex = 1;
			if (GetKey(olc::K2).bPressed) inputIndex = 2;
		}


		for (auto &gate : gates) {
			gate->update();
		}

		for (auto &gate : gates) {
			gate->output = gate->newOutput;
		}

		Clear(olc::WHITE);

		// Draw all connections first
		for (auto &gate : gates) {
			for (auto &input : gate->inputs) {
				if (auto input_ptr = input.lock()) {
					olc::Pixel color = input_ptr->output ? olc::RED : olc::BLACK;
					DrawLine(gate->x + size / 2, gate->y + size / 2, input_ptr->x + size / 2, input_ptr ->y + size / 2, color);
				}
			}
		}

		// Then draw all gates with the first part of their name on them
		for (auto &c : gates) {
			olc::Pixel color = c->output ? olc::RED : olc::GREY;
			FillRect(c->x, c->y, size, size, color);

			const std::string displayName = c->name.substr(0, c->name.find(" "));
			int xOffset = (displayName.size()/2.0)*8;
			DrawString(c->x + size / 2 - xOffset, c->y + size / 2 - 4, displayName, olc::BLACK);
		}

		// Draw current action string
		std::string gateString = "Placing ";
		if (isInGateMode) {
			switch (gateIndex) {
			case 0:
				gateString += "Output";
				break;
			case 1:
				gateString += "AND";
				break;
			case 2:
				gateString += "OR";
				break;
			case 3:
				gateString += "XOR";
				break;
			case 4:
				gateString += "NOT";
				break;
			case 5:
				gateString += "Input";
				break;
			}
		}
		else if (auto ptr = connectionSrc.lock()){
			DrawLine(ptr->x + size / 2, ptr->y + size / 2, GetMouseX(), GetMouseY(), olc::BLACK);
			gateString += "connection between " + ptr->name + " and input " + std::to_string(inputIndex);
		}
		DrawString(5, 5, gateString, olc::BLACK, 2);

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