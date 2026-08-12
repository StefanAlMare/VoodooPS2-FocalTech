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

void ApplePS2FocalTech::sendTouchData() {
    AbsoluteTime timestamp;
    clock_get_uptime(&timestamp);

    int count = 0;
    for (int i = 0; i < FOC_MAX_FINGERS; ++i) {
        const FocalTechVirtualFinger &state = _finger[i];
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

        if (_forceActive && i == _forceFinger) {
            transducer.supportsPressure = true;
            transducer.currentCoordinates.pressure = 255;
            transducer.currentCoordinates.width = 10;
        }

        ++count;
    }

    if (count >= 4) {
        UInt32 maxY = 0;
        int newThumb = 0;
        int oldThumb = 0;

        for (int i = 0; i < count; ++i) {
            if (_inputEvent.transducers[i].currentCoordinates.y > maxY) {
                maxY = _inputEvent.transducers[i].currentCoordinates.y;
                newThumb = i;
            }
            if (_inputEvent.transducers[i].fingerType == kMT2FingerTypeThumb) {
                oldThumb = i;
            }
        }

        _inputEvent.transducers[oldThumb].fingerType = _inputEvent.transducers[newThumb].fingerType;
        _inputEvent.transducers[newThumb].fingerType = kMT2FingerTypeThumb;
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

    const UInt32 currentButtons = _forceActive ? _rightButton : (_leftButton | _rightButton);
    if (currentButtons != _lastButtons) {
        sendButtons(currentButtons);
        _lastButtons = currentButtons;
    }
}

void ApplePS2FocalTech::forceTimerFired() {
    if (!_forceEligible || _forceActive || !_hardware.pressed || _clickRight ||
        _forceFinger < 0 || _forceFinger >= FOC_MAX_FINGERS) {
        return;
    }

    const FocalTechFingerState &raw = _hardware.fingers[_forceFinger];
    if (!raw.active || !raw.valid) {
        _forceEligible = false;
        return;
    }

    SInt32 dx = raw.x - _forceStartX;
    SInt32 dy = raw.y - _forceStartY;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx > kForceMoveCancel || dy > kForceMoveCancel) {
        _forceEligible = false;
        return;
    }

    _forceEligible = false;
    _forceActive = true;

    // Transition from the direct left-button channel to synthetic pressure.
    if (_lastButtons & 0x01U) {
        sendButtons(0);
        _lastButtons = 0;
    }

    sendTouchData();
}
