#include "DriverHelper.h"
#include "External/FixedMath/Fixed64.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

#define LEETMOUSE_PARAMS_DIR "/sys/module/leetmouse/parameters/"

template<typename Ty>
bool GetParameterTy(const std::string& param_name, Ty &value) {
    try {
        using namespace std;
        ifstream file(LEETMOUSE_PARAMS_DIR + param_name);

        if(file.bad())
            return false;

        file >> value;
        file.close();
        return true;
    }
    catch (std::exception& ex) {
        fprintf(stderr, "Error when reading parameter %s (%s)\n", param_name.c_str(), ex.what());
        return false;
    }
}

template<typename Ty>
bool SetParameterTy(const std::string& param_name, Ty value) {
    try {
        using namespace std;
        ofstream file(LEETMOUSE_PARAMS_DIR + param_name);

        if (file.bad())
            return false;

        file << value;
        file.close();
        return true;
    }
    catch (std::exception& ex) {
        fprintf(stderr, "Error when saving parameter %s (%s)\n", param_name.c_str(), ex.what());
        return false;
    }
}

namespace DriverHelper {

    bool GetParameterF(const std::string& param_name, float &value) {
        return GetParameterTy(param_name, value);
    }

    bool GetParameterI(const std::string &param_name, int &value) {
        return GetParameterTy(param_name, value);
    }

    bool GetParameterB(const std::string& param_name, bool &value) {
        int temp = 0;
        bool res = GetParameterTy(param_name, temp);
        value = temp == 1;
        return res;
    }

    bool GetParameterS(const std::string &param_name, std::string &value) {
        return GetParameterTy(param_name, value);
    }

    bool CleanParameters(int& fixed_num) {
        namespace fs = std::filesystem;

        for (const auto & entry : fs::directory_iterator(LEETMOUSE_PARAMS_DIR)) {
            std::string str;
            std::ifstream t(entry.path());
            if(!t.is_open() || t.bad() || t.fail())
                return false;
            std::stringstream buffer;
            buffer << t.rdbuf();
            str = buffer.str();
            //printf("param at %s = %s\n", entry.path().c_str(), str.c_str());

            //std::streampos size = t.tellg();
            //std::cout << "pos = " << size << std::endl;
            //t.clear();
            //t.seekp(0);
            // I assume this is enough to not leave behind some parts of the old values if the new ones are shorter
            t.close();

            try {
                // Integer written with FP64_Shift
                if (size_t bracket_pos = str.find('('), ll_pos = str.find("ll");
                    str.find("<< 32") != std::string::npos &&bracket_pos != std::string::npos && ll_pos != std::string::npos)
                {
                    fixed_num++;
                    std::ofstream o(entry.path());
                    if(!o.is_open() || o.bad() || o.fail())
                        return false;
                    std::string int_str = str.substr(bracket_pos + 1, ll_pos - bracket_pos - 1);
                    //printf("Clean param: %s\n", int_str.c_str());
                    o.write(int_str.c_str(), int_str.size());
                    o.close();
                }
                else if (ll_pos != std::string::npos) { // Floating point represented as a long long
                    fixed_num++;
                    size_t start_offset = bracket_pos == std::string::npos ? 0 : (bracket_pos + 1);
                    std::ofstream o(entry.path());
                    if(!o.is_open() || o.bad() || o.fail())
                        return false;
                    std::string int_str = str.substr(start_offset, ll_pos - start_offset);
                    FP_LONG fp_val = std::stoll(int_str);
                    char buf[24];
                    FP64_ToString(fp_val, buf, 6);
                    //printf("Clean param: %s, which is %s\n", int_str.c_str(), buf);
                    o.write(buf, strlen(buf));
                    o.close();
                }
                else { // Anything else is either 0 or not meant to be a floating point
                    //printf("Wrong format \\;\n");
                }
            }
            catch (const std::exception& ex) {
                fprintf(stderr, "Error parsing parameter %s!\n", entry.path().filename().c_str());
                return false;
            }

        }

        // Save the new (clean) parameters. Nothing should change, it just looks nicer.
        SaveParameters();

        return true;
    }

    bool SaveParameters() {
        return SetParameterTy("update", (int)1);
    }

    bool WriteParameterF(const std::string &param_name, float value) {
        return SetParameterTy(param_name, value);
    }

    bool WriteParameterI(const std::string &param_name, float value) {
        return SetParameterTy(param_name, value);
    }

    bool ValidateDirectory() {
        namespace fs = std::filesystem;
        try {
            auto dir = fs::directory_entry(LEETMOUSE_PARAMS_DIR);
            if(!dir.exists())
                return false;
        }
        catch (std::exception &ex){
            return false;
        }

        return true;
    }

