#
# Copyright (C) 2026 The Evolution X Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

ifeq ($(TARGET_DEVICE),griffin)

CAMERA_VENDOR_PATCH_SRC := vendor/sony/SemcCameraUI/proprietary/vendor/lib
CAMERA_VENDOR_STAMP := $(TARGET_OUT_VENDOR)/.camera_vendor_patched.stamp

CAMERA_VENDOR_TARGETS := \
    $(TARGET_OUT_VENDOR)/lib/libsomc_camerahal.so \
    $(TARGET_OUT_VENDOR)/lib/libmorpho_dual_camera.so \
    $(TARGET_OUT_VENDOR)/lib/libsomc_chokoballpal.so \
    $(TARGET_OUT_VENDOR)/lib/libexcal_core.so \
    $(TARGET_OUT_VENDOR)/lib/libsomc_camerapal.so \
    $(TARGET_OUT_VENDOR)/lib/libui-v34.so

$(CAMERA_VENDOR_STAMP): $(CAMERA_VENDOR_TARGETS) $(wildcard $(CAMERA_VENDOR_PATCH_SRC)/*.so)
	@echo "Applying Sony stock camera vendor library patches..."
	@mkdir -p $(TARGET_OUT_VENDOR)/lib
	$(hide) cp -f $(CAMERA_VENDOR_PATCH_SRC)/*.so $(TARGET_OUT_VENDOR)/lib/
	@touch $@

ifneq ($(INSTALLED_VENDORIMAGE_TARGET),)
$(INSTALLED_VENDORIMAGE_TARGET): $(CAMERA_VENDOR_STAMP)
endif

ifneq ($(BUILT_TARGET_FILES_DIR),)
$(BUILT_TARGET_FILES_DIR): $(CAMERA_VENDOR_STAMP)
endif

ifneq ($(BUILT_TARGET_FILES_PACKAGE),)
$(BUILT_TARGET_FILES_PACKAGE): $(CAMERA_VENDOR_STAMP)
endif

endif
