/*
 * VoodooPS2 FocalTech FTE0001 support
 * Copyright (c) 2026 StefanAlMare
 *
 * Distributed under the Apple Public Source License 2.0.
 * See LICENSE.md in the repository root.
 *
 * Independent implementation based on publicly documented/observable FTE0001
 * protocol behaviour. chilledHamza/VoodooPS2FocalTech is credited as an
 * important historical reference. No GPL source is incorporated here.
 */

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOWorkLoop.h>
#include <pexpert/pexpert.h>

#include "VoodooPS2Controller.h"
#include "../FTE0001.h"
#include "VoodooInputMultitouch/VoodooInputMessages.h"

OSDefineMetaClassAndStructors(ApplePS2FTE0001, IOService);

bool ApplePS2FTE0001::init(OSDictionary *properties) {
    return super::init(properties);
}

bool ApplePS2FTE0001::handleOpen(IOService *forClient, IOOptionBits options, void *arg) {
    if (forClient && forClient->getProperty(VOODOO_INPUT_IDENTIFIER)) {
        _voodooInput = forClient;
        _voodooInput->retain();
        return true;
    }
    return false;
}

void ApplePS2FTE0001::handleClose(IOService *forClient, IOOptionBits options) {
    if (forClient == _voodooInput) {
        OSSafeReleaseNULL(_voodooInput);
    }
}

bool ApplePS2FTE0001::handleIsOpen(const IOService *forClient) const {
    return forClient ? (_voodooInput == forClient) : (_voodooInput != nullptr);
}

UInt32 ApplePS2FTE0001::countAuxNubs() const {
    if (!_device) {
        return 0;
    }

    ApplePS2Controller *controller = _device->getController();
    if (!controller) {
        return 0;
    }

    UInt32 count = 0;
    OSIterator *iterator = controller->getChildIterator(gIOServicePlane);
    if (!iterator) {
        return 0;
    }

    while (IORegistryEntry *entry = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) {
        if (OSDynamicCast(ApplePS2MouseDevice, entry)) {
            ++count;
        }
    }

    iterator->release();
    return count;
}

ApplePS2FTE0001 *ApplePS2FTE0001::probe(IOService *provider, SInt32 *score) {
    if (!super::probe(provider, score)) {
        return nullptr;
    }

    int fteOptIn = 0;
    PE_parse_boot_argn("focfte", &fteOptIn, sizeof(fteOptIn));
    if (!fteOptIn) {
        return nullptr;
    }

    _device = OSDynamicCast(ApplePS2MouseDevice, provider);
    if (!_device) {
        return nullptr;
    }

    if (OSNumber *port = OSDynamicCast(OSNumber, _device->getProperty(kPortKey))) {
        _providerPort = port->unsigned32BitValue();
    }
    _auxNubCount = countAuxNubs();

    // The historical FTE0001 device was observed on a simple AUX topology.
    // Refuse muxed controllers for the first experimental implementation so
    // probing never sends FTE-specific commands across several AUX nubs.
    if (_auxNubCount > 1) {
        _device = nullptr;
        return nullptr;
    }

    _device->lock();
    const bool detected = detectFTE0001();
    _device->unlock();

    if (detected) {
        setProperty("FocalTechProtocol", "FTE0001");
        setProperty("FTE0001Experimental", kOSBooleanTrue);
        setProperty("FTE0001OptIn", kOSBooleanTrue);
        publishRuntimeDiagnostics(nullptr, false);
    }

    _device = nullptr;
    return detected ? this : nullptr;
}

