/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file ccsp_hal_emmc.h
 * @brief Interface definition for the RDK-B eMMC HAL: device and health information
 *        for the embedded MMC (eMMC) storage part.
 *
 * This header is the complete eMMC HAL contract. It declares the status codes an
 * implementation may return, the device, partition, diagnostic and health data types
 * a caller reads, and the two functions that retrieve them. The values reported are
 * read by the vendor implementation from the data the Linux block drivers populate,
 * as stated in the repository specification under `Theory of operation and key
 * concepts` in the HAL specification.
 *
 * The interface declares no initialization, open, close or teardown call: a caller
 * retrieves information on demand and no session is opened or closed. It also
 * declares no callback registration, so nothing here delivers an asynchronous
 * notification.
 *
 * Behaviour stated in this header is derived from these declarations and from the
 * repository specification. Nothing is presented as observed runtime behaviour.
 */

/**
 * @defgroup EMMC_HAL EMMC HAL
 *
 * The EMMC HAL (Hardware Abstraction Layer) is used in RDK-B (Reference Design Kit-Broadband) to provide an abstraction layer for EMMC (Embedded Multi-Media Controller) status.
 *
 * @defgroup EMMC_HAL_TYPES  EMMC HAL Data Types
 * @ingroup  EMMC_HAL
 *
 * @defgroup EMMC_HAL_APIS   EMMC HAL APIs
 * @ingroup  EMMC_HAL
 *
 **/

#ifndef __CCSP_HAL_EMMC_H__
#define __CCSP_HAL_EMMC_H__

#include <stdbool.h>

/**
 * @addtogroup EMMC_HAL_TYPES
 * @{
 */

/* Defines */
#define RDK_STMGR_MAX_DEVICES                        10 /*!< Element count of the per-device arrays in eSTMGRDeviceIDs and eSTMGRDeviceInfoList; the populated entries are bounded by the m_numOfDevices count that accompanies them. */
#define RDK_STMGR_MAX_STRING_LENGTH                 128 /*!< Size in bytes of every fixed identity and text array in this interface. This is the array size, so a caller must bound reads by it: the interface does not state whether an implementation NUL-terminates a value that fills the array. */
#define RDK_STMGR_PARTITION_LENGTH                  256 /*!< Size in bytes of the partition description array eSTMGRDeviceInfo.m_partitions, whose layout is vendor-specific. */
#define RDK_STMGR_DIAGNOSTICS_LENGTH                256 /*!< Size in bytes of the diagnostic text array eSTMGREventMessage.m_diagnostics. */
#define RDK_STMGR_DIAGNOSTICS_BLOB_LENGTH          2048 /*!< Size in bytes of the opaque diagnostics buffer eSTMGRHealthInfo.m_diagnostics.m_blob. */
#define RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES          20 /*!< Element count of the SMART attribute array in eSTMGRDiagAttributesList; the populated entries are bounded by the accompanying m_numOfAttributes count. */

/* Structs & Enums */
/**
 * @enum _stmgr_ReturnCode
 *
 * @brief Status codes returned by the eMMC HAL functions.
 *
 * These five codes are the complete set this interface defines. Every function
 * reports its outcome synchronously through its return value; no failure is reported
 * out of band, as stated under `Internal Error Handling` in the repository
 * specification.
 */
typedef enum _stmgr_ReturnCode {
    RDK_STMGR_RETURN_SUCCESS = 0,          /*!< The requested information was read and the caller's structure has been populated. */
    RDK_STMGR_RETURN_GENERIC_FAILURE = -1, /*!< The read failed for a reason the implementation attributes to itself or to the storage subsystem. The caller's structure must be treated as unpopulated. */
    RDK_STMGR_RETURN_INIT_FAILURE = -2,    /*!< The implementation could not bring up the resources it needs to reach the device. This interface declares no initialization call, so the code reports a failure internal to the implementation rather than a step the caller omitted. */
    /**
     * Which condition produced this code, whether the read had been attempted, and
     * whether anything was written through the caller's pointer are all unstated, so a
     * repeat of the same call in the same state must not be assumed to behave the same
     * way.
     *
     * Passing a non-NULL pointer is a caller pre-condition. NULL is the one invalid
     * input a callee can detect at all, so it is at most one example of what may map
     * here rather than the definition of the code, which this interface neither
     * restricts to NULL nor enumerates further. A caller must not read the absence of
     * this code as evidence that its arguments were accepted as valid.
     *
     * What cannot be reported here is a non-NULL pointer that does not address writable
     * storage of the declared type: C gives an implementation no way to determine where
     * a pointer came from or how far it extends, so such a call is written through as
     * though it were correct - undefined behaviour rather than a reported error.
     */
    RDK_STMGR_RETURN_INVALID_INPUT = -3,   /*!< The implementation reported the call as rejected on an input or state check it makes. The caller's structure must be treated as unpopulated, as it must for every failure code. */
    RDK_STMGR_RETURN_UNKNOWN_FAILURE = -4  /*!< The read failed for a reason the implementation cannot attribute. This interface does not partition the unattributable case further from RDK_STMGR_RETURN_GENERIC_FAILURE, so a caller cannot distinguish the two by cause. */
} eSTMGRReturns;

