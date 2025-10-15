#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_VULKAN)

using namespace mercury;
using namespace mercury::ll::graphics;

#include "vk_graphics.h"
#include "../../../graphics.h"
#include "vk_utils.h"

VkPhysicalDevice gVKPhysicalDevice = VK_NULL_HANDLE;

mercury::ll::graphics::AdapterInfo GetInfoFromPhysicalDevice(VkPhysicalDevice physicalDevice)
{
    mercury::ll::graphics::AdapterInfo info = {};

    VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, nullptr};

    VkPhysicalDeviceDriverProperties driverProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
    NextPChain(properties2.pNext, &driverProperties); 
    
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &accelerationStructureProperties);

    VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT blendOperationAdvancedProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &blendOperationAdvancedProperties);
    
    VkPhysicalDeviceClusterAccelerationStructurePropertiesNV clusterAccelerationStructureProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_ACCELERATION_STRUCTURE_PROPERTIES_NV};
    NextPChain(properties2.pNext, &clusterAccelerationStructureProperties);
    
    VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI clusterCullingShaderProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_PROPERTIES_HUAWEI};
    NextPChain(properties2.pNext, &clusterCullingShaderProperties);
    
    VkPhysicalDeviceComputeShaderDerivativesPropertiesKHR computeShaderDerivativesProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &computeShaderDerivativesProperties);
    
    VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterizationProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &conservativeRasterizationProperties);
    VkPhysicalDeviceCooperativeMatrix2PropertiesNV cooperativeMatrix2Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_2_PROPERTIES_NV};
    NextPChain(properties2.pNext, &cooperativeMatrix2Properties);
    VkPhysicalDeviceCooperativeMatrixPropertiesKHR cooperativeMatrixProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &cooperativeMatrixProperties);
    VkPhysicalDeviceCooperativeMatrixPropertiesNV cooperativeMatrixPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV};
    NextPChain(properties2.pNext, &cooperativeMatrixPropertiesNV);
    VkPhysicalDeviceCooperativeVectorPropertiesNV cooperativeVectorPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_VECTOR_PROPERTIES_NV};
    NextPChain(properties2.pNext, &cooperativeVectorPropertiesNV);
    VkPhysicalDeviceCopyMemoryIndirectPropertiesNV copyMemoryIndirectPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_PROPERTIES_NV};
    NextPChain(properties2.pNext, &copyMemoryIndirectPropertiesNV);
    VkPhysicalDeviceCudaKernelLaunchPropertiesNV cudaKernelLaunchPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUDA_KERNEL_LAUNCH_PROPERTIES_NV};
    NextPChain(properties2.pNext, &cudaKernelLaunchPropertiesNV);
    VkPhysicalDeviceCustomBorderColorPropertiesEXT customBorderColorPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &customBorderColorPropertiesEXT);
    VkPhysicalDeviceDepthStencilResolveProperties depthStencilResolveProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES};
    NextPChain(properties2.pNext, &depthStencilResolveProperties);
    VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT descriptorBufferDensityMapPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_DENSITY_MAP_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &descriptorBufferDensityMapPropertiesEXT);
    VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &descriptorBufferPropertiesEXT);
    VkPhysicalDeviceDescriptorBufferTensorPropertiesARM descriptorBufferTensorPropertiesARM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_TENSOR_PROPERTIES_ARM};
    NextPChain(properties2.pNext, &descriptorBufferTensorPropertiesARM);
    VkPhysicalDeviceDescriptorIndexingProperties descriptorIndexingProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES};
    NextPChain(properties2.pNext, &descriptorIndexingProperties);
    VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT deviceGeneratedCommandsPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &deviceGeneratedCommandsPropertiesEXT);
    VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV deviceGeneratedCommandsPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV};
    NextPChain(properties2.pNext, &deviceGeneratedCommandsPropertiesNV);
    VkPhysicalDeviceDiscardRectanglePropertiesEXT discardRectanglePropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &discardRectanglePropertiesEXT);
    VkPhysicalDeviceDisplacementMicromapPropertiesNV displacementMicromapPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISPLACEMENT_MICROMAP_PROPERTIES_NV};
    NextPChain(properties2.pNext, &displacementMicromapPropertiesNV);
    VkPhysicalDeviceDrmPropertiesEXT drmPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &drmPropertiesEXT);
    VkPhysicalDeviceExtendedDynamicState3PropertiesEXT extendedDynamicState3PropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &extendedDynamicState3PropertiesEXT);
    VkPhysicalDeviceExtendedSparseAddressSpacePropertiesNV extendedSparseAddressSpacePropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_SPARSE_ADDRESS_SPACE_PROPERTIES_NV};
    NextPChain(properties2.pNext, &extendedSparseAddressSpacePropertiesNV);
    VkPhysicalDeviceExternalComputeQueuePropertiesNV externalComputeQueuePropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_COMPUTE_QUEUE_PROPERTIES_NV};
    NextPChain(properties2.pNext, &externalComputeQueuePropertiesNV);
    VkPhysicalDeviceExternalMemoryHostPropertiesEXT externalMemoryHostPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &externalMemoryHostPropertiesEXT);
    VkPhysicalDeviceFloatControlsProperties floatControlsProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES};
    NextPChain(properties2.pNext, &floatControlsProperties);
    VkPhysicalDeviceFragmentDensityMap2PropertiesEXT fragmentDensityMap2PropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &fragmentDensityMap2PropertiesEXT);
    VkPhysicalDeviceFragmentDensityMapLayeredPropertiesVALVE fragmentDensityMapLayeredPropertiesVALVE = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_LAYERED_PROPERTIES_VALVE};
    NextPChain(properties2.pNext, &fragmentDensityMapLayeredPropertiesVALVE);
    VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT fragmentDensityMapOffsetPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &fragmentDensityMapOffsetPropertiesEXT);
    VkPhysicalDeviceFragmentDensityMapPropertiesEXT fragmentDensityMapPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &fragmentDensityMapPropertiesEXT);
    VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR fragmentShaderBarycentricPropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &fragmentShaderBarycentricPropertiesKHR);
    VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV fragmentShadingRateEnumsPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV};
    NextPChain(properties2.pNext, &fragmentShadingRateEnumsPropertiesNV);
    VkPhysicalDeviceFragmentShadingRatePropertiesKHR fragmentShadingRatePropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &fragmentShadingRatePropertiesKHR);
    VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT graphicsPipelineLibraryPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &graphicsPipelineLibraryPropertiesEXT);
    VkPhysicalDeviceHostImageCopyProperties hostImageCopyProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_PROPERTIES};
    NextPChain(properties2.pNext, &hostImageCopyProperties);
    VkPhysicalDeviceIDProperties idProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
    NextPChain(properties2.pNext, &idProperties);
    VkPhysicalDeviceImageAlignmentControlPropertiesMESA imageAlignmentControlPropertiesMESA = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ALIGNMENT_CONTROL_PROPERTIES_MESA};
    NextPChain(properties2.pNext, &imageAlignmentControlPropertiesMESA);
    VkPhysicalDeviceImageProcessing2PropertiesQCOM imageProcessing2PropertiesQCOM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_2_PROPERTIES_QCOM};
    NextPChain(properties2.pNext, &imageProcessing2PropertiesQCOM);
    VkPhysicalDeviceImageProcessingPropertiesQCOM imageProcessingPropertiesQCOM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_PROPERTIES_QCOM};
    NextPChain(properties2.pNext, &imageProcessingPropertiesQCOM);
    VkPhysicalDeviceInlineUniformBlockProperties inlineUniformBlockProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES};
    NextPChain(properties2.pNext, &inlineUniformBlockProperties);
    VkPhysicalDeviceLayeredApiPropertiesListKHR layeredApiPropertiesListKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LAYERED_API_PROPERTIES_LIST_KHR};
    NextPChain(properties2.pNext, &layeredApiPropertiesListKHR);
    VkPhysicalDeviceLayeredDriverPropertiesMSFT layeredDriverPropertiesMSFT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LAYERED_DRIVER_PROPERTIES_MSFT};
    NextPChain(properties2.pNext, &layeredDriverPropertiesMSFT);
    VkPhysicalDeviceLegacyVertexAttributesPropertiesEXT legacyVertexAttributesPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LEGACY_VERTEX_ATTRIBUTES_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &legacyVertexAttributesPropertiesEXT);
    VkPhysicalDeviceLineRasterizationProperties lineRasterizationProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES};
    NextPChain(properties2.pNext, &lineRasterizationProperties);
    VkPhysicalDeviceMaintenance3Properties maintenance3Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES};
    NextPChain(properties2.pNext, &maintenance3Properties);
    VkPhysicalDeviceMaintenance4Properties maintenance4Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES};
    NextPChain(properties2.pNext, &maintenance4Properties);
    VkPhysicalDeviceMaintenance5Properties maintenance5Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_PROPERTIES};
    NextPChain(properties2.pNext, &maintenance5Properties);
    VkPhysicalDeviceMaintenance6Properties maintenance6Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_PROPERTIES};
    NextPChain(properties2.pNext, &maintenance6Properties);
    VkPhysicalDeviceMaintenance7PropertiesKHR maintenance7PropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &maintenance7PropertiesKHR);
    VkPhysicalDeviceMaintenance9PropertiesKHR maintenance9PropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &maintenance9PropertiesKHR);
    VkPhysicalDeviceMapMemoryPlacedPropertiesEXT mapMemoryPlacedPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &mapMemoryPlacedPropertiesEXT);
    VkPhysicalDeviceMemoryDecompressionPropertiesNV memoryDecompressionPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_DECOMPRESSION_PROPERTIES_NV};
    NextPChain(properties2.pNext, &memoryDecompressionPropertiesNV);
    VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &meshShaderPropertiesEXT);
    VkPhysicalDeviceMeshShaderPropertiesNV meshShaderPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV};
    NextPChain(properties2.pNext, &meshShaderPropertiesNV);
    VkPhysicalDeviceMultiDrawPropertiesEXT multiDrawPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &multiDrawPropertiesEXT);
    VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX multiviewPerViewAttributesPropertiesNVX = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX};
    NextPChain(properties2.pNext, &multiviewPerViewAttributesPropertiesNVX);
    VkPhysicalDeviceMultiviewProperties multiviewProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES};
    NextPChain(properties2.pNext, &multiviewProperties);
    VkPhysicalDeviceNestedCommandBufferPropertiesEXT nestedCommandBufferPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NESTED_COMMAND_BUFFER_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &nestedCommandBufferPropertiesEXT);
    VkPhysicalDeviceOpacityMicromapPropertiesEXT opacityMicromapPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &opacityMicromapPropertiesEXT);
    VkPhysicalDeviceOpticalFlowPropertiesNV opticalFlowPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_PROPERTIES_NV};
    NextPChain(properties2.pNext, &opticalFlowPropertiesNV);
    VkPhysicalDevicePCIBusInfoPropertiesEXT pciBusInfoPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &pciBusInfoPropertiesEXT);
    VkPhysicalDevicePartitionedAccelerationStructurePropertiesNV partitionedAccelerationStructurePropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PARTITIONED_ACCELERATION_STRUCTURE_PROPERTIES_NV};
    NextPChain(properties2.pNext, &partitionedAccelerationStructurePropertiesNV);
    VkPhysicalDevicePerformanceQueryPropertiesKHR performanceQueryPropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &performanceQueryPropertiesKHR);
    VkPhysicalDevicePipelineBinaryPropertiesKHR pipelineBinaryPropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &pipelineBinaryPropertiesKHR);
    VkPhysicalDevicePipelineRobustnessProperties pipelineRobustnessProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES};
    NextPChain(properties2.pNext, &pipelineRobustnessProperties);
    VkPhysicalDevicePointClippingProperties pointClippingProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES};
    NextPChain(properties2.pNext, &pointClippingProperties);
    VkPhysicalDevicePortabilitySubsetPropertiesKHR portabilitySubsetPropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &portabilitySubsetPropertiesKHR);
    VkPhysicalDeviceProtectedMemoryProperties protectedMemoryProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES};
    NextPChain(properties2.pNext, &protectedMemoryProperties);
    VkPhysicalDeviceProvokingVertexPropertiesEXT provokingVertexPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &provokingVertexPropertiesEXT);
    VkPhysicalDevicePushDescriptorProperties pushDescriptorProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES};
    NextPChain(properties2.pNext, &pushDescriptorProperties);
    VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV rayTracingInvocationReorderPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_PROPERTIES_NV};
    NextPChain(properties2.pNext, &rayTracingInvocationReorderPropertiesNV);
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelinePropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &rayTracingPipelinePropertiesKHR);
    VkPhysicalDeviceRayTracingPropertiesNV rayTracingPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV};
    NextPChain(properties2.pNext, &rayTracingPropertiesNV);
    VkPhysicalDeviceRenderPassStripedPropertiesARM renderPassStripedPropertiesARM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RENDER_PASS_STRIPED_PROPERTIES_ARM};
    NextPChain(properties2.pNext, &renderPassStripedPropertiesARM);
    VkPhysicalDeviceRobustness2PropertiesKHR robustness2PropertiesKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_KHR};
    NextPChain(properties2.pNext, &robustness2PropertiesKHR);
    VkPhysicalDeviceSampleLocationsPropertiesEXT sampleLocationsPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &sampleLocationsPropertiesEXT);
    VkPhysicalDeviceSamplerFilterMinmaxProperties samplerFilterMinmaxProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES};
    NextPChain(properties2.pNext, &samplerFilterMinmaxProperties);
    VkPhysicalDeviceSchedulingControlsPropertiesARM schedulingControlsPropertiesARM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCHEDULING_CONTROLS_PROPERTIES_ARM};
    NextPChain(properties2.pNext, &schedulingControlsPropertiesARM);
    VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM shaderCoreBuiltinsPropertiesARM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_PROPERTIES_ARM};
    NextPChain(properties2.pNext, &shaderCoreBuiltinsPropertiesARM);
    VkPhysicalDeviceShaderCoreProperties2AMD shaderCoreProperties2AMD = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD};
    NextPChain(properties2.pNext, &shaderCoreProperties2AMD);
    VkPhysicalDeviceShaderCorePropertiesAMD shaderCorePropertiesAMD = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD};
    NextPChain(properties2.pNext, &shaderCorePropertiesAMD);
    VkPhysicalDeviceShaderCorePropertiesARM shaderCorePropertiesARM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_ARM};
    NextPChain(properties2.pNext, &shaderCorePropertiesARM);
    VkPhysicalDeviceShaderEnqueuePropertiesAMDX shaderEnqueuePropertiesAMDX = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ENQUEUE_PROPERTIES_AMDX};
    NextPChain(properties2.pNext, &shaderEnqueuePropertiesAMDX);
    VkPhysicalDeviceShaderIntegerDotProductProperties shaderIntegerDotProductProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES};
    NextPChain(properties2.pNext, &shaderIntegerDotProductProperties);
    VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT shaderModuleIdentifierPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &shaderModuleIdentifierPropertiesEXT);
    VkPhysicalDeviceShaderObjectPropertiesEXT shaderObjectPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &shaderObjectPropertiesEXT);
    VkPhysicalDeviceShaderSMBuiltinsPropertiesNV shaderSMBuiltinsPropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV};
    NextPChain(properties2.pNext, &shaderSMBuiltinsPropertiesNV);
    VkPhysicalDeviceShaderTileImagePropertiesEXT shaderTileImagePropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &shaderTileImagePropertiesEXT);
    VkPhysicalDeviceShadingRateImagePropertiesNV shadingRateImagePropertiesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV};
    NextPChain(properties2.pNext, &shadingRateImagePropertiesNV);
    VkPhysicalDeviceSubgroupProperties subgroupProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    NextPChain(properties2.pNext, &subgroupProperties);
    VkPhysicalDeviceSubgroupSizeControlProperties subgroupSizeControlProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES};
    NextPChain(properties2.pNext, &subgroupSizeControlProperties);
    VkPhysicalDeviceSubpassShadingPropertiesHUAWEI subpassShadingPropertiesHUAWEI = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_PROPERTIES_HUAWEI};
    NextPChain(properties2.pNext, &subpassShadingPropertiesHUAWEI);
    VkPhysicalDeviceTensorPropertiesARM tensorPropertiesARM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TENSOR_PROPERTIES_ARM};
    NextPChain(properties2.pNext, &tensorPropertiesARM);
    VkPhysicalDeviceTexelBufferAlignmentProperties texelBufferAlignmentProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES};
    NextPChain(properties2.pNext, &texelBufferAlignmentProperties);
    VkPhysicalDeviceTileMemoryHeapPropertiesQCOM tileMemoryHeapPropertiesQCOM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_MEMORY_HEAP_PROPERTIES_QCOM};
    NextPChain(properties2.pNext, &tileMemoryHeapPropertiesQCOM);
    VkPhysicalDeviceTileShadingPropertiesQCOM tileShadingPropertiesQCOM = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_SHADING_PROPERTIES_QCOM};
    NextPChain(properties2.pNext, &tileShadingPropertiesQCOM);
    VkPhysicalDeviceTimelineSemaphoreProperties timelineSemaphoreProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES};
    NextPChain(properties2.pNext, &timelineSemaphoreProperties);
    VkPhysicalDeviceTransformFeedbackPropertiesEXT transformFeedbackPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &transformFeedbackPropertiesEXT);
    VkPhysicalDeviceVertexAttributeDivisorProperties vertexAttributeDivisorProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES};
    NextPChain(properties2.pNext, &vertexAttributeDivisorProperties);
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT vertexAttributeDivisorPropertiesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT};
    NextPChain(properties2.pNext, &vertexAttributeDivisorPropertiesEXT);
    VkPhysicalDeviceVulkan11Properties vulkan11Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
    NextPChain(properties2.pNext, &vulkan11Properties);
    VkPhysicalDeviceVulkan12Properties vulkan12Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
    NextPChain(properties2.pNext, &vulkan12Properties);
    VkPhysicalDeviceVulkan13Properties vulkan13Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    NextPChain(properties2.pNext, &vulkan13Properties);
    VkPhysicalDeviceVulkan14Properties vulkan14Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES};
    NextPChain(properties2.pNext, &vulkan14Properties);
    
    #if defined(MERCURY_LL_OS_ANDROID)
    VkPhysicalDeviceExternalFormatResolvePropertiesANDROID externalFormatResolvePropertiesANDROID = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FORMAT_RESOLVE_PROPERTIES_ANDROID};
    NextPChain(properties2.pNext, &externalFormatResolvePropertiesANDROID);
    #endif

    vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

    if (properties2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        info.type = mercury::ll::graphics::AdapterInfo::Type::Discrete;
    }
    else if (properties2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        info.type = mercury::ll::graphics::AdapterInfo::Type::Integrated;
    }
    else
    {
        info.type = mercury::ll::graphics::AdapterInfo::Type::Unknown;
    }

    info.name = c8string(reinterpret_cast<const char8_t*>(properties2.properties.deviceName));
    info.vendor_name = c8string(reinterpret_cast<const char8_t*>(driverProperties.driverName));
    info.device_id = properties2.properties.deviceID;
    info.vendor_id = properties2.properties.vendorID;
    info.vendor = GetVendorFromVkVendorID(properties2.properties.vendorID);
    info.api_version = properties2.properties.apiVersion;
    info.driver_version = c8string(reinterpret_cast<const char8_t*>(driverProperties.driverInfo));

    MLOG_INFO(u8"Adapter Info: %s, Vendor: %s, Device ID: 0x%04X, Vendor ID: 0x%04X, API Version: %u.%u.%u, Driver Version: %s",
             info.name.c_str(), info.vendor_name.c_str(), info.device_id, info.vendor_id,
             VK_VERSION_MAJOR(info.api_version), VK_VERSION_MINOR(info.api_version), VK_VERSION_PATCH(info.api_version),
             info.driver_version.c_str());

	const char* fakeLayer = nullptr;
	//auto all_extensions = EnumerateVulkanObjects(physicalDevice, fakeLayer, vkEnumerateDeviceExtensionProperties);

    // for(auto& ext : all_extensions)
    // {
    //     MLOG_INFO(u8"Extension: %s", ext.extensionName);
    // }
    return info;
}

void Instance::AcquireAdapter(const AdapterSelectorInfo &selector_info)
{
    MERCURY_ASSERT(gAdapter == nullptr);

    u8 adapterIndex = SelectAdapterByHeuristic(selector_info);
    gVKPhysicalDevice = gAllPhysicalDevices[adapterIndex];

    VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceProperties2(gVKPhysicalDevice, &properties2);
    gPhysicalDeviceAPIVersion = properties2.properties.apiVersion;
    
    gAdapter = new Adapter();
}

void *Adapter::GetNativeHandle()
{
    return gVKPhysicalDevice; // null adapter has no native handle
}

void Adapter::Initialize()
{
    MLOG_DEBUG(u8"Initialize Adapter (Vulkan)");
}

void Adapter::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Adapter (Vulkan)");
}
#endif