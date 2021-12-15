// ---------------------------------------------------------------------
//
//  pluginMain.cpp
//  brSkinBrush
//
//  Created by guillaume using code from ingo on 11/18/18.
//  https://github.com/IngoClemens/brSkinBrush?fbclid=IwAR0VxH-zX51EwtPtX4-bvAoESL7YhjYC3_BJcyHbwV1qrUMx3uVvCVOwGFg
//  Copyright (c) 2018 ingo. All rights reserved.
//
// ---------------------------------------------------------------------

#include <string>

static const std::string kVERSION = "1.1.3";

#include <maya/MFnPlugin.h>

#include "functions.h"
#include "skinBrushTool.h"

// ---------------------------------------------------------------------
// initialization
// ---------------------------------------------------------------------

MStatus initializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj, "Ingo Clemens", kVERSION.c_str(), "Any");

    status = plugin.registerContextCommand("brSkinBrushContext", SkinBrushContextCmd::creator,
                                           "brSkinBrushCmd", skinBrushTool::creator);
    if (status != MStatus::kSuccess) status.perror("Register brSkinBrushContext failed.");

    return status;
}

MStatus uninitializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj, "Ingo Clemens", kVERSION.c_str(), "Any");

    status = plugin.deregisterContextCommand("brSkinBrushContext", "brSkinBrushCmd");
    if (status != MStatus::kSuccess) status.perror("Deregister brSkinBrushContext failed.");

    return status;
}

// ---------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2018 Ingo Clemens, brave rabbit
// brSkinBrush and brTransferWeights are under the terms of the MIT
// License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Author: Ingo Clemens    www.braverabbit.com
// ---------------------------------------------------------------------
