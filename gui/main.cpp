#include <cmath>
#include <csignal>
#include "gui.h"
#include "External/ImGui/implot.h"
#include "DriverHelper.h"
#include "FunctionHelper.h"
#include "ImGuiExtensions.h"
#include "chrono"

int selected_mode = 1;

const char* AccelModes[] = {"Current", "Linear", "Power", "Classic", "Motivity", "Jump", "Look Up Table"};
#define NUM_MODES (sizeof(AccelModes) / sizeof(char *))

Parameters params[NUM_MODES]; // Driver parameters for each mode
CachedFunction functions[NUM_MODES]; // Driver parameters for each mode
int used_mode = 1;
bool was_initialized = false;
bool has_privilege = false;

void ResetParameters(void);

int OnGui() {
    using namespace std::chrono;

    static float mouse_smooth = 0.8;
    static steady_clock::time_point last_apply_clicked;

    int hovered_mode = -1;

    /* ---------------------------- LEFT MODES WINDOW ---------------------------- */
    ImGui::SetNextWindowSizeConstraints({220, 0}, {FLT_MAX, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {12, 12});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {12, 12});
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if(ImGui::BeginChild("Modes", ImVec2(220, 0), ImGuiChildFlags_FrameStyle)) {
        ImGui::PopStyleColor();
        ImGui::SeparatorText("Mode Selection");
        for (int i = 1; i < NUM_MODES; i++) {
            const char *accel = AccelModes[i];
            if (ImGui::ModeSelectable(accel, i == selected_mode, 0, {-1, 0}))
                selected_mode = i;
            if(ImGui::IsItemHovered())
                hovered_mode = i;
        }
    }
    else
        ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    ImGui::SameLine();

    /* ---------------------------- MIDDLE PARAMETERS WINDOW ---------------------------- */
    ImGui::SetNextWindowSizeConstraints({220, -1}, {FLT_MAX, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10, 10});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if(ImGui::BeginChild("Parameters", ImVec2(220, -1), ImGuiChildFlags_FrameStyle)) {
        auto avail = ImGui::GetContentRegionAvail();
        ImGui::PopStyleColor();
        ImGui::SeparatorText("Parameters");
        ImGui::PushItemWidth(avail.x);

        bool change = false;

        // Display Global Parameters First
        change |= ImGui::DragFloat("##Sens_Param", &params[selected_mode].sens, 0.01, 0.01, 10, "Sensitivity %0.2f");
        change |= ImGui::DragFloat("##OutCap_Param", &params[selected_mode].outCap, 0.05, 0, 100, "Output Cap. %0.2f");
        change |= ImGui::DragFloat("##InCap_Param", &params[selected_mode].inCap, 0.1, 0, 200, "Input Cap. %0.2f");
        change |= ImGui::DragFloat("##Offset_Param", &params[selected_mode].offset, 0.05, -50, 50, "Offset %0.2f");
        change |= ImGui::DragFloat("##PreScale_Param", &params[selected_mode].preScale, 0.01, 0.01, 10, "Pre-Scale %0.2f");
        ImGui::SetItemTooltip("Used to adjust for DPI (Should be 800/DPI)");

        ImGui::SeparatorText("Advanced");

        // Mode Specific Parameters
        ImGui::PushID(selected_mode);
        switch(selected_mode) {
            case 0:
            {
                break;
            }
            case 1: // Linear
            {
                change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.001, 0.001, 2, "Acceleration %0.3f", ImGuiSliderFlags_Logarithmic);
                break;
            }
            case 2: // Power
            {
                change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.01, 0.01, 10, "Acceleration %0.2f");
                change |= ImGui::DragFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 0.01, 1, "Exponent %0.2f");
                break;
            }
            case 3: // Classic
            {
                change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.001, 0.001, 2, "Acceleration %0.3f");
                change |= ImGui::DragFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 2.01, 5, "Exponent %0.2f");
                break;
            }
            case 4: // Motivity
            {
                change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.01, 0.01, 10, "Acceleration %0.2f");
                change |= ImGui::DragFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0.05, 0.1, 50, "Start %0.2f");
                break;
            }
            case 5: // Jump
            {
                change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.01, 0, 10, "Acceleration %0.2f");
                change |= ImGui::DragFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0.05, 0.1, 50, "Start %0.2f");
                change |= ImGui::DragFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 0.01, 1, "Smoothness %0.2f");
                change |= ImGui::Checkbox("##Smoothing_Param", &params[selected_mode].useSmoothing);
                ImGui::SameLine(); ImGui::Text("Use Smoothing");
                break;
            }
            case 6: {
                static char LUT_user_data[4096];
                ImGui::Text("LUT data:");
                change |= ImGui::InputTextWithHint("##LUT data", "y1,y2,y3...", LUT_user_data, sizeof(LUT_user_data));
                ImGui::SetItemTooltip("Format: y1,y2,y3... (There are no 'x' values!)");
                change |= ImGui::DragFloat("##LUT_Stride_Param", &params[selected_mode].LUT_stride, 0.05, 0.05, 10, "Stride %0.2f");
                ImGui::SetItemTooltip("Gap between each 'y' value");
                if(ImGui::Button("Save", {-1, 0}))
                {
                    change |= true;

                    // Needs to be converted to int, because the kernel parameters don't deal too well with unsigned long longs
                    params[selected_mode].LUT_size = DriverHelper::ParseUserLutData(LUT_user_data,
                                                                                    params[selected_mode].LUT_data,
                                                                                    sizeof(params[selected_mode].LUT_data) /
                                                                                    sizeof(params[selected_mode].LUT_data[0]));
                }
                break;
            }
            default:
            {
                break;
            }
        }
        ImGui::PopID();

        if(change)
            functions[selected_mode].PreCacheFunc();

        ImGui::PopItemWidth();
    }
    else
        ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    ImGui::SameLine();


    ImGui::BeginGroup();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10, 10});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
    if(ImGui::BeginChild("PlotParameters", ImVec2(-1, (ImGui::GetFrameHeightWithSpacing()) * 1 + ImGui::GetStyle().FramePadding.y), ImGuiChildFlags_FrameStyle)) {
        auto avail = ImGui::GetContentRegionAvail();
        ImGui::PopStyleColor();
        ImGui::PushItemWidth(avail.x);
        ImGui::DragFloat("##MouseSmoothness", &mouse_smooth, 0.001, 0.0, 0.99, "Mouse Smoothness %0.2f");
        ImGui::PopItemWidth();
    }
    else
        ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    auto avail = ImGui::GetContentRegionAvail();

    /* ---------------------------- FUNCTION PLOT ---------------------------- */
    if(ImPlot::BeginPlot("Function Plot [Input / Output]", {-1, avail.y - 70})) {
        ImPlot::SetupAxis(ImAxis_X1, "Input Speed [counts / ms]");
        ImPlot::SetupAxis(ImAxis_Y1, "Output / Input Speed Ratio");

        static float last_frame_speed = 0;

        auto mouse_delta = ImGui::GetIO().MouseDelta;
        float mouse_speed = sqrtf(mouse_delta.x * mouse_delta.x + (mouse_delta.y * mouse_delta.y));
        float avg_speed = fmaxf(mouse_speed * (1 - mouse_smooth) + last_frame_speed * mouse_smooth, 0.01);
        ImPlotPoint mousePoint_main = ImPlotPoint(avg_speed, avg_speed < params[selected_mode].offset ?
            params[selected_mode].sens :
            functions[selected_mode].EvalFuncAt(avg_speed - params[selected_mode].offset));

        last_frame_speed = avg_speed;

        // Display currently applied parameters in the background
        if(was_initialized) {
            ImPlot::SetNextLineStyle(ImVec4(0.3, 0.3, 0.3, 1));
            ImPlot::PlotLine("Function in use", functions[0].values, PLOT_POINTS, functions[0].x_stride);
        }

        ImPlot::SetNextLineStyle(IMPLOT_AUTO_COL, 2);
        ImPlot::PlotLine("##ActivePlot", functions[selected_mode].values, PLOT_POINTS, functions[selected_mode].x_stride);

        ImPlot::PlotScatterG("Mouse Speed", [](int idx, void* data){return *(ImPlotPoint*)data; }, &mousePoint_main, 1);

        if(hovered_mode != -1 && selected_mode != hovered_mode) {
            ImPlot::SetNextLineStyle(ImVec4(0.7,0.7,0.3,1));
            ImPlot::PlotLine("##Hovered Function", functions[hovered_mode].values, PLOT_POINTS, functions[hovered_mode].x_stride);
        }

        ImPlot::EndPlot();
    }

    /* ---------------------------- BOTTOM BUTTONS ---------------------------- */
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10, 10});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
    if(ImGui::BeginChild("EndButtons", ImVec2(-1, -1), ImGuiChildFlags_FrameStyle)) {

        ImGui::PopStyleColor();

        ImGui::SetWindowFontScale(1.2f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.975, 0.9, 1).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.975, 0.82, 1).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.975, 0.75, 1).Value);
        if (ImGui::Button("Reset", {avail.x / 2 - 15, -1})) {
            ResetParameters();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Disable Apply button for 1.1 second after clicking it (this is a driver "limitation")
        ImGui::BeginDisabled(!has_privilege || !was_initialized ||
                            duration_cast<milliseconds>(steady_clock::now() - last_apply_clicked).count() < 1100 ||
                            (selected_mode == 6 /* LUT */ && params[selected_mode].LUT_size == 0)    );

        if (ImGui::Button("Apply", {-1, -1})) {
            params[selected_mode].SaveAll();
            functions[0] = functions[selected_mode];
            params[0] = params[selected_mode];
            used_mode = selected_mode;
            last_apply_clicked = steady_clock::now();
        }

        ImGui::EndDisabled();

        ImGui::SetWindowFontScale(1.f);
    }
    else
        ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    ImGui::EndGroup();

    if(!has_privilege)
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, ImGui::GetWindowHeight() - 40),
                                            ImColor::HSV(0.975, 0.9, 1).operator ImU32(), "Running without root privileges.\nSome functions will not be available");

    if(!was_initialized)
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 5),
                                            ImColor::HSV(0.975, 0.9, 1).operator ImU32(), "Could not read and initialize driver parameters, working on dummy data");

    return 0;
}

