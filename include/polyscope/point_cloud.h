#pragma once

#include "polyscope/structure.h"


namespace polyscope {

class PointCloud : public Structure {

public:

    // === Member functions ===

    // Render the the structure on screen
    virtual void draw() override;
    
    // Do setup work related to drawing, including allocating openGL data
    virtual void prepare() override;
    
    // Undo anything done in prepare(), including deallocating openGL data
    virtual void teardown() override;
    
    // Build the imgui display
    virtual void drawUI() override;

    // Render for picking
    virtual void drawPick() override;

};


}