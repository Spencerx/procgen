#ifndef PROCEDURAL_GUI_H
#define PROCEDURAL_GUI_H


#include <sstream>

#include "imgui/imgui.h"
#include "imgui/imgui-SFML.h"
#include "LSystem.h"
#include "RuleMapBuffer.h"
#include "LSystemBuffer.h"
#include "LSystemView.h"
#include "InterpretationMapBuffer.h"
#include "Turtle.h"
#include "helper_string.h"


// Define 'display()' and 'interact_with()' for the structs and classes to
// display.
// 
// The 'display()' functions display some of the data (attributes for example)
// at each frame to visualize important parameters and/or dynamic attributes.
//
// The 'interact_with()' functions not only display the data, but also give the
// possibility to modify them on-the-fly, allowing a great deal of
// interactivity.
//
// The 'name' parameter will be the name of the window. If 'display()' is called
// with the same name for two different objects, even with different types, they
// will be in the same window. Exception: the LSystemView which is a coherent
// and unique window.
//
// The 'main' parameter defines if the GUI will be simply displayed in the
// window (main=true) or if it will be displayed in a CollapsingHeader. Used
// mainly internally.
//
// The 'interact_with()' functions return 'true' if the object was modified by
// the GUI, 'false' otherwise.

namespace procgui {
    // Each widget in ImGui has an identifier persistent between two frames.
    // If two widget has the same identifier or if a widget does not have the
    // same identifier between two frames, chaos follow. In this architecture,
    // if two 'interact_buffer()' are called with the same object, it
    // misbehave. To correct this, we add a 'call_id' in procgui.cpp which keep
    // tracks of which call to a interact_with we are doing. We must reset it
    // each frame however, so new_frame is here.
    void new_frame();

    void display(const drawing::DrawingParameters& turtle,
                 const std::string& name,
                 bool main = true);
    
    void display(const LSystem& lsys, const std::string& name, bool main = true);
    
    void display(const drawing::InterpretationMap& map, const std::string& name, bool main = true);

    
    bool interact_with(drawing::DrawingParameters& turtle,
                       const std::string& name,
                       bool main = true);

    // Special case for the RuleMapBuffers
    template<typename Buffer>
    bool interact_with_buffer(Buffer& buffer,
                              std::function<bool(typename Buffer::const_iterator)> predecessor_fn);
    
    bool interact_with(LSystemBuffer& buffer, const std::string& name, bool main = true);

    bool interact_with(InterpretationMapBuffer& buffer, const std::string& name, bool main = true);

    bool interact_with(LSystemView& lsys_view, const std::string& name, bool main = true, bool* open = nullptr);
    
    #include "procgui.tpp"

}


#endif
