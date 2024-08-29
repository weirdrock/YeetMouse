#ifndef YEETMOUSE_DRIVERHELPER_H
#define YEETMOUSE_DRIVERHELPER_H

#include "string"

#define MAX_LUT_ARRAY_SIZE 128  // THIS NEEDS TO BE THE SAME AS IN THE DRIVER CODE

namespace DriverHelper {
    bool GetParameterF(const std::string& param_name, float& value);
    bool GetParameterI(const std::string& param_name, int& value);
    bool GetParameterB(const std::string& param_name, bool& value);
    bool GetParameterS(const std::string& param_name, std::string& value);

    bool WriteParameterF(const std::string& param_name, float value);
    bool WriteParameterI(const std::string& param_name, float value);

    bool SaveParameters();

    bool ValidateDirectory();

    /// Converts the ugly FP64 representation of user parameters to nice floating point values.\n\n
    bool CleanParameters(int& fixed_num);

    /// Returns the number of parsed values
    size_t ParseUserLutData(char* user_data, double* out_x, double* out_y, size_t out_size);

    /// Returns the number of parsed values
    size_t ParseDriverLutData(const char* user_data, double* out_x, double* out_y);

} // DriverHelper

struct Parameters {
    float sens = 1.0f;
    float outCap = 0.f;
    float inCap = 0.f;
    float offset = 0.0f;
    float accel = 2.0f;
    float exponent = 0.4f;
    float midpoint = 5.0f;
    float preScale = 1.0f;
    float scrollAccel = 1.0f;
    int accelMode = 0;
    bool useSmoothing = true; // true/false

    /// The issue of performance with LUT is currently solved with a fixed stride, but another approach would be to
    /// store separately x and y values, sort both by x values, and do a binary search every time you want to find points.
    /// ----- The so called "another approach" has been implemented -----
    //float LUT_stride = 1.f; // No longer used
    double LUT_data_x[MAX_LUT_ARRAY_SIZE];
    double LUT_data_y[MAX_LUT_ARRAY_SIZE];
    int LUT_size = 0;

    Parameters() = default;

    //Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent, float midpoint,
    //           float scrollAccel, int accelMode);

    bool SaveAll();
};

#endif //YEETMOUSE_DRIVERHELPER_H
