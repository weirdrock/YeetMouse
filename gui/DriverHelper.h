#ifndef YEETMOUSE_DRIVERHELPER_H
#define YEETMOUSE_DRIVERHELPER_H

#include <cmath>
#include <utility>
#include <vector>
#include <string>

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <filesystem>

#define MAX_LUT_ARRAY_SIZE 128  // THIS NEEDS TO BE THE SAME AS IN THE DRIVER CODE

#define DEG2RAD (M_PI / 180.0)

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

    // Handle Devices
    inline bool BindToDriver(const char* driver_name, std::string device_id) {
        auto f_path = std::string("/sys/bus/usb/drivers/") + driver_name + "/bind";
        std::ofstream file(std::string("/sys/bus/usb/drivers/") + driver_name + "/bind");
        if(file.bad())
            return false;

        bool write_status = (bool)(file << device_id);
        if(!write_status)
            return false;

        file.close();

        usleep(10000);

        // Check if the device was actually bound
        return std::filesystem::is_directory((std::string("/sys/bus/usb/drivers/") + driver_name + "/" + device_id));
    }

    inline bool UnBindFromDriver(const char* driver_name, std::string device_id) {
        auto f_path = std::string("/sys/bus/usb/drivers/") + driver_name + "/unbind";
        std::ofstream file(std::string("/sys/bus/usb/drivers/") + driver_name + "/unbind");
        if(file.bad())
            return false;

        bool write_status = (bool)(file << device_id);
        if(!write_status)
            return false;

        file.close();

        usleep(10000);

        // Check if the device was actually unbound
        return !std::filesystem::is_directory((std::string("/sys/bus/usb/drivers/") + driver_name + "/" + device_id));
    }

    struct DeviceInfo {
        std::string full_name;
        std::string name;
        std::string driver_name;
        std::string manufacturer;
        std::string device_id; //<controller>-<ports>:<configuration>-<interface>
        std::string manufacturer_id;
        std::string max_power;

        int interfaceClass = 0;
        int interfaceProtocol    = 0;
        int interfaceSubClass = 0;

        DeviceInfo() = default;
        explicit DeviceInfo(std::string id): device_id(std::move(id)) {};

        bool is_bound_to_leetmouse = false;
    };

    std::vector<DeviceInfo> DiscoverDevices();

} // DriverHelper

inline const char* interfaceClass2String(int interfaceClass) {
    switch (interfaceClass) {
        case 3:
            return "HID";
        case 1:
            return "Audio Interface";
        default:
            return "Other";
    }
}

inline const char* interfaceSubClass2String(int interfaceSubClass) {
    switch (interfaceSubClass) {
        case 0:
            return "No Subclass";
        case 1:
            return "Boot interface";
        default:
            return "Other";
    }
}

inline const char* interfaceProtocol2String(int interfaceProtocol, int interfaceClass = 0) {
    if(interfaceClass == 3) {
        switch (interfaceProtocol) {
            case 1:
                return "Keyboard";
            case 2:
                return "Mouse";
            default:
                return "Other";
        }
    }
    else {
        return "Other";
    }
}

// This is awfully slow btw.
inline std::string decodeVendorId(const std::string& vendorId) {
    std::ifstream file("/usr/share/hwdata/usb.ids");
    if (!file.is_open()) {
        return "Unknown";
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find('#') == 0) {
            continue; // skip comments
        }

        std::string id = line;
        if (id.find(vendorId) == 0) {
            return line.substr(line.find("  ") + 2);
        }
    }

    return "Unknown";
}

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
    float rotation = 0; // Stored in degrees, converted to radians when writing out

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