/**
 * @enum _stmgr_DeviceType
 *
 * @brief Class of storage medium a device or health record describes.
 *
 * The enumeration spans the storage classes of the `stmgr` (storage manager) data
 * model from which these declarations derive. An eMMC implementation reports
 * RDK_STMGR_DEVICE_TYPE_EMMCCARD; the remaining classes are declared because the
 * data types are shared, not because this HAL reports them.
 */
typedef enum _stmgr_DeviceType {
    RDK_STMGR_DEVICE_TYPE_HDD   = 0,    /*!< Hard disk drive. */
    RDK_STMGR_DEVICE_TYPE_SDCARD = 1,   /*!< Removable SD card. */
    RDK_STMGR_DEVICE_TYPE_USB = 2,      /*!< Device reached over USB mass storage. */
    RDK_STMGR_DEVICE_TYPE_FLASH = 3,    /*!< Raw flash part. */
    RDK_STMGR_DEVICE_TYPE_NVRAM = 4,    /*!< Non-volatile RAM. */
    RDK_STMGR_DEVICE_TYPE_EMMCCARD = 5, /*!< Embedded MMC part, which is the class an eMMC HAL implementation reports. */
    RDK_STMGR_DEVICE_TYPE_MAX = 6       /*!< Upper bound of the enumeration. It is a sentinel rather than a class of medium, so no field is populated with it. */
} eSTMGRDeviceType;

/**
 * @enum _stmgr_DeviceStatus
 *
 * @brief Condition flags reported for a storage device or partition.
 *
 * The non-zero members are bit flags, each occupying a distinct bit, so an
 * implementation may report several conditions at once by combining them with a
 * bitwise OR. A caller must therefore test a status field with a bitwise AND against
 * the flag of interest and must not compare the field for equality against a single
 * member. RDK_STMGR_DEVICE_STATUS_OK is the zero value, meaning that no flag is set,
 * and is the one member that cannot be tested with a mask: it is recognised by
 * comparing the whole field against zero.
 *
 * @note This interface exposes status values only. It does not specify which
 *       transitions between them are legal, or in what order they occur, so a caller
 *       must not infer a state machine from the members below.
 */
typedef enum _stmgr_DeviceStatus {
    RDK_STMGR_DEVICE_STATUS_OK              = 0,          /*!< No condition flag is set: no fault is reported for the device or partition. */
    RDK_STMGR_DEVICE_STATUS_READ_ONLY       = (1 << 0),   /*!< Reads are accepted but writes are not, so a caller must not expect a write to succeed. */
    RDK_STMGR_DEVICE_STATUS_NOT_PRESENT     = (1 << 1),   /*!< No medium is present, so the identity, capacity and free-space fields of the record carry no meaningful value. */
    RDK_STMGR_DEVICE_STATUS_NOT_QUALIFIED   = (1 << 2),   /*!< A medium is present but it is not a part the platform qualifies for use. */
    RDK_STMGR_DEVICE_STATUS_DISK_FULL       = (1 << 3),   /*!< No free space remains, so a caller must not expect a write that grows stored data to succeed. */
    RDK_STMGR_DEVICE_STATUS_READ_FAILURE    = (1 << 4),   /*!< A read against the medium failed. */
    RDK_STMGR_DEVICE_STATUS_WRITE_FAILURE   = (1 << 5),   /*!< A write against the medium failed. */
    RDK_STMGR_DEVICE_STATUS_UNKNOWN         = (1 << 6)    /*!< The implementation could not determine the condition of the device, so a caller cannot infer it from this field. */
} eSTMGRDeviceStatus;

/**
 * @enum _stmgr_events
 *
 * @brief Storage event identifiers carried by eSTMGREventMessage.
 *
 * @warning No function declared in this header delivers these events, and no callback
 *          type that would carry them is declared here; the repository specification
 *          states under `Asynchronous Notification Model` that there are no
 *          asynchronous notifications in the HAL specification. A caller cannot
 *          subscribe to a storage event through this interface; the identifiers are
 *          declared because the message type that uses them is part of the shared
 *          storage data model.
 *
 * @see eSTMGREventMessage
 */
typedef enum _stmgr_events {
    RDK_STMGR_EVENT_STATUS_CHANGED = 100, /*!< The condition flags reported for a device changed. */
    RDK_STMGR_EVENT_HEALTH_WARNING = 101, /*!< A health indicator the implementation monitors crossed a threshold. */
    RDK_STMGR_EVENT_DEVICE_FAILURE = 102  /*!< The device failed and can no longer be relied on for storage. */
} eSTMGREvents;

/**
 * @brief A bounded list of storage device identifiers.
 *
 * @note No function declared in this header populates this type. It is part of the
 *       shared storage data model from which these declarations derive.
 */