bool ApplePS2FTE0001::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }

    _device = OSDynamicCast(ApplePS2MouseDevice, provider);
    if (!_device) {
        return false;
    }
    _device->retain();

    if (OSNumber *port = OSDynamicCast(OSNumber, _device->getProperty(kPortKey))) {
        _providerPort = port->unsigned32BitValue();
    }
    _auxNubCount = countAuxNubs();

    _device->lock();
    if (!setupHardware(true)) {
        _device->unlock();
        OSSafeReleaseNULL(_device);
        return false;
    }

    _device->installInterruptAction(
        this,
        OSMemberFunctionCast(PS2InterruptAction, this, &ApplePS2FTE0001::interruptOccurred),
        OSMemberFunctionCast(PS2PacketAction, this, &ApplePS2FTE0001::packetReady));
    _interruptInstalled = true;

    setTouchPadEnable(true);
    _device->unlock();

    _device->installPowerControlAction(
        this,
        OSMemberFunctionCast(PS2PowerControlAction, this, &ApplePS2FTE0001::setDevicePowerState));
    _powerInstalled = true;

    return true;
}

void ApplePS2FTE0001::stop(IOService *provider) {
    if (_device) {
        setTouchPadEnable(false);

        if (_interruptInstalled) {
            _device->uninstallInterruptAction();
            _interruptInstalled = false;
        }

        if (_powerInstalled) {
            _device->uninstallPowerControlAction();
            _powerInstalled = false;
        }
    }

    OSSafeReleaseNULL(_voodooInput);
    OSSafeReleaseNULL(_device);
    super::stop(provider);
}

bool ApplePS2FTE0001::detectFTE0001() {
    if (!_device) {
        return false;
    }

    TPS2Request<1> disable;
    disable.commands[0].command = kPS2C_SendCommandAndCompareAck;
    disable.commands[0].inOrOut = kDP_SetDefaultsAndDisable;
    disable.commandsCount = 1;
    _device->submitRequestAndBlock(&disable);
    if (disable.commandsCount != 1) {
        return false;
    }

    // FTE0001 identification sequence: set sample-rate selector 0xA7 twice,
    // then request the three-byte status/product response.
    TPS2Request<8> request;
    request.commands[0].command = kPS2C_SendCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_SetMouseSampleRate;
    request.commands[1].command = kPS2C_SendCommandAndCompareAck;
    request.commands[1].inOrOut = 0xA7;
    request.commands[2].command = kPS2C_SendCommandAndCompareAck;
    request.commands[2].inOrOut = kDP_SetMouseSampleRate;
    request.commands[3].command = kPS2C_SendCommandAndCompareAck;
    request.commands[3].inOrOut = 0xA7;
    request.commands[4].command = kPS2C_SendCommandAndCompareAck;
    request.commands[4].inOrOut = kDP_GetMouseInformation;
    request.commands[5].command = kPS2C_ReadDataPort;
    request.commands[6].command = kPS2C_ReadDataPort;
    request.commands[7].command = kPS2C_ReadDataPort;
    request.commandsCount = 8;

    _device->submitRequestAndBlock(&request);
    if (request.commandsCount != 8) {
        return false;
    }

    _productId[0] = request.commands[5].inOrOut;
    _productId[1] = request.commands[6].inOrOut;
    _productId[2] = request.commands[7].inOrOut;

    setProperty("FTE0001Product0", _productId[0], 8);
    setProperty("FTE0001Product1", _productId[1], 8);
    setProperty("FTE0001Product2", _productId[2], 8);

    return _productId[0] == 0x58 && _productId[1] == 0x00 && _productId[2] == 0x05;
}

bool ApplePS2FTE0001::resetDevice() {
    if (!_device) {
        return false;
    }

    // FTE0001 hardware has historically benefited from two complete PS/2
    // reset cycles before entering advanced reporting mode.
    for (int pass = 0; pass < 2; ++pass) {
        TPS2Request<3> request;
        request.commands[0].command = kPS2C_SendCommandAndCompareAck;
        request.commands[0].inOrOut = kDP_Reset;
        request.commands[1].command = kPS2C_ReadDataPort;
        request.commands[2].command = kPS2C_ReadDataPort;
        request.commandsCount = 3;
        _device->submitRequestAndBlock(&request);

        if (request.commandsCount != 3 ||
            request.commands[1].inOrOut != kSC_Reset ||
            request.commands[2].inOrOut != kSC_ID) {
            return false;
        }
    }

    return true;
}