    size_t ParseUserLutData(char *szUser_data, double* out_x, double* out_y, size_t out_size) {
        std::stringstream ss(szUser_data);
        size_t idx = 0;

        double p = 0;
        while(idx < out_size * 2 && ss >> p) {
            ((idx % 2 == 0) ? out_x : out_y)[idx++ / 2] = p;

            int skipped = 0;
            char nextC = ss.peek();
            while (nextC == ',' || nextC == ';' || (skipped > 0 && isspace(nextC))) {
                ss.ignore();
                nextC = ss.peek();
                skipped++;
            }
        }

        //for(int i = 0; i < idx/2; i++) {
        //    printf("%f, ", out_x[i]);
        //}
        //printf("\n");

        // 1 element is not enough for a linear interpolation
        if(idx <= 2 || idx % 2 == 1) {
            strcpy(szUser_data, "Not enough values or bad formatting\0");
            return 0;
        }

        // Make sure all the data was parsed, if not then return 0
        if(!ss.eof()) {
            sprintf(szUser_data, "Too many samples! (%zu max)", out_size);
            fprintf(stderr, "Too many samples! (%zu max)\n", out_size);
            return 0;
        }

        // Zip the X and Y values for sorting
        std::pair<double, double> pairs[MAX_LUT_ARRAY_SIZE];
        for(int i = 0; i < idx/2; i++)
            pairs[i] = std::make_pair(out_x[i], out_y[i]);

        // Sort the values together (according to X). While preserving the ordering in case of equal X values
        std::sort(pairs, pairs + idx/2, [](std::pair<double, double> a, std::pair<double, double> b) {return a.first < b.first;});

        // Unzip
        for(int i = 0; i < idx/2; i++) {
            out_x[i] = pairs[i].first;
            out_y[i] = pairs[i].second;
        }

        return idx / 2;
    }

    size_t ParseDriverLutData(const char *szUser_data, double* out_x, double* out_y) {
        std::stringstream ss(szUser_data);
        size_t idx = 0;

        double p = 0;
        while(idx < MAX_LUT_ARRAY_SIZE && ss >> p) {
            (idx % 2 == 0 ? out_x : out_y)[idx++/2] = p;

            char nextC = ss.peek();
            if (nextC == ';')
                ss.ignore();
        }

        // 1 element is not enough for a linear interpolation
        if(idx <= 2 || idx % 2 == 1) {
            return 0;
        }

        return idx / 2;
    }
} // DriverHelper

//Parameters::Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent,
//                       float midpoint, float scrollAccel, int accelMode) : sens(sens), outCap(sensCap),
//                                                                           inCap(speedCap), offset(offset),
//                                                                           accel(accel), exponent(exponent),
//                                                                           midpoint(midpoint), scrollAccel(scrollAccel),
//                                                                           accelMode(accelMode) {}

std::string EncodeLutData(double* data_x, double* data_y, size_t size) {
    std::string res;

    for(int i = 0; i < size * 2; i++) {
        res += std::to_string(i % 2 == 0 ? data_x[i/2] : data_y[i/2]) + ";";
    }

    return res;
}

bool Parameters::SaveAll() {
    bool res = true;

    // General
    res &= SetParameterTy("Sensitivity", sens);
    res &= SetParameterTy("OutputCap", outCap);
    res &= SetParameterTy("InputCap", inCap);
    res &= SetParameterTy("Offset", offset);
    res &= SetParameterTy("AccelerationMode", accelMode);

    // Specific
    res &= SetParameterTy("Acceleration", accel);
    res &= SetParameterTy("Exponent", exponent);
    res &= SetParameterTy("Midpoint", midpoint);
    res &= SetParameterTy("PreScale", preScale);
    res &= SetParameterTy("UseSmoothing", useSmoothing);

    res &= SetParameterTy("RotationAngle", rotation * DEG2RAD);

    // LUT
    auto encodedLutData = EncodeLutData(LUT_data_x, LUT_data_y, LUT_size);
    if(!encodedLutData.empty()) {
        res &= SetParameterTy("LutSize", LUT_size);
        //res &= SetParameterTy("LutStride", LUT_stride);
        //printf("encoded: %s, size: %zu, stride: %i\n", encoded.c_str(), LUT_size, LUT_stride);
        res &= SetParameterTy("LutDataBuf", encodedLutData);
    }
    else if(accelMode == 6)
        return false;

    DriverHelper::SaveParameters();

    return res;
}

bool ParseInputDir(std::string path, DriverHelper::DeviceInfo& device) {
    DIR* path_dir = opendir(path.c_str());

    if(path_dir == nullptr)
        return false;

    // Got a level deeper, as there should only be one folder inside the input/ folder,
    // called inputXX/, where Xs are digits

    std::string input_dir;
    struct dirent* ent;
    while ((ent = readdir(path_dir)) != nullptr) {
        if(ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            std::string abs_path = path + "/" + ent->d_name;
            input_dir = abs_path;
            break;
        }
    }

    if(input_dir.empty())
        return false;

    std::ifstream file(input_dir + "/name");
    if(file.bad())
        return false;
    getline(file, device.full_name);
    if(device.full_name.empty())
        return false;

    device.name = device.full_name;

    file.close();

    file = std::ifstream(input_dir + "/id/vendor");
    if(file.bad())
        return false;
    if(!(file >> device.manufacturer_id))
        return false;

    device.manufacturer = decodeVendorId(device.manufacturer_id);
    if(device.manufacturer == "Unknown")
        device.manufacturer = "[" + device.manufacturer_id + "]";

    return true;
}

