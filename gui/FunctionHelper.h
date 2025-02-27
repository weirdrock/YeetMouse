#include "DriverHelper.h"

#ifndef GUI_FUNCTIONHELPER_H
#define GUI_FUNCTIONHELPER_H

#define PLOT_POINTS (512)
#define PLOT_X_RANGE (150)

#define LERP(a,b,x)     (((b) - (a)) * (x) + (a))

class CachedFunction {
public:
    float values[PLOT_POINTS]{0};
    float values_y[PLOT_POINTS]{0};
    float x_stride = 0;
    Parameters* params;

    CachedFunction(float xStride, Parameters *params);
    CachedFunction() {};

    float EvalFuncAt(float x);
    void PreCacheFunc();

    void SmoothBezier();

private:
    // Constant parameters for Jump
    float smoothness = 0;
    float C0 = 0;
};

#endif //GUI_FUNCTIONHELPER_H
