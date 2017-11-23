#pragma once

namespace polyscope {

class Structure {

public:

    // === Member functions ===

    // Render the the structure on screen
    virtual void draw() = 0;
    
    // Do setup work related to drawing, including allocating openGL data
    virtual void prepare() = 0;
    
    // Undo anything done in prepare(), including deallocating openGL data
    virtual void teardown() = 0;
    
    // Build the imgui display
    virtual void drawUI() = 0;

    // Render for picking
    virtual void drawPick() = 0;

};


} // namespace polyscope