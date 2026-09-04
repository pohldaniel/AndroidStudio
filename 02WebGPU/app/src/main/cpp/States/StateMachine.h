#pragma once

#include "IStateMachine.h"
#include "Event.h"

enum States {
	COLLADA,
	WIREFRAME,
	DEFERRED_RENDERING,
	COMPUTE_PARTICLE_LOGO,
	VOLUME_RENDERING,
	BOW_SIMULATION,
	AUDIO_DECODE,
	ISOMETRIC
};

class State;
class StateMachine : public IStateMachine<State> {

	friend class IStateMachine<State>;

public:

	StateMachine(const float& dt, const float& fdt);

	void fixedUpdate();
	void update();
	void render();
	void resizeState(int deltaW, int deltaH, States state);
	void popState();
	States getCurrentState();

	const float& m_fdt;
	const float& m_dt;

	static void ToggleWireframe();
	static bool& GetWireframeEnabled();
    static void DisableWireframe();

private:

	static bool WireframeEnabled;
};

class State : public IState<State> {

public:

	State(StateMachine& machine, States currentState);
	virtual ~State();

	States getCurrentState();

	virtual void OnMouseMotion(const Event::MouseMoveEvent& event);
	virtual void OnMouseButtonDown(const Event::MouseButtonEvent& event);
	virtual void OnMouseButtonUp(const Event::MouseButtonEvent& event);
	virtual void OnScroll(double xOffset, double yOffset);
    virtual void OnKeyDown(const Event::KeyboardEvent& event);
	virtual void OnKeyUp(const Event::KeyboardEvent& event);
	virtual void OnButton(const Event::MouseButtonEvent& event);

protected:

	StateMachine& m_machine;
	const float& m_fdt;
	const float& m_dt;

	States m_currentState;
};