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
#include <set>

#include "External/ImGui/imgui_internal.h"
#include "External/ImGui/implot.h"

#define YEETMOUSE_PARAMS_DIR "/sys/module/yeetmouse/parameters/"

template<typename Ty>
bool GetParameterTy(const std::string& param_name, Ty &value) {
    try {
        using namespace std;
        ifstream file(YEETMOUSE_PARAMS_DIR + param_name);

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

bool GetParameterTy(const std::string& param_name, std::string &value) {
    try {
        using namespace std;
        ifstream file(YEETMOUSE_PARAMS_DIR + param_name);

        if(file.bad())
            return false;

        std::stringstream ss;
        ss << file.rdbuf();
        value = ss.str();
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
        ofstream file(YEETMOUSE_PARAMS_DIR + param_name);

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

        for (const auto & entry : fs::directory_iterator(YEETMOUSE_PARAMS_DIR)) {
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
            auto dir = fs::directory_entry(YEETMOUSE_PARAMS_DIR);
            if(!dir.exists())
                return false;
        }
        catch (std::exception &ex){
            return false;
        }

        return true;
    }

    size_t ParseUserLutData(char *szUser_data, double* out_x, double* out_y, size_t out_size) {
        if(!szUser_data) {
            strcpy(szUser_data, "Bad data pointer\0");
            return 0;
        }

        std::stringstream ss(szUser_data);
        size_t idx = 0;

        // Skip 2 equal pairs (it would cause kernel to panic...)
        std::set<double> visited_x;
        bool was_last_x_dup = false;

        try {
            double p = 0;
            while (idx < out_size * 2 && ss >> p) {
                if (idx % 2 == 1 && was_last_x_dup && out_y[(idx - 2) / 2] == p) {
                    idx--;
                    was_last_x_dup = false;
                    int skipped = 0;
                    char nextC = ss.peek();
                    while (nextC == ',' || nextC == ';' || (skipped > 0 && isspace(nextC))) {
                        ss.ignore();
                        nextC = ss.peek();
                        skipped++;
                    }
                    continue;
                }

                was_last_x_dup = false;
                if (idx % 2 == 0) {
                    if (visited_x.find(p) != visited_x.end())
                        was_last_x_dup = true;
                    else
                        visited_x.insert(p);
                }
                ((idx % 2 == 0) ? out_x : out_y)[idx++ / 2] = p;

                //((idx % 2 == 0) ? out_x : out_y)[idx++ / 2] = p;

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
            if (idx <= 2 || idx % 2 == 1) {
                strcpy(szUser_data, "Not enough values or bad formatting\0");
                return 0;
            }

            // Make sure all the data was parsed, if not then return 0
            if (!ss.eof()) {
                sprintf(szUser_data, "Too many samples! (%zu max)", out_size);
                fprintf(stderr, "Too many samples! (%zu max)\n", out_size);
                return 0;
            }

            // Zip the X and Y values for sorting
            std::pair<double, double> pairs[MAX_LUT_ARRAY_SIZE];
            for (int i = 0; i < idx / 2; i++)
                pairs[i] = std::make_pair(out_x[i], out_y[i]);

            // Sort the values together (according to X). While preserving the ordering in case of equal X values
            std::sort(pairs, pairs + idx / 2,
                      [](std::pair<double, double> a, std::pair<double, double> b) { return a.first < b.first; });

            // Unzip
            for (int i = 0; i < idx / 2; i++) {
                out_x[i] = pairs[i].first;
                out_y[i] = pairs[i].second;
            }

            return idx / 2;
        }
        catch (std::exception& ex) {
            printf("Error parsing user LUT data: %s\n", ex.what());
            return 0;
        }
    }

    size_t ParseDriverLutData(const char *szUser_data, double* out_x, double* out_y) {
        std::stringstream ss(szUser_data);
        size_t idx = 0;

        double p = 0;
        while(idx < MAX_LUT_ARRAY_SIZE * 2 && ss >> p) {
            //printf("idx = %zu, p = %f\n", idx, p);
            (idx % 2 == 0 ? out_x : out_y)[idx++/2] = p;

            char nextC = ss.peek();
            if (nextC == ';')
                ss.ignore();

            //idx++;
        }

        // 1 element is not enough for a linear interpolation
        if(idx <= 2 || idx % 2 == 1) {
            return 0;
        }

        return idx / 2;
    }

    std::string EncodeLutData(double *data_x, double *data_y, size_t size) {
        std::string res;

        for(int i = 0; i < size * 2; i++) {
                res += std::to_string(i % 2 == 0 ? data_x[i/2] : data_y[i/2]) + ";";
        }

        return res;
    }
} // DriverHelper

//Parameters::Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent,
//                       float midpoint, float scrollAccel, int accelMode) : sens(sens), outCap(sensCap),
//                                                                           inCap(speedCap), offset(offset),
//                                                                           accel(accel), exponent(exponent),
//                                                                           midpoint(midpoint), scrollAccel(scrollAccel),
//                                                                           accelMode(accelMode) {}


void Parameters::ApplyCurveConstraints() {
    for (int i = 0; i < custom_curve_points.size(); i++) {
        float p_min = i > 0 ? custom_curve_points[i-1].x + 0.5f : 0;
        float p_max = i < custom_curve_points.size() - 1 ? custom_curve_points[i+1].x - 0.5f : 1000;

        custom_curve_points[i].x = std::clamp(custom_curve_points[i].x, p_min, p_max);

        if (i < custom_curve_points.size() - 1)
            custom_curve_control_points[i][0].x = std::clamp(custom_curve_control_points[i][0].x,
                custom_curve_points[i].x + CURVE_POINTS_MARGIN, custom_curve_points[i+1].x - CURVE_POINTS_MARGIN);
        if (i > 0)
            custom_curve_control_points[i-1][1].x = std::clamp(custom_curve_control_points[i-1][1].x,
                custom_curve_points[i-1].x + CURVE_POINTS_MARGIN,  custom_curve_points[i].x - CURVE_POINTS_MARGIN);
    }
}

// Cubic Bezier derivatives
ImVec2 BezierFirstOrderDerivative(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
    float u = (1-t);
    float w1 = 3 * (u * u);
    float w2 = 6 * (u * t);
    float w3 = 3 * (t * t);
    return ((p1-p0) * w1) + ((p2-p1) * w2) + ((p3-p2) * w3);
}

ImVec2 BezierSecondOrderDerivative(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
    float u = (1-t);
    float w1 = 6 * u;
    float w2 = 6 * t;
    return (p2-p1*2+p0) * w1 + (p3-p2*2+p1) * w2;
}

// Spreads points based on the rate of change and other things
// Sorry for the shear number of magic numbers in this function, there is just a lot to configure
void Parameters::ExportCurveToLUT() {
    const float CURVE_WEIGHT = 0.2;
    const float TIME_ADAPTIVE_FACTOR = 0.5;
    const int PRE_LUT_ARRAY_SIZE = 100;
    const float LENGTH_WEIGHT = 0.9;

    if (custom_curve_points.size() <= 1)
        return;

    auto& points = custom_curve_points;
    auto& control_points = custom_curve_control_points;
    LUT_size = 0;

    //std::vector<ImVec2> temp_LUT((custom_curve_points.size() - 1) * PRE_LUT_ARRAY_SIZE);
    std::vector<float> curve_arc_len((custom_curve_points.size() - 1) * PRE_LUT_ARRAY_SIZE);
    size_t idx = 0;
    std::vector<double> seg_len(custom_curve_points.size() - 1); // normalized
    //std::vector<double> seg_curviness(custom_curve_points.size() - 1);
    double len_sum = 0;
    double curv_sum = 0;
    // Pre calculate each segments' length
    auto last_p = points[0];
    for (int i = 0; i < custom_curve_points.size() - 1; i++) {
        auto last_dp = BezierSecondOrderDerivative(points[i], control_points[i][0], control_points[i][1], points[i+1], 0);
        seg_len[i] = 0;
        //temp_LUT[idx] = points[i];
        curve_arc_len[idx++] = 0;
        for (int j = 1; j < PRE_LUT_ARRAY_SIZE; j++) {
            const double t = j / (PRE_LUT_ARRAY_SIZE - 1.0);
            ImVec2 p = ImBezierCubicCalc(points[i], control_points[i][0], control_points[i][1], points[i+1], t);
            ImVec2 dp = BezierSecondOrderDerivative(points[i], control_points[i][0], control_points[i][1], points[i+1], t);
            //temp_LUT[idx] = p;
            //p.y *= 10;
            // double len = sqrt(x_diff * x_diff + y_diff * y_diff);
            // seg_len[i] += len;
            float dp_rate_of_change = std::fabs((p.y - last_p.y) / (t - ((j - 1) / (PRE_LUT_ARRAY_SIZE - 1.0))));
            ImVec2 dist = p - last_p;
            dist.y *= 30; // The graph is stretched horizontally, this accounts for that
            curve_arc_len[idx] = curve_arc_len[idx-1] + std::sqrt(ImLengthSqr(dist));
            seg_len[i] += std::sqrt(ImLengthSqr(dist));
            //seg_curviness[i] += sqrt(dp_rate_of_change);
            last_p = p;
            last_dp = dp;
            // last_p_x = p_x;
            // last_p_y = p_y;
            idx++;
        }
        len_sum += seg_len[i];
        //curv_sum += seg_curviness[i];
    }

    double len_mean = len_sum / (points.size() - 1);
    // Normalize lengths [0,1]
    for (auto& seg : seg_len)
        seg = seg / len_sum * (points.size() - 1);

    // for (auto& seg : seg_curviness) {
    //     //seg /= curv_sum;
    //     //printf("%f\t", seg);
    // }
    // printf("\n");

    // Linearize Bezier's domain (http://www.planetclegg.com/projects/WarpingTextToSplines.html)
    auto linear_map_t = [&curve_arc_len](float tg_arc_len, size_t start) {
        int l = 0, r = PRE_LUT_ARRAY_SIZE - 1;
        int index = 0;
        while (l < r) {
            index = (l + r) / 2;
            if (curve_arc_len[start + index] < tg_arc_len) {
                l = index + 1;
            } else {
                r = index;
            }
        }
        if (curve_arc_len[start + index] > tg_arc_len) {
            index--;
        }

        float lengthBefore = curve_arc_len[start + index];
        if (lengthBefore == tg_arc_len) {
            return (float)index / (PRE_LUT_ARRAY_SIZE-1);
        }

        // interpolate
        return (index + (tg_arc_len - lengthBefore) / (curve_arc_len[start + index + 1] - lengthBefore)) / (PRE_LUT_ARRAY_SIZE-1);
    };

    // Using premade LUT
    double u = 0; // [0,1]
    constexpr double U_STEP_FACTOR = 0.1;
    constexpr double u_step = U_STEP_FACTOR / (MAX_LUT_ARRAY_SIZE - 1);
    last_p = {0,0};
    ImVec2 last_dp = ImVec2(0, 0);
    double last_step = u_step;
    float last_rate_of_change = 0;
    float last_t = 0;
    float last_added_u = 0;
    double target_u = u_step; // Point at which to actually add a point (dynamically changes)
    for (int i = 0; LUT_size < (MAX_LUT_ARRAY_SIZE-1) && u < 1.f; i++) {
        float t = 0;
        //u = ((float)i / (MAX_LUT_ARRAY_SIZE - 1));

        int edge_idx = (int)(u * (points.size() - 1));

        float local_u = u * (points.size() - 1) - edge_idx;
        //printf("u = %f, edge_idx = %d, local_u = %f\n", u, edge_idx, local_u);
        float tg_arc_len = local_u * curve_arc_len[(edge_idx + 1) * PRE_LUT_ARRAY_SIZE - 1];
        //float tg_arc_len = u * curve_arc_len.back();

        //u_step = 1.0 / MAX_LUT_ARRAY_SIZE / ImLerp(1., seg_len[edge_idx], 0.8);

        int start = edge_idx * PRE_LUT_ARRAY_SIZE;
        t = linear_map_t(tg_arc_len, start);

        auto p = ImBezierCubicCalc(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t);

        float tg_arc_len_1 = (local_u + (last_step * (points.size() - 1))) * curve_arc_len[(edge_idx + 1) * PRE_LUT_ARRAY_SIZE - 1];
        float tg_arc_len_2 = (local_u + 0.01) * curve_arc_len[(edge_idx + 1) * PRE_LUT_ARRAY_SIZE - 1];
        float t_1 = linear_map_t(tg_arc_len_1, start);
        float t_2 = linear_map_t(tg_arc_len_2, start);
        //printf("t = %f, t_1 = %f, t_2 = %f, pos = (%f, %f)\n", t, t_1, t_2, p.x, p.y);
        auto p_1 = ImBezierCubicCalc(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t_1);
        auto p_2 = ImBezierCubicCalc(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t_2);

        auto dp = BezierFirstOrderDerivative(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t);
        auto dp_1 = BezierFirstOrderDerivative(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t_1);
        auto dp_2 = BezierFirstOrderDerivative(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t_2);

        auto d2p = BezierSecondOrderDerivative(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t);
        auto d2p_1 = BezierSecondOrderDerivative(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1], points[edge_idx+1], t);

        //auto rate_of_change = ((dp_1.y - dp.y) - (dp_2.y - dp_1.y)) * 20;
        // rate of change = curvature of the curve at time t
        //float rate_of_change = std::fabs((dp_1.y - dp.y) / (p_1.x - p.x)) * 8;
        float rate_of_change = 1;
        if (i > 0) {
            float first_space_derivative = (p_1.y - p.y) / (p_1.x - p.x);
            float second_space_derivative = (p_1.y - 2 * p.y + last_p.y) / ((p_1.x - p.x) * (p.x - last_p.x));
            //rate_of_change = std::sqrt(std::fabs((p_1.y - p.y) / (p_1.x - p.x) - ((p_2.y - p_1.y) / (p_2.x - p_1.x))) / (p_2.x - p.x) * 12); // Forward only
            //rate_of_change = std::fabs((p_1.y - p.y) / (last_step) - ((p.y - last_p.y) / (last_step))) / (p_2.x - p.x); // Combined - good?
            //rate_of_change = std::fabs(dp_1.y);
            //rate_of_change = std::fabs((p_1.y - p.y) * (t_1 - t)) * 200 * seg_len[edge_idx];
            //rate_of_change = std::fabs(d2p.y) / std::pow(1 + dp.y * dp.y, 3.0/2.0); // Curvature with respect to t
            //rate_of_change = std::fabs(d2p.y) / std::pow(1 + dp.y * dp.y, 3.0/2.0) / 10; // Curvature with the same respect (to mathematicians), just divided by 10...
            rate_of_change = std::sqrt(std::fabs(dp.x * d2p.y - dp.y * d2p.x) / std::pow(dp.y * dp.y*200 + dp.x * dp.x, 3.0/2.0) * 250); // Curvature with respect to t, but good
            //rate_of_change = 8*std::sqrt(std::fabs((p_1.y - p.y) / (p_1.x - p.x) - ((p.y - last_p.y) / (p.x - last_p.x))) * (((p.x - last_p.x) + (p_1.x - p.x)) / 2)); // Just space
            //rate_of_change = std::fabs((p_1.y - p.y) / (p_1.x - p.x) - ((p.y - last_p.y) / (p.x - last_p.x))) / pow(t_1 - t, 1) / 50; // Space and time aware 2 order
            //rate_of_change = std::pow(std::fabs((p_1.y - p.y) / (t_1 - t) - (p.y - last_p.y) / (t - last_t)) / (t - last_t) * 2, 2) / 4; // space non-linear
            //rate_of_change = std::fabs(p_1.y - 2 * p.y + last_p.y) / ((p_1.x - p.x) * (p.x - last_p.x)); // ?
        }
        //float rate_of_change_2 = std::sqrt(std::fabs(rate_of_change - last_rate_of_change) / (last_step * (points.size() - 1)) / 100);
        float rate_of_change_clamped = ImClamp(rate_of_change, 0.f, 1.0f);
        float rate_of_change_half_clamped = ImClamp(rate_of_change, 0.f, 2.f);
        //printf("rate_of_change = %f\n", rate_of_change);

        // use target_t instead?
        if (u + 0.000002 >= target_u) {
            LUT_data_x[LUT_size] = p.x;
            LUT_data_y[LUT_size] = p.y;

            LUT_size++;

            last_added_u = u;
        }
        last_p = p;
        last_dp = dp;
        last_rate_of_change = rate_of_change;
        last_t = t;

        double og_u = u;
        if (t <= 0.0001)
            u = (double)i / (MAX_LUT_ARRAY_SIZE - 1) * U_STEP_FACTOR + u_step; // recalibrate position
        else
            u += u_step;
        //u += ImLerp(u_step * 5, u_step, rate_of_change_clamped);

        // Adaptive points distribution
        // Function f that determines how much points to put on a given edge depending on how close (slow) it is.
        // This function (f) has to satisfy the following statement Integral{0->1}(f(x)dx) = 1
        // For example f(x) = ax + (1-0.5a)
        double adaptive_time_step = (TIME_ADAPTIVE_FACTOR * p.x / points.back().x + (1.f - TIME_ADAPTIVE_FACTOR*0.5f)) * u_step;
        //double next_u_step = ((u_step)/seg_len[edge_idx] / (points.size() - 1));//ImClamp( -1.0 / seg_len[edge_idx] - rate_of_change_half_clamped, 0.0, 1.0);
        //double next_u_step = ((((TIME_ADAPTIVE_FACTOR * p.x / points.back().x + (1.f - TIME_ADAPTIVE_FACTOR*0.5f)) * u_step) + ((u_step)/seg_len[edge_idx] / (points.size() - 1))) / 2);
        double next_u_step = adaptive_time_step / seg_len[edge_idx];
        // float next_u_step_factor = (TIME_ADAPTIVE_FACTOR * p.x / points.back().x + (1.f - TIME_ADAPTIVE_FACTOR*0.5f)) / ImLerp(1., seg_len[edge_idx], LENGTH_WEIGHT) * rate_of_change_half_clamped;
        //printf("next_u_step_factor = %f\n", next_u_step_factor);
        target_u = last_added_u + ImLerp(next_u_step * 4, next_u_step*0.9, rate_of_change_clamped) / U_STEP_FACTOR;
        //target_u = last_added_u + next_u_step / U_STEP_FACTOR;
        // Clamp u to the start of the next point [u = edge / (size-1)]
        if (u + 0.000002 > (edge_idx + 1.0f) / (points.size() - 1)) {
            target_u = (edge_idx + 1.0f) / (points.size() - 1);
            u = target_u + 0.00002;
            last_t = 0;
        }
        last_step = u-og_u;
    }

    LUT_data_x[LUT_size] = points.back().x;
    LUT_data_y[LUT_size++] = points.back().y;

    // Using direct bezier curves
    // ImVec2 last_point = points[0];
    // for (int i = 0; i < points.size() - 1; i++) {
    //     float rate_of_change_multi = 1;
    //     float t = 0;
    //     const int BEZIER_SEGMENTS_PER_EDGE = ImLerp(seg_len[i], seg_curviness[i] / curv_sum, CURVE_WEIGHT) * (MAX_LUT_ARRAY_SIZE-LUT_size-1);
    //     float segment_step = 1.0 / BEZIER_SEGMENTS_PER_EDGE;
    //     float last_t = -segment_step;
    //     ImVec2 last_dp = BezierFirstOrderDerivative(points[i], control_points[i][0], control_points[i][1], points[i + 1], 0);
    //     ImVec2 last_d2p = BezierSecondOrderDerivative(points[i], control_points[i][0], control_points[i][1], points[i + 1], 0);
    //     float last_rate_of_change = 0;
    //     for (int j = 0; t < 1; ++j) {
    //         ImVec2 dp = BezierFirstOrderDerivative(points[i], control_points[i][0], control_points[i][1], points[i+1], t);
    //         ImVec2 d2p = BezierSecondOrderDerivative(points[i], control_points[i][0], control_points[i][1], points[i+1], t) / 10;
    //         ImVec2 p = ImBezierCubicCalc(points[i], control_points[i][0], control_points[i][1], points[i+1], t);
    //         ImVec2 p_1 = ImBezierCubicCalc(points[i], control_points[i][0], control_points[i][1], points[i+1], t + segment_step);
    //         ImVec2 dp_1 = BezierFirstOrderDerivative(points[i], control_points[i][0], control_points[i][1], points[i+1], t + segment_step);
    //         // float x = (1-t)*(1-t)*(1-t)*points[i].x + 3*(1-t)*(1-t)*t*control_points[i][0].x + 3*(1-t)*t*t*control_points[i][1].x + t*t*t*points[i+1].x;
    //         float dp_rate_of_change = std::fabs((dp.y - dp_1.y) / segment_step);
    //         if (!isnan(dp_rate_of_change))
    //             rate_of_change_multi = std::clamp(dp_rate_of_change, 0.0f, 1.0f);
    //         //printf("rate_of_change_multi: %f\n", rate_of_change_multi);
    //         LUT_data_x[LUT_size] = p.x;
    //         LUT_data_y[LUT_size] = dp_rate_of_change;
    //         if (last_point.x > p.x) {
    //             printf("Bad Curve!\n");
    //             LUT_size = 0; // Bad Curve
    //             return;
    //         }
    //         last_dp = dp;
    //         last_d2p = d2p;
    //         last_point = p;
    //         LUT_size++;
    //
    //         last_rate_of_change = dp_rate_of_change;
    //
    //         last_t = t;
    //         //t+=segment_step;
    //         t += ImLerp(segment_step * 5.f, segment_step * 1.f, rate_of_change_multi);
    //
    //         // Adaptive points distribution
    //         // Function f that determines how much points to put on a given edge depending on how close (slow) it is.
    //         // This function (f) has to satisfy the following statement Integral{0->1}(f(x)dx) = 1
    //         // For example f(x) = ax + (1-0.5a)
    //         //segment_step = (TIME_ADAPTIVE_FACTOR * p.x / points.back().x + (1.f - TIME_ADAPTIVE_FACTOR*0.5f)) * 1.0f / BEZIER_SEGMENTS_PER_EDGE;
    //     }
    // }

    //printf("Lut_size: %d\n", LUT_size);
}

bool Parameters::SaveAll() {
    bool res = true;

    // General
    res &= SetParameterTy("Sensitivity", sens);
    res &= SetParameterTy("SensitivityY", use_anisotropy ? sensY : sens);
    res &= SetParameterTy("OutputCap", outCap);
    res &= SetParameterTy("InputCap", inCap);
    res &= SetParameterTy("Offset", offset);
    res &= SetParameterTy("AccelerationMode", accelMode);
    res &= SetParameterTy("RotationAngle", rotation * DEG2RAD);
    res &= SetParameterTy("AngleSnap_Threshold", as_threshold * DEG2RAD);
    res &= SetParameterTy("AngleSnap_Angle", as_angle * DEG2RAD);

    // Specific
    res &= SetParameterTy("Acceleration", accel);
    res &= SetParameterTy("Exponent", exponent);
    res &= SetParameterTy("Midpoint", midpoint);
    res &= SetParameterTy("PreScale", preScale);
    res &= SetParameterTy("UseSmoothing", useSmoothing);

    // LUT
    auto encodedLutData = DriverHelper::EncodeLutData(LUT_data_x, LUT_data_y, LUT_size);
    if(!encodedLutData.empty()) {
        res &= SetParameterTy("LutSize", LUT_size);
        //res &= SetParameterTy("LutStride", LUT_stride);
        //printf("encoded: %s, size: %zu, stride: %i\n", encoded.c_str(), LUT_size, LUT_stride);
        res &= SetParameterTy("LutDataBuf", encodedLutData);
    }
    else if(accelMode == 6)
        return false;

    if(res)
        res &= DriverHelper::SaveParameters();

    return res;
}
