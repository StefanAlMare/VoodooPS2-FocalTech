/*
 * VoodooPS2 FocalTech FTE0001 support
 * Copyright (c) 2026 StefanAlMare
 *
 * Distributed under the Apple Public Source License 2.0.
 * See LICENSE.md in the repository root.
 *
 * This is an independent implementation of the FTE0001 PS/2 protocol path.
 * The historical chilledHamza/VoodooPS2FocalTech project is credited as an
 * important public reference for FTE0001 protocol behaviour. No GPL source is
 * incorporated into this file.
 */

#ifndef _APPLEPS2FTE0001_H
#define _APPLEPS2FTE0001_H

#include "../VoodooPS2Controller/ApplePS2MouseDevice.h"
#include "VoodooInputMultitouch/VoodooInputEvent.h"

#define FTE_PACKET_SHORT_LENGTH 8
#define FTE_PACKET_LONG_LENGTH 16
#define FTE_PACKET_MAX_LENGTH 16
#define FTE_MAX_FINGERS 4

#define FTE_LOGICAL_MAX_X 0x08E0U
#define FTE_LOGICAL_MAX_Y 0x03E0U
#define FTE_PHYSICAL_MAX_X 8500U
#define FTE_PHYSICAL_MAX_Y 3710U

struct FTE0001FingerState {
    bool active {false};
    SInt32 x {0};
    SInt32 y {0};
};

struct FTE0001VirtualFinger {
    TouchCoordinates previous {};
    TouchCoordinates current {};
    bool touching {false};
};

class EXPORT ApplePS2FTE0001 : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(ApplePS2FTE0001);

private:
    ApplePS2MouseDevice *_device {nullptr};
    IOService *_voodooInput {nullptr};

    bool _interruptInstalled {false};
    bool _powerInstalled {false};
    bool _inputPublished {false};

    UInt32 _providerPort {0};
    UInt32 _auxNubCount {0};

    UInt64 _irqByteCount {0};
    UInt64 _packetCount {0};
    UInt64 _validPacketCount {0};
    UInt64 _invalidPacketCount {0};

    UInt32 _packetByteCount {0};
    UInt32 _packetFingerCount {0};
    RingBuffer<UInt8, FTE_PACKET_MAX_LENGTH * 32> _ringBuffer {};

    UInt8 _productId[3] {0, 0, 0};
    UInt32 _xMax {FTE_LOGICAL_MAX_X};
    UInt32 _yMax {FTE_LOGICAL_MAX_Y};

    FTE0001FingerState _hardware[FTE_MAX_FINGERS] {};
    FTE0001VirtualFinger _finger[FTE_MAX_FINGERS] {};

    VoodooInputEvent _inputEvent {};
    TrackpointReport _buttonReport {};
    UInt32 _buttons {0};
    UInt32 _lastButtons {0};

    UInt32 countAuxNubs() const;
    bool detectFTE0001();
    bool resetDevice();
    bool switchToAdvancedProtocol();
    bool setupHardware(bool publishInput);
    void publishInputProperties();
    void publishRuntimeDiagnostics(const UInt8 *packet, bool valid);

    bool processPacket(const UInt8 *packet);
    void reportState();
    void sendTouchData();
    void sendButtons(UInt32 buttons);

    void setTouchPadEnable(bool enable);
    void setDevicePowerState(UInt32 whatToDo);

    PS2InterruptResult interruptOccurred(UInt8 data);
    void packetReady();

    static MT2FingerType fingerTypeForIndex(int index);

protected:
    bool handleOpen(IOService *forClient, IOOptionBits options, void *arg) override;
    void handleClose(IOService *forClient, IOOptionBits options) override;
    bool handleIsOpen(const IOService *forClient) const override;

public:
    bool init(OSDictionary *properties) override;
    ApplePS2FTE0001 *probe(IOService *provider, SInt32 *score) override;
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
};

#endif
