#include <chrono>
#include <string>
#include <vector>
#include <format>
#include <numeric>

#include "component.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

enum class State {PLACING_GATE, DRAGGING_CONNECTION, DRAGGING_GATE, SELECTING_AREA};

struct CopiedGate {
	std::shared_ptr<Component> gate;
	std::vector<std::vector<int>> inputIndices;
};

class CircuitGUI : public olc::PixelGameEngine {
	std::vector<std::shared_ptr<Component>> gates;
	int size = 50;

	std::weak_ptr<Component> clickedGate;
	std::vector<std::weak_ptr<Component>> selectedGates;
	// TODO: copied gates should be unique ptr
	std::vector<CopiedGate> copiedGates;
	int clickedX = 0, copiedX = 0;
	int clickedY = 0, copiedY = 0;
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

	 std::shared_ptr<Component> createComponent(GateType type, std::string name, int x, int y) {
		 switch (type) {
		 case GateType::OUTPUT:
			 return std::make_shared<Output>("Output " + name, x, y);
			 break;
		 case GateType::AND:
			 return std::make_shared<AND>("AND " + name, x, y);
			 break;
		 case GateType::OR:
			 return std::make_shared<OR>("OR " + name, x, y);
			 break;
		 case GateType::XOR:
			 return std::make_shared<XOR>("XOR " + name, x, y);
			 break;
		 case GateType::NOT:
			 return std::make_shared<NOT>("NOT " + name, x, y);
			 break;
		 case GateType::INPUT:
			 return std::make_shared<Input>("Input " + name, x, y);
			 break;
		 }
	 }
	 std::shared_ptr<Component> createComponent(GateType type, std::string name, int x, int y, uint64_t id) {
		 switch (type) {
		 case GateType::OUTPUT:
			 return std::make_shared<Output>("Output " + name, x, y, id);
			 break;
		 case GateType::AND:
			 return std::make_shared<AND>("AND " + name, x, y, id);
			 break;
		 case GateType::OR:
			 return std::make_shared<OR>("OR " + name, x, y, id);
			 break;
		 case GateType::XOR:
			 return std::make_shared<XOR>("XOR " + name, x, y, id);
			 break;
		 case GateType::NOT:
			 return std::make_shared<NOT>("NOT " + name, x, y, id);
			 break;
		 case GateType::INPUT:
			 return std::make_shared<Input>("Input " + name, x, y, id);
			 break;
		 }
	 }

