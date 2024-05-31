#ifndef __xpuml_xpuml_h__
#define __xpuml_xpuml_h__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * On Windows, set up methods for DLL export
 * define XPUML_STATIC_IMPORT when using xpuml_loader library
 */
#if defined _WINDOWS
    #if !defined XPUML_STATIC_IMPORT
        #if defined XPUML_LIB_EXPORT
            #define DECLDIR __declspec(dllexport)
        #else
            #define DECLDIR __declspec(dllimport)
        #endif
    #else
        #define DECLDIR
    #endif
#else
    #define DECLDIR
#endif

/**
 * XPUML API versioning support
 */
#define XPUML_API_VERSION            1
#define XPUML_API_VERSION_STR        "1"
/**
 * Defining XPUML_NO_UNVERSIONED_FUNC_DEFS will disable "auto upgrading" of APIs.
 * e.g. the user will have to call xpumlInit_v2 instead of xpumlInit. Enable this
 * guard if you need to support older versions of the API
 */
#ifndef XPUML_NO_UNVERSIONED_FUNC_DEFS
    //#define xpumlInit                                    xpumlInit_v2
#endif // #ifndef XPUML_NO_UNVERSIONED_FUNC_DEFS

/***************************************************************************************************/
/** @defgroup xpumlDeviceStructs Device Structs
 *  @{
 */
/***************************************************************************************************/

/**
 * Special constant that some fields take when they are not available.
 * Used when only part of the struct is not available.
 *
 * Each structure explicitly states when to check for this value.
 */
#define XPUML_VALUE_NOT_AVAILABLE (-1)

typedef struct xpumlDevice_st* xpumlDevice_t;

/**
 * Buffer size guaranteed to be large enough for pci bus id
 */
#define XPUML_DEVICE_PCI_BUS_ID_BUFFER_SIZE      32

/**
 * PCI information about a XPU device.
 */
typedef struct xpumlPciInfo_st
{
    unsigned int domain;             //!< The PCI domain on which the device's bus resides, 0 to 0xffffffff
    unsigned int bus;                //!< The bus on which the device resides, 0 to 0xff
    unsigned int device;             //!< The device's id on the bus, 0 to 31
    unsigned int function;           //!< The function on the device, 0 to 7
    unsigned int pciDeviceId;        //!< The combined 16-bit device id and 16-bit vendor id

    unsigned int pciSubSystemId;     //!< The 32-bit Sub System Device ID

    char busId[XPUML_DEVICE_PCI_BUS_ID_BUFFER_SIZE]; //!< The tuple domain:bus:device.function PCI identifier (&amp; NULL terminator)

    unsigned long long reserved[9];  //!< Reserved, sizeof() = 8B * 16
} xpumlPciInfo_t;

/**
 * PCI format string for ::busId
 */
#define XPUML_DEVICE_PCI_BUS_ID_FMT                  "%08X:%02X:%02X.%01X"

/**
 * Utility macro for filling the pci bus id format from a xpumlPciInfo_t
 */
#define XPUML_DEVICE_PCI_BUS_ID_FMT_ARGS(pciInfo)   (pciInfo)->domain, \
                                                    (pciInfo)->bus,    \
                                                    (pciInfo)->device, \
                                                    (pciInfo)->function

/**
 * Utilization information for a device.
 * Each sample period may be between 1 second and 1/6 second, depending on the product being queried.
 */
typedef struct xpumlUtilization_st
{
    unsigned int xpu;                //!< Percent of time over the past sample period during which one or more kernels was executing on the XPU
    unsigned int memory;             //!< Percent of time over the past sample period during which global (device) memory was being read or written

    unsigned long long reserved[15]; //!< Reserved, sizeof() = 8B * 16
} xpumlUtilization_t;

/**
 * Memory allocation information for a device.
 */
typedef struct xpumlMemory_st
{
    unsigned long long pageSizeGlobalMemory;     //!< Global memory page size (in bytes)
    unsigned long long totalGlobalMemory;        //!< Total installed global memory (in bytes)
    unsigned long long freeGlobalMemory;         //!< Unallocated global memory (in bytes)
    unsigned long long usedGlobalMemory;         //!< Allocated global memory (in bytes). Note that the driver may set aside a small amount of memory for bookkeeping
    unsigned long long pageSizeL3Memory;         //!< L3 memory page size (in bytes)
    unsigned long long totalL3Memory;            //!< Total installed l3 memory (in bytes)
    unsigned long long freeL3Memory;             //!< Unallocated l3 memory (in bytes)
    unsigned long long usedL3Memory;             //!< Allocated l3 memory (in bytes). Note that the driver may set aside a small amount of memory for bookkeeping

    unsigned long long reserved[8];              //!< Reserved, sizeof() = 8B * 16
} xpumlMemory_t;

/**
 * Cores allocation information for a device.
 */
typedef struct xpumlCores_st
{
    unsigned long long totalCores;        //!< Total cores
    unsigned long long freeCores;         //!< Unallocated cores
    unsigned long long usedCores;         //!< Allocated cores
} xpumlCores_t;

/**
 * BAR4 Memory allocation Information for a device
 */
typedef struct xpumlBAR4Memory_st
{
    unsigned long long bar4Total;    //!< Total BAR4 Memory (in bytes)
    unsigned long long bar4Free;     //!< Unallocated BAR4 Memory (in bytes)
    unsigned long long bar4Used;     //!< Allocated BAR4 Memory (in bytes)

    unsigned long long reserved[13]; //!< Reserved, sizeof() = 8B * 16
} xpumlBAR4Memory_t;

/**
 * Information about running compute processes on the XPU
 */
typedef struct xpumlProcessInfo_st
{
    unsigned int        pid;                //!< Process ID
    unsigned long long  usedGlobalMemory;   //!< Amount of used global memory in bytes.
    unsigned long long  usedL3Memory;       //!< Amount of used l3 memory in bytes.
                                            //! Under WDDM, \ref XPUML_VALUE_NOT_AVAILABLE is always reported
                                            //! because Windows KMD manages all the memory and not the KUNLUN driver

    unsigned int        root_ns_pid;        //!< root namespace PID
    unsigned int        reserved_u32;       //!< Reserved
    unsigned long long  reserved[12];       //!< Reserved
} xpumlProcessInfo_t;

typedef struct xpumlDeviceAttributes_st
{
    int archId;                             //!< Arch ID enum
    int boardId;                            //!< Board ID enum
    int modelId;                            //!< Model ID enum
    unsigned int clusterCount;              //!< Cluster count
    unsigned int sdnnCount;                 //!< Sdnn count
    unsigned int dmaEngineCount;            //!< DMA Engine count
    unsigned int decoderCount;              //!< Decoder Engine count
    unsigned int encoderCount;              //!< Encoder Engine count
    unsigned int imgProcCount;              //!< ImgProc Engine count
    unsigned int maxVFCount;                //!< Maximum VF count
    unsigned long long globalMemorySizeMB;  //!< Device global memory size (in MiB)
    unsigned long long l3MemorySizeMB;      //!< Device l3 memory size (in MiB)

    unsigned long long sn[2];               //!< Serial Number

    unsigned long long reserved[7];         //!< Reserved
} xpumlDeviceAttributes_t;

/**
 * Represents level relationships within a system between two XPUs
 * The enums are spaced to allow for future relationships
 */
typedef enum xpumlXPULevel_enum
{
    XPUML_TOPOLOGY_INTERNAL           = 0,  // internal
    XPUML_TOPOLOGY_SINGLE             = 10, // all devices that only need traverse a single PCIe switch
    XPUML_TOPOLOGY_MULTIPLE           = 20, // all devices that need not traverse a host bridge
    XPUML_TOPOLOGY_HOSTBRIDGE         = 30, // all devices that are connected to the same host bridge
    XPUML_TOPOLOGY_NODE               = 40, // all devices that are connected to the same NUMA node but possibly multiple host bridges
    XPUML_TOPOLOGY_SYSTEM             = 50  // all devices in the system

    // there is purposefully no COUNT here because of the need for spacing above
} xpumlXPUTopologyLevel_t;

/* Compatibility for CPU->NODE renaming */
#define XPUML_TOPOLOGY_CPU XPUML_TOPOLOGY_NODE

/* P2P Capability Index Status*/
typedef enum xpumlXPUP2PStatus_enum
{
    XPUML_P2P_STATUS_OK     = 0,
    XPUML_P2P_STATUS_CHIPSET_NOT_SUPPORED,
    XPUML_P2P_STATUS_XPU_NOT_SUPPORTED,
    XPUML_P2P_STATUS_IOH_TOPOLOGY_NOT_SUPPORTED,
    XPUML_P2P_STATUS_DISABLED_BY_REGKEY,
    XPUML_P2P_STATUS_NOT_SUPPORTED,
    XPUML_P2P_STATUS_UNKNOWN

} xpumlXPUP2PStatus_t;

/* P2P Capability Index*/
typedef enum xpumlXPUP2PCapsIndex_enum
{
    XPUML_P2P_CAPS_INDEX_READ = 0,
    XPUML_P2P_CAPS_INDEX_WRITE,
    XPUML_P2P_CAPS_INDEX_KLINK,
    XPUML_P2P_CAPS_INDEX_ATOMICS,
    XPUML_P2P_CAPS_INDEX_PROP,
    XPUML_P2P_CAPS_INDEX_UNKNOWN
} xpumlXPUP2PCapsIndex_t;

/**
 *  Represents Type of Sampling Event
 */
typedef enum xpumlSamplingType_enum
{
    XPUML_TOTAL_POWER_SAMPLES        = 0, //!< To represent total power drawn by XPU
    XPUML_XPU_UTILIZATION_SAMPLES    = 1, //!< To represent percent of time during which one or more kernels was executing on the XPU
    XPUML_MEMORY_UTILIZATION_SAMPLES = 2, //!< To represent percent of time during which global (device) memory was being read or written
    XPUML_ENC_UTILIZATION_SAMPLES    = 3, //!< To represent percent of time during which ENC remains busy
    XPUML_DEC_UTILIZATION_SAMPLES    = 4, //!< To represent percent of time during which DEC remains busy
    XPUML_PROCESSOR_CLK_SAMPLES      = 5, //!< To represent processor clock samples
    XPUML_MEMORY_CLK_SAMPLES         = 6, //!< To represent memory clock samples

    // Keep this last
    XPUML_SAMPLINGTYPE_COUNT
} xpumlSamplingType_t;

/**
 * Represents the queryable PCIe utilization counters
 */
typedef enum xpumlPcieUtilCounter_enum
{
    XPUML_PCIE_UTIL_TX_BYTES             = 0, // 1KB granularity
    XPUML_PCIE_UTIL_RX_BYTES             = 1, // 1KB granularity

    // Keep this last
    XPUML_PCIE_UTIL_COUNT
} xpumlPcieUtilCounter_t;

/**
 * Represents the type for sample value returned
 */
typedef enum xpumlValueType_enum
{
    XPUML_VALUE_TYPE_DOUBLE = 0,
    XPUML_VALUE_TYPE_UNSIGNED_INT = 1,
    XPUML_VALUE_TYPE_UNSIGNED_LONG = 2,
    XPUML_VALUE_TYPE_UNSIGNED_LONG_LONG = 3,
    XPUML_VALUE_TYPE_SIGNED_LONG_LONG = 4,

    // Keep this last
    XPUML_VALUE_TYPE_COUNT
} xpumlValueType_t;


/**
 * Union to represent different types of Value
 */
typedef union xpumlValue_st
{
    double dVal;                    //!< If the value is double
    unsigned int uiVal;             //!< If the value is unsigned int
    unsigned long ulVal;            //!< If the value is unsigned long
    unsigned long long ullVal;      //!< If the value is unsigned long long
    signed long long sllVal;        //!< If the value is signed long long
} xpumlValue_t;

/**
 * Information for Sample
 */
typedef struct xpumlSample_st
{
    unsigned long long timeStamp;    //!< CPU Timestamp in microseconds
    xpumlValue_t sampleValue;        //!< Sample Value
} xpumlSample_t;

/**
 * Represents type of perf policy for which violation times can be queried
 */
