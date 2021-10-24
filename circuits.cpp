#include <chrono>
#include <string>
#include <vector>
#include <numeric>

#include "component.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

enum class State {PLACING_GATE, DRAGGING_CONNECTION, DRAGGING_GATES, SELECTING_AREA};
enum class SimulationState {PAUSED, STEP, RUNNING};

struct alignas(64) CopiedGate {
	std::shared_ptr<Component> gate;
	std::vector<int> inputIndices;
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
	Point clickedPixelPoint{ 0,0 };
	Point copyPoint{ 0,0 };

	int tileSize = 64;
	const int simulationsPerFrame = 1;

	float fElapsedTime = 0;
	int worldOffsetX = 0;
	int worldOffsetY = 0;
	
	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();

	/*
	 * Check the user input and perform the actions bound to the inputs
	*/
	// TODO: Save zoom and worldOffset
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
	bool checkCollision(std::weak_ptr<Component> &outGate, Point point) {
		for (std::shared_ptr<Component> &gate : gates) {
			if (point.x == gate->position.x && point.y == gate->position.y) {
				outGate = gate;
				return true;
			}
		}

		return false;
	}
	Point getWorldMousePos() {
		float x = (GetMouseX() + worldOffsetX - GetDrawTargetWidth() / 2);
		float y = (GetMouseY() + worldOffsetY - GetDrawTargetHeight() / 2);

		x /= tileSize;
		y /= tileSize;

		x = std::floorf(x);
		y = std::floorf(y);

		return Point{ static_cast<int>(x), static_cast<int>(y) };
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

		zoom();

		// Number keys
		handleNumberInputs();

		// Ctrl + shortcuts
		if (GetKey(olc::A).bPressed && GetKey(olc::CTRL).bHeld) { selectAllComponents(); }

		if (GetKey(olc::S).bPressed && GetKey(olc::CTRL).bHeld) { saveProject(); }

		if (GetKey(olc::C).bPressed && GetKey(olc::CTRL).bHeld) { copySelected(); }

		if (GetKey(olc::V).bPressed && GetKey(olc::CTRL).bHeld) { pasteComponents(); }

		// Based on state
		if (state == State::DRAGGING_CONNECTION) { autoPan(); }
		if (state == State::DRAGGING_GATES) { moveComponents(); }

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

	void mouseHeld() {
		if (state == State::PLACING_GATE) {
			Point delta = getWorldMousePos() - clickedPoint;

			if (GetKey(olc::SHIFT).bHeld) {
				pan(Point{ delta.x * tileSize, delta.y * tileSize });
				clickedPoint = getWorldMousePos();
			}
			else {
				Point pixelDelta = Point{ GetMouseX(), GetMouseY() } - clickedPixelPoint;
				if (std::abs(pixelDelta.x) > 5 || std::abs(pixelDelta.y) > 5) {
					startSelectingArea();
				}
			}
		}
	}
	void mouseReleased() {
		if (state == State::DRAGGING_CONNECTION) {
			std::weak_ptr<Component> gate;
			if (checkCollision(gate, getWorldMousePos())) {
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
	void mouseClicked() {
		clickedPoint = getWorldMousePos();
		clickedPixelPoint = Point{ GetMouseX(), GetMouseY() };

		if (checkCollision(clickedGate, clickedPoint)) {
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

	void handleNumberInputs() {
		if (state == State::PLACING_GATE) {
			if (GetKey(olc::K1).bPressed) selectedType = GateType::WIRE;
			if (GetKey(olc::K2).bPressed) selectedType = GateType::INPUT;
			if (GetKey(olc::K3).bPressed) selectedType = GateType::AND;
			if (GetKey(olc::K4).bPressed) selectedType = GateType::XOR;
			if (GetKey(olc::K5).bPressed) selectedType = GateType::OR;
			if (GetKey(olc::K6).bPressed) selectedType = GateType::NOT;
			if (GetKey(olc::K7).bPressed) selectedType = GateType::TIMER;
		}
		else if (state == State::DRAGGING_CONNECTION) {
			if (GetKey(olc::K1).bPressed) selectedInputIndex = 1;
			if (GetKey(olc::K2).bPressed) selectedInputIndex = 2;
		}
	}

	void copyComponents() {
		for (auto &selectedGate : selectedGates) {
			if (!selectedGate.expired()) {
				auto selectedPtr = selectedGate.lock();

				// Create a new instance of the component
				copiedGates.push_back({ createComponent(selectedPtr->getType(), "Tmp", selectedPtr->position, 0) });
				copiedGates.back().gate->inputs = selectedPtr->inputs;
				copiedGates.back().gate->output = selectedPtr->output;
				copiedGates.back().inputIndices.resize(copiedGates.back().gate->inputs.size(), -1);
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

		// Deselect all selected gates
		for (auto &gate : selectedGates) {
			if (!gate.expired()) {
				gate.lock()->selected = false;
			}
		}
		selectedGates.clear();

		// Check for collisions on pasted positions
		for (auto &gate : copiedGates) {
			std::weak_ptr<Component> tmpGate;
			if (checkCollision(tmpGate, gate.gate->position + delta)) {
				std::cout << "Gate collision on paste\n";
				return;
			}
		}

		int prevGateCount = gates.size();
		// Create of a new instance of the component 
		for (auto &gate : copiedGates) {
			gates.push_back(createComponent(gate.gate->getType(), "Copied test", gate.gate->position + delta));
			gates.back()->output = gate.gate->output;

			// Select each new instance of the gates
			gates.back()->selected = true;
			selectedGates.push_back(gates.back());
		}

		// Connect the copied components to the inputs that were also copied
		for (int j = 0; j < copiedGates.size(); j++) {
			for (int i = 0; i < copiedGates[j].inputIndices.size(); i++) {
				if (copiedGates[j].inputIndices[i] >= 0) {
					auto newConnectionPath = copiedGates[j].gate->inputs[i].points;
					for (auto &point : newConnectionPath) {
						point = point + delta;
					}
					gates[prevGateCount + j]->connectInput(gates[prevGateCount + copiedGates[j].inputIndices[i]], i, std::move(newConnectionPath));
				}

			}
		}
	}

	void removeComponent() {
		std::weak_ptr<Component> gate;
		if (checkCollision(gate, getWorldMousePos())) {
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

	void selectGatesInArea() {
		Point worldMousePos = getWorldMousePos();
		Point delta = worldMousePos - clickedPoint;

		for (auto &gate : gates) {
			if (delta.x < 0) {
				if (gate->position.x > clickedPoint.x || gate->position.x < worldMousePos.x) continue;
			}
			else {
				if (gate->position.x < clickedPoint.x || gate->position.x > worldMousePos.x) continue;
			}

			if (delta.y < 0) {
				if (gate->position.y > clickedPoint.y || gate->position.y < worldMousePos.y) continue;
			}
			else {
				if (gate->position.y < clickedPoint.y || gate->position.y > worldMousePos.y) continue;
			}

			selectedGates.push_back(gate);
			gate->selected = true;
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
	Point getConnectionLine() {
		Point src{ 0,0 };
		if (connectionPoints.empty()) {
			auto ptr = clickedGate.lock();
			src = ptr->position;
		}
		else {
			src = connectionPoints.back();
		}

		Point worldMousePos = getWorldMousePos();
		int deltaX = std::abs(src.x - worldMousePos.x);
		int deltaY = std::abs(src.y - worldMousePos.y);

		if (deltaX > deltaY) {
			return Point{ worldMousePos.x, src.y };
		}
		else {
			return Point{ src.x, worldMousePos.y };
		}
	}

	void pan(Point delta) {
		worldOffsetX -= delta.x;
		worldOffsetY -= delta.y;
	}

	void placeGate() {
		std::weak_ptr<Component> gate;
		if (!checkCollision(gate, getWorldMousePos())) {
			gates.push_back(createComponent(selectedType, "Test", getWorldMousePos()));
		}
		else {
			std::cout << "Space already occupied by component\n";
		}
	}
	void autoPan() {
		const int x = GetMouseX();
		const int y = GetMouseY();
		const float moveSpeed = 500;
		const int border = 30;

		if (x < border) {
			worldOffsetX -= static_cast<int>(moveSpeed * fElapsedTime);
		}
		else if (x > GetDrawTargetWidth() - border) {
			worldOffsetX += static_cast<int>(moveSpeed * fElapsedTime);
		}

		if (y < border) {
			worldOffsetY -= static_cast<int>(moveSpeed * fElapsedTime);
		}
		else if (y > GetDrawTargetHeight() - border) {
			worldOffsetY += static_cast<int>(moveSpeed * fElapsedTime);
		}
	}

	void moveComponent(std::shared_ptr<Component> &gate, Point delta) {
		gate->position = gate->position + delta;

		if (clickedGate.lock()->selected) {
			// Move each point of the connection
			for (auto &input : gate->inputs) {
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
	void moveComponents() {
		Point delta = getWorldMousePos() - clickedPoint;

		std::weak_ptr<Component> tmpGate;
		bool collision = false;

		// Move all selected gates if the clicked gate was selected
		if (clickedGate.lock()->selected) {
			// Check if any of the selected gates collide after move
			for (auto &gate : selectedGates) {
				if (auto ptr = gate.lock()) {
					if (checkCollision(tmpGate, getWorldMousePos())) {
						// If both colliding gates are selected then there is no collision after the move since both move
						if (!tmpGate.lock()->selected) {
							collision = true;
							break;
						}
					}
				}
			}

			// Move all selected gates if there was no collition
			if (!collision) {
				for (auto &gate : selectedGates) {
					moveComponent(gate.lock(), delta);
				}
				clickedPoint = getWorldMousePos();
			}
		}
		// Else just move the clicked component
		else {
			auto ptr = clickedGate.lock();
			if (!checkCollision(tmpGate, getWorldMousePos())) {
				moveComponent(ptr, delta);
				clickedPoint = getWorldMousePos();
			}
		}
	}

	void toggleComponent() {
		std::weak_ptr<Component> gate;
		if (checkCollision(gate, getWorldMousePos())) {
			auto ptr = gate.lock();
			ptr->output = !ptr->output;
		}
	}

	void zoom() {
		if (GetMouseWheel() < 0) {
			if (tileSize > 1) {
				tileSize /= 2;
				worldOffsetX /= 2;
				worldOffsetY /= 2;
			}
		}
		else if (GetMouseWheel() > 0) {
			tileSize *= 2;
			worldOffsetX *= 2;
			worldOffsetY *= 2;
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
	Point getPixelPoint(Point point) const {
		return Point{ (point.x * tileSize) - worldOffsetX + GetDrawTargetWidth() / 2, (point.y * tileSize) - worldOffsetY + GetDrawTargetHeight() / 2 };
	}
	// TODO: Add culling
	void drawConnectionPath(Point src, const Point &finalDst, const std::vector<Point> &points, olc::Pixel color) {
		for (const auto &point : points) {
			Point dst = getPixelPoint(point) + tileSize / 2;
			DrawLine(src.x, src.y, dst.x, dst.y, color);
			src = dst;
		}
		DrawLine(src.x, src.y, finalDst.x, finalDst.y, color);

	}
	void drawConnections() {
		for (auto &gate : gates) {
			for (auto &input : gate->inputs) {
				if (auto input_ptr = input.src.lock()) {
					olc::Pixel color = input_ptr->output ? olc::RED : olc::BLACK;
					drawConnectionPath(getPixelPoint(input_ptr->position) + tileSize / 2, getPixelPoint(gate->position) + tileSize / 2, input.points, color);
				}
			}
		}
	}
	void drawGates() {
		for (auto &c : gates) {
			Point position = getPixelPoint(c->position);

			if (position.x >= -tileSize && position.x < GetDrawTargetWidth() && position.y >= -tileSize && position.y < GetDrawTargetHeight()) {

				olc::Pixel color = c->output ? olc::RED : olc::GREY;
				FillRect(position.x, position.y, tileSize, tileSize, color);
				if (c->selected) {
					DrawRect(position.x, position.y, tileSize, tileSize, olc::BLACK);
					DrawRect(position.x + 1, position.y + 1, tileSize - 2, tileSize - 2, olc::BLACK);
				}

				if (tileSize > 8) {
					const std::string displayName = c->name.substr(0, c->name.find(' '));
					int xOffset = (displayName.size() / 2.0) * 8;
					DrawString(position.x + tileSize / 2 - xOffset, position.y + tileSize / 2 - 4, displayName, olc::BLACK);
				}
			}
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
				
				Point connectionLinePoint;
				std::weak_ptr<Component> gate;
				if (checkCollision(gate, getWorldMousePos())) {
					connectionLinePoint = getPixelPoint(gate.lock()->position) + tileSize / 2;
				}
				else {
					connectionLinePoint = getPixelPoint(getConnectionLine()) + tileSize / 2;
				}

				drawConnectionPath(getPixelPoint(ptr->position) + tileSize / 2, connectionLinePoint, connectionPoints, olc::DARK_RED);

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

				DrawLine(clickedPixelPoint.x, clickedPixelPoint.y, x, clickedPixelPoint.y, olc::BLACK);
				DrawLine(clickedPixelPoint.x, clickedPixelPoint.y, clickedPixelPoint.x, y, olc::BLACK);
				DrawLine(x, clickedPixelPoint.y, x, y, olc::BLACK);
				DrawLine(clickedPixelPoint.x, y, x, y, olc::BLACK);

				stateString = "Selecting area\n";
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
	if (gui.Construct(1280, 960, 1, 1)) {
		gui.Start();
	}
	return 0;
}