	 double saveProject() {
		 auto start = std::chrono::high_resolution_clock::now();
		 std::ofstream saveFile("save.txt");

		// Save all gates
		for (auto &gate : gates) {
			saveFile << gate->id << "," << static_cast<int>(gate->getType()) << "," << gate->output  << "," << gate->x << "," << gate->y << "\n";
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
			clickedX = GetMouseX();
			clickedY = GetMouseY();

			if (checkCollision(clickedGate, GetMouseX(), GetMouseY(), size)) {
				if (GetKey(olc::SHIFT).bHeld) {
					state = State::DRAGGING_GATE;
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
			}
		}

		if (GetMouse(0).bHeld) {
			if (state == State::PLACING_GATE) {
				int x = GetMouseX();
				int y = GetMouseY();
				int deltaX = x - clickedX;
				int deltaY = y - clickedY;

				if (std::abs(deltaX) > 5 || std::abs(deltaY) > 5) {
					state = State::SELECTING_AREA;
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
			else if (state == State::PLACING_GATE && !GetKey(olc::CTRL).bHeld) {
				gates.push_back(createComponent(selectedType, "Test", GetMouseX() - size / 2, GetMouseY() - size / 2));
			}
			else if (state == State::SELECTING_AREA) {
				if (!GetKey(olc::CTRL).bHeld) {
					for (auto &gate : selectedGates) {
						if (!gate.expired()) {
							gate.lock()->selected = false;
						}
					}
					selectedGates.clear();
				}
				int x = GetMouseX();
				int y = GetMouseY();
				int deltaX = x - clickedX;
				int deltaY = y - clickedY;
				
				for (auto &gate : gates) {
					if (deltaX < 0) {
						if (gate->x > clickedX || gate->x < x) continue;
					}
					else {
						if (gate->x < clickedX || gate->x > x) continue;
					}

					if (deltaY < 0) {
						if (gate->y > clickedY || gate->y < y) continue;
					}
					else {
						if (gate->y < clickedY || gate->y > y) continue;
					}

					selectedGates.push_back(gate);
					gate->selected = true;
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
				// Remove it from gates and selected gates
				gates.erase(std::find(gates.begin(), gates.end(), gate.lock()));
				for (auto it = selectedGates.begin(); it != selectedGates.end(); it++) {
					if ((*it).expired()) {
						selectedGates.erase(it);
						break;
					}
				}
			}
		}

		if (GetKey(olc::A).bPressed && GetKey(olc::CTRL).bHeld) {
			for (auto &gate : gates) {
				selectedGates.push_back(gate);
				gate->selected = true;
			}
		}

		if (GetKey(olc::S).bPressed && GetKey(olc::CTRL).bHeld) {
			double time = saveProject();
			std::cout << "Saved project in " << time << "ms\n";
		}

		if (GetKey(olc::C).bPressed && GetKey(olc::CTRL).bHeld) {
			copiedX = GetMouseX();
			copiedY = GetMouseY();
			if (!selectedGates.empty()) copiedGates.clear();
			for (auto &selectedGate : selectedGates) {
				if (!selectedGate.expired()) {
					auto selectedPtr = selectedGate.lock();

					// Create a new instance of the component
					copiedGates.push_back({ createComponent(selectedPtr->getType(), "Tmp", selectedPtr->x, selectedPtr->y, 0) });
					copiedGates.back().gate->inputs = selectedPtr->inputs;
					copiedGates.back().gate->output = selectedPtr->output;
					copiedGates.back().inputIndices.resize(copiedGates.back().gate->inputs.size());
				}
			}

			// For each gate
			for (auto &copiedGate : copiedGates) {
				int inputIndex = 0;

				// For each input
				for (auto &inputGate : copiedGate.gate->inputs) {
					if (!inputGate.expired()) {
						auto inputPtr = inputGate.lock();

						// If the input is also selected
						if (inputPtr->selected) {
							int inputSrcIndex = 0;

							// Find the input in the selectedGates vector and store the index in the CopiedGate struct
							for (auto &src : selectedGates) {
								if (!src.expired()) {
									if (src.lock()->id == inputPtr->id) {
										copiedGate.inputIndices[inputIndex].push_back(inputSrcIndex);
										break;
									}
								}
								inputSrcIndex++;
							}
						}
					}
					inputIndex++;
				}
			}
		}

		if (GetKey(olc::V).bPressed && GetKey(olc::CTRL).bHeld) {
			int deltaX = GetMouseX() - copiedX;
			int deltaY = GetMouseY() - copiedY;

			// Unselect all selected gates
			for (auto &gate : selectedGates) {
				if (!gate.expired()) {
					gate.lock()->selected = false;
				}
			}
			selectedGates.clear();

			int prevGateCount = gates.size();
			// Create of a new instance of the component 
			for (auto &gate : copiedGates) {
				gates.push_back(createComponent(gate.gate->getType(), "Copied test", gate.gate->x + deltaX, gate.gate->y + deltaY));
				gates.back()->output = gate.gate->output;

				// Select each new instance of the gates
				gates.back()->selected = true;
				selectedGates.push_back(gates.back());
			}

			// Connect the copied components to the inputs that were also copied
			for (int j = 0; j < copiedGates.size(); j++) {
				for (int i = 0; i < copiedGates[j].inputIndices.size(); i++) {
					for (auto &inputSrcIndex : copiedGates[j].inputIndices[i]) {
						gates[prevGateCount + j]->connectInput(gates[prevGateCount + inputSrcIndex], i);
					}
				}
			}
		}

		if (GetKey(olc::DEL).bPressed) {
			for (auto it = gates.begin(); it != gates.end();) {
				if ((*it)->selected) {
					it = gates.erase(it);
				}
				else {
					it++;
				}
			}
			selectedGates.clear();
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
		else if (state == State::DRAGGING_GATE) {
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
			case State::SELECTING_AREA: {
				int x = GetMouseX();
				int y = GetMouseY();

				DrawLine(clickedX, clickedY, x, clickedY, olc::BLACK);
				DrawLine(clickedX, clickedY, clickedX, y, olc::BLACK);
				DrawLine(x, clickedY, x, y, olc::BLACK);
				DrawLine(clickedX, y, x, y, olc::BLACK);

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
		if (!saveFile.is_open()) return 0;

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

					if (result.size() > 5) {
						std::cout << "Wrong structure in save file\n";
					}
					else {
						uint64_t id = std::stoull(result[0]);
						GateType type = static_cast<GateType>(std::stoi(result[1]));
						bool output = std::stoi(result[2]);
						int x = std::stoi(result[3]);
						int y = std::stoi(result[4]);

						gates.push_back(createComponent(type, "Test", x, y, id));
						gates.back()->output = output;
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