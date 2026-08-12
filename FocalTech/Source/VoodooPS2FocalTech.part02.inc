    request.commands[2].command = kPS2C_SendCommandAndCompareAck;
    request.commands[2].inOrOut = kDP_SetMouseScaling1To1;
    request.commands[3].command = kPS2C_SendCommandAndCompareAck;
    request.commands[3].inOrOut = kDP_GetMouseInformation;
    request.commands[4].command = kPS2C_ReadDataPort;
    request.commands[5].command = kPS2C_ReadDataPort;
    request.commands[6].command = kPS2C_ReadDataPort;
    request.commandsCount = 7;

    _device->submitRequestAndBlock(&request);
    if (request.commandsCount != 7) {
        return false;
    }

    const UInt8 b0 = request.commands[4].inOrOut;
    const UInt8 b1 = request.commands[5].inOrOut;
    const UInt8 b2 = request.commands[6].inOrOut;

    return b0 == 0x3c && b1 == 0xf0 && (b2 == 0xc8 || b2 == 0x00);
}

bool ApplePS2FocalTech::resetDevice() {
    if (!_device) {
        return false;
    }

    // Standard PS/2 reset: FF -> FA, then BAT completion AA and device ID 00.
    // This is the reset sequence used by the original VoodooPS2 Elan path
    // and by the FocalTech implementation that was validated on FLT0101.
    TPS2Request<3> request;
    request.commands[0].command = kPS2C_SendCommandAndCompareAck;
    request.commands[0].inOrOut = kDP_Reset;
    request.commands[1].command = kPS2C_ReadDataPort;
    request.commands[2].command = kPS2C_ReadDataPort;
    request.commandsCount = 3;

    _device->submitRequestAndBlock(&request);
    return request.commandsCount == 3 &&
           request.commands[1].inOrOut == kSC_Reset &&
           request.commands[2].inOrOut == kSC_ID;
}

bool ApplePS2FocalTech::readRegister(UInt8 reg, UInt8 *param) {
    if (!_device || !param) {
        return false;
    }

    // E6, E8 00, E8 00, E8 00, E8 <reg>.
    TPS2Request<9> select;
    select.commands[0].command = kPS2C_SendCommandAndCompareAck;
    select.commands[0].inOrOut = kDP_SetMouseScaling1To1;
    select.commands[1].command = kPS2C_SendCommandAndCompareAck;
    select.commands[1].inOrOut = kDP_SetMouseResolution;
    select.commands[2].command = kPS2C_SendCommandAndCompareAck;
    select.commands[2].inOrOut = 0;
    select.commands[3].command = kPS2C_SendCommandAndCompareAck;
    select.commands[3].inOrOut = kDP_SetMouseResolution;
    select.commands[4].command = kPS2C_SendCommandAndCompareAck;
    select.commands[4].inOrOut = 0;
    select.commands[5].command = kPS2C_SendCommandAndCompareAck;
    select.commands[5].inOrOut = kDP_SetMouseResolution;
    select.commands[6].command = kPS2C_SendCommandAndCompareAck;
    select.commands[6].inOrOut = 0;
    select.commands[7].command = kPS2C_SendCommandAndCompareAck;
    select.commands[7].inOrOut = kDP_SetMouseResolution;
    select.commands[8].command = kPS2C_SendCommandAndCompareAck;
    select.commands[8].inOrOut = reg;
    select.commandsCount = 9;

    _device->submitRequestAndBlock(&select);
    if (select.commandsCount != 9) {
        return false;
    }

    TPS2Request<4> info;
    info.commands[0].command = kPS2C_SendCommandAndCompareAck;
    info.commands[0].inOrOut = kDP_GetMouseInformation;
    info.commands[1].command = kPS2C_ReadDataPort;
    info.commands[2].command = kPS2C_ReadDataPort;
    info.commands[3].command = kPS2C_ReadDataPort;
    info.commandsCount = 4;

    _device->submitRequestAndBlock(&info);
    if (info.commandsCount != 4) {
        return false;
    }

    param[0] = info.commands[1].inOrOut;
    param[1] = info.commands[2].inOrOut;
    param[2] = info.commands[3].inOrOut;
    return true;
}

bool ApplePS2FocalTech::readDimensions() {
    UInt8 param[3] = {0, 0, 0};
    if (!readRegister(2, param)) {
        return false;
    }

    _xMax = static_cast<UInt32>(param[1]) * 128U;
    _yMax = static_cast<UInt32>(param[2]) * 128U;

    return _xMax >= 128U && _yMax >= 128U;
}

bool ApplePS2FocalTech::switchToNativeProtocol() {
    // F8 00, F8 00, F8 00, F8 01, E6.  F4 is sent only after the
    // interrupt action has been installed.
    TPS2Request<9> request;
    request.commands[0].command = kPS2C_SendCommandAndCompareAck;
    request.commands[0].inOrOut = FOC_PS2_CUSTOM_COMMAND;
    request.commands[1].command = kPS2C_SendCommandAndCompareAck;
    request.commands[1].inOrOut = 0;
    request.commands[2].command = kPS2C_SendCommandAndCompareAck;
    request.commands[2].inOrOut = FOC_PS2_CUSTOM_COMMAND;
    request.commands[3].command = kPS2C_SendCommandAndCompareAck;
    request.commands[3].inOrOut = 0;
    request.commands[4].command = kPS2C_SendCommandAndCompareAck;
    request.commands[4].inOrOut = FOC_PS2_CUSTOM_COMMAND;
    request.commands[5].command = kPS2C_SendCommandAndCompareAck;
    request.commands[5].inOrOut = 0;
    request.commands[6].command = kPS2C_SendCommandAndCompareAck;
    request.commands[6].inOrOut = FOC_PS2_CUSTOM_COMMAND;
    request.commands[7].command = kPS2C_SendCommandAndCompareAck;
    request.commands[7].inOrOut = 1;
    request.commands[8].command = kPS2C_SendCommandAndCompareAck;
    request.commands[8].inOrOut = kDP_SetMouseScaling1To1;
    request.commandsCount = 9;

    _device->submitRequestAndBlock(&request);
    return request.commandsCount == 9;
}

bool ApplePS2FocalTech::setupHardware(bool publishInput) {
    memset(&_hardware, 0, sizeof(_hardware));
    memset(&_finger, 0, sizeof(_finger));

    _packetByteCount = 0;
    _ringBuffer.reset();

    _irqByteCount = 0;
    _packetCount = 0;
    _validPacketCount = 0;
    _invalidPacketCount = 0;

    _leftButton = 0;
    _rightButton = 0;
    _lastButtons = 0;
    _clickWasPressed = false;
    _clickRight = false;
    _forceEligible = false;
    _forceActive = false;
    _forceFinger = -1;

    if (_forceTimer) {
        _forceTimer->cancelTimeout();
    }

    // Direct-signature devices (validated FLT0101 path) require the standard
    // PS/2 reset before reading register 2 and entering native protocol.
