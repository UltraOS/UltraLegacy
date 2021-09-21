#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

struct PACKED USBLEGCTLSTS {
    u32 USBSMIEnable : 1;
    u32 RsvdP : 3;
    u32 SMIOnHostSystemErrorEnable : 1;
    u32 RsvdP1 : 8;
    u32 SMIOnOSOwnershipEnable : 1;
    u32 SMIOnPCICommandEnable : 1;
    u32 SMIOnBAREnable : 1;
    u32 SMIOnEventInterrupt : 1;
    u32 RsvdP2 : 3;
    u32 SMIOnHostSystemError : 1;
    u32 RsvdZ : 8;
    u32 SMIOnOSOwnershipChange : 1; // <-| RW1C
    u32 SMIOnPCICommand : 1; // <-|
    u32 SMIOnBAR : 1; // <-|
};

struct PACKED SupportedProtocolDWORD0 {
    u8 CapabilityID;
    u8 NextCapabilityPointer;
    u8 MinorRevision;
    u8 MajorRevision;
};

struct PACKED SupportedProtocolDWORD1 {
    char NameString[4];
};

struct PACKED SupportedProtocolDWORD2 {
    u32 CompatiblePortOffset : 8;
    u32 CompatiblePortCount : 8;
    u32 ProtocolDefined : 12;
    u32 PSIC : 4;
};

struct PACKED PORTPMSCUSB2 {
    u32 L1S : 3;
    u32 RWE : 1;
    u32 BESL : 4;
    u32 L1DeviceSlot : 8;
    u32 HLE : 1;
    u32 RsvdP : 11;
    u32 PortTestControl : 4;
};

struct PACKED PORTPMSCUSB3 {
    u32 U1Timeout : 8;
    u32 U2Timeout : 8;
    u32 FLA : 1;
    u32 RsvdP : 15;
};

struct PACKED PORTLIUSB2 {
    u32 RsvdP;
};

struct PACKED PORTLIUSB3 {
    u32 LinkErrorCount : 16;
    u32 RLC : 4;
    u32 TLC : 4;
    u32 RsvdP : 8;
};

// Fully RsvdP if HLC = 0
struct PACKED PORTHLPMCUSB2 {
    u32 HIRDM : 2;
    u32 L1Timeout : 8;
    u32 BESLD : 4;
    u32 RsvdP : 18;
};

// Fully RsvdP if LSECC = 0
struct PACKED PORTHLPMCUSB3 {
    u32 LinkSoftErrorCount : 16;
    u32 RsvdP : 16;
};

struct PACKED PORTSC {
    u32 CCS : 1;
    u32 PED : 1;
    u32 RsvdZ : 1;
    u32 OCA : 1;
    u32 PR : 1;
    u32 PLS : 4;
    u32 PP : 1;
    u32 PortSpeed : 4;
    u32 PIC : 2;
    u32 LWS : 1;
    u32 CSC : 1;
    u32 PEC : 1;
    u32 WRC : 1;
    u32 OCC : 1;
    u32 PRC : 1;
    u32 PLC : 1;
    u32 CEC : 1;
    u32 CAS : 1;
    u32 WCE : 1;
    u32 WDE : 1;
    u32 WOE : 1;
    u32 RsvdZ1 : 2;
    u32 DR : 1;
    u32 WPR : 1;

    static constexpr size_t offset = 0x0;
};

struct PACKED PortRegister {
    u32 PORTSC;
    u32 PORTPMSC; // <-| Different meanings for USB2/USB3
    u32 PORTLI; // <-|
    u32 PORTHLPMC; // <-|
};

struct PACKED HCSPARAMS1 {
    u32 MaxSlots : 8;
    u32 MaxIntrs : 11;
    u32 Rsvd : 5;
    u32 MaxPorts : 8;

    static constexpr size_t offset = 0x4;
};

struct PACKED HCSPARAMS2 {
    u32 IST : 4;
    u32 ERSTMax : 4;
    u32 Rsvd : 13;
    u32 MaxScratchpadBufsHi : 5;
    u32 SPR : 1;
    u32 MaxScratchpadBufsLo : 5;

    [[nodiscard]] u32 max_scratchpad_bufs() const
    {
        u32 combined = MaxScratchpadBufsLo;
        combined |= static_cast<u32>(MaxScratchpadBufsHi) << 5;

        return combined;
    }

    static constexpr size_t offset = 0x8;
};