typedef struct _stmgr_DeviceIds {
    unsigned short m_numOfDevices;                                        /*!< Number of identifiers populated in m_deviceIDs. A caller must not read beyond this count, which never exceeds RDK_STMGR_MAX_DEVICES (10). */
    char m_deviceIDs[RDK_STMGR_MAX_DEVICES][RDK_STMGR_MAX_STRING_LENGTH]; /*!< Storage for up to RDK_STMGR_MAX_DEVICES (10) identifiers, each occupying RDK_STMGR_MAX_STRING_LENGTH (128) bytes. The identifier text is vendor-specific. */
} eSTMGRDeviceIDs;

/**
 * @brief Identity, capacity and condition of a single storage device.
 *
 * This is the record CcspHalEmmcGetDeviceInfo() populates. Every text field is a
 * fixed-size array of RDK_STMGR_MAX_STRING_LENGTH (128) bytes whose content is
 * supplied by the vendor implementation from the underlying block driver data, so a
 * caller reads it as vendor-specific text rather than parsing it against a format
 * this interface defines.
 *
 * @see CcspHalEmmcGetDeviceInfo
 */
typedef struct _stmgr_DeviceInfo {
    char m_deviceID[RDK_STMGR_MAX_STRING_LENGTH];         /*!< Vendor-specific identifier or serial number that distinguishes this device, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. */
    eSTMGRDeviceType m_type;                              /*!< Class of medium the record describes; an eMMC implementation reports RDK_STMGR_DEVICE_TYPE_EMMCCARD. @see eSTMGRDeviceType */
    unsigned long long m_capacity;                        /*!< Total capacity of the device. This interface does not state the unit, so a caller must not assume bytes; note that the capacity fields of eSTMGRPartitionInfo are stated in bytes while this one is not. */
    eSTMGRDeviceStatus m_status;                          /*!< Condition flags for the device. Several flags may be set at once, so the field is tested with a bitwise AND rather than compared for equality. @see eSTMGRDeviceStatus */
    char m_partitions[RDK_STMGR_PARTITION_LENGTH];        /*!< Description of the partitions present, in up to RDK_STMGR_PARTITION_LENGTH (256) bytes. The layout and any separator are vendor-specific, so a caller must not parse it against a fixed format. */
    char m_manufacturer[RDK_STMGR_MAX_STRING_LENGTH];     /*!< Manufacturer name reported for the device, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. */
    char m_model[RDK_STMGR_MAX_STRING_LENGTH];            /*!< Model name reported for the device, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. */
    char m_serialNumber[RDK_STMGR_MAX_STRING_LENGTH];     /*!< Serial number reported by the device itself, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. This interface states no relationship between this field and m_deviceID, so a caller must not assume the two carry the same value. */
    char m_firmwareVersion[RDK_STMGR_MAX_STRING_LENGTH];  /*!< Firmware revision reported by the device, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. The format is vendor-specific, so a caller compares it as opaque text rather than parsing it as a version number. */
    char m_hwVersion[RDK_STMGR_MAX_STRING_LENGTH];        /*!< Hardware revision reported by the device, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. The format is vendor-specific. */
    char m_ifATAstandard[RDK_STMGR_MAX_STRING_LENGTH];    /*!< ATA standard the device declares conformance to, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. An eMMC part need not declare one, in which case this interface does not state what the field contains. */
    bool m_hasSMARTSupport;                               /*!< True when the device reports SMART (Self-Monitoring, Analysis and Reporting Technology) data. When false, a caller must not expect the SMART-derived attributes of the health record to carry meaningful values. @see eSTMGRHealthInfo */
} eSTMGRDeviceInfo;

/**
 * @brief A bounded list of storage device information records.
 *
 * @note No function declared in this header populates this type. It is part of the
 *       shared storage data model from which these declarations derive;
 *       CcspHalEmmcGetDeviceInfo() reports a single device through
 *       eSTMGRDeviceInfo rather than a list.
 */
typedef struct _stmgr_DeviceInfos {
    unsigned short m_numOfDevices;                     /*!< Number of records populated in m_devices. A caller must not read beyond this count, which never exceeds RDK_STMGR_MAX_DEVICES (10). */
    eSTMGRDeviceInfo m_devices[RDK_STMGR_MAX_DEVICES]; /*!< Storage for up to RDK_STMGR_MAX_DEVICES (10) device records; only the first m_numOfDevices entries are populated. */
} eSTMGRDeviceInfoList;

/**
 * @brief Identity, capacity and condition of a single partition on a storage device.
 *
 * @note No function declared in this header populates this type. It is part of the
 *       shared storage data model from which these declarations derive.
 */
