#ifndef __skinBrushTool__enums__
#define __skinBrushTool__enums__

enum class ModifierKeys : int {
    NoModifier,
    Shift,
    Control,
    ControlShift
};

enum class ModifierCommands : int {
    Add = 0,
    Remove = 1,
    AddPercent = 2,
    Absolute = 3,
    Smooth = 4,
    Sharpen = 5,
    LockVertices = 6,
    UnlockVertices = 7
};

#endif