struct PACKED HCSPARAMS3 {
    u32 U1DeviceExitLatency : 8;
    u32 Rsvd : 8;
    u32 U2DeviceExitLatency : 16;

    static constexpr size_t offset = 0xC;
};

struct PACKED HCCPARAMS1 {
    u32 AC64 : 1;
    u32 BNC : 1;
    u32 CSZ : 1;
    u32 PPC : 1;
    u32 PIND : 1;
    u32 LHRC : 1;
    u32 LTC : 1;
    u32 NSS : 1;
    u32 PAE : 1;
    u32 SPC : 1;
    u32 SEC : 1;
    u32 CFC : 1;
    u32 MaxPSASize : 4;
    u32 xECP : 16;

    static constexpr size_t offset = 0x10;
};

struct PACKED HCCPARAMS2 {
    u32 U3C : 1;
    u32 CMC : 1;
    u32 FSC : 1;
    u32 CTC : 1;
    u32 LEC : 1;
    u32 CIC : 1;
    u32 ETC : 1;
    u32 ETC_TSC : 1;
    u32 GSC : 1;
    u32 VTC : 1;
    u32 Rsvd : 22;

    static constexpr size_t offset = 0x1C;
};

struct PACKED CapabilityRegisters {
    u8 CAPLENGTH;
    u8 Rsvd;
    u16 HCIVERSION;

    u32 HCSPARAMS1;
    u32 HCSPARAMS2;
    u32 HCSPARAMS3;
    u32 HCCPARAMS1;

    u32 DBOFF;
    u32 RTSOFF;

    u32 HCCPARAMS2;

    u32 VTIOSOFF;
};

struct PACKED USBCMD {
    u32 RS : 1;
    u32 HCRST : 1;
    u32 INTE : 1;
    u32 HSEE : 1;
    u32 RsvdP : 3;
    u32 LHCRST : 1;
    u32 CSS : 1;
    u32 CRS : 1;
    u32 EWE : 1;
    u32 EU3S : 1;
    u32 RsvdP1 : 1;
    u32 CME : 1;
    u32 ETE : 1;
    u32 TSC_EN : 1;
    u32 VTIOE : 1;
    u32 RsvdP2 : 15;

    static constexpr size_t offset = 0x0;
};

struct PACKED USBSTS {
    u32 HCH : 1;
    u32 RsvdZ : 1;
    u32 HSE : 1;
    u32 EINT : 1;
    u32 PCD : 1;
    u32 RsvdZ1 : 3;
    u32 SSS : 1;
    u32 RSS : 1;
    u32 SRE : 1;
    u32 CNR : 1;
    u32 HCE : 1;
    u32 RsvdZ2 : 19;

    static constexpr size_t offset = 0x4;
};

struct PACKED CRCR {
    u64 RCS : 1;
    u64 CS : 1;
    u64 CA : 1;
    u64 CRR : 1;
    u64 RsvdP : 2;
    u64 CommandRingPointerLo : 26;
    u64 CommandRingPointerHi : 32;

    static constexpr size_t offset = 0x18;
};

struct PACKED DCBAAP {
    u32 DeviceContextBaseAddressArrayPointerLo;
    u32 DeviceContextBaseAddressArrayPointerHi;

    static constexpr size_t offset = 0x30;
};

struct PACKED CONFIG {
    u32 MaxSlotsEn : 8;
    u32 U3E : 1;
    u32 CIE : 1;
    u32 RsvdP : 22;

    static constexpr size_t offset = 0x38;
};

struct PACKED OperationalRegisters {
    u32 USBCMD;
    u32 USBSTS;

    u32 PAGESIZE;

    u64 RsvdZ;

    u32 DNCTRL;
    u64 CRCR;

    u64 RsvdZ1[2];

    u64 DCBAAP;
    u32 CONFIG;

    u8 RsvdZ2[0x400 - 0x3C];

    PortRegister port_registers[];
};

struct PACKED InterrupterManagementRegister {
    u32 IP : 1;
    u32 IE : 1;
    u32 RsvdP : 30;

    static constexpr size_t offset_within_interrupter = 0x0;
};

struct PACKED InterrupterModerationRegister {
    u16 IMODI;
    u16 IMODC;

    static constexpr size_t offset_within_interrupter = 0x4;
};

struct PACKED EventRingSegmentTableBaseAddressRegister {
    u32 RsvdP : 6;
    u32 EventRingSegmentTableBaseAddressLo : 26;
    u32 EventRingSegmentTableBaseAddressHi;

