
#include <vsgvr/openxr/OpenXRGraphicsBindingVulkan2.h>

#include <vsg/core/Exception.h>

#include "OpenXRMacros.cpp"

#include <string>
#include <sstream>
#include <vector>

using namespace vsg;

namespace vsgvr {
    OpenXRGraphicsBindingVulkan2::OpenXRGraphicsBindingVulkan2(XrInstance instance, XrSystemId system, OpenXrTraits traits, OpenXrVulkanTraits vkTraits)
    {
        createVulkanInstance(instance, system, traits, vkTraits);
    }

    OpenXRGraphicsBindingVulkan2::~OpenXRGraphicsBindingVulkan2()
    {
        destroyVulkanInstance();
    }

    void OpenXRGraphicsBindingVulkan2::createVulkanInstance(XrInstance instance, XrSystemId system, OpenXrTraits traits, OpenXrVulkanTraits vkTraits)
    {
        {
            _graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR;
            _graphicsRequirements.next = nullptr;
            auto fn = (PFN_xrGetVulkanGraphicsRequirements2KHR)xr_pfn(instance, "xrGetVulkanGraphicsRequirements2KHR");
            xr_check(fn(instance, system, &_graphicsRequirements), "Failed to get Vulkan requirements");
        }
        // min/max vulkan api version supported by the XR runtime (XR_MAKE_VERSION)
        auto vkMinVersion = VK_MAKE_API_VERSION(0, XR_VERSION_MAJOR(_graphicsRequirements.minApiVersionSupported), XR_VERSION_MINOR(_graphicsRequirements.minApiVersionSupported), XR_VERSION_PATCH(_graphicsRequirements.minApiVersionSupported));
        auto vkMaxVersion = VK_MAKE_API_VERSION(0, XR_VERSION_MAJOR(_graphicsRequirements.maxApiVersionSupported), XR_VERSION_MINOR(_graphicsRequirements.maxApiVersionSupported), XR_VERSION_PATCH(_graphicsRequirements.maxApiVersionSupported));
        if (vkTraits.vulkanVersion > vkMaxVersion || vkTraits.vulkanVersion < vkMinVersion) {
            throw Exception({"OpenXR runtime doesn't support requested Vulkan version"});
        }

        std::vector<std::string> vkInstanceExtensions;
        {
          auto fn = (PFN_xrGetVulkanInstanceExtensionsKHR)xr_pfn(instance, "xrGetVulkanInstanceExtensionsKHR");
          uint32_t size = 0;
          xr_check(fn(instance, system, 0, &size, nullptr), "Failed to get instance extensions (num)");
          std::string names;
          names.reserve(size);
          xr_check(fn(instance, system, size, &size, names.data()), "Failed to get instance extensions");
          // Single-space delimited
          std::stringstream s(names);
          std::string name;
          while (std::getline(s, name)) vkInstanceExtensions.push_back(name);
        }
        for( auto& e : vkTraits.vulkanInstanceExtensions ) vkInstanceExtensions.push_back(e);

        std::vector<std::string> vkDeviceExtensions;
        {
          auto GetDeviceExtensions = (PFN_xrGetVulkanDeviceExtensionsKHR)xr_pfn(instance, "xrGetVulkanDeviceExtensionsKHR");
          uint32_t size = 0;
          xr_check(GetDeviceExtensions(instance, system, 0, &size, nullptr), "Failed to get device extensions (num)");
          std::string names;
          names.reserve(size);
          xr_check(GetDeviceExtensions(instance, system, size, &size, names.data()), "Failed to get device extensions");
          // Single-space delimited
          std::stringstream s(names);
          std::string name;
          while (std::getline(s, name)) vkDeviceExtensions.push_back(name);
        }

        // Initialise a vulkan instance, using the OpenXR device
        {
          auto applicationInfo = VkApplicationInfo();
          applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
          applicationInfo.pNext = nullptr;
          applicationInfo.pApplicationName = traits.applicationName.c_str();
          applicationInfo.applicationVersion = traits.applicationVersion;
          applicationInfo.pEngineName = traits.engineName.c_str();
          applicationInfo.engineVersion = traits.engineVersion;
          applicationInfo.apiVersion = vkTraits.vulkanVersion;

          std::vector<const char*> tmpInstanceExtensions;
          for (auto x : vkInstanceExtensions) tmpInstanceExtensions.push_back(x.data());

          auto instanceInfo = VkInstanceCreateInfo();
          instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
          instanceInfo.pNext = nullptr;
          instanceInfo.flags = 0;
          instanceInfo.pApplicationInfo = &applicationInfo;
          instanceInfo.enabledLayerCount = 0;
          instanceInfo.ppEnabledLayerNames = nullptr;
          instanceInfo.enabledExtensionCount = static_cast<uint32_t>(tmpInstanceExtensions.size());
          instanceInfo.ppEnabledExtensionNames = tmpInstanceExtensions.empty() ? nullptr : tmpInstanceExtensions.data();

          auto xrVulkanCreateInfo = XrVulkanInstanceCreateInfoKHR();
          xrVulkanCreateInfo.type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR;
          xrVulkanCreateInfo.next = NULL;
          xrVulkanCreateInfo.systemId = system;
          xrVulkanCreateInfo.createFlags = 0;
          xrVulkanCreateInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
          xrVulkanCreateInfo.vulkanCreateInfo = &instanceInfo;
          // TODO: Support for custom allocators - vsg has those
          xrVulkanCreateInfo.vulkanAllocator = NULL;

          auto CreateVulkanInstanceKHR = (PFN_xrCreateVulkanInstanceKHR)xr_pfn(instance, "xrCreateVulkanInstanceKHR");
          VkResult vkResult;
          VkInstance vkInstance;
          xr_check(CreateVulkanInstanceKHR(instance, &xrVulkanCreateInfo, &vkInstance, &vkResult), "Failed to create Vulkan Instance");

          _vkInstance = vsgvr::OpenXRVkInstance::create(vkInstance);
        }
      }
    void OpenXRGraphicsBindingVulkan2::createVulkanPhysicalDevice(XrInstance instance, XrSystemId system, OpenXrTraits traits, OpenXrVulkanTraits vkTraits)
    {
        // Now fetch the Vulkan device - OpenXR will require the specific device,
        // which is attached to the display
        {
          auto fn = (PFN_xrGetVulkanGraphicsDevice2KHR)xr_pfn(instance, "xrGetVulkanGraphicsDevice2KHR");
          
          auto info = XrVulkanGraphicsDeviceGetInfoKHR();
          info.type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR;
          info.next = nullptr;
          info.systemId = system;
          info.vulkanInstance = _vkInstance->getInstance();

          VkPhysicalDevice vkPhysicalDevice;
          xr_check(fn(instance, &info, &vkPhysicalDevice), "Failed to get Vulkan physical device from OpenXR");
          _vkPhysicalDevice = vsgvr::OpenXRVkPhysicalDevice::create(_vkInstance, vkPhysicalDevice);
        }
    }