bool ApplePS2FTE0001::switchToAdvancedProtocol() {
    if (!_device) {
        return false;
    }

    // Select FTE advanced reporting mode using two sample-rate encoded
    // selectors: 0xEA followed by 0xED.
    TPS2Request<4> request;
    request.commands[0].command = kPS2C_SendCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_SetMouseSampleRate;
    request.commands[1].command = kPS2C_SendCommandAndCompareAck;
    request.commands[1].inOrOut = 0xEA;
    request.commands[2].command = kPS2C_SendCommandAndCompareAck;
    request.commands[2].inOrOut = kDP_SetMouseSampleRate;
    request.commands[3].command = kPS2C_SendCommandAndCompareAck;
    request.commands[3].inOrOut = 0xED;
    request.commandsCount = 4;

    _device->submitRequestAndBlock(&request);
    return request.commandsCount == 4;
}

bool ApplePS2FTE0001::setupHardware(bool publishInput) {
    memset(_hardware, 0, sizeof(_hardware));
    memset(_finger, 0, sizeof(_finger));
    _packetByteCount = 0;
    _packetFingerCount = 0;
    _ringBuffer.reset();
    _buttons = 0;
    _lastButtons = 0;

    if (!resetDevice()) {
        return false;
    }

    if (!detectFTE0001()) {
        return false;
    }

    if (!switchToAdvancedProtocol()) {
        return false;
    }

    _xMax = FTE_LOGICAL_MAX_X;
    _yMax = FTE_LOGICAL_MAX_Y;

    if (publishInput && !_inputPublished) {
        publishInputProperties();
    }

    publishRuntimeDiagnostics(nullptr, false);
    return true;
}

void ApplePS2FTE0001::publishInputProperties() {
    setProperty(VOODOO_INPUT_LOGICAL_MAX_X_KEY, _xMax, 32);
    setProperty(VOODOO_INPUT_LOGICAL_MAX_Y_KEY, _yMax, 32);
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_X_KEY, FTE_PHYSICAL_MAX_X, 32);
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_Y_KEY, FTE_PHYSICAL_MAX_Y, 32);
    setProperty(VOODOO_INPUT_TRANSFORM_KEY, 0ull, 32);
    setProperty("VoodooInputSupported", kOSBooleanTrue);
    setProperty("FocalTechProtocol", "FTE0001");
    setProperty("FTE0001Experimental", kOSBooleanTrue);

    char info[96];
    snprintf(info, sizeof(info), "FocalTech FTE0001 PS/2, %u x %u (experimental)", _xMax, _yMax);
    setProperty("RM,TrackpadInfo", info);

    _inputPublished = true;
    registerService();
}

void ApplePS2FTE0001::publishRuntimeDiagnostics(const UInt8 *packet, bool valid) {
    setProperty("FocalTechProtocol", "FTE0001");
    setProperty("FTE0001Experimental", kOSBooleanTrue);
    setProperty("FTE0001OptIn", kOSBooleanTrue);
    setProperty("FocalTechPort", _providerPort, 32);
    setProperty("FocalTechAuxNubs", _auxNubCount, 32);
    setProperty("FocalTechIRQBytes", _irqByteCount, 64);
    setProperty("FocalTechPackets", _packetCount, 64);
    setProperty("FocalTechValidPackets", _validPacketCount, 64);
    setProperty("FocalTechInvalidPackets", _invalidPacketCount, 64);
    setProperty("FTE0001FingerCount", _packetFingerCount, 32);

    if (packet) {
        setProperty("FocalTechLastPacketValid", valid ? kOSBooleanTrue : kOSBooleanFalse);
        setProperty("FocalTechLastPacketLength", FTE_PACKET_MAX_LENGTH, 32);
        for (int i = 0; i < FTE_PACKET_MAX_LENGTH; ++i) {
            char key[32];
            snprintf(key, sizeof(key), "FocalTechLast%d", i);
            setProperty(key, packet[i], 8);
        }
    }
}

