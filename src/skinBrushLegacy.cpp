#include "skinBrushTool.h"

// ---------------------------------------------------------------------
// legacy viewport
// ---------------------------------------------------------------------

MStatus SkinBrushContext::doPress(MEvent &event) {
    MStatus status = doPressCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(status);

    doDrag(event);
    return MStatus::kSuccess;
}

MStatus SkinBrushContext::doDrag(MEvent &event) {
    MStatus status = MStatus::kSuccess;

    status = doDragCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(status);

    // To draw a view oriented circle in 3d space get the model view
    // matrix and reset the translation and scale. The points to draw
    // are then multiplied by the inverse matrix
    MMatrix viewMat;
    view.modelViewMatrix(viewMat);
    MTransformationMatrix transMat(viewMat);
    transMat.setTranslation(MVector(), MSpace::kWorld);
    const double scale[3] = {1.0, 1.0, 1.0};
    transMat.setScale(scale, MSpace::kWorld);
    viewMat = transMat.asMatrix();

    view.beginXorDrawing(false, true, (float)lineWidthVal, M3dView::kStippleNone);

    if (drawBrushVal || event.mouseButton() == MEvent::kMiddleMouse) {
        // Draw the circle in regular paint mode.
        // The range circle doens't get drawn here to avoid visual
        // clutter.
        if (event.mouseButton() == MEvent::kLeftMouse) {
            drawCircle(surfacePoints[0], viewMat, sizeVal);
        }
        // Adjusting the brush settings with the middle mouse button.
        else if (event.mouseButton() == MEvent::kMiddleMouse) {
            // When adjusting the size the circle needs to remain with
            // a static position but the size needs to change.
            if (sizeAdjust) {
                drawCircle(surfacePointAdjust, viewMat, adjustValue);
                if (volumeVal && drawRangeVal)
                    drawCircle(surfacePointAdjust, viewMat, adjustValue * rangeVal);
            }
            // When adjusting the strength the circle needs to remain
            // fixed and only the strength indicator changes.
            else {
                drawCircle(surfacePointAdjust, viewMat, sizeVal);
                if (volumeVal && drawRangeVal)
                    drawCircle(surfacePointAdjust, viewMat, sizeVal * rangeVal);

                // Drawing the strength line is not properly working in
                // this implementation. But since the legacy viewport
                // isn't considered current anymore it remains
                // unfinished.
                /*
                MPoint start(startScreenX, startScreenY);
                MPoint end(startScreenX, startScreenY + adjustValue * 500);
                glBegin(GL_LINES);
                glVertex2f((float)start.x, (float)start.y);
                glVertex2f((float)end.x, (float)end.y);
                glEnd();
                */
            }
        }
    }
    view.endXorDrawing();

    return status;
}

void SkinBrushContext::drawCircle(MPoint point, MMatrix mat, double radius) {
    unsigned int i;
    glBegin(GL_LINE_LOOP);
    for (i = 0; i < 360; i += 2) {
        double degInRad = i * DEGTORAD;
        MPoint circlePoint(cos(degInRad) * radius, sin(degInRad) * radius, 0.0);
        circlePoint *= mat.inverse();
        glVertex3f(float(point.x + circlePoint.x), float(point.y + circlePoint.y),
                   float(point.z + circlePoint.z));
    }
    glEnd();
}

MStatus SkinBrushContext::doRelease(MEvent &event) {
    doReleaseCommon(event);
    return MStatus::kSuccess;
}
