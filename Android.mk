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

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),griffin)

CAMERA_VENDOR_PATCH_SRC := $(LOCAL_PATH)/proprietary/vendor/lib
CAMERA_VENDOR_STAMP := $(PRODUCT_OUT)/vendor/.camera_vendor_patched.stamp

CAMERA_VENDOR_TARGETS := \
    $(PRODUCT_OUT)/vendor/lib/libsomc_camerahal.so \
    $(PRODUCT_OUT)/vendor/lib/libmorpho_dual_camera.so \
    $(PRODUCT_OUT)/vendor/lib/libsomc_chokoballpal.so \
    $(PRODUCT_OUT)/vendor/lib/libexcal_core.so \
    $(PRODUCT_OUT)/vendor/lib/libsomc_camerapal.so \
    $(PRODUCT_OUT)/vendor/lib/libui-v34.so

$(CAMERA_VENDOR_STAMP): $(CAMERA_VENDOR_TARGETS) $(wildcard $(CAMERA_VENDOR_PATCH_SRC)/*.so)
	@echo "Applying Sony stock camera vendor library patches..."
	@mkdir -p $(PRODUCT_OUT)/vendor/lib
	$(hide) cp -f $(CAMERA_VENDOR_PATCH_SRC)/*.so $(PRODUCT_OUT)/vendor/lib/
	@touch $@

$(PRODUCT_OUT)/vendor.img: $(CAMERA_VENDOR_STAMP)

endif
