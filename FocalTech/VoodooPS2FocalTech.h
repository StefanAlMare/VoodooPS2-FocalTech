/*
 * VoodooPS2 FocalTech support
 * Copyright (c) 2026 StefanAlMare
 *
 * Distributed under the Apple Public Source License 2.0.
 * See LICENSE.md in the repository root.
 *
 * FocalTech PS/2 protocol work is informed by the Linux kernel FocalTech
 * driver and by EMlyDinEsH's ApplePS2SmartTouchPad, which provided an
 * essential historical reference for FocalTech hardware compatibility.
 */

#ifndef _APPLEPS2FOCALTECH_H
#define _APPLEPS2FOCALTECH_H

#include "../VoodooPS2Controller/ApplePS2MouseDevice.h"
#include <IOKit/IOTimerEventSource.h>
#include "VoodooInputMultitouch/VoodooInputEvent.h"

#define FOC_PACKET_LENGTH        6
#define FOC_MAX_FINGERS          5
#define FOC_TOUCH                0x3
#define FOC_ABS                  0x6
#define FOC_REL                  0x9
#define FOC_PS2_CUSTOM_COMMAND   0xF8
#define FOC_UNITS_PER_MM         40

struct FocalTechFingerState {
    bool active {false};
    bool valid {false};
    SInt32 x {0};
    SInt32 y {0};
};

struct FocalTechHardwareState {
    FocalTechFingerState fingers[FOC_MAX_FINGERS] {};
    UInt32 width {0};
    bool pressed {false};
};

struct FocalTechVirtualFinger {
    TouchCoordinates previous {};
    TouchCoordinates current {};
    bool touching {false};
};

class EXPORT ApplePS2FocalTech : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(ApplePS2FocalTech);

private:
    ApplePS2MouseDevice *_device {nullptr};
    IOService *_voodooInput {nullptr};
    IOTimerEventSource *_forceTimer {nullptr};

    bool _interruptInstalled {false};
    bool _powerInstalled {false};
    bool _inputPublished {false};
    bool _legacyCompatibilityPath {false};
    UInt32 _providerPort {0};
    UInt32 _auxNubCount {0};

    UInt64 _irqByteCount {0};
    UInt64 _packetCount {0};
    UInt64 _validPacketCount {0};
    UInt64 _invalidPacketCount {0};

    UInt32 _packetByteCount {0};
    RingBuffer<UInt8, FOC_PACKET_LENGTH * 32> _ringBuffer {};

    UInt32 _xMax {0};
    UInt32 _yMax {0};

    FocalTechHardwareState _hardware {};
    FocalTechVirtualFinger _finger[FOC_MAX_FINGERS] {};

    VoodooInputEvent _inputEvent {};
    TrackpointReport _buttonReport {};
    UInt32 _leftButton {0};
    UInt32 _rightButton {0};
    UInt32 _lastButtons {0};

    bool _clickWasPressed {false};
    bool _clickRight {false};

    bool _forceEligible {false};
    bool _forceActive {false};
    int _forceFinger {-1};
    SInt32 _forceStartX {0};
    SInt32 _forceStartY {0};

    enum {
        kForceHoldMS = 2000,
        kForceMoveCancel = 100,
        kWakeDelayMS = 1000
    };

    UInt32 countAuxNubs() const;
    bool mayUseLegacyFallback() const;
    void publishRuntimeDiagnostics(const UInt8 *packet, bool valid);
    bool prepareLegacyControllerState();
    bool resetDevice();
    bool detectFocalTech();
    bool readRegister(UInt8 reg, UInt8 *param);
    bool readDimensions();
    bool switchToNativeProtocol();
    bool setupHardware(bool publishInput);
    void publishInputProperties();

    bool processPacket();
    void processTouchPacket(const UInt8 *packet);
    void processAbsolutePacket(const UInt8 *packet);
    void processRelativePacket(const UInt8 *packet);
    void reportState();
    void sendTouchData();
    void sendButtons(UInt32 buttons);

    void setTouchPadEnable(bool enable);
    void setDevicePowerState(UInt32 whatToDo);
    void forceTimerFired();

    PS2InterruptResult interruptOccurred(UInt8 data);
    void packetReady();

    static MT2FingerType fingerTypeForIndex(int index);

protected:
    bool handleOpen(IOService *forClient, IOOptionBits options, void *arg) override;
    void handleClose(IOService *forClient, IOOptionBits options) override;
    bool handleIsOpen(const IOService *forClient) const override;

public:
    bool init(OSDictionary *properties) override;
    ApplePS2FocalTech *probe(IOService *provider, SInt32 *score) override;
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
};

#endif