void ApplePS2FTE0001::setTouchPadEnable(bool enable) {
    if (!_device) {
        return;
    }

    TPS2Request<1> request;
    request.commands[0].command = kPS2C_SendCommandAndCompareAck;
    request.commands[0].inOrOut = enable ? kDP_Enable : kDP_SetDefaultsAndDisable;
    request.commandsCount = 1;
    _device->submitRequestAndBlock(&request);
}

void ApplePS2FTE0001::setDevicePowerState(UInt32 whatToDo) {
    if (!_device) {
        return;
    }

    if (whatToDo == kPS2C_DisableDevice) {
        setTouchPadEnable(false);
        return;
    }

    if (whatToDo != kPS2C_EnableDevice) {
        return;
    }

    IOSleep(750);
    _device->lock();
    const bool configured = setupHardware(false);
    if (configured) {
        setTouchPadEnable(true);
    }
    _device->unlock();
}

PS2InterruptResult ApplePS2FTE0001::interruptOccurred(UInt8 data) {
    ++_irqByteCount;

    if (_packetByteCount == 0 &&
        (data & 0xC8) != 0x08 &&
        (data & 0xF8) != 0xF8) {
        return kPS2IR_packetBuffering;
    }

    UInt8 *packet = _ringBuffer.head();
    packet[_packetByteCount++] = data;

    if (_packetByteCount == 5) {
        _packetFingerCount = static_cast<UInt32>(data & 0x03U) +
                            static_cast<UInt32>((data & 0x30U) >> 2);
    }

    if (_packetByteCount == FTE_PACKET_SHORT_LENGTH && _packetFingerCount <= 2) {
        for (int i = FTE_PACKET_SHORT_LENGTH; i < FTE_PACKET_MAX_LENGTH; ++i) {
            packet[i] = 0xFF;
        }
        _ringBuffer.advanceHead(FTE_PACKET_MAX_LENGTH);
        _packetByteCount = 0;
        return kPS2IR_packetReady;
    }

    if (_packetByteCount == FTE_PACKET_LONG_LENGTH) {
        _ringBuffer.advanceHead(FTE_PACKET_MAX_LENGTH);
        _packetByteCount = 0;
        return kPS2IR_packetReady;
    }

    return kPS2IR_packetBuffering;
}

void ApplePS2FTE0001::packetReady() {
    while (_ringBuffer.count() >= FTE_PACKET_MAX_LENGTH) {
        const UInt8 *packet = _ringBuffer.tail();
        ++_packetCount;

        const bool valid = processPacket(packet);
        if (valid) {
            ++_validPacketCount;
        } else {
            ++_invalidPacketCount;
        }

        if (_packetCount <= 4 || (_packetCount & 31U) == 0) {
            publishRuntimeDiagnostics(packet, valid);
        }

        _ringBuffer.advanceTail(FTE_PACKET_MAX_LENGTH);
    }
}

bool ApplePS2FTE0001::processPacket(const UInt8 *packet) {
    if (!packet) {
        return false;
    }

    if ((packet[0] & 0xC8) != 0x08 && (packet[0] & 0xF8) != 0xF8) {
        return false;
    }

    _buttons = 0;
    if (packet[0] & 0x01U) _buttons |= 0x01U;
    if (packet[0] & 0x02U) _buttons |= 0x02U;

    // 0x10 in the two mode bits is a non-coordinate report. Preserve button
    // state without inventing finger coordinates.
    if ((packet[0] & 0x30U) == 0x10U) {
        reportState();
        return true;
    }

    for (int i = 0; i < FTE_MAX_FINGERS; ++i) {
        const int j = i * 4;
        const UInt8 b1 = packet[j + 1];
        const UInt8 b2 = packet[j + 2];
        const UInt8 b3 = packet[j + 3];

        const bool present = !(b1 == 0xFF && b2 == 0xFF && b3 == 0xFF);
        _hardware[i].active = present;
        if (!present) {
            continue;
        }

        _hardware[i].x = static_cast<SInt32>((static_cast<UInt32>(b1) << 4) |
                                              ((b3 & 0xF0U) >> 4));
        _hardware[i].y = static_cast<SInt32>((static_cast<UInt32>(b2) << 4) |
                                              (b3 & 0x0FU));
    }

    reportState();
    return true;
}

