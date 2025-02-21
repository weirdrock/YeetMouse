yeetmouseOverlay:
{ pkgs, config, lib, ... }:

with lib;
let
  cfg = config.hardware.yeetmouse;

  accelerationModeValues = [ "linear" "power" "classic" "motivity" "jump" "lut" ];

  modeValueToInt = modeValue:
    (lists.findFirstIndex
      (value: modeValue == value) 0 accelerationModeValues) + 1;

  parametersType = types.submodule {
    options = {
      AccelerationMode = mkOption {
        type = types.enum accelerationModeValues;
        default = "linear";
        description = "Sets the algorithm to be used for acceleration";
      };
      InputCap = mkOption {
        type = types.float;
        default = 0.0;
        description = "Limit the maximum pointer speed before applying acceleration.";
      };
      Sensitivity = mkOption {
        type = types.float;
        default = 1.0;
        description = "Mouse base sensitivity.";
      };
      Acceleration = mkOption {
        type = types.float;
        default = 1.0;
        description = "Mouse acceleration sensitivity.";
      };
      OutputCap = mkOption {
        type = types.float;
        default = 0.0;
        description = "Cap maximum sensitivity.";
      };
      Offset = mkOption {
        type = types.float;
        default = 0.0;
        description = "Mouse base sensitivity.";
      };
      Exponent = mkOption {
        type = types.float;
        default = 1.0;
        description = "Exponent for algorithms that use it";
      };
      Midpoint = mkOption {
        type = types.float;
        default = 6.0;
        description = "Midpoint for sigmoid function";
      };
      PreScale = mkOption {
        type = types.float;
        default = 1.0;
        description = "Parameter to adjust for the DPI";
      };
      RotationAngle = mkOption {
        type = types.float;
        default = 0.0;
        description = "Amount of clockwise rotation (in radians)";
      };
      UseSmoothing = mkOption {
        type = types.bool;
        default = true;
        description = "Whether to smooth out functions (doesn't apply to all)";
      };
      ScrollsPerTick = mkOption {
        type = types.int;
        default = true;
        description = "Amount of lines to scroll per scroll-wheel tick.";
      };
    };
  };

  yeetmouse = pkgs.yeetmouse.override {
    kernel = config.boot.kernelPackages.kernel;
  };
in {
  options.hardware.yeetmouse = {
    enable = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = "Enable yeetmouse kernel module to add configurable mouse acceleration";
    };

    parameters = lib.mkOption {
      type = parametersType;
      default = { };
    };
  };

  config = lib.mkIf cfg.enable {
    nixpkgs.overlays = [ yeetmouseOverlay ];

    boot.extraModulePackages = [ yeetmouse ];
    environment.systemPackages = [ yeetmouse ];
    services.udev = {
      extraRules = let
        echo = "${pkgs.coreutils}/bin/echo";
        yeetmouseConfig = with cfg.parameters; pkgs.writeShellScriptBin "yeetmouseConfig" ''
          ${echo} "${toString Acceleration}" > /sys/module/yeetmouse/parameters/Acceleration
          ${echo} "${toString Exponent}" > /sys/module/yeetmouse/parameters/Exponent
          ${echo} "${toString InputCap}" > /sys/module/yeetmouse/parameters/InputCap
          ${echo} "${toString Midpoint}" > /sys/module/yeetmouse/parameters/Midpoint
          ${echo} "${toString Offset}" > /sys/module/yeetmouse/parameters/Offset
          ${echo} "${toString OutputCap}" > /sys/module/yeetmouse/parameters/OutputCap
          ${echo} "${toString PreScale}" > /sys/module/yeetmouse/parameters/PreScale
          ${echo} "${toString RotationAngle}" > /sys/module/yeetmouse/parameters/RotationAngle
          ${echo} "${toString Sensitivity}" > /sys/module/yeetmouse/parameters/Sensitivity
          ${echo} "${toString ScrollsPerTick}" > /sys/module/yeetmouse/parameters/ScrollsPerTick
          ${echo} "${toString (modeValueToInt AccelerationMode)}" > /sys/module/yeetmouse/parameters/AccelerationMode
          ${echo} "${if UseSmoothing then "1" else "0"}" > /sys/module/yeetmouse/parameters/UseSmoothing
          ${echo} "1" > /sys/module/yeetmouse/parameters/update
        '';
      in ''
        SUBSYSTEMS=="usb|input|hid", ATTRS{bInterfaceClass}=="03", ATTRS{bInterfaceSubClass}=="01", ATTRS{bInterfaceProtocol}=="02", ATTRS{bInterfaceNumber}=="00", RUN+="${yeetmouseConfig}/bin/yeetmouseConfig"
      '';
    };
  };
}
