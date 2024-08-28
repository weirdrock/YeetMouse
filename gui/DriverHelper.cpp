#include "DriverHelper.h"
#include "External/FixedMath/Fixed64.h"
//#include "../libfixmath/"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <sstream>

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

    size_t ParseUserLutData(char *szUser_data, double* out, size_t out_size) {
        std::stringstream ss(szUser_data);
        size_t idx = 0;

        double p = 0;
        while(idx < out_size && ss >> p) {
            out[idx++] = p;

            char nextC = ss.peek();
            if (nextC == ',' || nextC == ';')
                ss.ignore();
        }

        // 1 element is not enough for a linear interpolation
        if(idx <= 1) {
            strcpy(szUser_data, "Not enough values or bad formatting\0");
            return 0;
        }

        // Make sure all the data was parsed, if not then return 0
        if(!ss.eof()) {
            strcpy(szUser_data, "Too many samples\0");
            sprintf(szUser_data, "Too many samples! (%zu max)", out_size);
            fprintf(stderr, "Too many samples! (%zu max)\n", out_size);
            return 0;
        }

        return idx;
    }

    size_t ParseDriverLutData(const char *szUser_data, double* out) {
        std::stringstream ss(szUser_data);
        size_t idx = 0;

        double p = 0;
        while(idx < MAX_LUT_ARRAY_SIZE && ss >> p) {
            out[idx++] = p;

            char nextC = ss.peek();
            if (nextC == ';')
                ss.ignore();
        }

        // 1 element is not enough for a linear interpolation
        if(idx <= 1) {
            return 0;
        }

        return idx;
    }
} // DriverHelper

//Parameters::Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent,
//                       float midpoint, float scrollAccel, int accelMode) : sens(sens), outCap(sensCap),
//                                                                           inCap(speedCap), offset(offset),
//                                                                           accel(accel), exponent(exponent),
//                                                                           midpoint(midpoint), scrollAccel(scrollAccel),
//                                                                           accelMode(accelMode) {}

std::string EncodeLutData(double* data, size_t size) {
    std::string res;

    for(int i = 0; i < size; i++) {
        res += std::to_string(data[i]) + ";";
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

    // LUT
    auto encodedLutData = EncodeLutData(LUT_data, LUT_size);
    if(!encodedLutData.empty()) {
        res &= SetParameterTy("LutSize", LUT_size);
        res &= SetParameterTy("LutStride", LUT_stride);
        //printf("encoded: %s, size: %zu, stride: %i\n", encoded.c_str(), LUT_size, LUT_stride);
        res &= SetParameterTy("LutDataBuf", encodedLutData);
    }
    else
        return false;

    DriverHelper::SaveParameters();

    return res;
}