typedef struct _stmgr_PartitionInfo {
    char m_partitionId [RDK_STMGR_MAX_STRING_LENGTH];  /*!< Vendor-specific identifier that distinguishes this partition, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. */
    char m_name [RDK_STMGR_MAX_STRING_LENGTH];         /*!< Partition name, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. */
    char m_mountPath [RDK_STMGR_MAX_STRING_LENGTH];    /*!< File system path at which the partition is mounted, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. It is the path through which the contents of the partition are reached. */
    char m_format[RDK_STMGR_MAX_STRING_LENGTH];        /*!< File system format of the partition, for example ext4, NTFS or FAT32, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. This interface does not restrict the value to a fixed vocabulary. */
    eSTMGRDeviceStatus m_status;                       /*!< Condition flags for the partition, drawn from the same flag set as a device status. Several flags may be set at once, so the field is tested with a bitwise AND rather than compared for equality. @see eSTMGRDeviceStatus */
    unsigned long long m_capacity;                     /*!< Total capacity of the partition in bytes. */
    unsigned long long m_freeSpace;                    /*!< Free space remaining on the partition in bytes; it never exceeds m_capacity. */
    bool m_isTSBSupported;                             /*!< True when the partition may be used for Time-Shift Buffering (TSB); false when it may not. */
    bool m_isDVRSupported;                             /*!< True when the partition may be used for Digital Video Recording (DVR); false when it may not. */
} eSTMGRPartitionInfo;

/**
 * @brief One SMART diagnostic attribute, as a name and its value.
 */
typedef struct _stmgr_DiagnosticsAttributes {
    char m_name[RDK_STMGR_MAX_STRING_LENGTH];   /*!< Name identifying the SMART (Self-Monitoring, Analysis and Reporting Technology) attribute, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. The vocabulary of names is vendor-specific, so a caller matches on the names its vendor documents. */
    char m_value[RDK_STMGR_MAX_STRING_LENGTH];  /*!< Value of the attribute as a comma-separated string of up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes, so one attribute may carry several related readings in a single field. Both the field order and the units are vendor-specific. */
} eSTMGRDiagAttributes;

/**
 * @brief A bounded set of SMART diagnostic attributes, with the count that is valid.
 */
typedef struct _stmgr_DiagnosticsAttributeList {
    unsigned short m_numOfAttributes;                                        /*!< Number of attributes populated in m_diagnostics. A caller must not read beyond this count, which never exceeds RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20). */
    eSTMGRDiagAttributes m_diagnostics[RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES]; /*!< Storage for up to RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) attributes; only the first m_numOfAttributes entries are populated. */
} eSTMGRDiagAttributesList;

/**
 * @brief Operational and health state of a storage device, with its diagnostic
 *        attribute sets.
 *
 * This is the record CcspHalEmmcGetHealthInfo() populates. It carries the device
 * identity and class, two operational flags, one diagnostics union and four further
 * attribute lists. Each list is an eSTMGRDiagAttributesList, so each is bounded by
 * RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) entries and carries its own count of the
 * entries that are valid.
 *
 * @see CcspHalEmmcGetHealthInfo
 * @see eSTMGRDiagAttributesList
 */
typedef struct _stmgr_Health {
    char m_deviceID[RDK_STMGR_MAX_STRING_LENGTH];               /*!< Vendor-specific identifier or serial number that distinguishes the device this record describes, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. */
    eSTMGRDeviceType m_deviceType;                              /*!< Class of medium the record describes; an eMMC implementation reports RDK_STMGR_DEVICE_TYPE_EMMCCARD. @see eSTMGRDeviceType */
    bool m_isOperational;                                       /*!< True when the device is usable for storage operations; false when it is not. This interface declares it separately from m_isHealthy and states no relationship between the two, so a caller reads both rather than inferring one from the other. */
    bool m_isHealthy;                                           /*!< True when the implementation judges the device to be within its health limits; false when a limit has been reached or exceeded. The limits and the judgement are vendor-specific. */
    union {
        eSTMGRDiagAttributesList m_list;                        /*!< Diagnostics as structured SMART attributes: up to RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) name and value pairs, the valid count given by the list's own m_numOfAttributes. */
        char m_blob[RDK_STMGR_DIAGNOSTICS_BLOB_LENGTH];         /*!< Diagnostics as an opaque buffer of up to RDK_STMGR_DIAGNOSTICS_BLOB_LENGTH (2048) bytes. Its content and encoding are vendor-defined, so a caller can only interpret it against a vendor description. */
    } m_diagnostics;                                            /*!< Diagnostic information in one of two mutually exclusive forms, structured in m_list or opaque in m_blob. This interface does not specify how a caller determines which member is valid: no tag field or flag selects between them, so neither member may be read without a vendor agreement that establishes which form is populated. */
    eSTMGRDiagAttributesList m_lifetimesList;                   /*!< Attributes describing the device's consumed and remaining lifetime, bounded by RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) entries. */
    eSTMGRDiagAttributesList m_firstExceededConfiguredLife;     /*!< Attributes recorded when the device first exceeded the lifetime configured for it, bounded by RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) entries. An empty list, indicated by a zero count, means the implementation reports no such occurrence. */
    eSTMGRDiagAttributesList m_firstExceededMaxLife;            /*!< Attributes recorded when the device first exceeded its maximum rated lifetime, bounded by RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) entries. An empty list, indicated by a zero count, means the implementation reports no such occurrence. */
    eSTMGRDiagAttributesList m_healthStatesList;                /*!< Attributes describing the health states the implementation tracks for the device, bounded by RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) entries. */
} eSTMGRHealthInfo;