// Tries to get as much info as possible
void TryParsePhysicalNode(std::string path, DriverHelper::DeviceInfo& device) {
    DIR* path_dir = opendir(path.c_str());

    if(path_dir == nullptr)
        return;

    std::ifstream file = std::ifstream(path + "/manufacturer");
    if(file.good())
        getline(file, device.manufacturer);

    file = std::ifstream(path + "/product");
    if(file.good())
        getline(file, device.name);

    file = std::ifstream(path + "/bMaxPower");
    if(file.good())
        getline(file, device.max_power);
}

std::vector<DriverHelper::DeviceInfo> DriverHelper::DiscoverDevices() {

    // Enumerate all USB devices
    std::vector<std::string> devices_paths;
    std::vector<DriverHelper::DeviceInfo> devices;
    DIR* dir = opendir("/sys/bus/usb/devices");
    if (!dir) {
        std::cerr << "Failed to open /sys/bus/usb/devices" << std::endl;
        return {};
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        //printf("ent = %s\n", ent->d_name);
        if ((ent->d_type == DT_DIR || ent->d_type == DT_LNK) && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            std::string path = std::string("/sys/bus/usb/devices/") + ent->d_name;
            struct stat st;
            if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                devices_paths.emplace_back(ent->d_name);
            }
        }
    }
    closedir(dir);

    //printf("found %zu devices\n", devices_paths.size());

    // Iterate over devices_paths
    for (const auto& device : devices_paths) {
        std::string path = std::string("/sys/bus/usb/devices/") + device;
        int interface_class = -1;

        std::ifstream file(path + "/bInterfaceClass");
        file >> interface_class;
        file.close();

        if(interface_class != 3)
            continue;

        DriverHelper::DeviceInfo device_info{device};
        device_info.interfaceClass = interface_class;

        DIR* input_dir = opendir((path + "/input").c_str());
        if(input_dir) {
            if(!ParseInputDir((path + "/input"), device_info))
                continue;
        }
        else { // Try to look for a specific folder that might contain /input before giving up
            bool was_succ = false;
            DIR* dev_dir = opendir(path.c_str());
            while ((ent = readdir(dev_dir)) != nullptr) {
                if(ent->d_type == DT_DIR) {
                    // look for something like 0000:(a bunch of numbers).(a bunch of numbers)
                    if(isdigit(ent->d_name[0]) && isdigit(ent->d_name[1]) && isdigit(ent->d_name[2]) && strstr(ent->d_name, ":") && strstr(ent->d_name, ".")) {
                        auto inp_path = path + "/" + std::string(ent->d_name) + "/input";
                        was_succ = ParseInputDir(inp_path, device_info);
                        break;
                    }
                }
            }
            if(!was_succ)
                continue;
        }
        closedir(input_dir);

        DIR* driver_dir = opendir((path + "/driver").c_str());
        if(!driver_dir)
            continue;

        // symlink here is something like ../../../../../../../../bus/usb/, so yea...
        std::string driver_name_link = std::filesystem::read_symlink(path + "/driver");
        if(driver_name_link.find("/bus/usb") == std::string::npos)
            device_info.driver_name = "No driver";
        else
            device_info.driver_name = driver_name_link.substr(driver_name_link.find_last_of('/') + 1);

        file = std::ifstream(path + "/bInterfaceSubClass");
        if(file.good())
            file >> device_info.interfaceSubClass;
        file.close();

        file = std::ifstream(path + "/bInterfaceProtocol");
        if(file.good())
            file >> device_info.interfaceProtocol;
        file.close();

        // Look through physical nodes
        DIR* firm_node_dir = opendir((path + "/firmware_node").c_str());
        while ((ent = readdir(firm_node_dir)) != nullptr) {
            if(strstr(ent->d_name, "physical_node")) {
                auto test = path + "/firmware_node/" + ent->d_name;
                TryParsePhysicalNode(path + "/firmware_node/" + ent->d_name, device_info);
            }
        }

        device_info.is_bound_to_leetmouse = device_info.driver_name == "leetmouse";

        devices.push_back(device_info);

        //printf("found a device: %s, name: %s, driver: %s, vendor = %s, protocol = %i\n", device_info.device_id.c_str(), device_info.name.c_str(), device_info.driver_name.c_str(), device_info.manufacturer_id.c_str(), device_info.interfaceProtocol);

        closedir(driver_dir);
    }

    return devices;
}