    static constexpr size_t offset_within_interrupter = 0x10;
};

struct PACKED EventRingDequeuePointerRegister {
    u64 DESI : 3;
    u64 EHB : 1;
    u64 EventRingDequeuePointerLo : 28;
    u64 EventRingDequeuePointerHi : 32;

    static constexpr size_t offset_within_interrupter = 0x18;
};

struct PACKED InterrupterRegisterSet {
    InterrupterManagementRegister IMAN;
    InterrupterManagementRegister IMOD;
    u32 ERSTSZ;
    u32 ERSTBA;
    EventRingSegmentTableBaseAddressRegister ERST;
    EventRingDequeuePointerRegister ERDP;
};

struct PACKED RuntimeRegisters {
    u32 MFINDEX;
    u32 RsvdZ[7];
    InterrupterRegisterSet Interrupter[];
};

struct PACKED EventRingSegmentTableEntry {
    u32 RingSegmentBaseAddressLo;
    u32 RingSegmentBaseAddressHi;
    u32 RingSegmentSize;
    u32 RsvdZ;
};

enum class TRBType : u32 {
    Normal = 1,
    SetupStage = 2,
    DataStage = 3,
    StatusStage = 4,
    Isoch = 5,
    Link = 6,
    EventData = 7,
    NoOp = 8,
    EnableSlotCommand = 9,
    DisableSlotCommand = 10,
    AddressDeviceCommand = 11,
    ConfigureEndpointCommand = 12,
    EvaluateContextCommand = 13,
    ResetEndpointCommand = 14,
    StopEndpointCommand = 15,
    SetTRDequeuePointerCommand = 16,
    ResetDeviceCommand = 17,
    ForceEventCommand = 18,
    NegotiateBandwidthCommand = 19,
    SetLatencyToleranceValueCommand = 20,
    GetPortBandwidthCommand = 21,
    ForceHeaderCommand = 22,
    NoOpCommand = 23,
    GetExtendedPropertyCommand = 24,
    SetExtendedPropertyCommand = 25,
    TransferEvent = 32,
    CommandCompletionEvent = 33,
    PortStatusChangeEvent = 34,
    BandwidthRequestEvent = 35,
    DoorbellEvent = 36,
    HostControllerEvent = 37,
    DeviceNotificationEvent = 38,
    MFINDEXWrapEvent = 39,
};

enum class CC : u32 {
    Invalid = 0,
    Success = 1,
    DataBufferError = 2,
    BabbleDetectedError = 3,
    USBTransactionError = 4,
    TRBError = 5,
    StallError = 6,
    ResourceError = 7,
    BandwidthError = 8,
    NoSlotsAvailableError = 9,
    InvalidStreamTypeError = 10,
    SlotNotEnabledError = 11,
    EndpointNotEnabledError = 12,
    ShortPacket = 13,
    RingUnderrun = 14,
    RingOverrun = 15,
    VFEventRingFullError = 16,
    ParameterError = 17,
    BandwidthOverrunError = 18,
    ContextStateError = 19,
    NoPingResponseError = 20,
    EventRingFullError = 21,
    IncompatibleDeviceError = 22,
    MissedServiceError = 23,
    CommandRingStopped = 24,
    CommandAborted = 25,
    Stopped = 26,
    StoppedLengthInvalid = 27,
    StoppedShortPacket = 28,
    MaxExitLatencyTooLargeError = 29,
    IsochBufferOverrun = 31,
    EventLostError = 32,
    UndefinedError = 33,
    InvalidStreamIDError = 34,
    SecondaryBandwidthError = 35,
    SplitTransactionError = 36,
};

struct GenericTRB {
    u32 RzvdZ;
    u32 RzvdZ1;
    u32 RzvdZ2;
    u32 C : 1;
    u32 RzvdZ3 : 9;
    TRBType Type : 6;
    u32 RzvdZ4 : 16;
};

struct EnableSlotTRB : public GenericTRB {};
struct DisableSlotTRB {
    u32 RzvdZ;
    u32 RzvdZ1;
    u32 RzvdZ2;
    u32 C : 1;
    u32 RzvdZ3 : 9;
    TRBType Type : 6;
    u32 RzvdZ4 : 8;
    u32 SlotID : 8;
};

struct ResetDeviceTRB : public DisableSlotTRB {};

struct AddressDeviceTRB {
    u32 InputContextPointerLo;
    u32 InputContextPointerHi;
    u32 RzvdZ;
    u32 C : 1;
    u32 RzvdZ1 : 8;
    u32 BSR : 1;
    TRBType Type : 6;
    u32 RzvdZ2 : 8;
    u32 SlotID : 8;
};

