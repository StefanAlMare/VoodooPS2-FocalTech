
    _hardware.pressed = ((packet[0] >> 4) & 1) != 0;
    _hardware.fingers[id].x = static_cast<SInt32>(((packet[1] & 0x0f) << 8) | packet[2]);
    _hardware.fingers[id].y = static_cast<SInt32>((packet[3] << 8) | packet[4]);
    _hardware.fingers[id].valid = true;
    _hardware.width = packet[5] >> 4;
}

void ApplePS2FocalTech::processRelativePacket(const UInt8 *packet) {
    _hardware.pressed = (packet[0] >> 7) != 0;

    const int first = static_cast<int>((packet[0] >> 4) & 0x7) - 1;
    if (first >= 0 && first < FOC_MAX_FINGERS) {
        _hardware.fingers[first].x += static_cast<SInt8>(packet[1]);
        _hardware.fingers[first].y += static_cast<SInt8>(packet[2]);
    }

    const int second = static_cast<int>((packet[3] >> 4) & 0x7) - 1;
    if (second >= 0 && second < FOC_MAX_FINGERS) {
        _hardware.fingers[second].x += static_cast<SInt8>(packet[4]);
        _hardware.fingers[second].y += static_cast<SInt8>(packet[5]);
    }
}

void ApplePS2FocalTech::reportState() {
    if (_hardware.pressed && !_clickWasPressed) {
        bool found = false;
        UInt32 clickX = 0;
        UInt32 clickY = 0;
        int clickFinger = -1;

        for (int i = 0; i < FOC_MAX_FINGERS; ++i) {
            const FocalTechFingerState &raw = _hardware.fingers[i];
            if (!raw.active || !raw.valid) {
                continue;
            }

            SInt32 x = raw.x;
            SInt32 y = raw.y;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x > static_cast<SInt32>(_xMax)) x = _xMax;
            if (y > static_cast<SInt32>(_yMax)) y = _yMax;

            const UInt32 vx = static_cast<UInt32>(x);
            const UInt32 vy = _yMax - static_cast<UInt32>(y);

            if (!found || vy > clickY) {
                found = true;
                clickX = vx;
                clickY = vy;
                clickFinger = i;
            }
        }

        _clickRight =
            found &&
            clickY >= (_yMax * 70U) / 100U &&
            clickX >= _xMax / 2U;

        _forceEligible = false;
        _forceActive = false;
        _forceFinger = -1;
        if (_forceTimer) {
            _forceTimer->cancelTimeout();
        }

        if (!_clickRight && clickFinger >= 0 && _forceTimer) {
            const FocalTechFingerState &raw = _hardware.fingers[clickFinger];
            _forceFinger = clickFinger;
            _forceStartX = raw.x;
            _forceStartY = raw.y;
            _forceEligible = true;
            _forceTimer->setTimeoutMS(kForceHoldMS);
        }

        _clickWasPressed = true;
    } else if (!_hardware.pressed) {
        if (_forceTimer) {
            _forceTimer->cancelTimeout();
        }
        _forceEligible = false;
        _forceActive = false;
        _forceFinger = -1;
        _clickWasPressed = false;
        _clickRight = false;
    }

    if (_forceEligible && !_forceActive && _hardware.pressed &&
        _forceFinger >= 0 && _forceFinger < FOC_MAX_FINGERS) {
        const FocalTechFingerState &raw = _hardware.fingers[_forceFinger];

        if (!raw.active || !raw.valid) {
            _forceEligible = false;
            if (_forceTimer) _forceTimer->cancelTimeout();
        } else {
            SInt32 dx = raw.x - _forceStartX;
            SInt32 dy = raw.y - _forceStartY;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;

            if (dx > kForceMoveCancel || dy > kForceMoveCancel) {
                _forceEligible = false;
                if (_forceTimer) _forceTimer->cancelTimeout();
            }
        }
    }

    const UInt32 buttonMask = _hardware.pressed ? (_clickRight ? 0x02U : 0x01U) : 0U;

    for (int i = 0; i < FOC_MAX_FINGERS; ++i) {
        FocalTechFingerState &raw = _hardware.fingers[i];
        FocalTechVirtualFinger &finger = _finger[i];

        if (!raw.active || !raw.valid) {
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
        finger.current.width = static_cast<UInt8>(_hardware.width);
        finger.touching = true;

        if (!wasTouching) {
            finger.previous = finger.current;
        }
    }

    _leftButton = buttonMask & 0x01U;
    _rightButton = buttonMask & 0x02U;
    sendTouchData();
}

MT2FingerType ApplePS2FocalTech::fingerTypeForIndex(int index) {
    switch (index) {
        case 0: return kMT2FingerTypeIndexFinger;
        case 1: return kMT2FingerTypeMiddleFinger;
        case 2: return kMT2FingerTypeRingFinger;
        case 3: return kMT2FingerTypeThumb;
        case 4: return kMT2FingerTypeLittleFinger;
        default: return kMT2FingerTypeIndexFinger;
    }
}

void ApplePS2FocalTech::sendButtons(UInt32 buttons) {
    if (!_voodooInput) {
        return;
    }
