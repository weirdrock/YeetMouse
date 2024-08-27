#include "DriverHelper.h"
#include "External/FixedMath/Fixed64.h"
//#include "../libfixmath/"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>

#define LEETMOUSE_PARAMS_DIR "/sys/module/leetmouse/parameters/"

template<typename Ty>
bool GetParameterTy(const std::string& param_name, Ty &value) {
    using namespace std;
    ifstream file(LEETMOUSE_PARAMS_DIR + param_name);

    if(file.bad())
        return false;

    file >> value;
    file.close();
    return true;
}

template<typename Ty>
bool SetParameterTy(const std::string& param_name, Ty value) {
    using namespace std;
    ofstream file(LEETMOUSE_PARAMS_DIR + param_name);

    if(file.bad())
        return false;

    file << value;
    file.close();
    return true;
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
} // DriverHelper

//Parameters::Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent,
//                       float midpoint, float scrollAccel, int accelMode) : sens(sens), outCap(sensCap),
//                                                                           inCap(speedCap), offset(offset),
//                                                                           accel(accel), exponent(exponent),
//                                                                           midpoint(midpoint), scrollAccel(scrollAccel),
//                                                                           accelMode(accelMode) {}

bool Parameters::SaveAll() {
    bool res = true;

    res &= SetParameterTy("Sensitivity", sens);
    res &= SetParameterTy("OutputCap", outCap);
    res &= SetParameterTy("InputCap", inCap);
    res &= SetParameterTy("Offset", offset);
    res &= SetParameterTy("Acceleration", accel);
    res &= SetParameterTy("Exponent", exponent);
    res &= SetParameterTy("Midpoint", midpoint);
    res &= SetParameterTy("PreScale", preScale);
    res &= SetParameterTy("AccelerationMode", accelMode);
    res &= SetParameterTy("UseSmoothing", useSmoothing);
    DriverHelper::SaveParameters();

    return res;
}