void ApplePS2FTE0001::reportState() {
    for (int i = 0; i < FTE_MAX_FINGERS; ++i) {
        FTE0001VirtualFinger &finger = _finger[i];
        const FTE0001FingerState &raw = _hardware[i];

        if (!raw.active) {
            finger.touching = false;
            continue;
        }

        SInt32 x = raw.x;
        SInt32 y = raw.y;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > static_cast<SInt32>(_xMax)) x = _xMax;
        if (y > static_cast<SInt32>(_yMax)) y = _yMax;

        const bool wasTouching = finger.touching;
        finger.previous = finger.current;
        finger.current.x = static_cast<UInt32>(x);
        finger.current.y = _yMax - static_cast<UInt32>(y);
        finger.current.pressure = 0;
        finger.current.width = 0;
        finger.touching = true;

        if (!wasTouching) {
            finger.previous = finger.current;
        }
    }

    sendTouchData();
}

MT2FingerType ApplePS2FTE0001::fingerTypeForIndex(int index) {
    switch (index) {
        case 0: return kMT2FingerTypeIndexFinger;
        case 1: return kMT2FingerTypeMiddleFinger;
        case 2: return kMT2FingerTypeRingFinger;
        case 3: return kMT2FingerTypeThumb;
        default: return kMT2FingerTypeIndexFinger;
    }
}

void ApplePS2FTE0001::sendButtons(UInt32 buttons) {
    if (!_voodooInput) {
        return;
    }

    AbsoluteTime timestamp;
    clock_get_uptime(&timestamp);
    _buttonReport.timestamp = timestamp;
    _buttonReport.buttons = buttons;
    _buttonReport.dx = 0;
    _buttonReport.dy = 0;

    super::messageClient(
        kIOMessageVoodooTrackpointMessage,
        _voodooInput,
        &_buttonReport,
        sizeof(_buttonReport));
}

void ApplePS2FTE0001::sendTouchData() {
    AbsoluteTime timestamp;
    clock_get_uptime(&timestamp);

    int count = 0;
    for (int i = 0; i < FTE_MAX_FINGERS; ++i) {
        const FTE0001VirtualFinger &state = _finger[i];
        if (!state.touching) {
            continue;
        }

        VoodooInputTransducer &transducer = _inputEvent.transducers[count];
        transducer.currentCoordinates = state.current;
        transducer.previousCoordinates = state.previous;
        transducer.timestamp = timestamp;
        transducer.isValid = true;
        transducer.isPhysicalButtonDown = false;
        transducer.isTransducerActive = true;
        transducer.supportsPressure = false;
        transducer.secondaryId = i;
        transducer.fingerType = fingerTypeForIndex(count);
        transducer.type = FINGER;
        ++count;
    }

    for (int i = count; i < VOODOO_INPUT_MAX_TRANSDUCERS; ++i) {
        _inputEvent.transducers[i].isValid = false;
        _inputEvent.transducers[i].isPhysicalButtonDown = false;
        _inputEvent.transducers[i].isTransducerActive = false;
        _inputEvent.transducers[i].supportsPressure = false;
    }

    _inputEvent.contact_count = static_cast<UInt8>(count);
    _inputEvent.timestamp = timestamp;

    if (_voodooInput) {
        super::messageClient(
            kIOMessageVoodooInputMessage,
            _voodooInput,
            &_inputEvent,
            sizeof(_inputEvent));
    }

    if (_buttons != _lastButtons) {
        sendButtons(_buttons);
        _lastButtons = _buttons;
    }
}