typedef enum xpumlPerfPolicyType_enum
{
    XPUML_PERF_POLICY_POWER = 0,              //!< How long did power violations cause the XPU to be below application clocks
    XPUML_PERF_POLICY_THERMAL = 1,            //!< How long did thermal violations cause the XPU to be below application clocks
    XPUML_PERF_POLICY_SYNC_BOOST = 2,         //!< How long did sync boost cause the XPU to be below application clocks
    XPUML_PERF_POLICY_BOARD_LIMIT = 3,        //!< How long did the board limit cause the XPU to be below application clocks
    XPUML_PERF_POLICY_LOW_UTILIZATION = 4,    //!< How long did low utilization cause the XPU to be below application clocks
    XPUML_PERF_POLICY_RELIABILITY = 5,        //!< How long did the board reliability limit cause the XPU to be below application clocks

    XPUML_PERF_POLICY_TOTAL_APP_CLOCKS = 10,  //!< Total time the XPU was held below application clocks by any limiter (0 - 5 above)
    XPUML_PERF_POLICY_TOTAL_BASE_CLOCKS = 11, //!< Total time the XPU was held below base clocks

    // Keep this last
    XPUML_PERF_POLICY_COUNT
} xpumlPerfPolicyType_t;

/**
 * Struct to hold perf policy violation status data
 */
