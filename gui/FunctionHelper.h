#ifndef GUI_FUNCTIONHELPER_H
#define GUI_FUNCTIONHELPER_H


#include "DriverHelper.h"

#define PLOT_POINTS (512)
#define PLOT_X_RANGE (150)

#define LERP(a,b,x)     (((b) - (a)) * (x) + (a))

class CachedFunction {
public:
    float values[PLOT_POINTS]{0};
    float values_y[PLOT_POINTS]{0};
    float x_stride = 0;
    Parameters* params;

    bool isValid = true;

    CachedFunction(float xStride, Parameters *params);
    CachedFunction() {};

    float EvalFuncAt(float x);
    void PreCacheFunc(); // Also validates settings

    // Result also saved into 'bool isValid'
    bool ValidateSettings();

private:
    // Constant parameters for Jump
    float smoothness = 0;
    float C0 = 0;

    // Power
    float offset_x = 0;
    float power_constant = 0;
};

#endif //GUI_FUNCTIONHELPER_H