Parameters start_params;

void ResetParameters(void) {
    for(int mode = 0; mode < NUM_MODES; mode++) {
        params[mode] = start_params;
        params[mode].accelMode = mode == 0 ? used_mode : mode;

        if(mode == 6)
            memcpy(params[mode].LUT_data, start_params.LUT_data, start_params.LUT_size * sizeof(params[selected_mode].LUT_data[0]));

        if(mode == 3)
            params[mode].exponent = fmaxf(fminf(params[mode].exponent, 5), 2.1);

        if(mode == 5)
            params[mode].exponent = fmaxf(fminf(params[mode].exponent, 1), 0.1);

        if(mode == 2)
            params[mode].exponent = fmaxf(fminf(params[mode].exponent, 1), 0.1);

        functions[mode] = CachedFunction(((float)PLOT_X_RANGE) / PLOT_POINTS, &params[mode]);
        //printf("stride = %f\n", functions[mode].x_stride);
        functions[mode].PreCacheFunc();
    }
}

int main() {
    GUI::Setup(OnGui);
    ImPlot::CreateContext();

    ImGui::GetIO().IniFilename = nullptr;

    if (getuid()) {
        fprintf(stderr, "You are not a root!\n");
        has_privilege = false;
        //return 1;
    }
    else
        has_privilege = true;


    if (!DriverHelper::ValidateDirectory()) {
        fprintf(stderr,
                "LeetMouse directory doesnt exist!\nInstall the driver first, or check the parameters path.\n");
        return 2;
    }

    int fixed_num = 0;
    if(!DriverHelper::CleanParameters(fixed_num) && fixed_num != 0 && !has_privilege) {
        fprintf(stderr, "Could not setup driver params\n");
    }
    else {
        // Read driver parameters to a dummy aggregate
        DriverHelper::GetParameterF("Sensitivity", start_params.sens);
        DriverHelper::GetParameterF("OutputCap", start_params.outCap);
        DriverHelper::GetParameterF("InputCap", start_params.inCap);
        DriverHelper::GetParameterF("Offset", start_params.offset);
        DriverHelper::GetParameterF("Acceleration", start_params.accel);
        DriverHelper::GetParameterF("Exponent", start_params.exponent);
        DriverHelper::GetParameterF("Midpoint", start_params.midpoint);
        DriverHelper::GetParameterF("PreScale", start_params.preScale);
        DriverHelper::GetParameterI("AccelerationMode", start_params.accelMode);
        DriverHelper::GetParameterB("UseSmoothing", start_params.useSmoothing);
        DriverHelper::GetParameterI("LutSize", start_params.LUT_size);
        DriverHelper::GetParameterF("LutStride", start_params.LUT_stride);
        std::string Lut_dataBuf;
        DriverHelper::GetParameterS("LutDataBuf", Lut_dataBuf);
        DriverHelper::ParseDriverLutData(Lut_dataBuf.c_str(), start_params.LUT_data);

        used_mode = start_params.accelMode;

        selected_mode = start_params.accelMode % NUM_MODES;

        was_initialized = true;
    }


    ResetParameters();


    while (true) {
        if(GUI::RenderFrame())
            break;
    }

    GUI::ShutDown();

    return 0;
}