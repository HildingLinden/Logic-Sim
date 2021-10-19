#include <chrono>
#include <string>
#include <vector>
#include <format>
#include <numeric>

#include "component.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

enum class State {PLACING_GATE, DRAGGING_CONNECTION, DRAGGING_GATES, SELECTING_AREA};
enum class SimulationState {PAUSED, STEP, RUNNING};

struct alignas(64) CopiedGate {
	std::shared_ptr<Component> gate;
	std::vector<int> inputIndices;
	std::vector<std::vector<Point>> inputPaths;
};

class CircuitGUI : public olc::PixelGameEngine {
	State state = State::PLACING_GATE;
	SimulationState simulationState = SimulationState::PAUSED;
	GateType selectedType = GateType::WIRE;
	int selectedInputIndex = 1;

	std::vector<std::shared_ptr<Component>> gates;
	std::vector<std::weak_ptr<Component>> selectedGates;
	std::vector<CopiedGate> copiedGates;
	std::weak_ptr<Component> clickedGate;
	std::weak_ptr<Component> connectionSrcGate;
	std::vector<Point> connectionPoints;

	Point clickedPoint{ 0,0 };
	Point copyPoint{ 0,0 };

	const int size = 60;
	const int simulationsPerFrame = 1;

	float fElapsedTime = 0;
	int worldOffsetX = 0;
	int worldOffsetY = 0;
	
	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();

	Point getScreenPoint(Point point) const {
		return Point{ static_cast<int>(point.x - worldOffsetX), static_cast<int>(point.y - worldOffsetY) };
	}

