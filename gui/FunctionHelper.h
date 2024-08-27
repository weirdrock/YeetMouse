#include "DriverHelper.h"

#ifndef GUI_FUNCTIONHELPER_H
#define GUI_FUNCTIONHELPER_H

#define PLOT_POINTS (512)
#define PLOT_X_RANGE (150)

class CachedFunction {
public:
    float values[PLOT_POINTS]{0};
    float x_stride = 0;
    Parameters* params;

    CachedFunction(float xStride, Parameters *params);
    CachedFunction() {};

    float EvalFuncAt(float x)
    {
        x *= params->preScale;
        if(params->inCap > 0) {
            x = fminf(x, params->inCap);
        }
        float val = 0;
        switch (params->accelMode) {
            case 0:
            {
                break;
            }
            case 1: // Linear
            {
                val = params->accel * x + 1;
                break;
            }
            case 2: // Power
            {
                val = pow(x * params->accel, params->exponent);
                break;
            }
            case 3: // Classic
            {
                val = pow(x * params->accel, params->exponent - 1) + 1;
                break;
            }
            case 4: // Motivity
            {
                val = (params->accel - 1) / (1 + exp(params->midpoint - x)) + 1;
                break;
            }
            case 5: // Jump
            {
                double exp_param = smoothness * (params->midpoint - x);
                double D = exp(exp_param);
                if(params->useSmoothing) {
                    double integral = (params->accel - 1) * (x + ((log(1 + D)) / smoothness));
                    val = ((integral - C0) / x) + 1;
                }
                else {
                    val = (params->accel - 1) / (1 + D) + 1;
                }
                break;
            }
            default:
            {
                break;
            }
        }

        return ((params->outCap > 0) ? fminf(val, params->outCap) : val) * params->sens;
    }
    void PreCacheFunc();

private:
    // Constant parameters for Jump
    float smoothness = 0;
    float C0 = 0;
};

#endif //GUI_FUNCTIONHELPER_H