    void OpenXRGraphicsBindingVulkan2::createVulkanDevice(XrInstance instance, XrSystemId system, OpenXrTraits traits, OpenXrVulkanTraits vkTraits)
    {
        auto qFamily = _vkPhysicalDevice->getQueueFamily(VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT);
        if (qFamily < 0)
        {
          throw Exception({ "Failed to locate graphics queue" });
        }

        auto CreateVulkanDeviceKHR = (PFN_xrCreateVulkanDeviceKHR)xr_pfn(instance, "xrCreateVulkanDeviceKHR");

        // Create the Vulkan Device through OpenXR - It will add any extensions required
        float qPriorities[] = { 0.0f };
        VkDeviceQueueCreateInfo qInfo;
        qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qInfo.pNext = nullptr;
        qInfo.flags = 0;
        qInfo.queueFamilyIndex = qFamily;
        qInfo.queueCount = 1;
        qInfo.pQueuePriorities = qPriorities;

        auto enabledFeatures = VkPhysicalDeviceFeatures();
        enabledFeatures.samplerAnisotropy = VK_TRUE;

        auto deviceInfo = VkDeviceCreateInfo();
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pNext = nullptr;
        deviceInfo.flags = 0;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &qInfo;
        deviceInfo.enabledLayerCount = 0;
        deviceInfo.ppEnabledLayerNames = nullptr;
        deviceInfo.enabledExtensionCount = 0;
        deviceInfo.ppEnabledExtensionNames = nullptr;
        deviceInfo.pEnabledFeatures = &enabledFeatures;

        auto info = XrVulkanDeviceCreateInfoKHR();
        info.type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR;
        info.next = nullptr;
        info.systemId = system;
        info.createFlags = 0;
        info.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
        info.vulkanPhysicalDevice = _vkPhysicalDevice->getPhysicalDevice();
        info.vulkanCreateInfo = &deviceInfo;
        info.vulkanAllocator = nullptr;

        VkDevice vkDevice;
        VkResult vkResult;
        xr_check(CreateVulkanDeviceKHR(instance, &info, &vkDevice, &vkResult), "Failed to create Vulkan Device");

        _vkDevice = vsgvr::OpenXRVkDevice::create(_vkInstance, _vkPhysicalDevice, vkDevice);
        _vkDevice->getQueue(qFamily, 0); // To populate Device::_queues
    }

    void OpenXRGraphicsBindingVulkan2::destroyVulkanDevice() {
      _vkDevice = 0;
    }

    void OpenXRGraphicsBindingVulkan2::destroyVulkanPhysicalDevice() {
      _vkPhysicalDevice = 0;
    }

    void OpenXRGraphicsBindingVulkan2::destroyVulkanInstance()
    {
        _vkDevice = 0;
        _vkPhysicalDevice = 0;
        _vkInstance = 0;
    }

    XrGraphicsRequirementsVulkan2KHR _graphicsRequirements;
    XrGraphicsBindingVulkan2KHR _binding;
}