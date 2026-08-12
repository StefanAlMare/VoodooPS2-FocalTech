    // Do not reset after the FLT0102 legacy fallback, because that would undo
    // the controller/device state that made its signature accessible.
    if (!_legacyCompatibilityPath && !resetDevice()) {
        return false;
    }

    if (!readDimensions() || !switchToNativeProtocol()) {
        return false;
    }

    if (publishInput && !_inputPublished) {
        publishInputProperties();
    }

    return true;
}

void ApplePS2FocalTech::publishInputProperties() {
    setProperty(VOODOO_INPUT_LOGICAL_MAX_X_KEY, _xMax, 32);
    setProperty(VOODOO_INPUT_LOGICAL_MAX_Y_KEY, _yMax, 32);
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_X_KEY, (_xMax + 1) * 100 / FOC_UNITS_PER_MM, 32);
    setProperty(VOODOO_INPUT_PHYSICAL_MAX_Y_KEY, (_yMax + 1) * 100 / FOC_UNITS_PER_MM, 32);
    setProperty(VOODOO_INPUT_TRANSFORM_KEY, 0ull, 32);
    setProperty("VoodooInputSupported", kOSBooleanTrue);

    char info[96];
    snprintf(info, sizeof(info), "FocalTech PS/2, %u x %u", _xMax, _yMax);
    setProperty("RM,TrackpadInfo", info);

    _inputPublished = true;
    registerService();
}

void ApplePS2FocalTech::setTouchPadEnable(bool enable) {
    if (!_device) {
        return;
    }

    TPS2Request<1> request;
    request.commands[0].command = kPS2C_SendCommandAndCompareAck;
    request.commands[0].inOrOut = enable ? kDP_Enable : kDP_SetDefaultsAndDisable;
    request.commandsCount = 1;
    _device->submitRequestAndBlock(&request);
}

void ApplePS2FocalTech::setDevicePowerState(UInt32 whatToDo) {
    if (!_device) {
        return;
    }

    if (whatToDo == kPS2C_DisableDevice) {
        if (_forceTimer) {
            _forceTimer->cancelTimeout();
        }
        _forceEligible = false;
        _forceActive = false;
        setTouchPadEnable(false);
        return;
    }

    if (whatToDo != kPS2C_EnableDevice) {
        return;
    }

    IOSleep(kWakeDelayMS);

    _auxNubCount = countAuxNubs();
    _legacyCompatibilityPath = false;
    bool detected = detectFocalTech();
    if (!detected && mayUseLegacyFallback() && prepareLegacyControllerState()) {
        _legacyCompatibilityPath = true;
        detected = detectFocalTech();
    }
    if (!detected) {
        return;
    }

    _device->lock();
    const bool configured = setupHardware(false);
    if (configured) {
        setTouchPadEnable(true);
    }
    _device->unlock();
}

PS2InterruptResult ApplePS2FocalTech::interruptOccurred(UInt8 data) {
    ++_irqByteCount;
    UInt8 *packet = _ringBuffer.head();
    packet[_packetByteCount++] = data;

    if (_packetByteCount == FOC_PACKET_LENGTH) {
        _ringBuffer.advanceHead(FOC_PACKET_LENGTH);
        _packetByteCount = 0;
        return kPS2IR_packetReady;
    }

    return kPS2IR_packetBuffering;
}

void ApplePS2FocalTech::packetReady() {
    while (_ringBuffer.count() >= FOC_PACKET_LENGTH) {
        const UInt8 *packet = _ringBuffer.tail();
        ++_packetCount;

        const bool valid = processPacket();
        if (valid) {
            ++_validPacketCount;
        } else {
            ++_invalidPacketCount;
        }

        // Publish from packet/workloop context, never from the IRQ callback.
        // Updating every 32 packets avoids turning diagnostics into an IRQ load.
        if (_packetCount <= 4 || (_packetCount & 31U) == 0) {
            publishRuntimeDiagnostics(packet, valid);
        }

        _ringBuffer.advanceTail(FOC_PACKET_LENGTH);
    }
}

bool ApplePS2FocalTech::processPacket() {
    const UInt8 *packet = _ringBuffer.tail();

    switch (packet[0] & 0x0f) {
        case FOC_TOUCH:
            processTouchPacket(packet);
            break;
        case FOC_ABS:
            processAbsolutePacket(packet);
            break;
        case FOC_REL:
            processRelativePacket(packet);
            break;
        default:
            return false;
    }

    reportState();
    return true;
}

void ApplePS2FocalTech::processTouchPacket(const UInt8 *packet) {
    UInt8 fingers = packet[1];
    _hardware.pressed = ((packet[0] >> 4) & 1) != 0;

    for (int i = 0; i < FOC_MAX_FINGERS; ++i) {
        const bool active = (fingers & (1U << i)) != 0;
        _hardware.fingers[i].active = active;
        if (!active) {
            _hardware.fingers[i].valid = false;
        }
    }
}

void ApplePS2FocalTech::processAbsolutePacket(const UInt8 *packet) {
    const int id = static_cast<int>(packet[1] >> 4) - 1;
    if (id < 0 || id >= FOC_MAX_FINGERS) {
        return;
    }
