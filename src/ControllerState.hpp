#ifndef SIMPLE_GAME_TEMPLATE_CONTROLLER_STATE_HPP
#define SIMPLE_GAME_TEMPLATE_CONTROLLER_STATE_HPP

namespace ControllerButton
{

enum Button
{
    A = 1<<1,
    B = 1<<2,
    X = 1<<3,
    Y = 1<<4,
    Select = 1<<5,
    Start = 1<<6,
    LeftShoulder = 1<<7,
    RightShoulder = 1<<8,
    LeftTrigger = 1<<9,
    RightTrigger = 1<<10,
    LeftStick = 1<<11,
    RightStick = 1<<12,
};

}

struct ControllerState
{
    ControllerState()
        :
        leftXAxis(0),
        leftYAxis(0),
        rightXAxis(0),
        rightYAxis(0),
        buttons(0)
    {
    }

    float leftXAxis;
    float leftYAxis;
    float rightXAxis;
    float rightYAxis;

    int buttons;

    void setButton(int button, bool state)
    {
        if(state)
            buttons |= button;
        else
            buttons &= ~button;
    }

    bool getButton(int button) const
    {
        return (buttons & button) != 0;
    }

    void applyDifferencesOf(const ControllerState &oldState, const ControllerState &newState)
    {
        if(oldState.leftXAxis != newState.leftXAxis)
            leftXAxis = newState.leftXAxis;
        if(oldState.leftYAxis != newState.leftYAxis)
            leftYAxis = newState.leftYAxis;
        if(oldState.rightXAxis != newState.rightXAxis)
            rightXAxis = newState.rightXAxis;
        if(oldState.rightYAxis != newState.rightYAxis)
            rightYAxis = newState.rightYAxis;

#define APPLY_BUTTON_DIFFERENCE(button) \
        if(oldState.getButton(button) != newState.getButton(button)) \
            setButton(button, newState.getButton(button));

        APPLY_BUTTON_DIFFERENCE(ControllerButton::A);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::B);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::X);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::Y);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::Select);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::Start);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::LeftShoulder);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::RightShoulder);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::LeftTrigger);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::RightTrigger);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::LeftStick);
        APPLY_BUTTON_DIFFERENCE(ControllerButton::RightStick);
#undef APPLY_BUTTON_DIFFERENCE
    }
};

#endif //SIMPLE_GAME_TEMPLATE_CONTROLLER_STATE_HPP