// "The Evaluate Context Command TRB uses the same format as the
//  Address Device Command TRB ... the BSR field is not used."
struct EvaluateContextTRB : public AddressDeviceTRB {};

struct ConfigureEndpointTRB {
    u32 InputContextPointerLo;
    u32 InputContextPointerHi;
    u32 RzvdZ;
    u32 C : 1;
    u32 RzvdZ1 : 8;
    u32 DC : 1;
    TRBType Type : 6;
    u32 RzvdZ2 : 8;
    u32 SlotID : 8;
};

struct PortStatusChangeEventTRB {
    u32 RzvdZ : 24;
    u32 PortID : 8;
    u32 RzvdZ1;
    u32 RzvdZ2 : 24;
    CC CompletionCode : 8;
    u32 C : 1;
    u32 RzvdZ3 : 9;
    TRBType Type : 6;
    u32 RzvdZ4 : 16;
};

struct LinkTRB {
    u32 RingSegmentPointerLo;
    u32 RingSegmentPointerHi;
    u32 RsvdZ : 22;
    u32 InterrupterTarget : 10;
    u32 C : 1;
    u32 TC : 1;
    u32 RsvdZ1 : 2;
    u32 CH : 1;
    u32 IOC : 1;
    u32 RsvdZ2 : 4;
    TRBType Type : 6;
    u32 RsvdZ3 : 16;
};

struct PACKED DoorbellRegister {
    u16 DBTarget;
    u16 DBStreamID;
};

struct PACKED SlotContext {
    u32 RouteString : 20;
    u32 Speed : 4;
    u32 RsvdZ : 1;
    u32 MTT : 1;
    u32 Hub : 1;
    u32 ContextEntries : 5;

    u16 MaxExitLatency;
    u8 RootHubPortNumber;
    u8 NumberOfPorts;

    u32 ParentHubSlotID : 8;
    u32 ParentPortNumber : 8;
    u32 TTT : 2;
    u32 RzvdZ1 : 4;
    u32 InterrupterTarget : 10;

    u32 USBDeviceAddress : 8;
    u32 RzvdZ2 : 19;
    enum class State {
        Disabled = 0,
        Enabled = 0,
        Default = 1,
        Addressed = 2,
        Configured = 3,
    } SlotState : 5;

    u32 xHCIReserved[4];
    // might have 8 more DWORDs here depending on HCCPARAMS1::CSZ
};

struct PACKED EndpointContext {
    enum class State {
        Disabled = 0,
        Running = 1,
        Halted = 2,
        Stopped = 3,
        Error = 4,
    } EPState : 3;
    u32 RsvdZ : 5;
    u32 Mult : 2;
    u32 MaxPStreams : 5;
    u32 LSA : 1;
    u32 Interval : 8;
    u32 MaxESITPayloadHi : 8;

    u32 RsvdZ1 : 1;
    u32 CErr : 2;
    enum class Type {
        NotValid = 0,
        IsochOut = 1,
        BulkOut = 2,
        InterruptOut = 3,
        ControlBidirectional = 4,
        IsochIn = 5,
        BulkIn = 6,
        InterruptIn = 7
    } EPType : 3;
    u32 RzvdZ2 : 1;
    u32 HID : 1;
    u32 MaxBurstSize : 8;
    u32 MaxPacketSize : 16;

    u32 DCS : 1;
    u32 TRDequeuePointerLo : 31;
    u32 TRDequeuePointerHi;

    u32 AverageTRBLength : 16;
    u32 MaxESITPayloadLo : 16;

    u32 xHCIReserved[3];
    // might have 8 more DWORDs here depending on HCCPARAMS1::CSZ
};

struct PACKED InputControlContext {
    u32 D;
    u32 A;
    u32 RzvdZ;
    u32 RzvdZ1;
    u32 RzvdZ2;
    u32 RzvdZ3;
    u32 RzvdZ4;
    u32 ConfigurationValue : 8;
    u32 InterfaceNumber : 8;
    u32 AlternateSetting : 8;
    u32 RzvdZ5 : 8;
    // might have 8 more DWORDs here depending on HCCPARAMS1::CSZ
};

