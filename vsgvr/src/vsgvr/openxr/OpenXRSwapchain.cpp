#include <vsgvr/openxr/OpenXRSwapchain.h>

#include <vsg/core/Exception.h>
#include "OpenXRMacros.cpp"

#include <iostream>

using namespace vsg;

namespace vsgvr {

  OpenXRSwapchain::OpenXRSwapchain(XrSession session, VkFormat swapchainFormat, std::vector<XrViewConfigurationView> viewConfigs)
    : _swapchainFormat(swapchainFormat)
  {
    validateFormat(session);
    createSwapchain(session, viewConfigs);
  }

  OpenXRSwapchain::~OpenXRSwapchain()
  {
    destroySwapchain();
  }

  VkImage OpenXRSwapchain::acquireImage()
  {
    uint32_t index = 0;
    xr_check(xrAcquireSwapchainImage(_swapchain, nullptr, &index), "Failed to acquire image");
    return _swapchainImages[index];
  }

  bool OpenXRSwapchain::waitImage(XrDuration timeout)
  {
    auto info = XrSwapchainImageWaitInfo();
    info.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
    info.next = nullptr;
    info.timeout = timeout;
    auto result = xrWaitSwapchainImage(_swapchain, &info);
    if (result == XR_TIMEOUT_EXPIRED)
    {
      return false;
    }
    xr_check(result, "Failed to wait on image");
    return true;
  }

  void OpenXRSwapchain::releaseImage()
  {
    auto info = XrSwapchainImageReleaseInfo();
    info.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
    info.next = nullptr;
    xr_check(xrReleaseSwapchainImage(_swapchain, &info), "Failed to release image");
  }


  void OpenXRSwapchain::validateFormat(XrSession session)
  {
    uint32_t count = 0;
    xr_check(xrEnumerateSwapchainFormats(session, 0, &count, nullptr));
    std::vector<int64_t> formats(count);
    xr_check(xrEnumerateSwapchainFormats(session, formats.size(), &count, formats.data()));

    if (std::find(formats.begin(), formats.end(), static_cast<int64_t>(_swapchainFormat)) == formats.end())
    {
      throw Exception({ "OpenXR runtime doesn't support selected swapchain format (TODO: Preference based selection)" });
    }
  }

  void OpenXRSwapchain::createSwapchain(XrSession session, std::vector<XrViewConfigurationView> viewConfigs)
  {
    XrSwapchainUsageFlags usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    XrSwapchainCreateFlags createFlags = 0;

    auto info = XrSwapchainCreateInfo();
    info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    info.next = nullptr;
    info.createFlags = createFlags;
    info.usageFlags = usageFlags;
    info.format = _swapchainFormat;
    // TODO TODO
    info.sampleCount = viewConfigs[0].recommendedSwapchainSampleCount;
    info.width = viewConfigs[0].recommendedImageRectWidth;
    info.height = viewConfigs[1].recommendedImageRectHeight;
    // TODO TODO
    info.faceCount = 1;
    info.arraySize = 1;
    info.mipCount = 1;

    xr_check(xrCreateSwapchain(session, &info, &_swapchain));

    uint32_t imageCount = 0;
    xr_check(xrEnumerateSwapchainImages(_swapchain, 0, &imageCount, nullptr));

    std::vector<XrSwapchainImageVulkan2KHR> images(imageCount);
    xr_check(xrEnumerateSwapchainImages(_swapchain, images.size(), &imageCount,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())));

    for (auto& i : images)
    {
      _swapchainImages.push_back(i.image);
    }
  }

  void OpenXRSwapchain::destroySwapchain()
  {
    xr_check(xrDestroySwapchain(_swapchain));
  }
}