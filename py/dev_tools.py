"""
The windows device manipulation helper
The code is inspired by serial.tools.list_ports module
"""

import ctypes
import serial
import _winreg as reg
from serial.win32 import ULONG_PTR
from ctypes.wintypes import HANDLE
from ctypes.wintypes import BOOL
from ctypes.wintypes import HWND
from ctypes.wintypes import DWORD
from ctypes.wintypes import WORD
from ctypes.wintypes import LONG
from ctypes.wintypes import ULONG
from ctypes.wintypes import LPCSTR
from ctypes.wintypes import HKEY
from ctypes.wintypes import BYTE

NULL = 0
HDEVINFO = ctypes.c_void_p
PCTSTR = ctypes.c_char_p
PTSTR = ctypes.c_void_p
LPDWORD = PDWORD = ctypes.POINTER(DWORD)
LPBYTE = PBYTE = ctypes.c_void_p

ACCESS_MASK = DWORD
REGSAM = ACCESS_MASK

INVALID_HANDLE_VALUE = 0xffffffff

def byte_buffer(length):
    """Get a buffer for a string"""
    return (BYTE*length)()

def string(buffer):
    s = []
    for c in buffer:
        if c == 0: break
        s.append(chr(c & 0xff)) # "& 0xff": hack to convert signed to unsigned
    return ''.join(s)

def valid_handle(value, func, arguments):
    if value == 0:
        raise ctypes.WinError()
    return value

class GUID(ctypes.Structure):
    _fields_ = [
        ('Data1', DWORD),
        ('Data2', WORD),
        ('Data3', WORD),
        ('Data4', BYTE*8),
    ]
    def __str__(self):
        return "{%08x-%04x-%04x-%s-%s}" % (
            self.Data1,
            self.Data2,
            self.Data3,
            ''.join(["%02x" % d for d in self.Data4[:2]]),
            ''.join(["%02x" % d for d in self.Data4[2:]]),
        )

class SP_DEVINFO_DATA(ctypes.Structure):
    _fields_ = [
        ('cbSize', DWORD),
        ('ClassGuid', GUID),
        ('DevInst', DWORD),
        ('Reserved', ULONG_PTR),
    ]
    def __str__(self):
        return "ClassGuid:%s DevInst:%s" % (self.ClassGuid, self.DevInst)
PSP_DEVINFO_DATA = ctypes.POINTER(SP_DEVINFO_DATA)

class SP_CLASSINSTALL_HEADER(ctypes.Structure):
    _fields_ = [
        ('cbSize', DWORD),
        ('InstallFunction', DWORD)
    ]
PSP_CLASSINSTALL_HEADER = ctypes.POINTER(SP_CLASSINSTALL_HEADER)
    
class SP_PROPCHANGE_PARAMS(ctypes.Structure):
    _fields_ = [
        ('ClassInstallHeader', SP_CLASSINSTALL_HEADER),
        ('StateChange', DWORD),
        ('Scope', DWORD),
        ('HwProfile', DWORD),
    ]
PSP_PROPCHANGE_PARAMS = ctypes.POINTER(SP_PROPCHANGE_PARAMS)

PSP_DEVICE_INTERFACE_DETAIL_DATA = ctypes.c_void_p

setupapi = ctypes.windll.LoadLibrary("setupapi")
SetupDiDestroyDeviceInfoList = setupapi.SetupDiDestroyDeviceInfoList
SetupDiDestroyDeviceInfoList.argtypes = [HDEVINFO]
SetupDiDestroyDeviceInfoList.restype = BOOL

SetupDiClassGuidsFromName = setupapi.SetupDiClassGuidsFromNameA
SetupDiClassGuidsFromName.argtypes = [PCTSTR, ctypes.POINTER(GUID), DWORD, PDWORD]
SetupDiClassGuidsFromName.restype = BOOL

SetupDiEnumDeviceInfo = setupapi.SetupDiEnumDeviceInfo
SetupDiEnumDeviceInfo.argtypes = [HDEVINFO, DWORD, PSP_DEVINFO_DATA]
SetupDiEnumDeviceInfo.restype = BOOL

SetupDiGetClassDevs = setupapi.SetupDiGetClassDevsA
SetupDiGetClassDevs.argtypes = [ctypes.POINTER(GUID), PCTSTR, HWND, DWORD]
SetupDiGetClassDevs.restype = HDEVINFO
SetupDiGetClassDevs.errcheck = valid_handle

SetupDiOpenDevRegKey = setupapi.SetupDiOpenDevRegKey
SetupDiOpenDevRegKey.argtypes = [HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM]
SetupDiOpenDevRegKey.restype = HKEY

SetupDiSetClassInstallParams = setupapi.SetupDiSetClassInstallParamsA
SetupDiSetClassInstallParams.argtypes = [HDEVINFO, PSP_DEVINFO_DATA, PSP_PROPCHANGE_PARAMS, DWORD]
SetupDiSetClassInstallParams.restype = BOOL

SetupDiChangeState = setupapi.SetupDiChangeState
SetupDiChangeState.argtypes = [HDEVINFO, PSP_DEVINFO_DATA]
SetupDiChangeState.restype = BOOL

advapi32 = ctypes.windll.LoadLibrary("Advapi32")
RegCloseKey = advapi32.RegCloseKey
RegCloseKey.argtypes = [HKEY]
RegCloseKey.restype = LONG

RegQueryValueEx = advapi32.RegQueryValueExA
RegQueryValueEx.argtypes = [HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD]
RegQueryValueEx.restype = LONG