	/*
	 * Check the user input and perform the actions bound to the inputs
	*/
	void saveProject() {
		auto start = std::chrono::high_resolution_clock::now();
		std::ofstream saveFile("save.txt");

		// Save all gates
		for (auto &gate : gates) {
			saveFile << gate->id << "," << static_cast<int>(gate->getType()) << "," << gate->output << "," << gate->position.x << "," << gate->position.y << "\n";
		}

		saveFile << "-\n";

		// Save all connections
		for (auto &gate : gates) {
			for (int i = 0; i < gate->inputs.size(); i++) {
				if (auto input_ptr = gate->inputs[i].src.lock()) {
					saveFile << gate->id << "," << input_ptr->id << "," << i;
					for (auto &point : gate->inputs[i].points) {
						saveFile << "," << point.x << "," << point.y;
					}
					saveFile << "\n";
				}
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "Saved project in " << time << "ms\n";
	}
	bool loadGates(const std::string &line) {
		bool done = false;

		if (line.front() == '-') {
			if (!gates.empty()) {
				Component::GUID = gates.back()->id + 1;
			}
			done = true;
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
				auto type = static_cast<GateType>(std::stoi(result[1]));
				auto output = static_cast<bool>(std::stoi(result[2]));
				int x = std::stoi(result[3]);
				int y = std::stoi(result[4]);

				gates.push_back(createComponent(type, "Test", Point{x, y}, id));
				gates.back()->output = output;
			}
		}

		return done;
	}
	void loadConnections(const std::string &line) {
		std::stringstream ss(line);
		std::string token;
		std::vector<std::string> result;
		while (std::getline(ss, token, ',')) {
			result.push_back(token);
		}

		uint64_t dstId = std::stoull(result[0]);
		uint64_t srcId = std::stoull(result[1]);
		int inputIndex = std::stoi(result[2]);

		// TODO: Save srcGate since multiple inputs to the same 
		// TODO: Search for the gates with binary search
		std::shared_ptr<Component> srcGate;
		std::shared_ptr<Component> dstGate;
		bool srcFound = false;
		bool dstFound = false;
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
			std::vector<Point> connectionPoints;
			for (int i = 3; i < result.size(); i += 2) {
				connectionPoints.push_back({ std::stoi(result[i]), std::stoi(result[i + 1]) });
			}
			dstGate->connectInput(srcGate, inputIndex, connectionPoints);
		}
	}
	void loadProject() {
		auto start = std::chrono::high_resolution_clock::now();

		std::ifstream saveFile("save.txt");
		if (!saveFile.is_open()) { return; }

		bool gatesDone = false;
		std::string line;
		while (std::getline(saveFile, line)) {
			if (!gatesDone) {
				gatesDone = loadGates(line);
			}
			else {
				loadConnections(line);
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "Loaded project in " << time << "ms\n";
	}

	static std::shared_ptr<Component> createComponent(GateType type, const std::string &name, Point point) {
		 switch (type) {
		 case GateType::WIRE:
			 return std::make_shared<WIRE>("Wire " + name, point);
		 case GateType::AND:
			 return std::make_shared<AND>("AND " + name, point);
		 case GateType::OR:
			 return std::make_shared<OR>("OR " + name, point);
		 case GateType::XOR:
			 return std::make_shared<XOR>("XOR " + name, point);
		 case GateType::NOT:
			 return std::make_shared<NOT>("NOT " + name, point);
		 case GateType::INPUT:
			 return std::make_shared<Input>("Input " + name, point);
		 case GateType::TIMER:
			 return std::make_shared<TIMER>("Timer " + name, point);
		 }
	 }
	static std::shared_ptr<Component> createComponent(GateType type, const std::string &name, Point point, uint64_t id) {
		 switch (type) {
		 case GateType::WIRE:
			 return std::make_shared<WIRE>("Wire " + name, point, id);
		 case GateType::AND:
			 return std::make_shared<AND>("AND " + name, point, id);
		 case GateType::OR:
			 return std::make_shared<OR>("OR " + name, point, id);
		 case GateType::XOR:
			 return std::make_shared<XOR>("XOR " + name, point, id);
		 case GateType::NOT:
			 return std::make_shared<NOT>("NOT " + name, point, id);
		 case GateType::INPUT:
			 return std::make_shared<Input>("Input " + name, point, id);
		 case GateType::TIMER:
			 return std::make_shared<TIMER>("Timer " + name, point, id);
		 }
	 }
	bool checkCollision(std::weak_ptr<Component> &outGate, Point point, int32_t size) {
		for (std::shared_ptr<Component> &gate : gates) {
			if (point.x > gate->position.x && point.x < gate->position.x + size && point.y > gate->position.y && point.y < gate->position.y + size) {
				outGate = gate;
				return true;
			}
		}

		return false;
	}
	Point getWorldMousePos() {
		return Point{ GetMouseX() + worldOffsetX, GetMouseY() + worldOffsetY };
	}
	void copyComponents() {
		for (auto &selectedGate : selectedGates) {
			if (!selectedGate.expired()) {
				auto selectedPtr = selectedGate.lock();

				// Create a new instance of the component
				copiedGates.push_back({ createComponent(selectedPtr->getType(), "Tmp", selectedPtr->position, 0) });
				copiedGates.back().gate->inputs = selectedPtr->inputs;
				copiedGates.back().gate->output = selectedPtr->output;
				copiedGates.back().inputIndices.resize(copiedGates.back().gate->inputs.size());
				copiedGates.back().inputPaths.resize(copiedGates.back().gate->inputs.size());
			}
		}
	}
	void copyConnections() {
		// For each gate
		for (auto &copiedGate : copiedGates) {
			int inputIndex = 0;

			// For each input
			for (auto &inputGate : copiedGate.gate->inputs) {
				if (!inputGate.src.expired()) {
					auto inputPtr = inputGate.src.lock();

					// If the input is also selected
					if (inputPtr->selected) {
						int inputSrcIndex = 0;

						// Find the input in the selectedGates vector and store the index in the CopiedGate struct
						for (auto &src : selectedGates) {
							if (!src.expired()) {
								if (src.lock()->id == inputPtr->id) {
									copiedGate.inputIndices[inputIndex] = inputSrcIndex;
									break;
								}
							}
							inputSrcIndex++;
						}

						copiedGate.inputPaths[inputIndex] = inputGate.points;
					}
				}
				inputIndex++;
			}
		}
	}
	void copySelected() {
		copyPoint = getWorldMousePos();
		if (!selectedGates.empty()) { copiedGates.clear(); }
		
		copyComponents();
		copyConnections();
	}
	void pasteComponents() {
		Point delta = getWorldMousePos() - copyPoint;

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
			gates.push_back(createComponent(gate.gate->getType(), "Copied test", Point{ gate.gate->position.x + delta.x, gate.gate->position.y + delta.y }));
			gates.back()->output = gate.gate->output;

			// Select each new instance of the gates
			gates.back()->selected = true;
			selectedGates.push_back(gates.back());
		}

		// Connect the copied components to the inputs that were also copied
		for (int j = 0; j < copiedGates.size(); j++) {
			for (int i = 0; i < copiedGates[j].inputIndices.size(); i++) {
				auto newConnectionPath = copiedGates[j].inputPaths[i];
				for (auto &point : newConnectionPath) {
					point.x += delta.x;
					point.y += delta.y;
				}
				gates[prevGateCount + j]->connectInput(gates[prevGateCount + copiedGates[j].inputIndices[i]], i, std::move(newConnectionPath));

			}
		}
	}
	void removeComponent() {
		std::weak_ptr<Component> gate;
		if (checkCollision(gate, getWorldMousePos(), size)) {
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
	void startDraggingConnection() {
		connectionSrcGate = clickedGate;
		state = State::DRAGGING_CONNECTION;
	}
	void startDraggingGate() {
		state = State::DRAGGING_GATES;
	}
	void startSelectingArea() {
		state = State::SELECTING_AREA;
	}
	void stopDraggingConnection(std::weak_ptr<Component> &gate) {
		auto ptr = gate.lock();
		auto clickedPtr = connectionSrcGate.lock();

		if (clickedPtr != ptr) {
			//std::cout << "Connected " << clickedPtr->name << " to input " << selectedInputIndex - 1 << " of " << ptr->name << std::endl;
			ptr->connectInput(clickedPtr, selectedInputIndex - 1, connectionPoints);
		}
		connectionPoints.clear();
		state = State::PLACING_GATE;
	}
	void stopDraggingGate() {
		state = State::PLACING_GATE;
	}
	void selectGatesInArea() {
		Point worldMousePos = getWorldMousePos();
		Point delta = worldMousePos - clickedPoint;

		for (auto &gate : gates) {
			if (delta.x < 0) {
				if (gate->position.x > clickedPoint.x || gate->position.x + size < worldMousePos.x) continue;
			}
			else {
				if (gate->position.x + size < clickedPoint.x || gate->position.x > worldMousePos.x) continue;
			}

			if (delta.y < 0) {
				if (gate->position.y > clickedPoint.y || gate->position.y + size < worldMousePos.y) continue;
			}
			else {
				if (gate->position.y + size < clickedPoint.y || gate->position.y > worldMousePos.y) continue;
			}

			selectedGates.push_back(gate);
			gate->selected = true;
		}
	}
	void stopSelectingArea() {
		// Deselect previously selected gates if ctrl is held
		if (!GetKey(olc::CTRL).bHeld) {
			for (auto &gate : selectedGates) {
				if (!gate.expired()) {
					gate.lock()->selected = false;
				}
			}
			selectedGates.clear();
		}

		selectGatesInArea();

		state = State::PLACING_GATE;
	}
	void toggleGateSelection() {
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
	void addPointToConnectionPath() {
		connectionPoints.push_back(getConnectionLine());
	}
	void pan(Point delta) {
		worldOffsetX -= delta.x;
		worldOffsetY -= delta.y;
	}
	void placeGate() {
		gates.push_back(createComponent(selectedType, "Test", getWorldMousePos() - size / 2));
	}
	Point getConnectionLine() {
		Point src{0,0};
		if (connectionPoints.empty()) {
			auto ptr = clickedGate.lock();
			src = Point{ ptr->position + size / 2 };
		}
		else {
			src = connectionPoints.back();
		}

		Point worldMousePos = getWorldMousePos();
		int deltaX = std::abs(src.x - worldMousePos.x);
		int deltaY = std::abs(src.y - worldMousePos.y);

		if (deltaX > deltaY) {
			return Point{ worldMousePos.x, src.y};
		}
		else {
			return Point{ src.x, worldMousePos.y };
		}
	}
	void mouseClicked() {
		clickedPoint = getWorldMousePos();

		if (checkCollision(clickedGate, clickedPoint, size)) {
			if (GetKey(olc::SHIFT).bHeld) {
				startDraggingGate();
			}
			else if (GetKey(olc::CTRL).bHeld) {
				toggleGateSelection();
			}
			else {
				if (state != State::DRAGGING_CONNECTION) {
					startDraggingConnection();
				}
			}
		}
		else if (state == State::DRAGGING_CONNECTION) {
			addPointToConnectionPath();
		}
	}
	void moveComponent() {
		Point delta = getWorldMousePos() - clickedPoint;

		// Move all selected components and their connection paths if the component that is clicked is also selected
		if (clickedGate.lock()->selected) {
			for (auto &gate : selectedGates) {
				if (auto ptr = gate.lock()) {
					ptr->position = ptr->position + delta;

					// Move each point of the connection
					for (auto &input : ptr->inputs) {
						if (auto srcPtr = input.src.lock()) {
							if (srcPtr->selected) {
								for (auto &point : input.points) {
									point = point + delta;
								}
							}
						}
					}
				}
			}
		}
		// Else just move the clicked component
		else {
			auto ptr = clickedGate.lock();
			ptr->position = ptr->position + delta;
		}

		clickedPoint = getWorldMousePos();
	}
	void autoPan() {
		const int x = GetMouseX();
		const int y = GetMouseY();
		const int moveSpeed = 500;
		const int border = 30;

		if (x < border) {
			worldOffsetX -= moveSpeed * fElapsedTime;
		}
		else if (x > GetDrawTargetWidth() - border) {
			worldOffsetX += moveSpeed * fElapsedTime;
		}

		if (y < border) {
			worldOffsetY -= moveSpeed * fElapsedTime;
		}
		else if (y > GetDrawTargetHeight() - border) {
			worldOffsetY += moveSpeed * fElapsedTime;
		}
	}
	void mouseHeld() {
		if (state == State::PLACING_GATE) {
			Point delta = getWorldMousePos() - clickedPoint;

			if (GetKey(olc::SHIFT).bHeld) {
				pan(delta);
				clickedPoint = getWorldMousePos();
			}
			else if (std::abs(delta.x) > 5 || std::abs(delta.y) > 5) {
				startSelectingArea();
			}
		}
	}
	void mouseReleased() {
		if (state == State::DRAGGING_CONNECTION) {
			std::weak_ptr<Component> gate;
			if (checkCollision(gate, getWorldMousePos(), size)) {
				stopDraggingConnection(gate);
			}
		}
		else if (state == State::PLACING_GATE && !GetKey(olc::CTRL).bHeld && !GetKey(olc::SHIFT).bHeld) {
			placeGate();
		}
		else if (state == State::SELECTING_AREA) {
			stopSelectingArea();
		}
		else if (state == State::DRAGGING_GATES) {
			stopDraggingGate();
		}
	}
	void toggleComponent() {
		std::weak_ptr<Component> gate;
		if (checkCollision(gate, getWorldMousePos(), size)) {
			auto ptr = gate.lock();
			ptr->output = !ptr->output;
		}
	}
	void selectAllComponents() {
		for (auto &gate : gates) {
			if (!gate->selected) {
				selectedGates.push_back(gate);
				gate->selected = true;
			}
		}
	}
	void handleNumberInputs() {
		if (state == State::PLACING_GATE) {
			if (GetKey(olc::K1).bPressed) selectedType = GateType::WIRE;
			if (GetKey(olc::K2).bPressed) selectedType = GateType::INPUT;
			if (GetKey(olc::K3).bPressed) selectedType = GateType::AND;
			if (GetKey(olc::K4).bPressed) selectedType = GateType::XOR;
			if (GetKey(olc::K5).bPressed) selectedType = GateType::OR;
			if (GetKey(olc::K6).bPressed) selectedType = GateType::NOT;
			if (GetKey(olc::K7).bPressed) selectedType = GateType::TIMER;
		} else if (state == State::DRAGGING_CONNECTION) {
			if (GetKey(olc::K1).bPressed) selectedInputIndex = 1;
			if (GetKey(olc::K2).bPressed) selectedInputIndex = 2;
		}
	}
	void removeSelectedComponents() {
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
	void handleUserInput() {
		// Left mouse
		if (GetMouse(0).bPressed) { mouseClicked(); }
		if (GetMouse(0).bHeld) { mouseHeld(); }
		if (GetMouse(0).bReleased) { mouseReleased(); }

		// Right mouse
		if (GetMouse(1).bPressed && state == State::PLACING_GATE) { toggleComponent(); }

		// Middle mouse
		if (GetMouse(2).bPressed && state == State::PLACING_GATE) { removeComponent(); }

		// Number keys
		handleNumberInputs();

		// Ctrl + shortcuts
		if (GetKey(olc::A).bPressed && GetKey(olc::CTRL).bHeld) { selectAllComponents(); }

		if (GetKey(olc::S).bPressed && GetKey(olc::CTRL).bHeld) { saveProject(); }

		if (GetKey(olc::C).bPressed && GetKey(olc::CTRL).bHeld) { copySelected(); }

		if (GetKey(olc::V).bPressed && GetKey(olc::CTRL).bHeld) { pasteComponents(); }

		// Based on state
		if (state == State::DRAGGING_CONNECTION) { autoPan(); }
		if (state == State::DRAGGING_GATES) { moveComponent(); }

		// Based on simulationState
		if (simulationState == SimulationState::STEP || (simulationState == SimulationState::RUNNING && GetKey(olc::SPACE).bPressed)) { simulationState = SimulationState::PAUSED; }  // If simulationState is step change to pause since we only step once
		else if (simulationState == SimulationState::PAUSED && GetKey(olc::RIGHT).bPressed) { simulationState = SimulationState::STEP; }
		else if (simulationState == SimulationState::PAUSED && GetKey(olc::SPACE).bPressed) { simulationState = SimulationState::RUNNING; }

		// Shift + shortcuts
		if (GetKey(olc::K0).bPressed && GetKey(olc::SHIFT).bHeld) {
			worldOffsetX = 0;
			worldOffsetY = 0;
		}

		// Del
		if (GetKey(olc::DEL).bPressed) { removeSelectedComponents(); }

		if (GetKey(olc::ESCAPE).bPressed && state == State::DRAGGING_CONNECTION) {
			state = State::PLACING_GATE;
			connectionPoints.clear();
			clickedPoint = getWorldMousePos();
		}
	}

	/*
	 * Simulate the logic in the gates
	*/
	double simulate(int steps = 1) {
		auto start = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < steps; i++) {
			for (auto &gate : gates) {
				gate->update();
			}

			for (auto &gate : gates) {
				gate->output = gate->newOutput;
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::milli>(end - start).count();
	}

	/*
	 * Draw the gates, connections and items related to the current action
	*/
	bool isConnectionVisible(Point src, Point dst) {
		int width = GetDrawTargetWidth();
		int height = GetDrawTargetHeight();

		bool left = src.x < 0 && dst.x < 0;
		bool bottom = src.y < 0 && dst.y < 0;
		bool right = src.x > width && dst.x > width;
		bool top = src.y > height && dst.y > height;

		return !(left || right || top || bottom);
	}
	void drawConnections() {
		for (auto &gate : gates) {
			for (auto &input : gate->inputs) {
				if (auto input_ptr = input.src.lock()) {
					olc::Pixel color = input_ptr->output ? olc::RED : olc::BLACK;
					Point src{ input_ptr->position + size / 2 };
					for (auto &point : input.points) {
						// Only draw connections that are inside th ewindow
						if (isConnectionVisible(getScreenPoint(src), getScreenPoint(point))) {
							DrawLine(src.x - worldOffsetX, src.y - worldOffsetY, point.x - worldOffsetX, point.y - worldOffsetY, color);
						}
						src = point;
					}
					if (isConnectionVisible(getScreenPoint(src), getScreenPoint(gate->position))) {
						DrawLine(src.x - worldOffsetX, src.y - worldOffsetY, gate->position.x - worldOffsetX + size / 2, gate->position.y - worldOffsetY + size / 2, color);
					}
				}
			}
		}
	}
	void drawGates() {
		for (auto &c : gates) {
			int x = c->position.x - worldOffsetX;
			int y = c->position.y - worldOffsetY;

			if (x + size < 0 || x > GetDrawTargetWidth() || y + size < 0 || y > GetDrawTargetHeight()) continue;

			olc::Pixel color = c->output ? olc::RED : olc::GREY;
			FillRect(x, y, size, size, color);
			if (c->selected) {
				DrawRect(x, y, size, size, olc::BLACK);
				DrawRect(x + 1, y + 1, size - 2, size - 2, olc::BLACK);
			}

			const std::string displayName = c->name.substr(0, c->name.find(' '));
			int xOffset = (displayName.size() / 2.0) * 8;
			DrawString(x + size / 2 - xOffset, y + size / 2 - 4, displayName, olc::BLACK);
		}
	}
	void drawMisc() {
		std::string stateString;

		switch (state) {
			// Constructing string with gate name if placing
			case State::PLACING_GATE: {
				stateString = "Placing ";
				switch (selectedType) {
				case GateType::WIRE:
					stateString += "Wire";
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
				case GateType::TIMER:
					stateString += "Timer";
					break;
				}
				break;
			}
			// Drawing currently dragging connection
			case State::DRAGGING_CONNECTION: {
				auto ptr = connectionSrcGate.lock();
				Point src{ ptr->position + size / 2 };
				for (auto &point : connectionPoints) {
					DrawLine(src.x - worldOffsetX, src.y - worldOffsetY, point.x - worldOffsetX, point.y - worldOffsetY, olc::GREEN);
					src = point;
				}
				Point connectionLinePoint = getConnectionLine();

				std::weak_ptr<Component> gate;
				if (checkCollision(gate, getWorldMousePos(), size)) {
					connectionLinePoint = gate.lock()->position + size / 2;
				}
				
				DrawLine(src.x - worldOffsetX , src.y - worldOffsetY, connectionLinePoint.x - worldOffsetX, connectionLinePoint.y - worldOffsetY, olc::BLACK);
				
				stateString = "Connecting " + ptr->name + " and input " + std::to_string(selectedInputIndex);
				break;
			}
			case State::DRAGGING_GATES: {
				auto ptr = clickedGate.lock();
				stateString = "Moving " + ptr->name;
				break;
			}
			// Drawing a rectangle for the currently selected area
			case State::SELECTING_AREA: {
				int x = GetMouseX();
				int y = GetMouseY();
				Point worldClickedPoint = Point{ static_cast<int>(clickedPoint.x - worldOffsetX), static_cast<int>(clickedPoint.y - worldOffsetY) };

				DrawLine(worldClickedPoint.x, worldClickedPoint.y, x, worldClickedPoint.y, olc::BLACK);
				DrawLine(worldClickedPoint.x, worldClickedPoint.y, worldClickedPoint.x, y, olc::BLACK);
				DrawLine(x, worldClickedPoint.y, x, y, olc::BLACK);
				DrawLine(worldClickedPoint.x, y, x, y, olc::BLACK);

				break;
			}
		}

		DrawString(5, 5, stateString, olc::BLACK, 2);

		std::string simulationStateString;
		if (simulationState == SimulationState::RUNNING) {
			simulationStateString += "RUNNING";
		}
		else {
			simulationStateString += " PAUSED";
		}
		
		DrawString(GetDrawTargetWidth()-122, 5, simulationStateString, olc::BLACK, 2);
	}
	double draw() {
		auto start = std::chrono::high_resolution_clock::now();

		Clear(olc::WHITE);

		drawConnections();

		drawGates();

		drawMisc();

		auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::milli>(end - start).count();
	}

public:
	bool OnUserCreate() override {
		loadProject();
		return true;
	}
	bool OnUserDestroy() override {
		saveProject();
		return true;
	}
	bool OnUserUpdate(float fElapsedTime_) override {
		fElapsedTime = fElapsedTime_;
		
		// User input
		handleUserInput();

		// Simulation update
		double simulationTime = 0;
		if (simulationState == SimulationState::RUNNING) {
			simulationTime = simulate(simulationsPerFrame);
		}
		else if (simulationState == SimulationState::STEP) {
			simulationTime = simulate(1);
		}

		// Drawing
		double drawTime = draw();

		sAppName = "Logic Simulator - Simulation: " + std::to_string(simulationTime) + "ms, Drawing : " + std::to_string(drawTime) + "ms\n";

		auto end = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(9 - std::chrono::duration<double, std::milli>(end - start).count()));
		start = end;
		return true;
	}
};

// TODO: Create copy and move assignment and constructor for component that updates the parent ptr to avoid having to store all components as ptrs
// TODO: Sort by location to make collision detection faster
// TODO: Use coordinates as id?
// TODO: Decouple connection completely? storing each connection as input and output
int main() {
	CircuitGUI gui;
	if (gui.Construct(1300, 1300, 1, 1)) {
		gui.Start();
	}
	return 0;
}