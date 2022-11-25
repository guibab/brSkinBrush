#ifndef __skinBrushTool__enums__
#define __skinBrushTool__enums__

enum class ModifierKeys : int {
    NoModifier,
    Shift,
    Control,
    ControlShift
};

enum class ModifierCommands : int {
    Add = 1,
    Remove = 2,
    AddPercent = 3,
    Absolute = 4,
    Smooth = 5,
    Sharpen = 6,
    LockVertices = 7,
    UnlockVertices = 8
};

#endif
