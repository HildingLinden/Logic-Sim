#include <chrono>
#include <string>
#include <vector>
#include <format>
#include <numeric>

#include "component.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

enum class State {PLACING_GATE, DRAGGING_CONNECTION, DRAGGING_GATE};
class CircuitGUI : public olc::PixelGameEngine {
	std::vector<std::shared_ptr<Component>> gates;
	int size = 50;

	std::weak_ptr<Component> clickedGate;
	std::vector<std::weak_ptr<Component>> selectedGates;
	int clickedX = 0;
	int clickedY = 0;
	State state = State::PLACING_GATE;
	GateType selectedType = GateType::AND;
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

	 double saveProject() {
		 auto start = std::chrono::high_resolution_clock::now();
		 std::ofstream saveFile("save.txt");

		// Save all gates
		for (auto &gate : gates) {
			saveFile << gate->id << "," << static_cast<int>(gate->getType()) << "," << gate->x << "," << gate->y << "\n";
		}

		saveFile << "-\n";

		// Save all connections
		for (auto &gate : gates) {
			for (int i = 0; i < gate->inputs.size(); i++) {
				if (auto input_ptr = gate->inputs[i].lock()) {
					saveFile << gate->id << "," << input_ptr->id << "," << i << "\n";
				}
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::milli>(end - start).count();
	 }
	void handleUserInput() {
		// Add gate to the pixel x,y when clicking on an empty spot
		if (GetMouse(0).bPressed) {
			if (checkCollision(clickedGate, GetMouseX(), GetMouseY(), size)) {
				if (GetKey(olc::SHIFT).bHeld) {
					state = State::DRAGGING_GATE;
					clickedX = GetMouseX();
					clickedY = GetMouseY();
				}
				else if (GetKey(olc::CTRL).bHeld) {
					auto ptr = clickedGate.lock();
					if (!ptr->selected) {
						selectedGates.push_back(clickedGate);
						ptr->selected = true;
					}
					else {
						for (auto it = selectedGates.begin(); it != selectedGates.end(); it++) {
							if ((*it).lock()->id == ptr->id) {
								selectedGates.erase(it);
								break;
							}
						}
						ptr->selected = false;
					}

				}
				else {
					state = State::DRAGGING_CONNECTION;
				}
			} else {
				int x = GetMouseX() - size / 2;
				int y = GetMouseY() - size / 2;

				switch (selectedType) {
				case GateType::OUTPUT:
					gates.push_back(std::make_shared<Output>("Output test", x, y));
					//std::cout << "Add Output at " << x << " " << y << ", with id " << gates.back()->id << std::endl;
					break;
				case GateType::AND:
					gates.push_back(std::make_shared<AND>("AND test", x, y));
					//std::cout << "Add AND at " << x << " " << y << ", with id " << gates.back()->id << std::endl;
					break;
				case GateType::OR:
					gates.push_back(std::make_shared<OR>("OR test", x, y));
					//std::cout << "Add OR at " << x << " " << y << ", with id " << gates.back()->id << std::endl;
					break;
				case GateType::XOR:
					gates.push_back(std::make_shared<XOR>("XOR test", x, y));
					//std::cout << "Add XOR at " << x << " " << y << ", with id " << gates.back()->id << std::endl;
					break;
				case GateType::NOT:
					gates.push_back(std::make_shared<NOT>("NOT test", x, y));
					//std::cout << "Add NOT at " << x << " " << y << ", with id " << gates.back()->id << std::endl;
					break;
				case GateType::INPUT:
					gates.push_back(std::make_shared<Input>("Input test", x, y));
					//std::cout << "Add Input at " << x << " " << y << ", with id " << gates.back()->id << std::endl;
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
						//std::cout << "Connected " << clickedPtr->name << " to input " << inputIndex - 1 << " of " << ptr->name << std::endl;
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

		if (GetKey(olc::S).bPressed && GetKey(olc::CTRL).bHeld) {
			double time = saveProject();
			std::cout << "Saved project in " << time << "ms\n";
		}

		// Change the chosen gate or chosen input when a number is pressed
		if (state == State::PLACING_GATE) {
			if (GetKey(olc::K1).bPressed) selectedType = GateType::OUTPUT;
			if (GetKey(olc::K2).bPressed) selectedType = GateType::INPUT;
			if (GetKey(olc::K3).bPressed) selectedType = GateType::AND;
			if (GetKey(olc::K4).bPressed) selectedType = GateType::XOR;
			if (GetKey(olc::K5).bPressed) selectedType = GateType::OR;
			if (GetKey(olc::K6).bPressed) selectedType = GateType::NOT;
		}
		else if (state == State::DRAGGING_CONNECTION) {
			if (GetKey(olc::K1).bPressed) inputIndex = 1;
			if (GetKey(olc::K2).bPressed) inputIndex = 2;
		}

		if (state == State::DRAGGING_GATE) {
			int deltaX = GetMouseX() - clickedX;
			int deltaY = GetMouseY() - clickedY;

			if (clickedGate.lock()->selected) {
				for (auto &gate : selectedGates) {
					auto ptr = gate.lock();
					ptr->x += deltaX;
					ptr->y += deltaY;
				}
			}
			else {
				auto ptr = clickedGate.lock();
				ptr->x += deltaX;
				ptr->y += deltaY;
			}

			clickedX = GetMouseX();
			clickedY = GetMouseY();
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
		return std::chrono::duration<double, std::milli>(end - start).count();
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
			if (c->selected) {
				DrawRect(c->x, c->y, size, size, olc::BLACK);
				DrawRect(c->x+1, c->y+1, size-2, size-2, olc::BLACK);
			}

			const std::string displayName = c->name.substr(0, c->name.find(" "));
			int xOffset = (displayName.size() / 2.0) * 8;
			DrawString(c->x + size / 2 - xOffset, c->y + size / 2 - 4, displayName, olc::BLACK);
		}

		// Draw current action string
		std::string stateString;
		switch (state) {
			case State::PLACING_GATE: {
				stateString = "Placing ";
				switch (selectedType) {
				case GateType::OUTPUT:
					stateString += "Output";
					break;
				case GateType::AND:
					stateString += "AND";
					break;
				case GateType::OR:
					stateString += "OR";
					break;
				case GateType::XOR:
					stateString += "XOR";
					break;
				case GateType::NOT:
					stateString += "NOT";
					break;
				case GateType::INPUT:
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
		return std::chrono::duration<double, std::milli>(end - start).count();
	}

	double loadProject() {
		auto start = std::chrono::high_resolution_clock::now();

		std::ifstream saveFile("save.txt");
		std::array<char, 51> buffer;
		std::fill(buffer.begin(), buffer.end(), 0);

		bool gatesDone = false;
		std::string line;
		while (std::getline(saveFile, line)) {
			if (!gatesDone) {
				if (line.front() == '-') {
					gatesDone = true;
					if (!gates.empty()) {
						Component::GUID = gates.back()->id + 1;
					}
				}
				else {
					std::stringstream ss(line);
					std::string token;
					std::vector<std::string> result;
					while (std::getline(ss, token, ',')) {
						result.push_back(token);
					}

					if (result.size() > 4) {
						std::cout << "Wrong structure in save file\n";
					}
					else {
						uint64_t id = std::stoull(result[0]);
						GateType type = static_cast<GateType>(std::stoi(result[1]));
						int x = std::stoi(result[2]);
						int y = std::stoi(result[3]);

						switch (type) {
						case GateType::OUTPUT:
							gates.push_back(std::make_shared<Output>("Output test", x, y, id));
							break;
						case GateType::AND:
							gates.push_back(std::make_shared<AND>("AND test", x, y, id));
							break;
						case GateType::OR:
							gates.push_back(std::make_shared<OR>("OR test", x, y, id));
							break;
						case GateType::XOR:
							gates.push_back(std::make_shared<XOR>("XOR test", x, y, id));
							break;
						case GateType::NOT:
							gates.push_back(std::make_shared<NOT>("NOT test", x, y, id));
							break;
						case GateType::INPUT:
							gates.push_back(std::make_shared<Input>("Input test", x, y, id));
							break;
						}
					}
				}
			}
			else {
				std::stringstream ss(line);
				std::string token;
				std::vector<std::string> result;
				while (std::getline(ss, token, ',')) {
					result.push_back(token);
				}

				if (result.size() > 3) {
					std::cout << "Wrong structure in save file\n";
				}
				else {
					uint64_t dstId = std::stoull(result[0]);
					uint64_t srcId = std::stoull(result[1]);
					int inputIndex = std::stoi(result[2]);

					// TODO: Save srcGate since multiple inputs to the same 
					// TODO: Search for the gates with binary search
					std::shared_ptr<Component> srcGate;
					std::shared_ptr<Component> dstGate;
					bool srcFound = false, dstFound = false;
					for (auto it = gates.begin(); it != gates.end() && !(srcFound && dstFound); it++) {
						if (!srcFound) {
							if ((*it)->id == srcId) {
								srcGate = *it;
								srcFound = true;
							}
						}
						if (!dstFound) {
							if ((*it)->id == dstId) {
								dstGate = *it;
								dstFound = true;
							}
						}
					}

					if (!srcFound || !dstFound) {
						std::cout << "Gate not found when loading connections\n";
					}
					else {
						dstGate->connectInput(srcGate, inputIndex);
					}
				}
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::milli>(end - start).count();
	}

public:
	CircuitGUI() {
		sAppName = "Circuit Sim";
	}

	bool OnUserCreate() override {
		double time = loadProject();
		std::cout << "Loaded project in " << time << "ms\n";
		return true;
	}

	bool OnUserDestroy() override {
		double time = saveProject();
		std::cout << "Saved project in " << time << "ms\n";
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
// TODO: Sort by location to make collision detection faster
// TODO: Use coordinates as id?
int main() {
	CircuitGUI gui;
	if (gui.Construct(1300, 1300, 1, 1)) {
		gui.Start();
	}
	return 0;
}