/**
 * @brief A storage event, the device it concerns and the diagnostic context for it.
 *
 * @warning No function declared in this header delivers this message, and no callback
 *          type that would carry it is declared here; the repository specification
 *          states under `Asynchronous Notification Model` that there are no
 *          asynchronous notifications in the HAL specification.
 *
 * @see eSTMGREvents
 */
typedef struct _stmgr_EventMessage {
    eSTMGREvents m_eventType;                         /*!< Which of the three storage events this message reports. @see eSTMGREvents */
    char m_deviceID[RDK_STMGR_MAX_STRING_LENGTH];     /*!< Vendor-specific identifier of the device the event concerns, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. This interface does not state that the value matches the identifier a device or health record reports for the same device, so a caller must not correlate on it without a vendor agreement. */
    eSTMGRDeviceType m_deviceType;                    /*!< Class of medium the device belongs to. @see eSTMGRDeviceType */
    eSTMGRDeviceStatus m_deviceStatus;                /*!< Condition flags for the device at the time of the event. Several flags may be set at once, so the field is tested with a bitwise AND rather than compared for equality. @see eSTMGRDeviceStatus */
    char m_description[RDK_STMGR_MAX_STRING_LENGTH];  /*!< Human-readable description of the event, in up to RDK_STMGR_MAX_STRING_LENGTH (128) bytes. The text is vendor-specific and is intended for logging rather than for programmatic matching. */
    char m_diagnostics[RDK_STMGR_DIAGNOSTICS_LENGTH]; /*!< Diagnostic detail for the event, in up to RDK_STMGR_DIAGNOSTICS_LENGTH (256) bytes. The content is vendor-specific. */
} eSTMGREventMessage;

/**
 * @brief The device context that accompanies a storage callback.
 *
 * @warning No function declared in this header registers a callback, so nothing
 *          delivers this structure, and the repository specification states under
 *          `Asynchronous Notification Model` that there are no asynchronous
 *          notifications in the HAL specification.
 */
typedef struct _stmgr_CallBackData{
   bool isSDCard;       /*!< True when the device the callback concerns is an SD card; false for any other class of medium, including an embedded MMC part. */
   char mountPath[200]; /*!< File system path at which the device is mounted, through which its contents are reached. The array holds 200 bytes. Unlike every other array in this interface the bound is a literal rather than a named constant, so a caller bounds reads and writes by 200 bytes and cannot derive the bound from a macro. */
}eSTMGRCallBackData;

/** @} */  //END OF GROUP EMMC_HAL_TYPES

// HAL Functions

/*
 * Return code contract for the functions below: each reports its outcome
 * synchronously through its eSTMGRReturns return value, as stated under `Internal
 * Error Handling` in the repository specification. The five codes eSTMGRReturns
 * declares are the complete set an implementation may return, and each function
 * documents the condition under which it returns each of them together with the
 * action a client takes in response.
 */

/**
 * @addtogroup EMMC_HAL_APIS
 * @{
 */

