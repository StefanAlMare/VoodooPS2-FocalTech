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

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOWorkLoop.h>

#include "VoodooPS2Controller.h"
#include "VoodooPS2FocalTech.h"
#include "VoodooInputMultitouch/VoodooInputMessages.h"

OSDefineMetaClassAndStructors(ApplePS2FocalTech, IOService);

bool ApplePS2FocalTech::init(OSDictionary *properties) {
    return super::init(properties);
}

bool ApplePS2FocalTech::handleOpen(IOService *forClient, IOOptionBits options, void *arg) {
    if (forClient && forClient->getProperty(VOODOO_INPUT_IDENTIFIER)) {
        _voodooInput = forClient;
        _voodooInput->retain();
        return true;
    }
    return false;
}

void ApplePS2FocalTech::handleClose(IOService *forClient, IOOptionBits options) {
    if (forClient == _voodooInput) {
        OSSafeReleaseNULL(_voodooInput);
    }
}

bool ApplePS2FocalTech::handleIsOpen(const IOService *forClient) const {
    return forClient ? (_voodooInput == forClient) : (_voodooInput != nullptr);
}

ApplePS2FocalTech *ApplePS2FocalTech::probe(IOService *provider, SInt32 *score) {
    if (!super::probe(provider, score)) {
        return nullptr;
    }

    _device = OSDynamicCast(ApplePS2MouseDevice, provider);
    if (!_device) {
        return nullptr;
    }

    if (OSNumber *port = OSDynamicCast(OSNumber, _device->getProperty(kPortKey))) {
        _providerPort = port->unsigned32BitValue();
    } else {
        _providerPort = 0;
    }
    _auxNubCount = countAuxNubs();

    _legacyCompatibilityPath = false;
    bool detected = detectFocalTech();

    // On a muxed controller there are several AUX nubs.  Never manipulate
    // global i8042/keyboard state while probing the empty mux ports.
    // Only a nub that answers FocalTech directly may attach there.
    if (!detected && mayUseLegacyFallback() && prepareLegacyControllerState()) {
        _legacyCompatibilityPath = true;
        detected = detectFocalTech();
    }

    if (detected) {
        publishRuntimeDiagnostics(nullptr, false);
    }

    _device = nullptr;
    return detected ? this : nullptr;
}

bool ApplePS2FocalTech::start(IOService *provider) {
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
    publishRuntimeDiagnostics(nullptr, false);

    IOWorkLoop *workLoop = getWorkLoop();
    if (!workLoop) {
        OSSafeReleaseNULL(_device);
        return false;
    }

    _forceTimer = IOTimerEventSource::timerEventSource(
        this,
        OSMemberFunctionCast(
            IOTimerEventSource::Action,
            this,
            &ApplePS2FocalTech::forceTimerFired));

    if (!_forceTimer || workLoop->addEventSource(_forceTimer) != kIOReturnSuccess) {
        OSSafeReleaseNULL(_forceTimer);
        OSSafeReleaseNULL(_device);
        return false;
    }

    _device->lock();

    if (!setupHardware(true)) {
        _device->unlock();
        workLoop->removeEventSource(_forceTimer);
        OSSafeReleaseNULL(_forceTimer);
        OSSafeReleaseNULL(_device);
        return false;
    }

    _device->installInterruptAction(
        this,
        OSMemberFunctionCast(PS2InterruptAction, this, &ApplePS2FocalTech::interruptOccurred),
        OSMemberFunctionCast(PS2PacketAction, this, &ApplePS2FocalTech::packetReady));
    _interruptInstalled = true;

    setTouchPadEnable(true);
    _device->unlock();

    _device->installPowerControlAction(
        this,
        OSMemberFunctionCast(PS2PowerControlAction, this, &ApplePS2FocalTech::setDevicePowerState));
    _powerInstalled = true;

    return true;
}

void ApplePS2FocalTech::stop(IOService *provider) {
    if (_forceTimer) {
        _forceTimer->cancelTimeout();
        if (IOWorkLoop *workLoop = getWorkLoop()) {
            workLoop->removeEventSource(_forceTimer);
        }
        OSSafeReleaseNULL(_forceTimer);
    }

    if (_device) {
        setTouchPadEnable(false);

        if (_interruptInstalled) {
            _device->uninstallInterruptAction();
            _interruptInstalled = false;
        }