DIGCF_PRESENT = 2
DICS_FLAG_GLOBAL = 1
DICS_FLAG_CONFIGSPECIFIC = 2
DIREG_DEV = 1
DIF_PROPERTYCHANGE = 0x12

DICS_ENABLE  = 1
DICS_DISABLE = 2
DICS_PROPCHANGE = 3

KEY_QUERY_VALUE = 1
KEY_READ = 0x20019
KEY_WRITE= 0x20006

Ports = serial.to_bytes([80, 111, 114, 116, 115]) # "Ports"
PortName = serial.to_bytes([80, 111, 114, 116, 78, 97, 109, 101]) # "PortName"

def dev_find_port(port):
    """
    Find port given its 'COMn' system name.
    Returns the tuple of device info handle and device information.
    """
    GUIDs = (GUID*8)() # so far only seen one used, so hope 8 are enough...
    guids_size = DWORD()
    found = False

    if not SetupDiClassGuidsFromName(
            Ports,
            GUIDs,
            ctypes.sizeof(GUIDs),
            ctypes.byref(guids_size)):
        raise ctypes.WinError()

    # repeat for all possible GUIDs
    for index in range(guids_size.value):
        hdi = SetupDiGetClassDevs(
                ctypes.byref(GUIDs[index]),
                None,
                NULL,
                DIGCF_PRESENT)

        devinfo = SP_DEVINFO_DATA()
        devinfo.cbSize = ctypes.sizeof(devinfo)
        index = 0
        while SetupDiEnumDeviceInfo(hdi, index, ctypes.byref(devinfo)):
            index += 1
            # get the real com port name
            hkey = SetupDiOpenDevRegKey(
                    hdi,
                    ctypes.byref(devinfo),
                    DICS_FLAG_GLOBAL,
                    0,
                    DIREG_DEV,
                    KEY_READ)
            port_name_buffer = byte_buffer(250)
            port_name_length = ULONG(ctypes.sizeof(port_name_buffer))
            RegQueryValueEx(
                    hkey,
                    PortName,
                    None,
                    None,
                    ctypes.byref(port_name_buffer),
                    ctypes.byref(port_name_length))
            RegCloseKey(hkey)

            if string(port_name_buffer) == port:
                return hdi, devinfo

        SetupDiDestroyDeviceInfoList(hdi)

    return None

def dev_touch((hdi, devinfo), action=DICS_PROPCHANGE):
    """
    Trigger device state change.
    Should be run under administrator account otherwise the access denied exception will be raised.
    """
    header = SP_CLASSINSTALL_HEADER()
    header.cbSize = ctypes.sizeof(header)
    header.InstallFunction = DIF_PROPERTYCHANGE;
    propchangeparams = SP_PROPCHANGE_PARAMS()
    propchangeparams.ClassInstallHeader = header
    propchangeparams.Scope = DICS_FLAG_GLOBAL
    propchangeparams.HwProfile = 0
    propchangeparams.StateChange = action

    if not SetupDiSetClassInstallParams(
            hdi,
            ctypes.byref(devinfo),
            ctypes.byref(propchangeparams),
            ctypes.sizeof(propchangeparams)):
        raise ctypes.WinError()

    if not SetupDiChangeState(hdi, ctypes.byref(devinfo)):
        raise ctypes.WinError()

def dev_open_reg((hdi, devinfo), access=KEY_QUERY_VALUE):
    """Open device registry key and return its handle"""
    hkey = SetupDiOpenDevRegKey(
                hdi,
                ctypes.byref(devinfo),
                DICS_FLAG_GLOBAL,
                0,
                DIREG_DEV,
                access)
    if hkey == INVALID_HANDLE_VALUE:
        raise ctypes.WinError()
    return hkey

def dev_close_reg(hkey):
    """Close device registry key handle"""
    RegCloseKey(hkey)

def dev_get_latency(dev):
    """Get device latency configuration"""
    hkey = dev_open_reg(dev)
    try:
        return reg.QueryValueEx(int(hkey), 'LatencyTimer')[0]
    finally:
        dev_close_reg(hkey)

def dev_set_latency(dev, lat=1):
    """Set device latency configuration"""
    hkey = dev_open_reg(dev, KEY_WRITE)
    try:
        reg.SetValueEx(int(hkey), 'LatencyTimer', 0, reg.REG_DWORD, lat)
    finally:
        dev_close_reg(hkey)

def dev_close((hdi, devinfo)):
    """Close device info handle"""
    SetupDiDestroyDeviceInfoList(hdi)

def is_admin():
    """Check if current user is administrator"""
    return ctypes.windll.shell32.IsUserAnAdmin() != 0

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Command line driver
#
if __name__ == '__main__':
    import sys
    if not is_admin():
        print 'Warning: not Administrator'
    port = sys.argv[1]
    dev = dev_find_port(port)
    if dev is not None:
        if '--enable' in sys.argv[2:]:
            dev_touch(dev, DICS_ENABLE)
            print 'enabled'
        elif '--disable' in sys.argv[2:]:
            dev_touch(dev, DICS_DISABLE)
            print 'disabled'
        elif '--touch' in sys.argv[2:]:
            dev_touch(dev)
            print 'touched'
        elif '--lat' in sys.argv[2:]:
            dev_set_latency(dev, int(sys.argv[-1]))
            print 'latency set'
        else:
            print 'LatencyTimer =', dev_get_latency(dev)
        dev_close(dev)
    else:
        print port, 'not found'