/**
 * @brief Reads the health record of the eMMC device into a caller-supplied structure.
 *
 * Populates the caller's eSTMGRHealthInfo with the device identity and class, the
 * operational and health flags, the diagnostics union and the four lifetime and
 * health attribute lists. The values are those the vendor implementation reads from
 * the data the Linux block drivers populate, as stated under `Theory of operation and
 * key concepts` in the repository specification. A caller
 * invokes this at runtime whenever health information is needed; the repository
 * specification states under `Initialization and Startup` that it is not called
 * during system bootup.
 *
 * @param[out] pHealthInfo Pointer to a caller-allocated eSTMGRHealthInfo that the
 *                         implementation fills in. The caller guarantees - and the
 *                         implementation cannot verify - that it is non-NULL, that it
 *                         addresses writable storage of at least
 *                         sizeof(eSTMGRHealthInfo) bytes correctly aligned for that
 *                         type, and that the storage remains valid for the whole call.
 *                         C gives an implementation no way to learn where a non-NULL
 *                         pointer came from or how far it extends, so a pointer that is
 *                         merely wrong - one addressing a smaller object, a released
 *                         allocation, an interior position or automatic storage that
 *                         has gone out of scope - is written through as though it were
 *                         correct, corrupting the caller's memory. That outcome is not
 *                         reported through the return value and cannot be. A NULL
 *                         pointer, by contrast, is an input an implementation can test
 *                         for and reject with RDK_STMGR_RETURN_INVALID_INPUT; that is
 *                         the one invalid input this interface names, not a definition
 *                         of the code, and which further invalid-input or invalid-state
 *                         conditions an implementation maps to it is unspecified here.
 *                         The caller both allocates and releases that storage, which
 *                         the `Memory Model` topic of the repository specification
 *                         places on the caller; the implementation manages and releases
 *                         only its own internal allocations. This interface does not
 *                         specify whether the implementation retains the pointer beyond
 *                         the call, and it declares no call through which a retained
 *                         pointer could be withdrawn, so a caller must not read the
 *                         return as permission to release or reuse the storage: it
 *                         keeps the storage valid and unreused unless it has
 *                         established the retention behaviour with its vendor. The
 *                         repository specification records the same absence under
 *                         `Memory Model` and imposes no non-retention obligation of its
 *                         own, so there is no obligation here for a caller to rely on
 *                         and none for it to be denied.
 *                         The record's fixed arrays bound what may be read from it:
 *                         m_deviceID holds RDK_STMGR_MAX_STRING_LENGTH (128) bytes;
 *                         each of the five eSTMGRDiagAttributesList members it
 *                         contains - the m_list arm of m_diagnostics, m_lifetimesList,
 *                         m_firstExceededConfiguredLife, m_firstExceededMaxLife and
 *                         m_healthStatesList - holds at most
 *                         RDK_STMGR_MAX_DIAGNOSTIC_ATTRIBUTES (20) entries and
 *                         reports the valid count in its own m_numOfAttributes; and
 *                         the m_blob arm of m_diagnostics holds
 *                         RDK_STMGR_DIAGNOSTICS_BLOB_LENGTH (2048) bytes.
 *
 * @pre The interface declares no initialization, open or teardown call, so it imposes no
 *      call ordering on the caller and no session is opened or closed. The only
 *      pre-condition is the one stated on pHealthInfo above, and the caller is the only
 *      party that can satisfy it. A NULL argument is one an implementation can detect,
 *      and RDK_STMGR_RETURN_INVALID_INPUT is the code available for reporting a rejected
 *      input; a non-NULL pointer that does not address writable storage of the declared
 *      type is not detectable by an implementation and is therefore undefined behaviour
 *      rather than a reported error. A caller satisfies the pre-condition itself rather
 *      than relying on any rejection: this interface does not state which inputs or
 *      states beyond NULL an implementation checks.
 *
 * @post On RDK_STMGR_RETURN_SUCCESS the fields the implementation was able to read
 *       are populated. This interface does not state that fields it could not read
 *       are zeroed or otherwise initialised, so a caller must not treat an unset
 *       field as meaningful. On any failure code no field of the record may be
 *       relied on.
 *
 * @returns eSTMGRReturns - the outcome of the read, reported synchronously.
 * @retval RDK_STMGR_RETURN_SUCCESS - The health record was read; the caller may read
 *         pHealthInfo subject to the count and bound rules above.
 * @retval RDK_STMGR_RETURN_GENERIC_FAILURE - The read failed for a reason the
 *         implementation attributes to itself or to the storage subsystem. The client
 *         discards the record, logs the failure and may retry; a persistent failure is
 *         reported to the vendor implementation's owner rather than worked around.
 * @retval RDK_STMGR_RETURN_INIT_FAILURE - The implementation could not bring up the
 *         resources it needs to reach the device. Since this interface declares no
 *         initialization call, there is no caller-side step to repeat: the client
 *         treats health information as unavailable for now and may retry later.
 * @retval RDK_STMGR_RETURN_INVALID_INPUT - The implementation reported the call as
 *         rejected on an input or state check. This code does not establish, and a
 *         client must not infer: which condition produced it; whether the read had been
 *         attempted; or whether anything was written through pHealthInfo, which is why
 *         the record must not be read after any failure. NULL is the one invalid input a
 *         callee can detect at all and what a client checks first, but it is at most one
 *         example of what may map here, not the definition of the code, which is neither
 *         NULL-only nor enumerated. Whether repeating the call in the same state yields
 *         the same code is unstated, so no retry policy may rest on it. It is also not
 *         evidence that a non-NULL pointer's storage was validated, which no
 *         implementation can do, so a wrong pointer is not reported here.
 * @retval RDK_STMGR_RETURN_UNKNOWN_FAILURE - The read failed for a reason the
 *         implementation cannot attribute. The client acts as for
 *         RDK_STMGR_RETURN_GENERIC_FAILURE; this interface does not partition the two
 *         by cause, so no finer diagnosis can be drawn from the distinction.
 *
 * @note m_diagnostics is a union. This interface does not specify how a caller
 *       determines which member is valid, so neither m_list nor m_blob may be read
 *       without a vendor agreement establishing which form the implementation
 *       populates.
 * @note m_hasSMARTSupport, reported by CcspHalEmmcGetDeviceInfo(), tells a caller
 *       whether the device reports SMART data at all, and therefore whether the
 *       SMART-derived attributes of this record can carry meaningful values. It is a
 *       field of eSTMGRDeviceInfo, not a discriminator for the union above.
 * @note The call is synchronous and may block until the eMMC hardware is ready, which
 *       the repository specification states under `Initialization and Startup`. Under
 *       `Blocking calls` it is expected to complete within a period commensurate with
 *       the complexity of the operation, and to apply a timeout where a response may
 *       be absent. This interface states no numeric timeout, so a caller that cannot
 *       tolerate an unbounded wait imposes its own bound.
 *
 * @warning An implementation cannot validate the storage pHealthInfo addresses. NULL it
 *          can test for; the provenance and extent of a non-NULL pointer it cannot,
 *          because C exposes neither to the callee. Whatever other inputs or states an
 *          implementation may reject with RDK_STMGR_RETURN_INVALID_INPUT, a merely
 *          wrong pointer is not among them: a caller that passes an unchecked, foreign
 *          or already-released pointer gets its own memory overwritten rather than a
 *          return code. The pointer has to be established as correct at the call site.
 * @warning This interface is not required to be thread safe, and the repository
 *          specification places on the calling module the obligation to make its calls
 *          into the eMMC HAL in a thread safe manner - that is, to serialise them.
 *          The functions are expected to be callable from multiple processes, and a
 *          vendor implementation may use internal threads or events provided it
 *          synchronises them and cleans them up on closure.
 *
 * @see eSTMGRHealthInfo
 * @see eSTMGRReturns
 * @see CcspHalEmmcGetDeviceInfo
 */
