# Cutl

Cutl is a graphics engine that works as a thin layer over Vulkan.
It does much of the heavy lifting for initialization while still exposing the inner Vulkan values.

## Features

Cutl provides basic initalization for:
- Windows through GLFW
- Contexts (instance, surface, physical and logical device, queues)
- Renderers (swapchain, frame objects, command buffers)
- Pipelines
- Buffers

Cutl also provides a simple render sequence, using a `CuScene` you fill a command buffer at draw time to submit.

## Usage Notes

By default, Cutl enforces bindless buffers, using `bufferDeviceAddress` with push constants to send buffer data.

## Requirements

Cutl requires Vulkan 1.4 support, as well as support for the following core extensions:
- fillModeNonSolid (1.0)
- shaderInt64 (1.0)
- descriptorIndexing (1.2)
- scalarBlockLayout (1.2)
- timelineSemaphore (1.2)
- bufferDeviceAddress (1.2)
- synchronization2 (1.3)
- dynamicRendering (1.3)
- maintenance4 (1.3)
- maintenance5 (1.4)
- pushDescriptor (1.4)
