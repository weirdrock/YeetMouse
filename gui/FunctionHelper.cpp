#include <cmath>

#include "FunctionHelper.h"

#include <array>

#include "External/ImGui/imgui_internal.h"

CachedFunction::CachedFunction(float xStride, Parameters *params)
        : x_stride(xStride), params(params) { }

float CachedFunction::EvalFuncAt(float x) {
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
            // Might cause issues with high exponent's argument values
            double exp_param = smoothness * (params->midpoint - x);
            double D = exp(exp_param);
            if(params->useSmoothing) {
                double integral = (params->accel - 1) * (x + (log(1 + D) / smoothness));
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
            int l = 0, r = params->LUT_size - 1;
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

            int best_point = params->LUT_size - 1;
            while (l <= r) {
                int mid = (r + l) / 2;

                if (x > params->LUT_data_x[mid]) {
                    l = mid + 1;
                } else {
                    best_point = mid;
                    r = mid - 1;
                }
            }

            l = best_point-1;
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
        case AccelMode_CustomCurve:
        {


            // Just do exactly what's done in LUT
            if(params->LUT_size == 0)
                break;

            if(x < params->LUT_data_x[0]) {
                val = params->LUT_data_y[0];
                break;
            }

            // Binary Search for the closest value smaller than x, so the n+1 value is greater than x
            int l = 0, r = params->LUT_size - 1;
            int best_point = params->LUT_size - 1;
            while (l <= r) {
                int mid = (r + l) / 2;

                if (x > params->LUT_data_x[mid]) {
                    l = mid + 1;
                } else {
                    best_point = mid;
                    r = mid - 1;
                }
            }

            l = best_point-1;
            //printf("Found best x (for %f) idx: %i (%f), val: %f\n", x, l, params->LUT_data_x[l], params->LUT_data_y[l]);

            int pos = l;//std::min(l, (int)params->LUT_size - 2);
            float p = params->LUT_data_y[(int)(pos)]; // p element
            float p1 = params->LUT_data_y[(int)(pos) + 1]; // p + 1 element

            // derived from this (lerp): frac * params->LUT_data_x[l + 1] + params->LUT_data_x[l] = x
            float frac = (x - params->LUT_data_x[l]) / (params->LUT_data_x[l + 1] - params->LUT_data_x[l]);

            // Interpolate between p and p+1 elements
            val = LERP(p, p1, frac);
            break;



            // // find the closes curve point
            // auto& points = params->custom_curve_points;
            // auto& control_points = params->custom_curve_control_points;
            // if (points.size() < 2)
            //     break;
            // int best_idx = 0;
            // for (int i = points.size()-2; i >= 0; i--) {
            //     if (points[i].x < x) {
            //         best_idx = i;
            //         break;
            //     }
            // }
            //
            // // val = EvaluateSpline(x, seg);
            // // break;
            //
            // // if (best_idx == -1) {
            // //     val = 1;
            // //     break;
            // // }
            // // how far between the points are we
            // float frac = (x - points[best_idx].x) / (points[best_idx + 1].x - points[best_idx].x);
            // //frac = (x - control_points[best_idx][0].x) / (control_points[best_idx][1].x - control_points[best_idx][0].x);
            // auto point = ImBezierCubicCalc(points[best_idx], control_points[best_idx][0], control_points[best_idx][1], points[best_idx + 1], frac);
            // //printf("frac = %f, x = %f, bezier x = %f, f(x) = %f, best_idx = %d\n", frac, x, point.x, point.y, best_idx);
            // val = point.y;
            break;
        }
        default:
        {
            break;
        }
    }

    return ((params->outCap > 0) ? fminf(val, params->outCap) : val) * params->sens;
}