eSTMGRReturns CcspHalEmmcGetHealthInfo (eSTMGRHealthInfo* pHealthInfo);

/**
 * @brief Reads the identity and condition of the eMMC device into a caller-supplied
 *        structure.
 *
 * Populates the caller's eSTMGRDeviceInfo with the device identifier, class,
 * capacity, condition flags, partition description, manufacturer, model, serial
 * number, firmware and hardware revisions, declared ATA standard and SMART support
 * flag. The values are those the vendor implementation reads from the data the Linux
 * block drivers populate, as stated under `Theory of operation and key concepts` in
 * the repository specification. A caller invokes this at
 * runtime whenever device information is needed; the repository specification states
 * under `Initialization and Startup` that it is not called during system bootup.
 *
 * @param[out] pDeviceInfo Pointer to a caller-allocated eSTMGRDeviceInfo that the
 *                         implementation fills in. The caller guarantees - and the
 *                         implementation cannot verify - that it is non-NULL, that it
 *                         addresses writable storage of at least
 *                         sizeof(eSTMGRDeviceInfo) bytes correctly aligned for that
 *                         type, and that the storage remains valid for the whole call.
 *                         C gives an implementation no way to learn where a non-NULL
 *                         pointer came from or how far it extends, so a pointer that is
 *                         merely wrong - one addressing a smaller object, a released
 *                         allocation, an interior position or automatic storage that
 *                         has gone out of scope - is written through as though it were
 *                         correct, corrupting the caller's memory. That outcome is not
 *                         reported through the return value and cannot be. A NULL
 *                         pointer, by contrast, is an input an implementation can test
 *                         for and reject with RDK_STMGR_RETURN_INVALID_INPUT; that is
 *                         the one invalid input this interface names, not a definition
 *                         of the code, and which further invalid-input or invalid-state
 *                         conditions an implementation maps to it is unspecified here.
 *                         The caller both allocates and releases that storage, which
 *                         the `Memory Model` topic of the repository specification
 *                         places on the caller; the implementation manages and releases
 *                         only its own internal allocations. This interface does not
 *                         specify whether the implementation retains the pointer beyond
 *                         the call, and it declares no call through which a retained
 *                         pointer could be withdrawn, so a caller must not read the
 *                         return as permission to release or reuse the storage: it
 *                         keeps the storage valid and unreused unless it has
 *                         established the retention behaviour with its vendor. The
 *                         repository specification records the same absence under
 *                         `Memory Model` and imposes no non-retention obligation of its
 *                         own, so there is no obligation here for a caller to rely on
 *                         and none for it to be denied.
 *                         The record's fixed arrays bound what may be read from it:
 *                         m_partitions holds RDK_STMGR_PARTITION_LENGTH (256) bytes,
 *                         and each of m_deviceID, m_manufacturer, m_model,
 *                         m_serialNumber, m_firmwareVersion, m_hwVersion and
 *                         m_ifATAstandard holds
 *                         RDK_STMGR_MAX_STRING_LENGTH (128) bytes. Because that is
 *                         the array size and this interface does not state whether a
 *                         value that fills an array is NUL-terminated, a caller bounds
 *                         every read by the array size rather than relying on a
 *                         terminator.
 *
 * @pre The interface declares no initialization, open or teardown call, so it imposes no
 *      call ordering on the caller and no session is opened or closed. The only
 *      pre-condition is the one stated on pDeviceInfo above, and the caller is the only
 *      party that can satisfy it. A NULL argument is one an implementation can detect,
 *      and RDK_STMGR_RETURN_INVALID_INPUT is the code available for reporting a rejected
 *      input; a non-NULL pointer that does not address writable storage of the declared
 *      type is not detectable by an implementation and is therefore undefined behaviour
 *      rather than a reported error. A caller satisfies the pre-condition itself rather
 *      than relying on any rejection: this interface does not state which inputs or
 *      states beyond NULL an implementation checks.
 *
 * @post On RDK_STMGR_RETURN_SUCCESS the fields the implementation was able to read
 *       are populated. This interface does not state that fields it could not read
 *       are zeroed or otherwise initialised, so a caller must not treat an unset
 *       field as meaningful. On any failure code no field of the record may be
 *       relied on.
 *
 * @returns eSTMGRReturns - the outcome of the read, reported synchronously.
 * @retval RDK_STMGR_RETURN_SUCCESS - The device record was read; the caller may read
 *         pDeviceInfo subject to the bound rules above.
 * @retval RDK_STMGR_RETURN_GENERIC_FAILURE - The read failed for a reason the
 *         implementation attributes to itself or to the storage subsystem. The client
 *         discards the record, logs the failure and may retry; a persistent failure is
 *         reported to the vendor implementation's owner rather than worked around.
 * @retval RDK_STMGR_RETURN_INIT_FAILURE - The implementation could not bring up the
 *         resources it needs to reach the device. Since this interface declares no
 *         initialization call, there is no caller-side step to repeat: the client
 *         treats device information as unavailable for now and may retry later.
 * @retval RDK_STMGR_RETURN_INVALID_INPUT - The implementation reported the call as
 *         rejected on an input or state check. This code does not establish, and a
 *         client must not infer: which condition produced it; whether the read had been
 *         attempted; or whether anything was written through pDeviceInfo, which is why
 *         the record must not be read after any failure. NULL is the one invalid input a
 *         callee can detect at all and what a client checks first, but it is at most one
 *         example of what may map here, not the definition of the code, which is neither
 *         NULL-only nor enumerated. Whether repeating the call in the same state yields
 *         the same code is unstated, so no retry policy may rest on it. It is also not
 *         evidence that a non-NULL pointer's storage was validated, which no
 *         implementation can do, so a wrong pointer is not reported here.
 * @retval RDK_STMGR_RETURN_UNKNOWN_FAILURE - The read failed for a reason the
 *         implementation cannot attribute. The client acts as for
 *         RDK_STMGR_RETURN_GENERIC_FAILURE; this interface does not partition the two
 *         by cause, so no finer diagnosis can be drawn from the distinction.
 *
 * @note m_status is a set of bit flags, so a caller tests it with a bitwise AND
 *       against the flag of interest and recognises RDK_STMGR_DEVICE_STATUS_OK by
 *       comparing the whole field against zero. Several conditions may be reported at
 *       once.
 * @note m_hasSMARTSupport is what tells a caller whether the device reports SMART
 *       data, and therefore whether the SMART-derived attributes returned by
 *       CcspHalEmmcGetHealthInfo() can carry meaningful values.
 * @note m_capacity carries no unit in this interface. The capacity and free-space
 *       fields of eSTMGRPartitionInfo are stated in bytes, but that statement does not
 *       extend to this field, so a caller must not assume bytes for it.
 * @note The call is synchronous and may block until the eMMC hardware is ready, which
 *       the repository specification states under `Initialization and Startup`. Under
 *       `Blocking calls` it is expected to complete within a period commensurate with
 *       the complexity of the operation, and to apply a timeout where a response may
 *       be absent. This interface states no numeric timeout, so a caller that cannot
 *       tolerate an unbounded wait imposes its own bound.
 *
 * @warning An implementation cannot validate the storage pDeviceInfo addresses. NULL it
 *          can test for; the provenance and extent of a non-NULL pointer it cannot,
 *          because C exposes neither to the callee. Whatever other inputs or states an
 *          implementation may reject with RDK_STMGR_RETURN_INVALID_INPUT, a merely
 *          wrong pointer is not among them: a caller that passes an unchecked, foreign
 *          or already-released pointer gets its own memory overwritten rather than a
 *          return code. The pointer has to be established as correct at the call site.
 * @warning This interface is not required to be thread safe, and the repository
 *          specification places on the calling module the obligation to make its calls
 *          into the eMMC HAL in a thread safe manner - that is, to serialise them.
 *          The functions are expected to be callable from multiple processes, and a
 *          vendor implementation may use internal threads or events provided it
 *          synchronises them and cleans them up on closure.
 *
 * @see eSTMGRDeviceInfo
 * @see eSTMGRReturns
 * @see CcspHalEmmcGetHealthInfo
 */
eSTMGRReturns CcspHalEmmcGetDeviceInfo (eSTMGRDeviceInfo* pDeviceInfo);

/** @} */  //END OF GROUP EMMC_HAL_APIS

#endif /* __CCSP_HAL_EMMC_H__ */

