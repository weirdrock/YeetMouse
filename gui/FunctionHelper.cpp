#include <cmath>

#define M_PI 3.14159265358979323846f

#include "FunctionHelper.h"

CachedFunction::CachedFunction(float xStride, Parameters *params)
        : x_stride(xStride), params(params) { }

void CachedFunction::PreCacheFunc() {

    // Pre-Cache constants
    switch (params->accelMode) {
        case 0:
        {
            break;
        }
        case 1:
        {
            break;
        }
        case 2:
        {
            break;
        }
        case 3:
        {
            break;
        }
        case 4:
        {

        }
        case 5:
        {
            //printf("exp = %.2f, mid = %.2f\n", params->exponent, params->midpoint);
            smoothness = (2 * M_PI) / (params->exponent * params->midpoint);
            //printf("sm = %.2f\n", smoothness);
            //C0 = params->accel * params->midpoint; // Fast approx
            C0 = (params->accel - 1) * (smoothness * params->midpoint + logf(1+expf(-smoothness * params->midpoint)))/smoothness;
            break;
        }
        default:
        {
            break;
        }
    }

    float x = -params->offset + 0.01;
    for(int i = 0; i < PLOT_POINTS; i++) {
        if(x < 0) { // skip offset
            values[i] = params->sens;
            x += x_stride;
            continue;
        }
        float val = EvalFuncAt(x);
        values[i] = val; // fabsf(params->outCap) > 0.01 ? fminf(val, params->outCap) : val;
        x += x_stride;
    }
}
