[Defines]
  VENDOR_NAME                    = Xiaomi
  PLATFORM_NAME                  = elish
  PLATFORM_GUID                  = 28f1a3bf-193a-47e3-a7b9-5a435eaab2ee
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Qualcomm/sm8250/sm8250.fdf
  DEVICE_DXE_FV_COMPONENTS       = Platform/Xiaomi/sm8250/elish.fdf.inc

!include Platform/Qualcomm/sm8250/sm8250.dsc

[BuildOptions.common]
  GCC:*_*_AARCH64_CC_FLAGS = -DENABLE_SIMPLE_INIT

[PcdsFixedAtBuild.common]
  # Supplement only actual RAM above the original platform map.
  gQcomTokenSpaceGuid.PcdAdditionalRamBase|0x200000000
  gQcomTokenSpaceGuid.PcdMipiFrameBufferWidth|1600
  gQcomTokenSpaceGuid.PcdMipiFrameBufferHeight|2560

  # Simple Init
  gSimpleInitTokenSpaceGuid.PcdGuiDefaultDPI|320

  gRenegadePkgTokenSpaceGuid.PcdDeviceVendor|"Xiaomi"
  gRenegadePkgTokenSpaceGuid.PcdDeviceProduct|"Mi Pad 5 Pro"
  gRenegadePkgTokenSpaceGuid.PcdDeviceCodeName|"elish"

# Explicitly invoked upstream slot switcher; no automatic BootSlotDxe writes.
[Components.common]

  GPLDrivers/Application/SwitchSlotsApp/SwitchSlotsApp.inf {
    <LibraryClasses>
      BootSlotLib|GPLDrivers/Library/BootSlotLib/BootSlotLib.inf
  }
