
        if (_powerInstalled) {
            _device->uninstallPowerControlAction();
            _powerInstalled = false;
        }
    }

    OSSafeReleaseNULL(_voodooInput);
    OSSafeReleaseNULL(_device);
    super::stop(provider);
}

UInt32 ApplePS2FocalTech::countAuxNubs() const {
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

    while (IORegistryEntry *entry =
               OSDynamicCast(IORegistryEntry, iterator->getNextObject())) {
        if (OSDynamicCast(ApplePS2MouseDevice, entry)) {
            ++count;
        }
    }

    iterator->release();
    return count;
}

bool ApplePS2FocalTech::mayUseLegacyFallback() const {
    // Legacy compatibility manipulates global i8042 state and the keyboard
    // port.  It is safe only on the simple two-port controller layout where
    // there is exactly one AUX nub.  On muxed controllers, probe only the nub
    // that answers the FocalTech signature directly.
    return _auxNubCount <= 1;
}

void ApplePS2FocalTech::publishRuntimeDiagnostics(const UInt8 *packet, bool valid) {
    setProperty("FocalTechPort", _providerPort, 32);
    setProperty("FocalTechAuxNubs", _auxNubCount, 32);
    setProperty("FocalTechIRQBytes", _irqByteCount, 64);
    setProperty("FocalTechPackets", _packetCount, 64);
    setProperty("FocalTechValidPackets", _validPacketCount, 64);
    setProperty("FocalTechInvalidPackets", _invalidPacketCount, 64);
    setProperty("FocalTechLegacyFallback",
                _legacyCompatibilityPath ? kOSBooleanTrue : kOSBooleanFalse);

    if (packet) {
        setProperty("FocalTechLastType", packet[0] & 0x0f, 8);
        setProperty("FocalTechLast0", packet[0], 8);
        setProperty("FocalTechLast1", packet[1], 8);
        setProperty("FocalTechLast2", packet[2], 8);
        setProperty("FocalTechLast3", packet[3], 8);
        setProperty("FocalTechLast4", packet[4], 8);
        setProperty("FocalTechLast5", packet[5], 8);
        setProperty("FocalTechLastPacketValid",
                    valid ? kOSBooleanTrue : kOSBooleanFalse);
    }
}

bool ApplePS2FocalTech::prepareLegacyControllerState() {
    if (!_device) {
        return false;
    }

    ApplePS2Controller *controller = _device->getController();
    if (!controller) {
        return false;
    }

    _device->lock();

    // Match the legacy startup state that proved necessary on FLT0102:
    // mouse IRQ off, mouse clock enabled, F5 keyboard, F5 AUX, flush,
    // keyboard reset.  All operations use public VoodooPS2 APIs.
    controller->setCommandByte(0, kCB_EnableMouseIRQ | kCB_DisableMouseClock);

    TPS2Request<1> keyboardDisable;
    keyboardDisable.port = kPS2KbdIdx;
    keyboardDisable.commands[0].command = kPS2C_SendCommandAndCompareAck;
    keyboardDisable.commands[0].inOrOut = kDP_SetDefaultsAndDisable;
    keyboardDisable.commandsCount = 1;
    controller->submitRequestAndBlock(&keyboardDisable);

    TPS2Request<1> auxDisable;
    auxDisable.commands[0].command = kPS2C_SendCommandAndCompareAck;
    auxDisable.commands[0].inOrOut = kDP_SetDefaultsAndDisable;
    auxDisable.commandsCount = 1;
    _device->submitRequestAndBlock(&auxDisable);

    TPS2Request<1> flushBeforeReset;
    flushBeforeReset.commands[0].command = kPS2C_FlushDataPort;
    flushBeforeReset.commandsCount = 1;
    _device->submitRequestAndBlock(&flushBeforeReset);

    TPS2Request<1> keyboardReset;
    keyboardReset.port = kPS2KbdIdx;
    keyboardReset.commands[0].command = kPS2C_SendCommandAndCompareAck;
    keyboardReset.commands[0].inOrOut = kDP_Reset;
    keyboardReset.commandsCount = 1;
    controller->submitRequestAndBlock(&keyboardReset);

    IOSleep(50);

    TPS2Request<1> flushAfterReset;
    flushAfterReset.commands[0].command = kPS2C_FlushDataPort;
    flushAfterReset.commandsCount = 1;
    _device->submitRequestAndBlock(&flushAfterReset);

    // A late keyboard reset occurs after the keyboard driver may already have
    // started, so restore scanning before leaving the compatibility sequence.
    TPS2Request<1> keyboardEnable;
    keyboardEnable.port = kPS2KbdIdx;
    keyboardEnable.commands[0].command = kPS2C_SendCommandAndCompareAck;
    keyboardEnable.commands[0].inOrOut = kDP_Enable;
    keyboardEnable.commandsCount = 1;
    controller->submitRequestAndBlock(&keyboardEnable);

    controller->setCommandByte(0, kCB_EnableMouseIRQ | kCB_DisableMouseClock);

    const bool ok =
        keyboardDisable.commandsCount == 1 &&
        auxDisable.commandsCount == 1 &&
        keyboardReset.commandsCount == 1 &&
        keyboardEnable.commandsCount == 1;

    _device->unlock();
    return ok;
}

bool ApplePS2FocalTech::detectFocalTech() {
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

    // Legacy FocalTech signature: E6, E6, E6, E9 + 3-byte reply.
    TPS2Request<7> request;
    request.commands[0].command = kPS2C_SendCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_SetMouseScaling1To1;
    request.commands[1].command = kPS2C_SendCommandAndCompareAck;
    request.commands[1].inOrOut = kDP_SetMouseScaling1To1;