static_assert(sizeof(USBLEGCTLSTS) == 4, "Incorrect size of USBLEGCTLSTS");
static_assert(sizeof(SupportedProtocolDWORD0) == 4, "Incorrect size of SupportedProtocolDWORD0");
static_assert(sizeof(SupportedProtocolDWORD1) == 4, "Incorrect size of SupportedProtocolDWORD1");
static_assert(sizeof(SupportedProtocolDWORD2) == 4, "Incorrect size of SupportedProtocolDWORD2");

static_assert(sizeof(CapabilityRegisters) == 36, "Incorrect size of capability registers");
static_assert(sizeof(HCSPARAMS1) == 4, "Incorrect size of HCSPARAMS1");
static_assert(sizeof(HCSPARAMS2) == 4, "Incorrect size of HCSPARAMS2");
static_assert(sizeof(HCSPARAMS3) == 4, "Incorrect size of HCSPARAMS3");
static_assert(sizeof(HCCPARAMS1) == 4, "Incorrect size of HCCPARAMS1");
static_assert(sizeof(HCCPARAMS2) == 4, "Incorrect size of HCCPARAMS2");

static_assert(sizeof(OperationalRegisters) == 0x400, "Incorrect size of operational registers");
static_assert(sizeof(USBCMD) == 4, "Incorrect size of USBCMD");
static_assert(sizeof(USBSTS) == 4, "Incorrect size of USBSTS");
static_assert(sizeof(CRCR) == 8, "Incorrect size of CRCR");
static_assert(sizeof(DCBAAP) == 8, "Incorrect size of DCBAAP");
static_assert(sizeof(CONFIG) == 4, "Incorrect size of CONFIG");

static_assert(sizeof(PortRegister) == 16, "Incorrect size of port register");
static_assert(sizeof(PORTSC) == 4, "Incorrect size of PORTSC");

static_assert(sizeof(PORTPMSCUSB2) == 4, "Incorrect size of PORTPMSC for USB2");
static_assert(sizeof(PORTPMSCUSB3) == 4, "Incorrect size of PORTPMSC for USB3");

static_assert(sizeof(PORTLIUSB2) == 4, "Incorrect size of PORTPMSC for USB2");
static_assert(sizeof(PORTLIUSB3) == 4, "Incorrect size of PORTPMSC for USB3");

static_assert(sizeof(PORTHLPMCUSB2) == 4, "Incorrect size of PORTHLPMC for USB2");
static_assert(sizeof(PORTHLPMCUSB3) == 4, "Incorrect size of PORTHLPMC for USB3");

static_assert(sizeof(GenericTRB) == 4 * sizeof(u32), "Incorrect size of GenericTRB");
static_assert(sizeof(LinkTRB) == 4 * sizeof(u32), "Incorrect size of LinkTRB");
static_assert(sizeof(EnableSlotTRB) == 4 * sizeof(u32), "Incorrect size of EnableSlotTRB");
static_assert(sizeof(DisableSlotTRB) == 4 * sizeof(u32), "Incorrect size of DisableSlotTRB");
static_assert(sizeof(ResetDeviceTRB) == 4 * sizeof(u32), "Incorrect size of ResetDeviceTRB");
static_assert(sizeof(AddressDeviceTRB) == 4 * sizeof(u32), "Incorrect size of AddressDeviceTRB");
static_assert(sizeof(EvaluateContextTRB) == 4 * sizeof(u32), "Incorrect size of EvaluateContextTRB");
static_assert(sizeof(ConfigureEndpointTRB) == 4 * sizeof(u32), "Incorrect size of ConfigureEndpointTRB");

static_assert(sizeof(InterrupterManagementRegister) == 4, "Incorrect size of InterrupterManagementRegister");
static_assert(sizeof(InterrupterModerationRegister) == 4, "Incorrect size of InterrupterModerationRegister");
static_assert(sizeof(EventRingSegmentTableBaseAddressRegister) == 8, "Incorrect size of EventRingSegmentTableBaseAddressRegister");
static_assert(sizeof(EventRingDequeuePointerRegister) == 8, "Incorrect size of EventRingDequeuePointerRegister");
static_assert(sizeof(InterrupterRegisterSet) == 0x20, "Incorrect size of InterrupterRegisterSet");
static_assert(sizeof(RuntimeRegisters) == 0x20, "Incorrect size of RuntimeRegisters");

static_assert(sizeof(SlotContext) == 0x20, "Incorrect size of SlotContext");
static_assert(sizeof(EndpointContext) == 0x20, "Incorrect size of EndpointContext");
static_assert(sizeof(InputControlContext) == 0x20, "Incorrect size of InputControlContext");

}
