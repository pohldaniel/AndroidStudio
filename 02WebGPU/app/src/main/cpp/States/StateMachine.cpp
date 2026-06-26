#include "StateMachine.h"

bool StateMachine::WireframeEnabled = false;
bool StateMachine::WireframeToggled = false;

StateMachine::StateMachine(const float& dt, const float& fdt) : m_fdt(fdt),  m_dt(dt) {

}

void StateMachine::fixedUpdate() {
	if (!m_states.empty())
		m_states.top()->fixedUpdate();
}

void StateMachine::update() {
	if (!m_states.empty()) {
		m_states.top()->update();
		if (!m_states.top()->isRunning()) {
			States state = m_states.top()->getCurrentState();
			delete m_states.top();
			m_states.pop();
			if (!m_states.empty())
				m_states.top()->OnReEnter(state);
		}
	}else {
		m_isRunning = false;
	}
}

void StateMachine::render() {
	if (!m_states.empty()) {
		m_states.top()->render();
	}
}

void StateMachine::resizeState(int deltaW, int deltaH, States state) {
	if (m_states.empty()) return;

	if (m_states.top()->getCurrentState() == state) {
		m_states.top()->resize(deltaW, deltaH);
	}else {
		State* temp = m_states.top();
		m_states.pop();
		resizeState(deltaW, deltaH, state);
		m_states.push(temp);
	}
}

void StateMachine::ToggleWireframe() {
	WireframeEnabled = !WireframeEnabled;
	WireframeToggled = true;
}

bool& StateMachine::GetWireframeEnabled() {
	return WireframeEnabled;
}

bool StateMachine::IsWireframeToggled(){
	if (WireframeToggled) {
		WireframeToggled = false;
		return true;
	}
	return false;
}
/////////////////////////////////////////////
State::State(StateMachine& machine, States currentState) : m_machine(machine), m_fdt(machine.m_fdt), m_dt(machine.m_dt) {
	m_currentState = currentState;
}

State::~State() {

}

States State::getCurrentState() {
	return m_currentState;
}

void State::OnMouseMotion(const Event::MouseMoveEvent& event){

}

void State::OnMouseButtonDown(const Event::MouseButtonEvent& event){

}

void State::OnMouseButtonUp(const Event::MouseButtonEvent& event){

}

void State::OnScroll(double xOffset, double yOffset){

}

void State::OnKeyDown(const Event::KeyboardEvent& event){

}

void State::OnKeyUp(const Event::KeyboardEvent& event){
	
}