void CachedFunction::PreCacheFunc() {

    // Pre-Cache constants
    switch (params->accelMode) {
        case AccelMode_Current:
        {
            break;
        }
        case AccelMode_Linear:
        {
            break;
        }
        case AccelMode_Power:
        {
            break;
        }
        case AccelMode_Classic:
        {
            break;
        }
        case AccelMode_Motivity:
        {
            break;
        }
        case AccelMode_Jump:
        {
            //printf("exp = %.2f, mid = %.2f\n", params->exponent, params->midpoint);
            smoothness = (2 * M_PI) / (params->exponent * params->midpoint);
            //printf("sm = %.2f\n", smoothness);
            //C0 = params->accel * params->midpoint; // Fast approx
            C0 = (params->accel - 1) * (smoothness * params->midpoint + logf(1+expf(-smoothness * params->midpoint)))/smoothness;
            break;
        }
        case AccelMode_Lut:
        {
            break;
        }
        case AccelMode_CustomCurve:
        {
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
            values_y[i] = params->sensY;
            x += x_stride;
            continue;
        }
        float val = EvalFuncAt(x);
        values[i] = val; // fabsf(params->outCap) > 0.01 ? fminf(val, params->outCap) : val;
        if (params->use_anisotropy)
            values_y[i] = val / params->sens * params->sensY;
        x += x_stride;
    }
}

// https://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit
void CachedFunction::SmoothBezier() {
    //bezier_control_points.clear();
    auto& center_points = params->custom_curve_points;
    auto& bezier_control_points = params->custom_curve_control_points;

    int n = center_points.size() - 1;
    if (n < 1) {
        bezier_control_points.clear();
        return;
    }

    if (n == 1) {
        bezier_control_points.resize(2);
        // Special case: Bezier curve should be a straight line.

        // 3P1 = 2P0 + P3
        bezier_control_points[0][0] = (center_points[0] * 2 + center_points[1]) / 3;

        // P2 = 2P1 – P0
        bezier_control_points[0][1] = bezier_control_points[0][0] * 2 - center_points[0];

        return;
    }

    center_points.emplace_back(center_points[n] * 2 - center_points[n-1]); // Push dummy point at the end

    n += 1;
    bezier_control_points.resize(n);

    // Calculate first Bezier control points
    // Right hand side vector
    ImVec2 *rhs = new ImVec2[n];

    // Set right hand side X values
    for (int i = 1; i < n - 1; ++i)
        rhs[i] = center_points[i] * 4 +  center_points[i + 1] * 2;
    rhs[0] = center_points[0] + center_points[1] * 2;
    rhs[n - 1] = (center_points[n - 1] * 8 + center_points[n]) / 2.0;
    // Get first control points

    ImVec2 *x = new ImVec2[n]; // Solution vector.
    ImVec2 *tmp = new ImVec2[n]; // Temp workspace.

    ImVec2 b = {2.0, 2.0};
    x[0] = rhs[0] / b;
    for (int i = 1; i < n; i++) // Decomposition and forward substitution.
    {
        tmp[i] = ImVec2(1, 1) / b;
        b = (i < n - 1 ? ImVec2(4, 4) : ImVec2(3.5, 3.5)) - tmp[i];
        x[i] = (rhs[i] - x[i - 1]) / b;
    }
    for (int i = 1; i < n; i++)
        x[n - i - 1] -= tmp[n - i] * x[n - i]; // Backsubstitution.


    for (int i = 0; i < n; ++i) {
        // First control point
        if (!center_points[i].is_locked)
            bezier_control_points[i][0] = x[i];
        // Second control point
        if (i == n -1 || !center_points[i+1].is_locked)  {
            if (i < n - 1) {
                bezier_control_points[i][1] = center_points[i + 1] * 2 - x[i + 1];
            }
            else
                bezier_control_points[i][1] = center_points[n] + x[n - 1] / 2;
        }
    }

    center_points.pop_back(); // Pop the back dummy
    bezier_control_points.pop_back();

    params->ApplyCurveConstraints();

    delete[] rhs;
    delete[] x;
    delete[] tmp;
}
