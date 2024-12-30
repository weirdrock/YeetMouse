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

    float EvalFuncAt(float x)
    {
        x *= params->preScale;
        if(params->inCap > 0) {
            x = fminf(x, params->inCap);
        }
        float val = 0;
        switch (params->accelMode) {
            case AccelMode_Current:
            {
                break;
            }
            case AccelMode_Linear: // Linear
            {
                val = params->accel * x + 1;
                break;
            }
            case AccelMode_Power: // Power
            {
                val = pow(x * params->accel, params->exponent);
                break;
            }
            case AccelMode_Classic: // Classic
            {
                val = pow(x * params->accel, params->exponent - 1) + 1;
                break;
            }
            case AccelMode_Motivity: // Motivity
            {
                val = (params->accel - 1) / (1 + exp(params->midpoint - x)) + 1;
                break;
            }
            case AccelMode_Jump: // Jump
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
            case AccelMode_Lut: // LUT
            {
                if(params->LUT_size == 0)
                    break;

                if(x < params->LUT_data_x[0]) {
                    val = params->LUT_data_y[0];
                    break;
                }

                // Binary Search for the closest value smaller than x, so the n+1 value is greater than x
                int l = 0, r = params->LUT_size - 2;
                //while(l <= r) {
                //    int mid = (r + l) / 2;
                //
                //    if(x > params->LUT_data_x[mid]) {
                //        l = mid + 1;
                //    }
                //    else if(x < params->LUT_data_x[mid]) {
                //        r = mid - 1;
                //    }
                //    else { // This should never happen
                //        break;
                //    }
                //}

                while (l < r) {
                    int mid = (r + l) / 2;

                    if (x > params->LUT_data_x[mid]) {
                        l = mid + 1;
                    } else {
                        r = mid - 1;
                    }
                }

                l = r;
                //printf("Found best x (for %f) idx: %i (%f), val: %f\n", x, l, params->LUT_data_x[l], params->LUT_data_y[l]);

                int pos = l;//std::min(l, (int)params->LUT_size - 2);
                float p = params->LUT_data_y[(int)(pos)]; // p element
                float p1 = params->LUT_data_y[(int)(pos) + 1]; // p + 1 element

                // derived from this (lerp): frac * params->LUT_data_x[l + 1] + params->LUT_data_x[l] = x
                float frac = (x - params->LUT_data_x[l]) / (params->LUT_data_x[l + 1] - params->LUT_data_x[l]);

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
