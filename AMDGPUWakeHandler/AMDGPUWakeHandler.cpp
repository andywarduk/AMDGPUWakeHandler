
#include <IOKit/IOLib.h>
#include "AMDGPUWakeHandler.hpp"

#define kMyNumberOfStates 2
#define kIOPMPowerOff 0

// This required macro defines the class's constructors, destructors,
// and several other methods I/O Kit requires.
OSDefineMetaClassAndStructors(AMDGPUWakeHandler, IOService)

static inline void outb(unsigned short port, unsigned char value)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (port), "a" (value));
}

// Define the driver's superclass.
#define super IOService

bool AMDGPUWakeHandler::init(OSDictionary *dict)
{
    bool result = super::init(dict);
    IOLog("Initializing\n");
    return result;
}

void AMDGPUWakeHandler::free(void)
{
    IOLog("Freeing\n");
    super::free();
}

IOService *AMDGPUWakeHandler::probe(IOService *provider,
                                    SInt32 *score)
{
    IOService *result = super::probe(provider, score);
    IOLog("Probing\n");
    return result;
}

bool AMDGPUWakeHandler::start(IOService *provider)
{
    bool result = super::start(provider);
    IOLog("Starting\n");
    PMinit();
    provider->joinPMtree(this);
    static IOPMPowerState myPowerStates[kMyNumberOfStates] = {
        {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
    };

    registerPowerDriver (this, myPowerStates, kMyNumberOfStates);

    return result;
}

void AMDGPUWakeHandler::stop(IOService *provider)
{
    IOLog("Stopping\n");
    PMstop();
    super::stop(provider);
}

void AMDGPUWakeHandler::disableGPU()
{
    IOLog("Disabling GPU\n");
    // GMUX_PORT_SWITCH_DDC - 1=IGD, 2=DIS
    outb(0x728, 0x1);
    // GMUX_PORT_SWITCH_DISPLAY - 2=IGD, 3=DIS
    outb(0x710, 0x2);
    // GMUX_PORT_SWITCH_EXTERNAL - 2=IGD, 3=DIS
    outb(0x740, 0x2);
    // GMUX_PORT_DISCRETE_POWER - 0-3?
    outb(0x750, 0x0);
}

void AMDGPUWakeHandler::setBacklight(uint32_t Intensity)
{
    IOLog("Setting backlight to %u\n", Intensity);
    // Write to GMUX_PORT_BRIGHTNESS -> GMUX_PORT_BRIGHTNESS+3
    outb(0x774, Intensity & 0xff);
    outb(0x775, (Intensity >> 8) & 0xff);
    outb(0x776, (Intensity >> 16) & 0xff);
    outb(0x777, (Intensity >> 24) & 0xff);
}

// Wakeup backlight intensity
#define BacklightIntensity 120000

IOReturn AMDGPUWakeHandler::setPowerState ( unsigned long whichState, IOService * whatDevice )
{
    if ( kIOPMPowerOff != whichState ) {
        IOLog("Waking up\n");
        // Disable GPU and set backlight several times after wakeup to make sure it sticks
        // (and we aren't overruled by another driver?)
        for(int i = 0; i < 20; i++){
            this->disableGPU();
            this->setBacklight(BacklightIntensity);
            IOSleep(500);
        }
    }
    return kIOPMAckImplied;
}