typedef struct xpumlViolationTime_st
{
    unsigned long long referenceTime;  //!< referenceTime represents CPU timestamp in microseconds
    unsigned long long violationTime;  //!< violationTime in Nanoseconds
} xpumlViolationTime_t;

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlDeviceEnumvs Device Enums
 *  @{
 */
/***************************************************************************************************/

/**
 * Generic enable/disable enum.
 */
typedef enum xpumlEnableState_enum
{
    XPUML_FEATURE_DISABLED    = 0,     //!< Feature disabled
    XPUML_FEATURE_ENABLED     = 1      //!< Feature enabled
} xpumlEnableState_t;

//! Generic flag used to specify the default behavior of some functions. See description of particular functions for details.
#define xpumlFlagDefault     0x00
//! Generic flag used to force some behavior. See description of particular functions for details.
#define xpumlFlagForce       0x01

/**
 * The device arch enum
 */
typedef enum xpumlDeviceArch_enum
{
    XPUML_DEVICE_ARCH_UNKNOWN          = 0,

    XPUML_DEVICE_ARCH_KL1              = 1,
    XPUML_DEVICE_ARCH_KL2              = 2,
    XPUML_DEVICE_ARCH_KL3              = 3,

    // Keep this last
    XPUML_DEVICE_ARCH_COUNT
} xpumlDeviceArch_t;

/**
 * The device board enum
 */
typedef enum xpumlDeviceBoard_enum
{
    XPUML_DEVICE_BOARD_UNKNOWN          = 0,

    XPUML_DEVICE_BOARD_KL1_K100         = 1,
    XPUML_DEVICE_BOARD_KL1_K200         = 2,

    XPUML_DEVICE_BOARD_KL2              = 98,
    XPUML_DEVICE_BOARD_KL2_R100         = 99,
    XPUML_DEVICE_BOARD_KL2_R200         = 100,
    XPUML_DEVICE_BOARD_KL2_R300         = 101,
    XPUML_DEVICE_BOARD_KL2_R200_8F      = 102,
    XPUML_DEVICE_BOARD_KL2_R200_8FS     = 103,
    XPUML_DEVICE_BOARD_KL2_R200_DEBUG_BOARD = 104,
    XPUML_DEVICE_BOARD_KL2_R420         = 105,
    XPUML_DEVICE_BOARD_KL2_RG800        = 106,

    // Keep this last
    XPUML_DEVICE_BOARD_COUNT
} xpumlDeviceBoard_t;

/**
 * Clock types.
 *
 * All speeds are in Mhz.
 */
typedef enum xpumlClockType_enum
{
    XPUML_CLOCK_XPU       = 0,        //!< XPU clock domain

    // Keep this last
    XPUML_CLOCK_COUNT //!< Count of clock types
} xpumlClockType_t;

/**
 * The device model enum
 */
typedef enum xpumlDeviceModel_enum
{
    // be compatible with DeviceModel in xpu/defs.h
    XPUML_DEVICE_MODEL_UNKNOWN                        = 9999,

    XPUML_DEVICE_MODEL_KL1_K200                       = 0,
    XPUML_DEVICE_MODEL_KL1_K100                       = 1,
    XPUML_DEVICE_MODEL_KL1_END                        = 1,

    XPUML_DEVICE_MODEL_KL2_BEGIN                      = 2,
    XPUML_DEVICE_MODEL_KL2_R200                       = 2,
    XPUML_DEVICE_MODEL_KL2_R300                       = 3,
    XPUML_DEVICE_MODEL_KL2_R200_8F                    = 4,
    XPUML_DEVICE_MODEL_KL2_R200_8FS                   = 5,
    XPUML_DEVICE_MODEL_KL2_R100                       = 6,
    XPUML_DEVICE_MODEL_KL2_R200_DEBUG_BOARD           = 7,
    XPUML_DEVICE_MODEL_KL2_R420                       = 8,
    XPUML_DEVICE_MODEL_KL2_RG800                      = 9,

    XPUML_DEVICE_MODEL_KL2_R200_SRIOV_PF              = 200,
    XPUML_DEVICE_MODEL_KL2_R200_SRIOV_VF_ONE_OF_ONE   = 201,
    XPUML_DEVICE_MODEL_KL2_R200_SRIOV_VF_ONE_OF_TWO   = 202,
    XPUML_DEVICE_MODEL_KL2_R200_SRIOV_VF_ONE_OF_THREE = 203,

    XPUML_DEVICE_MODEL_KL2_R300_SRIOV_PF              = 204,
    XPUML_DEVICE_MODEL_KL2_R300_SRIOV_VF_ONE_OF_ONE   = 205,
    XPUML_DEVICE_MODEL_KL2_R300_SRIOV_VF_ONE_OF_TWO   = 206,
    XPUML_DEVICE_MODEL_KL2_R300_SRIOV_VF_ONE_OF_THREE = 207,

    XPUML_DEVICE_MODEL_KL2_R200_8F_SRIOV_PF              = 208,
    XPUML_DEVICE_MODEL_KL2_R200_8F_SRIOV_VF_ONE_OF_ONE   = 209,
    XPUML_DEVICE_MODEL_KL2_R200_8F_SRIOV_VF_ONE_OF_TWO   = 210,
    XPUML_DEVICE_MODEL_KL2_R200_8F_SRIOV_VF_ONE_OF_THREE = 211,

    XPUML_DEVICE_MODEL_KL2_R200_8FS_SRIOV_PF              = 212,
    XPUML_DEVICE_MODEL_KL2_R200_8FS_SRIOV_VF_ONE_OF_ONE   = 213,
    XPUML_DEVICE_MODEL_KL2_R200_8FS_SRIOV_VF_ONE_OF_TWO   = 214,
    XPUML_DEVICE_MODEL_KL2_R200_8FS_SRIOV_VF_ONE_OF_THREE = 215,

    XPUML_DEVICE_MODEL_KL2_RG800_SRIOV_PF              = 216,
    XPUML_DEVICE_MODEL_KL2_RG800_SRIOV_VF_ONE_OF_ONE   = 217,
    XPUML_DEVICE_MODEL_KL2_RG800_SRIOV_VF_ONE_OF_TWO   = 218,
    XPUML_DEVICE_MODEL_KL2_RG800_SRIOV_VF_ONE_OF_THREE = 219,

    XPUML_DEVICE_MODEL_KL2_END                    = 299,

    // KL3
    XPUML_DEVICE_MODEL_KL3_BEGIN                  = 300,
    XPUML_DEVICE_MODEL_KL3                        = 300,
    XPUML_DEVICE_MODEL_KL3_END                    = 599,

    // Keep this last
    XPUML_DEVICE_MODEL_COUNT
} xpumlDeviceModel_t;

/**
 * Temperature thresholds.
 */
typedef enum xpumlTemperatureThresholds_enum
{
    XPUML_TEMPERATURE_THRESHOLD_SHUTDOWN      = 0, // Temperature at which the XPU will
                                                   // shut down for HW protection
    XPUML_TEMPERATURE_THRESHOLD_SLOWDOWN      = 1, // Temperature at which the XPU will
                                                   // begin HW slowdown
    XPUML_TEMPERATURE_THRESHOLD_MEM_MAX       = 2, // Memory Temperature at which the XPU will
                                                   // begin SW slowdown
    XPUML_TEMPERATURE_THRESHOLD_XPU_MAX       = 3, // XPU Temperature at which the XPU
                                                   // can be throttled below base clock
    XPUML_TEMPERATURE_THRESHOLD_ACOUSTIC_MIN  = 4, // Minimum XPU Temperature that can be
                                                   // set as acoustic threshold
    XPUML_TEMPERATURE_THRESHOLD_ACOUSTIC_CURR = 5, // Current temperature that is set as
                                                   // acoustic threshold.
    XPUML_TEMPERATURE_THRESHOLD_ACOUSTIC_MAX  = 6, // Maximum XPU temperature that can be
                                                   // set as acoustic threshold.
    // Keep this last
    XPUML_TEMPERATURE_THRESHOLD_COUNT
} xpumlTemperatureThresholds_t;

/**
 * Temperature sensors.
 */
typedef enum xpumlTemperatureSensors_enum
{
    XPUML_TEMPERATURE_XPU      = 0,    //!< Temperature sensor for the XPU die

    // Keep this last
    XPUML_TEMPERATURE_COUNT
} xpumlTemperatureSensors_t;

/**
 * Return values for XPUML API calls.
 */
typedef enum xpumlReturn_enum
{
    // cppcheck-suppress *
    XPUML_SUCCESS = 0,                        //!< The operation was successful
    XPUML_ERROR_UNINITIALIZED = 1,            //!< XPUML was not first initialized with xpumlInit()
    XPUML_ERROR_INVALID_ARGUMENT = 2,         //!< A supplied argument is invalid
    XPUML_ERROR_NOT_SUPPORTED = 3,            //!< The requested operation is not available on target device
    XPUML_ERROR_NO_PERMISSION = 4,            //!< The current user does not have permission for operation
    XPUML_ERROR_ALREADY_INITIALIZED = 5,      //!< Deprecated: Multiple initializations are now allowed through ref counting
    XPUML_ERROR_NOT_FOUND = 6,                //!< A query to find an object was unsuccessful
    XPUML_ERROR_INSUFFICIENT_SIZE = 7,        //!< An input argument is not large enough
    XPUML_ERROR_INSUFFICIENT_POWER = 8,       //!< A device's external power cables are not properly attached
    XPUML_ERROR_DRIVER_NOT_LOADED = 9,        //!< KUNLUN driver is not loaded
    XPUML_ERROR_TIMEOUT = 10,                 //!< User provided timeout passed
    XPUML_ERROR_IRQ_ISSUE = 11,               //!< KUNLUN Kernel detected an interrupt issue with a XPU
    XPUML_ERROR_LIBRARY_NOT_FOUND = 12,       //!< XPUML Shared Library couldn't be found or loaded
    XPUML_ERROR_FUNCTION_NOT_FOUND = 13,      //!< Local version of XPUML doesn't implement this function
    XPUML_ERROR_CORRUPTED_INFOROM = 14,       //!< infoROM is corrupted
    XPUML_ERROR_XPU_IS_LOST = 15,             //!< The XPU has fallen off the bus or has otherwise become inaccessible
    XPUML_ERROR_RESET_REQUIRED = 16,          //!< The XPU requires a reset before it can be used again
    XPUML_ERROR_OPERATING_SYSTEM = 17,        //!< The XPU control device has been blocked by the operating system/cgroups
    XPUML_ERROR_LIB_RM_VERSION_MISMATCH = 18, //!< RM detects a driver/library version mismatch
    XPUML_ERROR_IN_USE = 19,                  //!< An operation cannot be performed because the XPU is currently in use
    XPUML_ERROR_MEMORY = 20,                  //!< Insufficient memory
    XPUML_ERROR_NO_DATA = 21,                 //!< No data
    XPUML_ERROR_VXPU_ECC_NOT_SUPPORTED = 22,  //!< The requested vxpu operation is not available on target device, becasue ECC is enabled
    XPUML_ERROR_INSUFFICIENT_RESOURCES = 23,  //!< Ran out of critical resources, other than memory
    XPUML_ERROR_FREQ_NOT_SUPPORTED = 24,  //!< Ran out of critical resources, other than memory
    XPUML_ERROR_UNKNOWN = 999                 //!< An internal driver error occurred
} xpumlReturn_t;

/**
 * See \ref xpumlDeviceGetMemoryErrorCounter
 */
typedef enum xpumlMemoryLocation_enum
{
    XPUML_MEMORY_LOCATION_GLOBAL_MEMORY        = 0,    //!< Global memory
    XPUML_MEMORY_LOCATION_L3_MEMORY            = 1,    //!< L3 memory

    // Keep this last
    XPUML_MEMORY_LOCATION_COUNT              //!< This counts the number of memory locations the driver knows about
} xpumlMemoryLocation_t;

/** @} */

/***************************************************************************************************/
/** @addtogroup virtualXPU
 *  @{
 */
/***************************************************************************************************/
/** @defgroup xpumlVirtualXPUEnums vXPU Enums
 *  @{
 */
/***************************************************************************************************/

/*!
 * XPU virtualization mode types.
 */
typedef enum xpumlXPUVirtualizationMode {
    XPUML_XPU_VIRTUALIZATION_MODE_NONE = 0,  //!< Represents Bare Metal XPU
    XPUML_XPU_VIRTUALIZATION_MODE_PASSTHROUGH = 1,  //!< Device is associated with XPU-Passthorugh
    XPUML_XPU_VIRTUALIZATION_MODE_VXPU = 2,  //!< Device is associated with vXPU inside virtual machine.
    XPUML_XPU_VIRTUALIZATION_MODE_HOST_VXPU = 3,  //!< Device is associated with VGX hypervisor in vXPU mode
    XPUML_XPU_VIRTUALIZATION_MODE_HOST_VSGA = 4   //!< Device is associated with VGX hypervisor in vSGA mode
} xpumlXPUVirtualizationMode_t;

/**
 * Host vXPU modes
 */
typedef enum xpumlHostVxpuMode_enum
{
    XPUML_HOST_VXPU_MODE_NON_SRIOV    = 0,     //!< Non SR-IOV mode
    XPUML_HOST_VXPU_MODE_SRIOV_OFF    = 1,     //!< SR-IOV off mode
    XPUML_HOST_VXPU_MODE_SRIOV_ON     = 2,     //!< SR-IOV on mode
} xpumlHostVxpuMode_t;

/*!
 * Types of VM identifiers
 */
typedef enum xpumlVxpuVmIdType {
    XPUML_VXPU_VM_ID_DOMAIN_ID = 0, //!< VM ID represents DOMAIN ID
    XPUML_VXPU_VM_ID_UUID = 1       //!< VM ID represents UUID
} xpumlVxpuVmIdType_t;

/**
 * vXPU GUEST info state.
 */
typedef enum xpumlVxpuGuestInfoState_enum
{
    XPUML_VXPU_INSTANCE_GUEST_INFO_STATE_UNINITIALIZED = 0,  //!< Guest-dependent fields uninitialized
    XPUML_VXPU_INSTANCE_GUEST_INFO_STATE_INITIALIZED   = 1   //!< Guest-dependent fields initialized
} xpumlVxpuGuestInfoState_t;

/**
 * XPUML Device state
 */
typedef enum xpumlDeviceState_enum
{
    XPUML_DEVICE_STATE_UNUSED  = 0,
    XPUML_DEVICE_STATE_RUNNING = 201,
    XPUML_DEVICE_STATE_IN_RESET = 205,
    XPUML_DEVICE_STATE_ERROR = 207,
    XPUML_DEVICE_STATE_IN_EXCEPTION = 208,
} xpumlDeviceState_t;

/** @} */

/***************************************************************************************************/

/** @defgroup xpumlVxpuConstants vXPU Constants
 *  @{
 */
/***************************************************************************************************/

#define XPUML_VXPU_NAME_BUFFER_SIZE          64

#define INVALID_XPU_INSTANCE_PROFILE_ID     0xFFFFFFFF

#define INVALID_XPU_INSTANCE_ID             0xFFFFFFFF

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlVxpuStructs vXPU Structs
 *  @{
 */
/***************************************************************************************************/

typedef unsigned int xpumlVxpuTypeId_t;

typedef unsigned int xpumlVxpuInstance_t;

/**
 * Structure to store utilization value and process Id
 */
typedef struct xpumlProcessUtilizationSample_st
{
    unsigned int        pid;            //!< PID of process
    unsigned long long  timeStamp;      //!< CPU Timestamp in microseconds
    unsigned int        smUtil;         //!< SM (3D/Compute) Util Value
    unsigned int        memUtil;        //!< Frame Buffer Memory Util Value
    unsigned int        encUtil;        //!< Encoder Util Value
    unsigned int        decUtil;        //!< Decoder Util Value
} xpumlProcessUtilizationSample_t;

/**
 * Simplified chip architecture
 */
typedef unsigned int xpumlDeviceArchitecture_t;

/**
 * PCI bus types
 */
#define XPUML_BUS_TYPE_UNKNOWN  0
#define XPUML_BUS_TYPE_PCI      1
#define XPUML_BUS_TYPE_PCIE     2
#define XPUML_BUS_TYPE_FPCI     3
#define XPUML_BUS_TYPE_AGP      4

typedef unsigned int xpumlBusType_t;

/** @} */
/** @} */

/***************************************************************************************************/
/** @defgroup xpumlFieldValueEnums Field Value Enums
 *  @{
 */
/***************************************************************************************************/

/**
 * Field Identifiers.
 *
 * All Identifiers pertain to a device. Each ID is only used once and is guaranteed never to change.
 */
#define XPUML_FI_DEV_ECC_CURRENT           1   //!< Current ECC mode. 1=Active. 0=Inactive
#define XPUML_FI_MAX 161 //!< One greater than the largest field ID defined above

/**
 * Information for a Field Value Sample
 */
typedef struct xpumlFieldValue_st
{
    unsigned int fieldId;       //!< ID of the XPUML field to retrieve. This must be set before any call that uses this struct. See the constants starting with XPUML_FI_ above.
    unsigned int scopeId;       //!< Scope ID can represent data used by XPUML depending on fieldId's context. For example, for KLink throughput counter data, scopeId can represent linkId.
    long long timestamp;        //!< CPU Timestamp of this value in microseconds since 1970
    long long latencyUsec;      //!< How long this field value took to update (in usec) within XPUML. This may be averaged across several fields that are serviced by the same driver call.
    xpumlValueType_t valueType;  //!< Type of the value stored in value
    xpumlReturn_t xpumlReturn;    //!< Return code for retrieving this value. This must be checked before looking at value, as value is undefined if xpumlReturn != XPUML_SUCCESS
    xpumlValue_t value;          //!< Value for this field. This is only valid if xpumlReturn == XPUML_SUCCESS
} xpumlFieldValue_t;

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlDrainDefs definitions related to the drain state
 *  @{
 */
/***************************************************************************************************/

/**
 *  Is the XPU device to be removed from the kernel by xpumlDeviceRemoveXPU()
 */
typedef enum xpumlDetachXPUState_enum
{
    XPUML_DETACH_XPU_KEEP         = 0,
    XPUML_DETACH_XPU_REMOVE
} xpumlDetachXPUState_t;

/**
 *  Parent bridge PCIe link state requested by xpumlDeviceRemoveXPU()
 */
typedef enum xpumlPcieLinkState_enum
{
    XPUML_PCIE_LINK_KEEP         = 0,
    XPUML_PCIE_LINK_SHUT_DOWN
} xpumlPcieLinkState_t;

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlInitializationAndCleanup Initialization and Cleanup
 * This chapter describes the methods that handle XPUML initialization and cleanup.
 * It is the user's responsibility to call \ref xpumlInit() before calling any other methods, and
 * xpumlShutdown() once XPUML is no longer being used.
 *  @{
 */
/***************************************************************************************************/

#define XPUML_INIT_FLAG_NO_XPUS      1   //!< Don't fail xpumlInit() when no XPUs are found
#define XPUML_INIT_FLAG_NO_ATTACH    2   //!< Don't attach XPUs

/**
 * Initialize XPUML, but don't initialize any XPUs yet.
 *
 * \note xpumlInit_v3 introduces a "flags" argument, that allows passing boolean values
 *       modifying the behaviour of xpumlInit().
 * \note In XPUML 5.319 new xpumlInit_v2 has replaced xpumlInit"_v1" (default in XPUML 4.304 and older) that
 *       did initialize all XPU devices in the system.
 *
 * This allows XPUML to communicate with a XPU
 * when other XPUs in the system are unstable or in a bad state.  When using this API, XPUs are
 * discovered and initialized in xpumlDeviceGetHandleBy* functions instead.
 *
 * \note To contrast xpumlInit_v2 with xpumlInit"_v1", XPUML 4.304 xpumlInit"_v1" will fail when any detected XPU is in
 *       a bad or unstable state.
 *
 * For all products.
 *
 * This method, should be called once before invoking any other methods in the library.
 * A reference count of the number of initializations is maintained.  Shutdown only occurs
 * when the reference count reaches zero.
 *
 * @return
 *         - \ref XPUML_SUCCESS                   if XPUML has been properly initialized
 *         - \ref XPUML_ERROR_DRIVER_NOT_LOADED   if KUNLUN driver is not running
 *         - \ref XPUML_ERROR_NO_PERMISSION       if XPUML does not have permission to talk to the driver
 *         - \ref XPUML_ERROR_UNKNOWN             on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlInit(void);

/**
 * xpumlInitWithFlags is a variant of xpumlInit(), that allows passing a set of boolean values
 *       modifying the behaviour of xpumlInit().
 *       Other than the "flags" parameter it is completely similar to \ref xpumlInit_v2.
 *
 * For all products.
 *
 * @param flags                                 behaviour modifier flags
 *
 * @return
 *         - \ref XPUML_SUCCESS                   if XPUML has been properly initialized
 *         - \ref XPUML_ERROR_DRIVER_NOT_LOADED   if KUNLUN driver is not running
 *         - \ref XPUML_ERROR_NO_PERMISSION       if XPUML does not have permission to talk to the driver
 *         - \ref XPUML_ERROR_UNKNOWN             on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlInitWithFlags(unsigned int flags);

/**
 * Shut down XPUML by releasing all XPU resources previously allocated with \ref xpumlInit_v2().
 *
 * For all products.
 *
 * This method should be called after XPUML work is done, once for each call to \ref xpumlInit_v2()
 * A reference count of the number of initializations is maintained.  Shutdown only occurs
 * when the reference count reaches zero.  For backwards compatibility, no error is reported if
 * xpumlShutdown() is called more times than xpumlInit().
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if XPUML has been properly shut down
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlShutdown(void);

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlErrorReporting Error reporting
 * This chapter describes helper functions for error reporting routines.
 *  @{
 */
/***************************************************************************************************/

/**
 * Helper method for converting XPUML error codes into readable strings.
 *
 * For all products.
 *
 * @param result                               XPUML error code to convert
 *
 * @return String representation of the error.
 *
 */
const DECLDIR char* xpumlErrorString(xpumlReturn_t result);
/** @} */


/***************************************************************************************************/
/** @defgroup xpumlConstants Constants
 *  @{
 */
/***************************************************************************************************/

/**
 * Buffer size guaranteed to be large enough for storing XPU identifiers.
 */
#define XPUML_DEVICE_UUID_BUFFER_SIZE                  96

/**
 * Buffer size guaranteed to be large enough for \ref xpumlDeviceGetBoardPartNumber
 */
#define XPUML_DEVICE_PART_NUMBER_BUFFER_SIZE           80

/**
 * Buffer size guaranteed to be large enough for \ref xpumlSystemGetDriverVersion
 */
#define XPUML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE        80

/**
 * Buffer size guaranteed to be large enough for \ref xpumlSystemGetXPUMLVersion
 */
#define XPUML_SYSTEM_XPUML_VERSION_BUFFER_SIZE         80

/**
 * Buffer size guaranteed to be large enough for storing XPU device names.
 */
#define XPUML_DEVICE_NAME_BUFFER_SIZE                  96

/**
 * Buffer size guaranteed to be large enough for \ref xpumlDeviceGetSerial
 */
#define XPUML_DEVICE_SERIAL_BUFFER_SIZE                (16 + 1)

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlSystemQueries System Queries
 * This chapter describes the queries that XPUML can perform against the local system. These queries
 * are not device-specific.
 *  @{
 */
/***************************************************************************************************/

/**
 * Retrieves the version of the system's graphics driver.
 *
 * For all products.
 *
 * The version identifier is an alphanumeric string.  It will not exceed 80 characters in length
 * (including the NULL terminator).  See \ref xpumlConstants::XPUML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE.
 *
 * @param version                              Reference in which to return the version identifier
 * @param length                               The maximum allowed length of the string returned in \a version
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a version has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a version is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a length is too small
 */
xpumlReturn_t DECLDIR xpumlSystemGetDriverVersion(char *version, unsigned int length);

/**
 * Retrieves the version of the XPUML library.
 *
 * For all products.
 *
 * The version identifier is an alphanumeric string.  It will not exceed 80 characters in length
 * (including the NULL terminator).  See \ref xpumlConstants::XPUML_SYSTEM_XPUML_VERSION_BUFFER_SIZE.
 *
 * @param version                              Reference in which to return the version identifier
 * @param length                               The maximum allowed length of the string returned in \a version
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a version has been set
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a version is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a length is too small
 */
xpumlReturn_t DECLDIR xpumlSystemGetXPUMLVersion(char *version, unsigned int length);

/**
 * Retrieves the version of the CUDA driver.
 *
 * For all products.
 *
 * The CUDA driver version returned will be retreived from the currently installed version of CUDA.
 * If the cuda library is not found, this function will return a known supported version number.
 *
 * @param cudaDriverVersion                    Reference in which to return the version identifier
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a cudaDriverVersion has been set
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a cudaDriverVersion is NULL
 */
//xpumlReturn_t DECLDIR xpumlSystemGetXPURTVersion(char *version, unsigned int length);

/**
 * Macros for converting the CUDA driver version number to Major and Minor version numbers.
 */
// FIXME(miaotianxiang):
#define XPUML_XPU_RT_VERSION_MAJOR(v) ((v)/1000)
#define XPUML_XPU_RT_VERSION_MINOR(v) (((v)%1000)/10)

/**
 * Gets name of the process with provided process id
 *
 * For all products.
 *
 * Returned process name is cropped to provided length.
 * name string is encoded in ANSI.
 *
 * @param pid                                  The identifier of the process
 * @param name                                 Reference in which to return the process name
 * @param length                               The maximum allowed length of the string returned in \a name
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a name has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a name is NULL or \a length is 0.
 *         - \ref XPUML_ERROR_NOT_FOUND         if process doesn't exists
 *         - \ref XPUML_ERROR_NO_PERMISSION     if the user doesn't have permission to perform this operation
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlSystemGetProcessName(unsigned int pid, char *name, unsigned int length);

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlDeviceQueries Device Queries
 * This chapter describes that queries that XPUML can perform against each device.
 * In each case the device is identified with an xpumlDevice_t handle. This handle is obtained by
 * calling one of \ref xpumlDeviceGetHandleByIndex_v2(), \ref xpumlDeviceGetHandleBySerial(),
 * \ref xpumlDeviceGetHandleByPciBusId_v2(). or \ref xpumlDeviceGetHandleByUUID().
 *  @{
 */
/***************************************************************************************************/

 /**
 * Retrieves the number of compute devices in the system. A compute device is a single XPU.
 *
 * For all products.
 *
 * Note: New xpumlDeviceGetCount_v2 (default in XPUML 5.319) returns count of all devices in the system
 *       even if xpumlDeviceGetHandleByIndex_v2 returns XPUML_ERROR_NO_PERMISSION for such device.
 *       Update your code to handle this error, or use XPUML 4.304 or older xpuml header file.
 *       For backward binary compatibility reasons _v1 version of the API is still present in the shared
 *       library.
 *       Old _v1 version of xpumlDeviceGetCount doesn't count devices that XPUML has no permission to talk to.
 *
 * @param deviceCount                          Reference in which to return the number of accessible devices
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a deviceCount has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a deviceCount is NULL
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCount(unsigned int *deviceCount);

/**
 * Get attributes (engine counts etc.) for the given XPUML device handle.
 *
 * @note This API currently only supports MIG device handles.
 *
 * For Ampere &tm; or newer fully supported devices.
 * Supported on Linux only.
 *
 * @param device                               XPUML device handle
 * @param attributes                           Device attributes
 *
 * @return
 *        - \ref XPUML_SUCCESS                  if \a device attributes were successfully retrieved
 *        - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a device handle is invalid
 *        - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *        - \ref XPUML_ERROR_NOT_SUPPORTED      if this query is not supported by the device
 *        - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetAttributes(xpumlDevice_t device, xpumlDeviceAttributes_t *attributes);

/**
 * Acquire the handle for a particular device, based on its index.
 *
 * For all products.
 *
 * Valid indices are derived from the \a accessibleDevices count returned by
 *   \ref xpumlDeviceGetCount_v2(). For example, if \a accessibleDevices is 2 the valid indices
 *   are 0 and 1, corresponding to XPU 0 and XPU 1.
 *
 * The order in which XPUML enumerates devices has no guarantees of consistency between reboots. For that reason it
 *   is recommended that devices be looked up by their PCI ids or UUID. See
 *   \ref xpumlDeviceGetHandleByUUID() and \ref xpumlDeviceGetHandleByPciBusId_v2().
 *
 * Note: The XPUML index may not correlate with other APIs, such as the CUDA device index.
 *
 * Starting from XPUML 5, this API causes XPUML to initialize the target XPU
 * XPUML may initialize additional XPUs if:
 *  - The target XPU is an SLI slave
 *
 * Note: New xpumlDeviceGetCount_v2 (default in XPUML 5.319) returns count of all devices in the system
 *       even if xpumlDeviceGetHandleByIndex_v2 returns XPUML_ERROR_NO_PERMISSION for such device.
 *       Update your code to handle this error, or use XPUML 4.304 or older xpuml header file.
 *       For backward binary compatibility reasons _v1 version of the API is still present in the shared
 *       library.
 *       Old _v1 version of xpumlDeviceGetCount doesn't count devices that XPUML has no permission to talk to.
 *
 *       This means that xpumlDeviceGetHandleByIndex_v2 and _v1 can return different devices for the same index.
 *       If you don't touch macros that map old (_v1) versions to _v2 versions at the top of the file you don't
 *       need to worry about that.
 *
 * @param index                                The index of the target XPU, >= 0 and < \a accessibleDevices
 * @param device                               Reference in which to return the device handle
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a device has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a index is invalid or \a device is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_POWER if any attached devices have improperly attached external power cables
 *         - \ref XPUML_ERROR_NO_PERMISSION      if the user doesn't have permission to talk to this device
 *         - \ref XPUML_ERROR_IRQ_ISSUE          if KUNLUN kernel detected an interrupt issue with the attached XPUs
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 *
 * @see xpumlDeviceGetIndex
 * @see xpumlDeviceGetCount
 */
xpumlReturn_t DECLDIR xpumlDeviceGetHandleByIndex(unsigned int index, xpumlDevice_t *device);

/**
 * Retrieves physical id for the device. The physical id for the device is such that the xpu device node file for
 * each XPU will have the form /dev/xpu[physical id].
 *
 * @param devID                                The physical device id of the target XPU
 * @param device                               Reference in which to return the device handle
 *
 * @returns
 *         - \ref XPUML_SUCCESS                 if \a name has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetId(xpumlDevice_t device, int *devId);

/**
 * Acquire the handle for a particular device, based on its board serial number.
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * This number corresponds to the value printed directly on the board, and to the value returned by
 *   \ref xpumlDeviceGetSerial().
 *
 * @deprecated Since more than one XPU can exist on a single board this function is deprecated in favor
 *             of \ref xpumlDeviceGetHandleByUUID.
 *             For dual XPU boards this function will return XPUML_ERROR_INVALID_ARGUMENT.
 *
 * Starting from XPUML 5, this API causes XPUML to initialize the target XPU
 * XPUML may initialize additional XPUs as it searches for the target XPU
 *
 * @param serial                               The board serial number of the target XPU
 * @param device                               Reference in which to return the device handle
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a device has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a serial is invalid, \a device is NULL or more than one
 *                                              device has the same serial (dual XPU boards)
 *         - \ref XPUML_ERROR_NOT_FOUND          if \a serial does not match a valid device on the system
 *         - \ref XPUML_ERROR_INSUFFICIENT_POWER if any attached devices have improperly attached external power cables
 *         - \ref XPUML_ERROR_IRQ_ISSUE          if KUNLUN kernel detected an interrupt issue with the attached XPUs
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if any XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 *
 * @see xpumlDeviceGetSerial
 * @see xpumlDeviceGetHandleByUUID
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetHandleBySerial(const char *serial, xpumlDevice_t *device);

/**
 * Acquire the handle for a particular device, based on its globally unique immutable UUID associated with each device.
 *
 * For all products.
 *
 * @param uuid                                 The UUID of the target XPU or MIG instance
 * @param device                               Reference in which to return the device handle or MIG device handle
 *
 * Starting from XPUML 5, this API causes XPUML to initialize the target XPU
 * XPUML may initialize additional XPUs as it searches for the target XPU
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a device has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a uuid is invalid or \a device is null
 *         - \ref XPUML_ERROR_NOT_FOUND          if \a uuid does not match a valid device on the system
 *         - \ref XPUML_ERROR_INSUFFICIENT_POWER if any attached devices have improperly attached external power cables
 *         - \ref XPUML_ERROR_IRQ_ISSUE          if KUNLUN kernel detected an interrupt issue with the attached XPUs
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if any XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 *
 * @see xpumlDeviceGetUUID
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetHandleByUUID(const char *uuid, xpumlDevice_t *device);

/**
 * Acquire the handle for a particular device, based on its PCI bus id.
 *
 * For all products.
 *
 * This value corresponds to the xpumlPciInfo_t::busId returned by \ref xpumlDeviceGetPciInfo_v3().
 *
 * Starting from XPUML 5, this API causes XPUML to initialize the target XPU
 * XPUML may initialize additional XPUs if:
 *  - The target XPU is an SLI slave
 *
 * \note XPUML 4.304 and older version of xpumlDeviceGetHandleByPciBusId"_v1" returns XPUML_ERROR_NOT_FOUND
 *       instead of XPUML_ERROR_NO_PERMISSION.
 *
 * @param pciBusId                             The PCI bus id of the target XPU
 * @param device                               Reference in which to return the device handle
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a device has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a pciBusId is invalid or \a device is NULL
 *         - \ref XPUML_ERROR_NOT_FOUND          if \a pciBusId does not match a valid device on the system
 *         - \ref XPUML_ERROR_INSUFFICIENT_POWER if the attached device has improperly attached external power cables
 *         - \ref XPUML_ERROR_NO_PERMISSION      if the user doesn't have permission to talk to this device
 *         - \ref XPUML_ERROR_IRQ_ISSUE          if KUNLUN kernel detected an interrupt issue with the attached XPUs
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetHandleByPciBusId(const char *pciBusId, xpumlDevice_t *device);

/**
 * Retrieves the name of this device.
 *
 * For all products.
 *
 * The name is an alphanumeric string that denotes a particular product, e.g. Tesla &tm; C2070. It will not
 * exceed 96 characters in length (including the NULL terminator).  See \ref
 * xpumlConstants::XPUML_DEVICE_NAME_BUFFER_SIZE.
 *
 * When used with MIG device handles the API returns MIG device names which can be used to identify devices
 * based on their attributes.
 *
 * @param device                               The identifier of the target device
 * @param name                                 Reference in which to return the product name
 * @param length                               The maximum allowed length of the string returned in \a name
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a name has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, or \a name is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a length is too small
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetName(xpumlDevice_t device, char *name, unsigned int length);

/**
 * Retrieves the XPUML index of this device.
 *
 * For all products.
 *
 * Valid indices are derived from the \a accessibleDevices count returned by
 *   \ref xpumlDeviceGetCount_v2(). For example, if \a accessibleDevices is 2 the valid indices
 *   are 0 and 1, corresponding to XPU 0 and XPU 1.
 *
 * The order in which XPUML enumerates devices has no guarantees of consistency between reboots. For that reason it
 *   is recommended that devices be looked up by their PCI ids or XPU UUID. See
 *   \ref xpumlDeviceGetHandleByPciBusId_v2() and \ref xpumlDeviceGetHandleByUUID().
 *
 * When used with MIG device handles this API returns indices that can be
 * passed to \ref xpumlDeviceGetMigDeviceHandleByIndex to retrieve an identical handle.
 * MIG device indices are unique within a device.
 *
 * Note: The XPUML index may not correlate with other APIs, such as the CUDA device index.
 *
 * @param device                               The identifier of the target device
 * @param index                                Reference in which to return the XPUML index of the device
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a index has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, or \a index is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 *
 * @see xpumlDeviceGetHandleByIndex()
 * @see xpumlDeviceGetCount()
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetIndex(xpumlDevice_t device, unsigned int *index);

/**
 * Retrieves the globally unique board serial number associated with this device's board.
 *
 * For all products with an inforom.
 *
 * The serial number is an alphanumeric string that will not exceed 30 characters (including the NULL terminator).
 * This number matches the serial number tag that is physically attached to the board.  See \ref
 * xpumlConstants::XPUML_DEVICE_SERIAL_BUFFER_SIZE.
 *
 * @param device                               The identifier of the target device
 * @param serial                               Reference in which to return the board/module serial number
 * @param length                               The maximum allowed length of the string returned in \a serial
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a serial has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, or \a serial is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a length is too small
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetSerial(xpumlDevice_t device, char *serial, unsigned int length);

/**
 * Retrieve the status for a given p2p capability index between a given pair of XPU
 *
 * @param device1                              The first device
 * @param device2                              The second device
 * @param p2pIndex                             p2p Capability Index being looked for between \a device1 and \a device2
 * @param p2pStatus                            Reference in which to return the status of the \a p2pIndex
 *                                             between \a device1 and \a device2
 * @return
 *         - \ref XPUML_SUCCESS         if \a p2pStatus has been populated
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT     if \a device1 or \a device2 or \a p2pIndex is invalid or \a p2pStatus is NULL
 *         - \ref XPUML_ERROR_UNKNOWN              on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetP2PStatus(xpumlDevice_t device1, xpumlDevice_t device2, xpumlXPUP2PCapsIndex_t p2pIndex, xpumlXPUP2PStatus_t *p2pStatus);

/**
 * Retrieves the globally unique immutable UUID associated with this device, as a 5 part hexadecimal string,
 * that augments the immutable, board serial identifier.
 *
 * For all products.
 *
 * The UUID is a globally unique identifier. It is the only available identifier for pre-Fermi-architecture products.
 * It does NOT correspond to any identifier printed on the board.  It will not exceed 96 characters in length
 * (including the NULL terminator).  See \ref xpumlConstants::XPUML_DEVICE_UUID_V2_BUFFER_SIZE.
 *
 * When used with MIG device handles the API returns globally unique UUIDs which can be used to identify MIG
 * devices across both XPU and MIG devices. UUIDs are immutable for the lifetime of a MIG device.
 *
 * @param device                               The identifier of the target device
 * @param uuid                                 Reference in which to return the XPU UUID
 * @param length                               The maximum allowed length of the string returned in \a uuid
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a uuid has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, or \a uuid is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a length is too small
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetUUID(xpumlDevice_t device, char *uuid, unsigned int length);

/**
 * Retrieves minor number for the device.
 *
 * For all products.
 * Supported only for Linux
 *
 * @param device                                The identifier of the target device
 * @param minorNumber                           Reference in which to return the minor number for the device
 * @return
 *         - \ref XPUML_SUCCESS                 if the minor number is successfully retrieved
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a minorNumber is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if this query is not supported by the device
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetMinorNumber(xpumlDevice_t device, unsigned int *minorNumber);

/**
 * Retrieves the PCI attributes of this device.
 *
 * For all products.
 *
 * See \ref xpumlPciInfo_t for details on the available PCI info.
 *
 * @param device                               The identifier of the target device
 * @param pci                                  Reference in which to return the PCI info
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a pci has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a pci is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetPciInfo(xpumlDevice_t device, xpumlPciInfo_t *pci);

/**
 * Retrieves the maximum PCIe link generation possible with this device and system
 *
 * I.E. for a generation 2 PCIe device attached to a generation 1 PCIe bus the max link generation this function will
 * report is generation 1.
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * @param device                               The identifier of the target device
 * @param maxLinkGen                           Reference in which to return the max PCIe link generation
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a maxLinkGen has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a maxLinkGen is null
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if PCIe link information is not available
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetMaxPcieLinkGeneration(xpumlDevice_t device, unsigned int *maxLinkGen);

/**
 * Retrieves the maximum PCIe link width possible with this device and system
 *
 * I.E. for a device with a 16x PCIe bus width attached to a 8x PCIe system bus this function will report
 * a max link width of 8.
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * @param device                               The identifier of the target device
 * @param maxLinkWidth                         Reference in which to return the max PCIe link generation
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a maxLinkWidth has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a maxLinkWidth is null
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if PCIe link information is not available
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetMaxPcieLinkWidth(xpumlDevice_t device, unsigned int *maxLinkWidth);

/**
 * Retrieves the current PCIe link generation
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * @param device                               The identifier of the target device
 * @param currLinkGen                          Reference in which to return the current PCIe link generation
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a currLinkGen has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a currLinkGen is null
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if PCIe link information is not available
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetCurrPcieLinkGeneration(xpumlDevice_t device, unsigned int *currLinkGen);

/**
 * Retrieves the current PCIe link width
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * @param device                               The identifier of the target device
 * @param currLinkWidth                        Reference in which to return the current PCIe link generation
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a currLinkWidth has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a currLinkWidth is null
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if PCIe link information is not available
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetCurrPcieLinkWidth(xpumlDevice_t device, unsigned int *currLinkWidth);

/**
 * Retrieve PCIe utilization information.
 * This function is querying a byte counter over a 20ms interval and thus is the
 *   PCIe throughput over that interval.
 *
 * For Maxwell &tm; or newer fully supported devices.
 *
 * This method is not supported in virtual machines running virtual XPU (vXPU).
 *
 * @param device                               The identifier of the target device
 * @param counter                              The specific counter that should be queried \ref xpumlPcieUtilCounter_t
 * @param value                                Reference in which to return throughput in KB/s
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a value has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device or \a counter is invalid, or \a value is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetPcieThroughput(xpumlDevice_t device, xpumlPcieUtilCounter_t counter, unsigned int *value);

/**
 * Retrieves the current clock speeds for the device.
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * See \ref xpumlClockType_t for details on available clock information.
 *
 * @param device                               The identifier of the target device
 * @param type                                 Identify which clock domain to query
 * @param clock                                Reference in which to return the clock speed in MHz
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a clock has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a clock is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device cannot report the specified clock
 *         - \ref XPUML_ERROR_GPU_IS_LOST       if the target GPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetClockInfo(xpumlDevice_t device, xpumlClockType_t type, unsigned int *clock);

/**
 * Retrieves the current running state readings for the device.
 *
 * For all products.
 *
 * See \ref xpumlDeviceState_t for details.
 *
 * @param device                               The identifier of the target device
 * @param state                                Reference in which to return the device state reading
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a temp has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, or \a state is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetState(xpumlDevice_t device, xpumlDeviceState_t *state);

/**
 * Retrieves the current temperature readings for the device, in degrees C.
 *
 * For all products.
 *
 * See \ref xpumlTemperatureSensors_t for details on available temperature sensors.
 *
 * @param device                               The identifier of the target device
 * @param sensorType                           Flag that indicates which sensor reading to retrieve
 * @param temp                                 Reference in which to return the temperature reading
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a temp has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a sensorType is invalid or \a temp is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not have the specified sensor
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetTemperature(xpumlDevice_t device, xpumlTemperatureSensors_t sensorType, unsigned int *temp);

/**
 * Retrieves the temperature threshold for the XPU with the specified threshold type in degrees C.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * See \ref xpumlTemperatureThresholds_t for details on available temperature thresholds.
 *
 * @param device                               The identifier of the target device
 * @param thresholdType                        The type of threshold value queried
 * @param temp                                 Reference in which to return the temperature reading
 * @return
 *         - \ref XPUML_SUCCESS                 if \a temp has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a thresholdType is invalid or \a temp is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not have a temperature sensor or is unsupported
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetTemperatureThreshold(xpumlDevice_t device, xpumlTemperatureThresholds_t thresholdType, unsigned int *temp);

/**
 * Sets the temperature threshold for the XPU with the specified threshold type in degrees C.
 *
 * For Maxwell &tm; or newer fully supported devices.
 *
 * See \ref xpumlTemperatureThresholds_t for details on available temperature thresholds.
 *
 * @param device                               The identifier of the target device
 * @param thresholdType                        The type of threshold value to be set
 * @param temp                                 Reference which hold the value to be set
 * @return
 *         - \ref XPUML_SUCCESS                 if \a temp has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a thresholdType is invalid or \a temp is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not have a temperature sensor or is unsupported
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceSetTemperatureThreshold(xpumlDevice_t device, xpumlTemperatureThresholds_t thresholdType, int *temp);

/**
 * Retrieves current clocks throttling reasons.
 *
 * For all fully supported products.
 *
 * \note More than one bit can be enabled at the same time. Multiple reasons can be affecting clocks at once.
 *
 * @param device                                The identifier of the target device
 * @param clocksThrottleReasons                 Reference in which to return bitmask of active clocks throttle
 *                                                  reasons
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a clocksThrottleReasons has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a clocksThrottleReasons is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 *
 * @see xpumlClocksThrottleReasons
 * @see xpumlDeviceGetSupportedClocksThrottleReasons
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetCurrentClocksThrottleReasons(xpumlDevice_t device, unsigned long long *clocksThrottleReasons);

/**
 * Retrieves bitmask of supported clocks throttle reasons that can be returned by
 * \ref xpumlDeviceGetCurrentClocksThrottleReasons
 *
 * For all fully supported products.
 *
 * This method is not supported in virtual machines running virtual XPU (vXPU).
 *
 * @param device                               The identifier of the target device
 * @param supportedClocksThrottleReasons       Reference in which to return bitmask of supported
 *                                              clocks throttle reasons
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a supportedClocksThrottleReasons has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a supportedClocksThrottleReasons is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 *
 * @see xpumlClocksThrottleReasons
 * @see xpumlDeviceGetCurrentClocksThrottleReasons
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetSupportedClocksThrottleReasons(xpumlDevice_t device, unsigned long long *supportedClocksThrottleReasons);

/**
 * Retrieves the power management limit associated with this device.
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * The power limit defines the upper boundary for the card's power draw. If
 * the card's total power draw reaches this limit the power management algorithm kicks in.
 *
 * This reading is only available if power management mode is supported.
 * See \ref xpumlDeviceGetPowerManagementMode.
 *
 * @param device                               The identifier of the target device
 * @param limit                                Reference in which to return the power management limit in milliwatts
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a limit has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a limit is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetPowerManagementLimit(xpumlDevice_t device, unsigned int *limit);

/**
 * Retrieves power usage for this XPU in milliwatts and its associated circuitry (e.g. memory)
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * On Fermi and Kepler XPUs the reading is accurate to within +/- 5% of current power draw.
 *
 * It is only available if power management mode is supported. See \ref xpumlDeviceGetPowerManagementMode.
 *
 * @param device                               The identifier of the target device
 * @param power                                Reference in which to return the power usage information
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a power has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a power is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support power readings
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetPowerUsage(xpumlDevice_t device, unsigned int *power);

/**
 * Retrieves the amount of used, free and total memory available on the device, in bytes.
 *
 * For all products.
 *
 * Enabling ECC reduces the amount of total available memory, due to the extra required parity bits.
 * Under WDDM most device memory is allocated and managed on startup by Windows.
 *
 * Under Linux and Windows TCC, the reported amount of used memory is equal to the sum of memory allocated
 * by all active channels on the device.
 *
 * See \ref xpumlMemory_t for details on available memory info.
 *
 * @note In MIG mode, if device handle is provided, the API returns aggregate
 *       information, only if the caller has appropriate privileges. Per-instance
 *       information can be queried by using specific MIG device handles.
 *
 * @param device                               The identifier of the target device
 * @param memory                               Reference in which to return the memory information
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a memory has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_NO_PERMISSION     if the user doesn't have permission to perform this operation
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a memory is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetMemoryInfo(xpumlDevice_t device, xpumlMemory_t *memory);

/**
 * Retrieves the device boardId from 0-N.
 * Devices with the same boardId indicate XPUs connected to the same PLX.  Use in conjunction with
 *  \ref xpumlDeviceGetMultiXPUBoard() to decide if they are on the same board as well.
 *  The boardId returned is a unique ID for the current configuration.  Uniqueness and ordering across
 *  reboots and system configurations is not guaranteed (i.e. if a Tesla K40c returns 0x100 and
 *  the two XPUs on a Tesla K10 in the same system returns 0x200 it is not guaranteed they will
 *  always return those values but they will always be different from each other).
 *
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * @param device                               The identifier of the target device
 * @param boardId                              Reference in which to return the device's board ID
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a boardId has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a boardId is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetBoardId(xpumlDevice_t device, unsigned int *boardId);

/**
 * Retrieves whether the device is on a Multi-XPU Board
 * Devices that are on multi-XPU boards will set \a multiXPUBool to a non-zero value.
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * @param device                               The identifier of the target device
 * @param multiXPUBool                         Reference in which to return a zero or non-zero value
 *                                                 to indicate whether the device is on a multi XPU board
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a multiXPUBool has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a multiXPUBool is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetMultiXPUBoard(xpumlDevice_t device, unsigned int *multiXPUBool);

/**
 * Retrieves the current utilization rates for the device's major subsystems.
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * See \ref xpumlUtilization_t for details on available utilization rates.
 *
 * \note During driver initialization when ECC is enabled one can see high XPU and Memory Utilization readings.
 *       This is caused by ECC Memory Scrubbing mechanism that is performed during driver initialization.
 *
 * @note On MIG-enabled XPUs, querying device utilization rates is not currently supported.
 *
 * @param device                               The identifier of the target device
 * @param utilization                          Reference in which to return the utilization information
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a utilization has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a utilization is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceGetUtilizationRates(xpumlDevice_t device, xpumlUtilization_t *utilization);

/**
 * Retrieves the current utilization and sampling size in microseconds for the Encoder
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @note On MIG-enabled XPUs, querying encoder utilization is not currently supported.
 *
 * @param device                               The identifier of the target device
 * @param utilization                          Reference to an unsigned int for encoder utilization info
 * @param samplingPeriodUs                     Reference to an unsigned int for the sampling period in US
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a utilization has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a utilization is NULL, or \a samplingPeriodUs is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetEncoderUtilization(xpumlDevice_t device, unsigned int *utilization, unsigned int *samplingPeriodUs);

/**
 * Retrieves the current capacity of the device's encoder, as a percentage of maximum encoder capacity with valid values in the range 0-100.
 *
 * For Maxwell &tm; or newer fully supported devices.
 *
 * @param device                            The identifier of the target device
 * @param encoderQueryType                  Type of encoder to query
 * @param encoderCapacity                   Reference to an unsigned int for the encoder capacity
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a encoderCapacity is fetched
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a encoderCapacity is NULL, or \a device or \a encoderQueryType
 *                                              are invalid
 *         - \ref XPUML_ERROR_NOT_SUPPORTED      if device does not support the encoder specified in \a encodeQueryType
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetEncoderCapacity (xpumlDevice_t device, xpumlEncoderType_t encoderQueryType, unsigned int *encoderCapacity);

/**
 * Retrieves the current encoder statistics for a given device.
 *
 * For Maxwell &tm; or newer fully supported devices.
 *
 * @param device                            The identifier of the target device
 * @param sessionCount                      Reference to an unsigned int for count of active encoder sessions
 * @param averageFps                        Reference to an unsigned int for trailing average FPS of all active sessions
 * @param averageLatency                    Reference to an unsigned int for encode latency in microseconds
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a sessionCount, \a averageFps and \a averageLatency is fetched
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a sessionCount, or \a device or \a averageFps,
 *                                              or \a averageLatency is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetEncoderStats (xpumlDevice_t device, unsigned int *sessionCount, unsigned int *averageFps, unsigned int *averageLatency);

/**
 * Retrieves information about active encoder sessions on a target device.
 *
 * An array of active encoder sessions is returned in the caller-supplied buffer pointed at by \a sessionInfos. The
 * array elememt count is passed in \a sessionCount, and \a sessionCount is used to return the number of sessions
 * written to the buffer.
 *
 * If the supplied buffer is not large enough to accomodate the active session array, the function returns
 * XPUML_ERROR_INSUFFICIENT_SIZE, with the element count of xpumlEncoderSessionInfo_t array required in \a sessionCount.
 * To query the number of active encoder sessions, call this function with *sessionCount = 0.  The code will return
 * XPUML_SUCCESS with number of active encoder sessions updated in *sessionCount.
 *
 * For Maxwell &tm; or newer fully supported devices.
 *
 * @param device                            The identifier of the target device
 * @param sessionCount                      Reference to caller supplied array size, and returns the number of sessions.
 * @param sessionInfos                      Reference in which to return the session information
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a sessionInfos is fetched
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE  if \a sessionCount is too small, array element count is returned in \a sessionCount
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a sessionCount is NULL.
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_NOT_SUPPORTED      if this query is not supported by \a device
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetEncoderSessions(xpumlDevice_t device, unsigned int *sessionCount, xpumlEncoderSessionInfo_t *sessionInfos);

/**
 * Retrieves the current utilization and sampling size in microseconds for the Decoder
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @note On MIG-enabled XPUs, querying decoder utilization is not currently supported.
 *
 * @param device                               The identifier of the target device
 * @param utilization                          Reference to an unsigned int for decoder utilization info
 * @param samplingPeriodUs                     Reference to an unsigned int for the sampling period in US
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a utilization has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a utilization is NULL, or \a samplingPeriodUs is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetDecoderUtilization(xpumlDevice_t device, unsigned int *utilization, unsigned int *samplingPeriodUs);

#define XPUML_DEVICE_MAX_PROCESS_COUNT 100 // the max count of processes which user can query.
/**
 * Get information about processes with a compute context on a device
 *
 * For Fermi &tm; or newer fully supported devices.
 *
 * This function returns information only about compute running processes (e.g. CUDA application which have
 * active context). Any graphics applications (e.g. using OpenGL, DirectX) won't be listed by this function.
 *
 * To query the current number of running compute processes, call this function with *infoCount = 0. The
 * return code will be XPUML_ERROR_INSUFFICIENT_SIZE, or XPUML_SUCCESS if none are running. For this call
 * \a infos is allowed to be NULL.
 *
 * The usedXPUMemory field returned is all of the memory used by the application.
 *
 * Keep in mind that information returned by this call is dynamic and the number of elements might change in
 * time. Allocate more space for \a infos table in case new compute processes are spawned.
 *
 * @note In MIG mode, if device handle is provided, the API returns aggregate information, only if
 *       the caller has appropriate privileges. Per-instance information can be queried by using
 *       specific MIG device handles.
 *       Querying per-instance information using MIG device handles is not supported if the device is in vXPU Host virtualization mode.
 *
 * @param device                               The device handle or MIG device handle
 * @param infoCount                            Reference in which to provide the \a infos array size, and
 *                                             to return the number of returned elements
 * @param infos                                Reference in which to return the process information
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a infoCount and \a infos have been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a infoCount indicates that the \a infos array is too small
 *                                             \a infoCount will contain minimal amount of space necessary for
 *                                             the call to complete
 *         - \ref XPUML_ERROR_NO_PERMISSION     if the user doesn't have permission to perform this operation
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, either of \a infoCount or \a infos is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if this query is not supported by \a device
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 *
 * @see \ref xpumlSystemGetProcessName
 */
xpumlReturn_t DECLDIR xpumlDeviceGetComputeRunningProcesses(xpumlDevice_t device, unsigned int *infoCount, xpumlProcessInfo_t *infos);

/**
 * Check if the XPU devices are on the same physical board.
 *
 * For all fully supported products.
 *
 * @param device1                               The first XPU device
 * @param device2                               The second XPU device
 * @param onSameBoard                           Reference in which to return the status.
 *                                              Non-zero indicates that the XPUs are on the same board.
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a onSameBoard has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a dev1 or \a dev2 are invalid or \a onSameBoard is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if this check is not supported by the device
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the either XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceOnSameBoard(xpumlDevice_t device1, xpumlDevice_t device2, int *onSameBoard);

/**
 * Gets recent samples for the XPU.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * Based on type, this method can be used to fetch the power, utilization or clock samples maintained in the buffer by
 * the driver.
 *
 * Power, Utilization and Clock samples are returned as type "unsigned int" for the union xpumlValue_t.
 *
 * To get the size of samples that user needs to allocate, the method is invoked with samples set to NULL.
 * The returned samplesCount will provide the number of samples that can be queried. The user needs to
 * allocate the buffer with size as samplesCount * sizeof(xpumlSample_t).
 *
 * lastSeenTimeStamp represents CPU timestamp in microseconds. Set it to 0 to fetch all the samples maintained by the
 * underlying buffer. Set lastSeenTimeStamp to one of the timeStamps retrieved from the date of the previous query
 * to get more recent samples.
 *
 * This method fetches the number of entries which can be accommodated in the provided samples array, and the
 * reference samplesCount is updated to indicate how many samples were actually retrieved. The advantage of using this
 * method for samples in contrast to polling via existing methods is to get get higher frequency data at lower polling cost.
 *
 * @note On MIG-enabled XPUs, querying the following sample types, XPUML_XPU_UTILIZATION_SAMPLES, XPUML_MEMORY_UTILIZATION_SAMPLES
 *       XPUML_ENC_UTILIZATION_SAMPLES and XPUML_DEC_UTILIZATION_SAMPLES, is not currently supported.
 *
 * @param device                        The identifier for the target device
 * @param type                          Type of sampling event
 * @param lastSeenTimeStamp             Return only samples with timestamp greater than lastSeenTimeStamp.
 * @param sampleValType                 Output parameter to represent the type of sample value as described in xpumlSampleVal_t
 * @param sampleCount                   Reference to provide the number of elements which can be queried in samples array
 * @param samples                       Reference in which samples are returned

 * @return
 *         - \ref XPUML_SUCCESS                 if samples are successfully retrieved
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a samplesCount is NULL or
 *                                             reference to \a sampleCount is 0 for non null \a samples
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if this query is not supported by the device
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_NOT_FOUND         if sample entries are not found
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetSamples(xpumlDevice_t device, xpumlSamplingType_t type, unsigned long long lastSeenTimeStamp,
        //xpumlValueType_t *sampleValType, unsigned int *sampleCount, xpumlSample_t *samples);

/**
 * Gets Total, Available and Used size of BAR1 memory.
 *
 * BAR1 is used to map the FB (device memory) so that it can be directly accessed by the CPU or by 3rd party
 * devices (peer-to-peer on the PCIE bus).
 *
 * @note In MIG mode, if device handle is provided, the API returns aggregate
 *       information, only if the caller has appropriate privileges. Per-instance
 *       information can be queried by using specific MIG device handles.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param device                               The identifier of the target device
 * @param bar1Memory                           Reference in which BAR1 memory
 *                                             information is returned.
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if BAR1 memory is successfully retrieved
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a bar1Memory is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if this query is not supported by the device
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 *
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetBAR4MemoryInfo(xpumlDevice_t device, xpumlBAR4Memory_t *bar4Memory);

/**
 * Gets the device's interrupt number
 *
 * @param device                               The identifier of the target device
 * @param irqNum                               The interrupt number associated with the specified device
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if irq number is successfully retrieved
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, or \a irqNum is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if this query is not supported by the device
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetIrqNum(xpumlDevice_t device, unsigned int *irqNum);

/**
 * @}
 */

/** @addtogroup xpumlDeviceQueries
 *  @{
 */

/**
 * Get architecture for device
 *
 * @param device                               The identifier of the target device
 * @param arch                                 Reference where architecture is returned, if call successful.
 *                                             Set to XPUML_DEVICE_ARCH_* upon success
 *
 * @return
 *         - \ref XPUML_SUCCESS                 Upon success
 *         - \ref XPUML_ERROR_UNINITIALIZED     If library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  If \a device or \a arch (output refererence) are invalid
 */
xpumlReturn_t DECLDIR xpumlDeviceGetArchitecture(xpumlDevice_t device, xpumlDeviceArchitecture_t *arch);

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlDeviceCommands Device Commands
 *  This chapter describes XPUML operations that change the state of the device.
 *  Each of these requires root/admin access. Non-admin users will see an XPUML_ERROR_NO_PERMISSION
 *  error code when invoking any of these methods.
 *  @{
 */
/***************************************************************************************************/

/**
 * @}
 */

/** @addtogroup xpumlAccountingStats
 *  @{
 */

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlEvents Event Handling Methods
 * This chapter describes methods that XPUML can perform against each device to register and wait for
 * some event to occur.
 *  @{
 */
/***************************************************************************************************/

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlZPI Drain states
 * This chapter describes methods that XPUML can perform against each device to control their drain state
 * and recognition by XPUML and KUNLUN kernel driver. These methods can be used with out-of-band tools to
 * power on/off XPUs, enable robust reset scenarios, etc.
 *  @{
 */
/***************************************************************************************************/

/**
 * This method will remove the specified XPU from the view of both XPUML and the KUNLUN kernel driver
 * as long as no other processes are attached. If other processes are attached, this call will return
 * XPUML_ERROR_IN_USE and the XPU will be returned to its original "draining" state. Note: the
 * only situation where a process can still be attached after xpumlDeviceModifyDrainState() is called
 * to initiate the draining state is if that process was using, and is still using, a XPU before the
 * call was made. Also note, persistence mode counts as an attachment to the XPU thus it must be disabled
 * prior to this call.
 *
 * For long-running XPUML processes please note that this will change the enumeration of current XPUs.
 * For example, if there are four XPUs present and XPU1 is removed, the new enumeration will be 0-2.
 * Also, device handles after the removed XPU will not be valid and must be re-established.
 * Must be run as administrator.
 * For Linux only.
 *
 * For Pascal &tm; or newer fully supported devices.
 * Some Kepler devices supported.
 *
 * @param pciInfo                              The PCI address of the XPU to be removed
 * @param xpuState                             Whether the XPU is to be removed, from the OS
 *                                             see \ref xpumlDetachXPUState_t
 * @param linkState                            Requested upstream PCIe link state, see \ref xpumlPcieLinkState_t
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if counters were successfully reset
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a xpumlIndex is invalid
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device doesn't support this feature
 *         - \ref XPUML_ERROR_IN_USE            if the device is still in use and cannot be removed
 */
//xpumlReturn_t DECLDIR xpumlDeviceRemoveXPU(xpumlPciInfo_t *pciInfo, xpumlDetachXPUState_t xpuState, xpumlPcieLinkState_t linkState);

/**
 * Request the OS and the KUNLUN kernel driver to rediscover a portion of the PCI subsystem looking for XPUs that
 * were previously removed. The portion of the PCI tree can be narrowed by specifying a domain, bus, and device.
 * If all are zeroes then the entire PCI tree will be searched.  Please note that for long-running XPUML processes
 * the enumeration will change based on how many XPUs are discovered and where they are inserted in bus order.
 *
 * In addition, all newly discovered XPUs will be initialized and their ECC scrubbed which may take several seconds
 * per XPU. Also, all device handles are no longer guaranteed to be valid post discovery.
 *
 * Must be run as administrator.
 * For Linux only.
 *
 * For Pascal &tm; or newer fully supported devices.
 * Some Kepler devices supported.
 *
 * @param pciInfo                              The PCI tree to be searched.  Only the domain, bus, and device
 *                                             fields are used in this call.
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if counters were successfully reset
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a pciInfo is invalid
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the operating system does not support this feature
 *         - \ref XPUML_ERROR_OPERATING_SYSTEM  if the operating system is denying this feature
 *         - \ref XPUML_ERROR_NO_PERMISSION     if the calling process has insufficient permissions to perform operation
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceDiscoverXPUs (xpumlPciInfo_t *pciInfo);

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlFieldValueQueries Field Value Queries
 *  This chapter describes XPUML operations that are associated with retrieving Field Values from XPUML
 *  @{
 */
/***************************************************************************************************/

/**
 * Request values for a list of fields for a device. This API allows multiple fields to be queried at once.
 * If any of the underlying fieldIds are populated by the same driver call, the results for those field IDs
 * will be populated from a single call rather than making a driver call for each fieldId.
 *
 * @param device                               The device handle of the XPU to request field values for
 * @param valuesCount                          Number of entries in values that should be retrieved
 * @param values                               Array of \a valuesCount structures to hold field values.
 *                                             Each value's fieldId must be populated prior to this call
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if any values in \a values were populated. Note that you must
 *                                             check the xpumlReturn field of each value for each individual
 *                                             status
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a values is NULL
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetFieldValues(xpumlDevice_t device, int valuesCount, xpumlFieldValue_t *values);

/** @} */

/***************************************************************************************************/
/** @defgroup vXPU Enums, Constants and Structs
 *  @{
 */
/** @} */
/***************************************************************************************************/

/***************************************************************************************************/
/** @defgroup xpumlVirtualXPUQueries vXPU APIs
 * This chapter describes operations that are associated with KUNLUN vXPU Software products.
 *  @{
 */
/***************************************************************************************************/

/**
 * This method is used to get the virtualization mode corresponding to the XPU.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param device                    Identifier of the target device
 * @param pVirtualMode              Reference to virtualization mode. One of XPUML_XPU_VIRTUALIZATION_?
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a pVirtualMode is fetched
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a device is invalid or \a pVirtualMode is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetVirtualizationMode(xpumlDevice_t device, xpumlXPUVirtualizationMode_t *pVirtualMode);

/**
 * Queries if SR-IOV host operation is supported on a vXPU supported device.
 *
 * Checks whether SR-IOV host capability is supported by the device and the
 * driver, and indicates device is in SR-IOV mode if both of these conditions
 * are true.
 *
 * @param device                                The identifier of the target device
 * @param pHostVxpuMode                         Reference in which to return the current vXPU mode
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if device's vXPU mode has been successfully retrieved
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a device handle is 0 or \a pVxpuMode is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED      if \a device doesn't support this feature.
 *         - \ref XPUML_ERROR_UNKNOWN            if any unexpected error occurred
 */
xpumlReturn_t DECLDIR xpumlDeviceGetHostVxpuMode(xpumlDevice_t device, xpumlHostVxpuMode_t *pHostVxpuMode);

/**
 * @brief xpumlDeviceSetSriovVfNum
 *
 * @param device                                The identifier of the target device
 * @param vfNum                                 The number of vfs need to be created
 *
 * @returns
 *         - \ref XPUML_SUCCESS                  if device's vXPU mode has been successfully retrieved
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a device handle is 0 or \a pVxpuMode is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED      if \a device doesn't support this feature.
 *         - \ref XPUML_ERROR_UNKNOWN            if any unexpected error occurred
 */
xpumlReturn_t DECLDIR xpumlDeviceSetSriovVfNum(xpumlDevice_t device, int vfNum);

/**
 * This method is used to set the virtualization mode corresponding to the XPU.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param device                    Identifier of the target device
 * @param virtualMode               virtualization mode. One of XPUML_XPU_VIRTUALIZATION_?
 *
 * @return
 *         - \ref XPUML_SUCCESS                  if \a pVirtualMode is set
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a device is invalid or \a pVirtualMode is NULL
 *         - \ref XPUML_ERROR_XPU_IS_LOST        if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_NOT_SUPPORTED      if setting of virtualization mode is not supported.
 *         - \ref XPUML_ERROR_NO_PERMISSION      if setting of virtualization mode is not allowed for this client.
 */
//xpumlReturn_t DECLDIR xpumlDeviceSetVirtualizationMode(xpumlDevice_t device, xpumlXPUVirtualizationMode_t virtualMode);

/**
 * Retrieves the current utilization and process ID
 *
 * For Maxwell &tm; or newer fully supported devices.
 *
 * Reads recent utilization of XPU SM (3D/Compute), framebuffer, video encoder, and video decoder for processes running.
 * Utilization values are returned as an array of utilization sample structures in the caller-supplied buffer pointed at
 * by \a utilization. One utilization sample structure is returned per process running, that had some non-zero utilization
 * during the last sample period. It includes the CPU timestamp at which  the samples were recorded. Individual utilization values
 * are returned as "unsigned int" values.
 *
 * To read utilization values, first determine the size of buffer required to hold the samples by invoking the function with
 * \a utilization set to NULL. The caller should allocate a buffer of size
 * processSamplesCount * sizeof(xpumlProcessUtilizationSample_t). Invoke the function again with the allocated buffer passed
 * in \a utilization, and \a processSamplesCount set to the number of entries the buffer is sized for.
 *
 * On successful return, the function updates \a processSamplesCount with the number of process utilization sample
 * structures that were actually written. This may differ from a previously read value as instances are created or
 * destroyed.
 *
 * lastSeenTimeStamp represents the CPU timestamp in microseconds at which utilization samples were last read. Set it to 0
 * to read utilization based on all the samples maintained by the driver's internal sample buffer. Set lastSeenTimeStamp
 * to a timeStamp retrieved from a previous query to read utilization since the previous query.
 *
 * @note On MIG-enabled XPUs, querying process utilization is not currently supported.
 *
 * @param device                    The identifier of the target device
 * @param utilization               Pointer to caller-supplied buffer in which guest process utilization samples are returned
 * @param processSamplesCount       Pointer to caller-supplied array size, and returns number of processes running
 * @param lastSeenTimeStamp         Return only samples with timestamp greater than lastSeenTimeStamp.

 * @return
 *         - \ref XPUML_SUCCESS                 if \a utilization has been populated
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a utilization is NULL, or \a samplingPeriodUs is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED     if the device does not support this feature
 *         - \ref XPUML_ERROR_NOT_FOUND         if sample entries are not found
 *         - \ref XPUML_ERROR_XPU_IS_LOST       if the target XPU has fallen off the bus or is otherwise inaccessible
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetProcessUtilization(xpumlDevice_t device, xpumlProcessUtilizationSample_t *utilization,
                                              //unsigned int *processSamplesCount, unsigned long long lastSeenTimeStamp);

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlVxpu vXPU Management
 * @{
 *
 * This chapter describes APIs supporting KUNLUN vXPU.
 */
/***************************************************************************************************/

/**
 * Retrieve the supported vXPU types on a physical XPU (device).
 *
 * An array of supported vXPU types for the physical XPU indicated by \a device is returned in the caller-supplied buffer
 * pointed at by \a vxpuTypeIds. The element count of xpumlVxpuTypeId_t array is passed in \a vxpuCount, and \a vxpuCount
 * is used to return the number of vXPU types written to the buffer.
 *
 * If the supplied buffer is not large enough to accomodate the vXPU type array, the function returns
 * XPUML_ERROR_INSUFFICIENT_SIZE, with the element count of xpumlVxpuTypeId_t array required in \a vxpuCount.
 * To query the number of vXPU types supported for the XPU, call this function with *vxpuCount = 0.
 * The code will return XPUML_ERROR_INSUFFICIENT_SIZE, or XPUML_SUCCESS if no vXPU types are supported.
 *
 * @param device                   The identifier of the target device
 * @param vxpuCount                Pointer to caller-supplied array size, and returns number of vXPU types
 * @param vxpuTypeIds              Pointer to caller-supplied array in which to return list of vXPU types
 *
 * @return
 *         - \ref XPUML_SUCCESS                      successful completion
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE      \a vxpuTypeIds buffer is too small, array element count is returned in \a vxpuCount
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT       if \a vxpuCount is NULL or \a device is invalid
 *         - \ref XPUML_ERROR_NOT_SUPPORTED          if vXPU is not supported by the device
 *         - \ref XPUML_ERROR_UNKNOWN                on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetSupportedVxpus(xpumlDevice_t device, unsigned int *vxpuCount, xpumlVxpuTypeId_t *vxpuTypeIds);

/**
 * Retrieve the currently creatable vXPU types on a physical XPU (device).
 *
 * An array of creatable vXPU types for the physical XPU indicated by \a device is returned in the caller-supplied buffer
 * pointed at by \a vxpuTypeIds. The element count of xpumlVxpuTypeId_t array is passed in \a vxpuCount, and \a vxpuCount
 * is used to return the number of vXPU types written to the buffer.
 *
 * The creatable vXPU types for a device may differ over time, as there may be restrictions on what type of vXPU types
 * can concurrently run on a device.  For example, if only one vXPU type is allowed at a time on a device, then the creatable
 * list will be restricted to whatever vXPU type is already running on the device.
 *
 * If the supplied buffer is not large enough to accomodate the vXPU type array, the function returns
 * XPUML_ERROR_INSUFFICIENT_SIZE, with the element count of xpumlVxpuTypeId_t array required in \a vxpuCount.
 * To query the number of vXPU types createable for the XPU, call this function with *vxpuCount = 0.
 * The code will return XPUML_ERROR_INSUFFICIENT_SIZE, or XPUML_SUCCESS if no vXPU types are creatable.
 *
 * @param device                   The identifier of the target device
 * @param vxpuCount                Pointer to caller-supplied array size, and returns number of vXPU types
 * @param vxpuTypeIds              Pointer to caller-supplied array in which to return list of vXPU types
 *
 * @return
 *         - \ref XPUML_SUCCESS                      successful completion
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE      \a vxpuTypeIds buffer is too small, array element count is returned in \a vxpuCount
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT       if \a vxpuCount is NULL
 *         - \ref XPUML_ERROR_NOT_SUPPORTED          if vXPU is not supported by the device
 *         - \ref XPUML_ERROR_UNKNOWN                on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetCreatableVxpus(xpumlDevice_t device, unsigned int *vxpuCount, xpumlVxpuTypeId_t *vxpuTypeIds);

/**
 * Retrieve the class of a vXPU type. It will not exceed 64 characters in length (including the NUL terminator).
 * See \ref xpumlConstants::XPUML_DEVICE_NAME_BUFFER_SIZE.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param vxpuTypeId               Handle to vXPU type
 * @param vxpuTypeClass            Pointer to string array to return class in
 * @param size                     Size of string
 *
 * @return
 *         - \ref XPUML_SUCCESS                   successful completion
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT    if \a vxpuTypeId is invalid, or \a vxpuTypeClass is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE   if \a size is too small
 *         - \ref XPUML_ERROR_UNKNOWN             on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlVxpuTypeGetClass(xpumlVxpuTypeId_t vxpuTypeId, char *vxpuTypeClass, unsigned int *size);

/**
 * Retrieve the vXPU type name.
 *
 * The name is an alphanumeric string that denotes a particular vXPU, e.g. GRID M60-2Q. It will not
 * exceed 64 characters in length (including the NUL terminator).  See \ref
 * xpumlConstants::XPUML_DEVICE_NAME_BUFFER_SIZE.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param vxpuTypeId               Handle to vXPU type
 * @param vxpuTypeName             Pointer to buffer to return name
 * @param size                     Size of buffer
 *
 * @return
 *         - \ref XPUML_SUCCESS                 successful completion
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a vxpuTypeId is invalid, or \a name is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a size is too small
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlVxpuTypeGetName(xpumlVxpuTypeId_t vxpuTypeId, char *vxpuTypeName, unsigned int *size);

/**
 * Retrieve the device ID of a vXPU type.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param vxpuTypeId               Handle to vXPU type
 * @param deviceID                 Device ID and vendor ID of the device contained in single 32 bit value
 * @param subsystemID              Subsytem ID and subsytem vendor ID of the device contained in single 32 bit value
 *
 * @return
 *         - \ref XPUML_SUCCESS                 successful completion
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a vxpuTypeId is invalid, or \a deviceId or \a subsystemID are NULL
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlVxpuTypeGetDeviceID(xpumlVxpuTypeId_t vxpuTypeId, unsigned long long *deviceID, unsigned long long *subsystemID);

/**
 * Retrieve the maximum number of vXPU instances creatable on a device for given vXPU type
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param device                   The identifier of the target device
 * @param vxpuTypeId               Handle to vXPU type
 * @param vxpuInstanceCount        Pointer to get the max number of vXPU instances
 *                                 that can be created on a deicve for given vxpuTypeId
 * @return
 *         - \ref XPUML_SUCCESS                 successful completion
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a vxpuTypeId is invalid or is not supported on target device,
 *                                             or \a vxpuInstanceCount is NULL
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlVxpuTypeGetMaxInstances(xpumlDevice_t device, xpumlVxpuTypeId_t vxpuTypeId, unsigned int *vxpuInstanceCount);

/**
 * Retrieve the active vXPU instances on a device.
 *
 * An array of active vXPU instances is returned in the caller-supplied buffer pointed at by \a vxpuInstances. The
 * array elememt count is passed in \a vxpuCount, and \a vxpuCount is used to return the number of vXPU instances
 * written to the buffer.
 *
 * If the supplied buffer is not large enough to accomodate the vXPU instance array, the function returns
 * XPUML_ERROR_INSUFFICIENT_SIZE, with the element count of xpumlVxpuInstance_t array required in \a vxpuCount.
 * To query the number of active vXPU instances, call this function with *vxpuCount = 0.  The code will return
 * XPUML_ERROR_INSUFFICIENT_SIZE, or XPUML_SUCCESS if no vXPU Types are supported.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param device                   The identifier of the target device
 * @param vxpuCount                Pointer which passes in the array size as well as get
 *                                 back the number of types
 * @param vxpuInstances            Pointer to array in which to return list of vXPU instances
 *
 * @return
 *         - \ref XPUML_SUCCESS                  successful completion
 *         - \ref XPUML_ERROR_UNINITIALIZED      if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT   if \a device is invalid, or \a vxpuCount is NULL
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE  if \a size is too small
 *         - \ref XPUML_ERROR_NOT_SUPPORTED      if vXPU is not supported by the device
 *         - \ref XPUML_ERROR_UNKNOWN            on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetActiveVxpus(xpumlDevice_t device, unsigned int *vxpuCount, xpumlVxpuInstance_t *vxpuInstances);

/**
 * Retrieve the UUID of a vXPU instance.
 *
 * The UUID is a globally unique identifier associated with the vXPU, and is returned as a 5-part hexadecimal string,
 * not exceeding 80 characters in length (including the NULL terminator).
 * See \ref xpumlConstants::XPUML_DEVICE_UUID_BUFFER_SIZE.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param vxpuInstance             Identifier of the target vXPU instance
 * @param uuid                     Pointer to caller-supplied buffer to hold vXPU UUID
 * @param size                     Size of buffer in bytes
 *
 * @return
 *         - \ref XPUML_SUCCESS                 successful completion
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a vxpuInstance is 0, or \a uuid is NULL
 *         - \ref XPUML_ERROR_NOT_FOUND         if \a vxpuInstance does not match a valid active vXPU instance on the system
 *         - \ref XPUML_ERROR_INSUFFICIENT_SIZE if \a size is too small
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlVxpuInstanceGetUUID(xpumlVxpuInstance_t vxpuInstance, char *uuid, unsigned int size);

/**
 * Retrieve the vXPU type of a vXPU instance.
 *
 * Returns the vXPU type ID of vxpu assigned to the vXPU instance.
 *
 * For Kepler &tm; or newer fully supported devices.
 *
 * @param vxpuInstance             Identifier of the target vXPU instance
 * @param vxpuTypeId               Reference to return the vxpuTypeId
 *
 * @return
 *         - \ref XPUML_SUCCESS                 if \a vxpuTypeId has been set
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \a vxpuInstance is 0, or \a vxpuTypeId is NULL
 *         - \ref XPUML_ERROR_NOT_FOUND         if \a vxpuInstance does not match a valid active vXPU instance on the system
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlVxpuInstanceGetType(xpumlVxpuInstance_t vxpuInstance, xpumlVxpuTypeId_t *vxpuTypeId);

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlUtil vXPU Utilization and Accounting
 * This chapter describes operations that are associated with vXPU Utilization and Accounting.
 *  @{
 */
/***************************************************************************************************/

/** @} */

/***************************************************************************************************/
/** @defgroup xpumlMultiInstanceXPU Multi Instance XPU Management
 * This chapter describes XPUML operations that are associated with Multi Instance XPU management.
 *  @{
 */
/***************************************************************************************************/

/**
 * Get the type of the XPU Bus (PCIe, PCI, ...)
 *
 * @param device                               The identifier of the target device
 * @param type                                 The PCI Bus type
 *
 * return
 *         - \ref XPUML_SUCCESS                 if the bus \a type is successfully retreived
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \device is invalid or \type is NULL
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
//xpumlReturn_t DECLDIR xpumlDeviceGetBusType(xpumlDevice_t device, xpumlBusType_t *type);


#define XPUML_CXPU_MEM_UNLIMIT (~0ull)
#define XPUML_CXPU_INST_ID_LEN (12)

/**
 * @brief xpumlDeviceSetCxpuInstanceMemoryLimit
 *
 * See \ref xpumlMemoryLocation_t for details on available memoryType.
 *
 * @param device                               The identifier of the target device
 * @param instanceId                           The identifier of cxpu instance
 * @param memoryType                           The target memory type
 * @param limitBytes                           The memory limit bytes of the memoryType
 *
 * return
 *         - \ref XPUML_SUCCESS                 if the bus \a type is successfully retreived
 *         - \ref XPUML_ERROR_UNINITIALIZED     if the library has not been successfully initialized
 *         - \ref XPUML_ERROR_INVALID_ARGUMENT  if \device is invalid or \type is NULL
 *         - \ref XPUML_ERROR_UNKNOWN           on any unexpected error
 */
xpumlReturn_t DECLDIR xpumlDeviceSetCxpuInstanceMemoryLimit(xpumlDevice_t device, char *instanceId,
        xpumlMemoryLocation_t memType, unsigned long long limitBytes);

/**
 * @brief xpumlDeviceSetCxpuInstanceCoreLimit
 *
 * @param device                               The identifier of the target device
 * @param instanceId                           The identifier of cxpu instance
 * @param cores                                 The memory cores of the instance
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceSetCxpuInstanceCoreLimit(xpumlDevice_t device, char *instanceId, unsigned cores);

/**
 * @brief xpumlDeviceCreateCxpuInstance
 *
 * @param device                               The identifier of the target device
 * @param instanceId                           The identifier of cxpu instance
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceCreateCxpuInstance(xpumlDevice_t device, char *instanceId);

/**
 * @brief xpumlDeviceDestroyCxpuInstance
 *
 * @param device                               The identifier of the target device
 * @param instanceId                           The identifier of cxpu instance
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceDestroyCxpuInstance(xpumlDevice_t device, char *instanceId);

/**
 * @brief xpumlDeviceEnableCxpuMemoryLimit
 *
 * @param device                               The identifier of the target device
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceEnableCxpuMemoryLimit(xpumlDevice_t device);

/**
 * @brief xpumlDeviceEnableCxpuCoreLimit
 *
 * @param device                               The identifier of the target device
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceEnableCxpuCoreLimit(xpumlDevice_t device);

/**
 * @brief xpumlDeviceGetCxpuMemoryInfo
 *
 * See \ref xpumlMemory_t for details on available memory info.
 *
 * @param device                               XPUML device handle
 * @param memory                               Reference in which to return the memory information
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCxpuMemoryInfo(xpumlDevice_t device, xpumlMemory_t *memory);

/**
 * @brief xpumlDeviceGetCxpuInstanceCount
 *
 * @param device                               XPUML device handle
 * @param count                                Active cXPU instances count of the device
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCxpuInstanceCount(xpumlDevice_t device, unsigned int *count);

/**
 * @brief xpumlDeviceGetCxpuMaxInstanceCount
 *
 * @param device                               XPUML device handle
 * @param count                                The max cXPU instances count of the device
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCxpuMaxInstanceCount(xpumlDevice_t device, unsigned int *count);

/**
 * @brief xpumlDeviceGetCxpuInstanceId
 *
 * @param device                               XPUML device handle
 * @param index                                The index of cXPU instance (0 ~ Max cXPU instance count)
 * @param instanceId                           The cXPU instance ID of this instance
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCxpuInstanceId(xpumlDevice_t device, unsigned int index, char *instanceId);

/**
 * @brief xpumlDeviceGetCxpuInstanceMemoryInfo
 *
 * See \ref xpumlMemory_t for details on available memory info.
 *
 * @param device                               XPUML device handle
 * @param instanceId                           The cXPU instance ID of the instance
 * @param memory                               Reference in which to return the memory information
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCxpuInstanceMemoryInfo(xpumlDevice_t device, char *instanceId, xpumlMemory_t *memory);

/**
 * @brief xpumlDeviceGetCxpuInstanceCoreInfo
 *
 * See \ref xpumlCores_t for details on available core info.
 *
 * @param device                               XPUML device handle
 * @param instanceId                           The cXPU instance ID of the instance
 * @param cores                                Reference in which to return the core information
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCxpuInstanceCoreInfo(xpumlDevice_t device, char *instanceId, xpumlCores_t *cores);

/**
 * @brief xpumlDeviceGetCxpuCoreInfo
 *
 * See \ref xpumlCores_t for details on available memory info.
 *
 * @param device                               XPUML device handle
 * @param cores                                Reference in which to return the core information
 *
 * @returns
 */
xpumlReturn_t DECLDIR xpumlDeviceGetCxpuCoreInfo(xpumlDevice_t device, xpumlCores_t *cores);

/** @} */

/**
 * XPUML API versioning support
 */

#ifdef XPUML_NO_UNVERSIONED_FUNC_DEFS
xpumlReturn_t DECLDIR xpumlInit(void);
#endif // #ifdef XPUML_NO_UNVERSIONED_FUNC_DEFS

#if defined(XPUML_NO_UNVERSIONED_FUNC_DEFS)
// We don't define APIs to run new versions if this guard is present so there is
// no need to undef
#elif defined(__XPUML_API_VERSION_INTERNAL)
#undef xpumlInit
#endif

#ifdef __cplusplus
}
#endif

#endif
