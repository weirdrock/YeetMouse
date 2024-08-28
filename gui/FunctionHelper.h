#include "DriverHelper.h"

#ifndef GUI_FUNCTIONHELPER_H
#define GUI_FUNCTIONHELPER_H

#define PLOT_POINTS (512)
#define PLOT_X_RANGE (150)

#define LERP(a,b,x)     (((b) - (a)) * (x) + (a))

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
            case 6: // LUT
            {
                if(params->LUT_size == 0)
                    break;

                //if(x > (params->LUT_size - 1) / params->LUT_stride) { // Out of range
                //    int end_idx = (params->LUT_size - 1);
                //    float p1 = params->LUT_data[end_idx];
                //    float p = params->LUT_data[end_idx - 1];
                //    float slope = (p1 - p) / params->LUT_stride;
                //
                //    val = slope * (x - ((params->LUT_size - 0) / params->LUT_stride)) + p1 + slope;
                //
                //    break;
                //}
                int pos = std::min((int)(params->LUT_stride * x), (int)params->LUT_size - 2);
                float p = params->LUT_data[(int)(pos)]; // p element
                float p1 = params->LUT_data[(int)(pos) + 1]; // p + 1 element

                float frac = (params->LUT_stride * x) - pos;

                //printf("frac: %f\n", frac);

                // Interpolate between p and p+1 elements
                val = LERP(p, p1, frac);
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
