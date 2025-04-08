#include "CYDevice/CYDeviceFatory.hpp"
#include "CYDeviceImpl.hpp"

CYDEVICE_NAMESPACE_BEGIN

CYDeviceFactory::CYDeviceFactory()
{
}

CYDeviceFactory::~CYDeviceFactory()
{
}

ICYDevice* CYDeviceFactory::CreateDevice()
{
    return new CYDeviceImpl();
}

void CYDeviceFactory::DestroyDevice(ICYDevice*& pDevice)
{
    delete pDevice;
    pDevice = nullptr;
}

CYDEVICE_NAMESPACE_END