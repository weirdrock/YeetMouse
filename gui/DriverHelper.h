#ifndef YEETMOUSE_DRIVERHELPER_H
#define YEETMOUSE_DRIVERHELPER_H

#include "string"

namespace DriverHelper {
    bool GetParameterF(const std::string& param_name, float& value);
    bool GetParameterI(const std::string& param_name, int& value);
    bool GetParameterB(const std::string& param_name, bool& value);

    bool WriteParameterF(const std::string& param_name, float value);
    bool WriteParameterI(const std::string& param_name, float value);

    bool SaveParameters();

    bool ValidateDirectory();

    /// Converts the ugly FP64 representation of user parameters to nice floating point values.\n\n
    bool CleanParameters(int& fixed_num);

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

    Parameters() = default;

    //Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent, float midpoint,
    //           float scrollAccel, int accelMode);

    bool SaveAll();
};

#endif //YEETMOUSE_DRIVERHELPER_H
