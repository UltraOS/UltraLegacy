#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

struct PACKED SetupData {
    enum class Recipient : u16 {
        DEVICE = 0,
        INTERFACE = 1,
        ENDPOINT = 2,
        OTHER = 3
    } recipient : 5;

    enum class Type : u16 {
        STANDARD = 0,
        CLASS = 1,
        VENDOR = 2,
    } type : 2;

    enum class DataTransferDirection : u16 {
        HostToDevice = 0,
        DeviceToHost = 1
    } data_transfer_direction : 1;

    enum class Request : u16 {
        GET_STATUS = 0,
        CLEAR_FEATURE = 1,
        SET_FEATURE = 3,
        SET_ADDRESS = 5,
        GET_DESCRIPTOR = 6,
        SET_DESCRIPTOR = 7,
        GET_CONFIGURATION = 8,
        SET_CONFIGURATION = 9,
        GET_INTERFACE = 10,
        SET_INTERFACE = 11,
        SYNCH_FRAME = 12
    } bRequest : 8;

    u16 wValue;
    u16 wIndex;
    u16 wLength;
};

enum class DescriptorType : u8 {
    DEVICE = 1,
    CONFIGURATION = 2,
    STRING = 3,
    INTERFACE = 4,
    ENDPOINT = 5,
    DEVICE_QUALIFIER = 6,
    OTHER_SPEED_CONFIGURATION = 7,
    INTERFACE_POWER = 8
};

struct PACKED DeviceDescriptor {
    u8 bLength;
    DescriptorType bDescriptorType;
    u16 bcdUSB;
    u8 bDeviceClass;
    u8 bDeviceSubClass;
    u8 bDeviceProtocol;
    u8 bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8 iManufacturer;
    u8 iProduct;
    u8 iSerialNumber;
    u8 bNumConfigurations;
};

static_assert(sizeof(SetupData) == 8, "Incorrect size of SetupData");
static_assert(sizeof(DeviceDescriptor) == 18, "Incorrect size of DeviceDescriptor");

}
