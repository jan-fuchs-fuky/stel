#! /usr/bin/python.exe
# -*- coding: utf-8 -*-

#
# $Date$
# $Rev$
# $URL$
#

import os
import sys
import time
import platform
import base64
import traceback
import logging
import socket

from ctypes import *
from SimpleXMLRPCServer import SimpleXMLRPCServer
from SimpleXMLRPCServer import SimpleXMLRPCRequestHandler
from logging.handlers import TimedRotatingFileHandler

HOST = "192.168.193.199"
PORT = 23

logger = logging.getLogger("gandalf_xmlrpc_server")

class RequestHandler(SimpleXMLRPCRequestHandler):
    rpc_paths = ('/RPC2',)

class py_PicamCameraID(Structure):
    _fields_ = [
        ("model", c_int),
        ("computer_interface", c_int),
        ("sensor_name", (c_char * 64)),
        ("serial_number", (c_char * 64)),
    ]

class py_PicamAcquisitionStatus(Structure):
    _fields_ = [
        ("running", c_int),
        ("errors", c_int),
        ("readout_rate", c_double),
    ]

class py_PicamAvailableData(Structure):
    _fields_ = [
        ("initial_readout", c_void_p),
        ("readout_count", c_int64),
    ]

class PylonCCD:
    def __init__(self):
        self.data_base64 = ""
        self.gan_available_data = py_PicamAvailableData()
        self.gan_camera = c_void_p()

        if (platform.system() == "Linux"):
            #self.picam = CDLL("/usr/local/lib64/libpicam.so")
            self.picam = CDLL("libpicam.so")
        else:
            self.picam = CDLL("Picam.dll")

        major = c_int()
        minor = c_int()
        distribution = c_int()
        released = c_int()

        self.picam.Picam_GetVersion(byref(major), byref(minor), byref(distribution), byref(released))

        logger.info("picam version %i.%i.%i.%i" % \
            (major.value, minor.value, distribution.value, released.value))

        self.init()

    def init(self):
        inited = c_int()
        self.picam.Picam_IsLibraryInitialized(byref(inited))

        if (inited):
            error = self.picam.Picam_UninitializeLibrary()
            logger.info("Picam_UninitializeLibrary() => %i" % error)

        error = self.picam.Picam_InitializeLibrary()
        logger.info("Picam_InitializeLibrary() => %i" % error)

        self.gan_camera = c_void_p()
        gan_camera_id = py_PicamCameraID()
        
        if (platform.system() != "Linux"):
            for i in range(6):
                error = self.picam.Picam_OpenFirstCamera(byref(self.gan_camera))
                logger.info("Picam_OpenFirstCamera() => %i" % error)

                if (error != 0):
                    if (i >= 5):
                        logger.error("Camera not opened")
                        return -1

                    time.sleep(10)
                else:
                    break
        else:
            PicamModel_Pylon2KBExcelon = c_int(431)
            self.picam.Picam_ConnectDemoCamera(PicamModel_Pylon2KBExcelon, "12345", byref(gan_camera_id))
            self.picam.Picam_OpenCamera(byref(gan_camera_id), byref(self.gan_camera))

        error = self.picam.Picam_GetCameraID(self.gan_camera, byref(gan_camera_id))
        if (error == 0):
            logger.info("model=%i, computer_interface=%i, sensor_name=%s, serial_number=%s" % \
                (gan_camera_id.model, gan_camera_id.computer_interface, \
                gan_camera_id.sensor_name, gan_camera_id.serial_number))
        else:
            logger.error("Picam_GetCameraID() failed => %i" % error)
            return -1

        return 0

    def uninit(self):
        if (self.gan_camera):
            error = self.picam.Picam_CloseCamera(self.gan_camera)
            print "Picam_CloseCamera() => %i" % error

        error = self.picam.Picam_UninitializeLibrary()
        print "Picam_UninitializeLibrary() => %i" % error

        return 0

    def expose_init(self, exptime, readout_speed, shutter, gain):
        running = c_int(1)
        error = self.picam.Picam_IsAcquisitionRunning(self.gan_camera, byref(running))
        logger.info("Picam_IsAcquisitionRunning(): => %i [running = %i]" % (error, running.value))

        #if (running.value):
        #    error = self.picam.Picam_StopAcquisition(self.gan_camera)
        #    logger.info("Picam_StopAcquisition(): => %i" % error)

        #    while (running.value):
        #        error = self.picam.Picam_IsAcquisitionRunning(self.gan_camera, byref(running))
        #        logger.info("Picam_IsAcquisitionRunning(): => %i [running = %i]" % (error, running.value))
        #        time.sleep(0.5)

        # ADC SPEED
        PicamParameter_AdcSpeed = c_int(50462753)
        #speed = c_double(4.0) # MHz

        #error = self.picam.Picam_GetParameterFloatingPointValue(
        #    self.gan_camera,
        #    PicamParameter_AdcSpeed,
        #    byref(speed)
        #)
        #logger.info("PicamParameter_AdcSpeed => %i [value = %f]" % (error, speed.value))

        #speed.value = 4.0
        speed = c_double(readout_speed / 1000.0) # MHz
        settable = c_int()

        error = self.picam.Picam_CanSetParameterFloatingPointValue(
            self.gan_camera,
            PicamParameter_AdcSpeed,
            speed,
            byref(settable)
        )
        logger.info("PicamParameter_AdcSpeed => %i [settable = %i]" % (error, settable.value))

        error = self.picam.Picam_SetParameterFloatingPointValue(
            self.gan_camera,
            PicamParameter_AdcSpeed,
            speed
        )
        logger.info("PicamParameter_AdcSpeed => %i" % error)
        if (error != 0):
            return error

        # SHUTTER
        PicamParameter_ShutterTimingMode = c_int(50593816)

        #PicamShutterTimingMode_Normal = c_int(1)
        #PicamShutterTimingMode_AlwaysClosed = c_int(2)
        #PicamShutterTimingMode_AlwaysOpen = c_int(3)
        #PicamShutterTimingMode_OpenBeforeTrigger = c_int(4)

        if (shutter == 1):
            parameter_shutter = c_int(1)
        else:
            parameter_shutter = c_int(2)

        error = self.picam.Picam_SetParameterIntegerValue(
            self.gan_camera,
            PicamParameter_ShutterTimingMode,
            parameter_shutter
        )
        logger.info("PicamParameter_ShutterTimingMode => %i [shutter = %i]" % (error, shutter))

        # ADC ANALOG GAIN
        PicamParameter_AdcAnalogGain = c_int(50593827)
        #PicamAdcAnalogGain_Low = c_int(1)
        #PicamAdcAnalogGain_Medium = c_int(2)
        #PicamAdcAnalogGain_High = c_int(3)
        parameter_gain = c_int(gain)

        error = self.picam.Picam_SetParameterIntegerValue(
            self.gan_camera,
            PicamParameter_AdcAnalogGain,
            parameter_gain
        )
        logger.info("PicamParameter_AdcAnalogGain => %i [gain = %i]" % (error, gain))

        # READOUT COUNT
        PicamParameter_ReadoutCount = c_int(33947688)
        readout_count = c_int64(1)

        error = self.picam.Picam_SetParameterLargeIntegerValue(
            self.gan_camera,
            PicamParameter_ReadoutCount,
            readout_count
        )
        logger.info("PicamParameter_ReadoutCount => %i" % error)

        ## TRIGGER RESPONSE
        PicamParameter_TriggerResponse = c_int(50593822)
        PicamTriggerResponse_ExposeDuringTriggerPulse = c_int(4)

        error = self.picam.Picam_SetParameterIntegerValue(
            self.gan_camera,
            PicamParameter_TriggerResponse,
            PicamTriggerResponse_ExposeDuringTriggerPulse
        )
        logger.info("PicamParameter_TriggerResponse => %i" % error)

        ## OUTPUT SIGNAL
        PicamParameter_OutputSignal = c_int(50593824)
        PicamOutputSignal_AlwaysHigh = c_int(5)

        # not supported
        #PicamOutputSignal_AlwaysLow = c_int(4)

        error = self.picam.Picam_SetParameterIntegerValue(
            self.gan_camera,
            PicamParameter_OutputSignal,
            PicamOutputSignal_AlwaysHigh
        )
        logger.info("PicamParameter_OutputSignal => %i" % error)

        # EXPOSURE TIME
        PicamParameter_ExposureTime = c_int(33685527)
        exptime = c_double(-1)
        #exptime = c_double(0)
        #exptime = c_double(exptime * 1000.0)

        error = self.picam.Picam_SetParameterFloatingPointValue(
            self.gan_camera,
            PicamParameter_ExposureTime,
            exptime
        )
        logger.info("PicamParameter_ExposureTime => %i" % error)

        self.commit_parameters()
        return 0

    def set_ttl_out(self, value):
        #running = 0

        #while (running == 0):
        #    running = c_int(1)
        #    error = self.picam.Picam_IsAcquisitionRunning(self.gan_camera, byref(running))
        #    logger.info("Picam_IsAcquisitionRunning(): => %i [running = %i]" % (error, running.value))
        #    time.sleep(0.1)

        time.sleep(1)

        #if (value == 0):
        #    value = 255
        #else:
        #    value = 0

        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((HOST, PORT))
            s.sendall("%c%c%c%c%c%c%c" % (0xFF, 0xFA, 0x2C, 0x33, int(value), 0xFF, 0xFA))
            data = s.recv(7)

            answer = []
            for c in data:
                answer.append("%02X " % ord(c))

            logger.info("set_ttl_out(%i) => %s" % (value, "".join(answer)))
            s.close()
        except:
            logger.exception("set_ttl_out(%s) exception" % value)
            return 4

        return 0

    def expose_start(self):
        error = self.picam.Picam_StartAcquisition(self.gan_camera)
        logger.info("Picam_StartAcquisition(): => %i" % error)

        return error

    def expose(self):
        return 0

    def readout(self):
        #readout_time_out = c_int(-1) # infinity
        readout_time_out = c_int(100) # miliseconds
        self.gan_available_data = py_PicamAvailableData()
        status = py_PicamAcquisitionStatus()

        error = self.picam.Picam_WaitForAcquisitionUpdate(
            self.gan_camera,
            readout_time_out,
            byref(self.gan_available_data),
            byref(status)
        )

        logger.info("Picam_WaitForAcquisitionUpdate(): => %i [readout_count = %i, running = %i]" % \
            (error, self.gan_available_data.readout_count, status.running))

        running = c_int(1)
        error = self.picam.Picam_IsAcquisitionRunning(self.gan_camera, byref(running))
        logger.info("Picam_IsAcquisitionRunning(): => %i [running = %i]" % (error, running.value))

        if ((error in [0, 32]) and (self.gan_available_data.readout_count != 1)):
            return 1
        elif (self.gan_available_data.readout_count == 1):
            return 0

        return -1

    def save_raw_image(self):
        logger.info("save_raw_image()")

        gan_data_size = 2048*512*2
        data = cast(self.gan_available_data.initial_readout, POINTER(c_int16 * (2048 * 512)))
        #print str(data)

        fo = open("./data.raw", "wb")
        fo.write(data.contents)
        fo.close()

        self.data_base64 = base64.b64encode(data.contents)
        return self.data_base64

    def get_data(self):
        return self.data_base64

    def expose_end(self):
        return 0

    def expose_uninit(self):
        self.readout()
        #error = self.picam.Picam_StopAcquisition(self.gan_camera)
        #logger.info("Picam_StopAcquisition(): => %i" % error)
        #return error
        return 0

    def get_temp(self):
        PicamParameter_SensorTemperatureReading = 16908303
        temp = c_double()

        error = self.picam.Picam_ReadParameterFloatingPointValue(
            self.gan_camera,
            PicamParameter_SensorTemperatureReading,
            byref(temp)
        )
        logger.info("PicamParameter_SensorTemperatureReading => %i [temp = %f]" % (error, temp.value))

        return temp.value

    def set_parameter_temp(self, temp):
        PicamParameter_SensorTemperatureSetPoint = 33685518
        temp = c_double(temp)

        error = self.picam.Picam_SetParameterFloatingPointValue(
            self.gan_camera,
            PicamParameter_SensorTemperatureSetPoint,
            temp
        )
        logger.info("PicamParameter_SensorTemperatureSetPoint => %i [temp = %f]" % (error, temp.value))

        return error

    def commit_parameters(self):
        committed = c_int()
        error = self.picam.Picam_AreParametersCommitted(self.gan_camera, byref(committed))
        logger.info("Picam_AreParametersCommitted(): committed = %i => %i" % (committed.value, error))

        failed_parameters = c_void_p()
        failed_parameters_count = c_int()

        error = self.picam.Picam_CommitParameters(
            self.gan_camera,
            byref(failed_parameters),
            byref(failed_parameters_count)
        )
        logger.info("Picam_CommitParameters(): => %i [failed_parameters_count = %i]" % \
            (error, failed_parameters_count.value))

        self.picam.Picam_DestroyParameters(failed_parameters)

        return error

    def set_temp(self, temp):
        self.set_parameter_temp(temp)
        self.commit_parameters()

        return 0

    def set_exptime(self, exptime):
        # EXPOSURE TIME
        PicamParameter_ExposureTime = c_int(33685527)
        exptime = c_double(exptime * 1000.0)

        error = self.picam.Picam_SetParameterFloatingPointValueOnline(
            self.gan_camera,
            PicamParameter_ExposureTime,
            exptime
        )
        logger.info("PicamParameter_ExposureTime ONLINE => %i" % error)

        return error

    def expose_stop(self):
        # ABORT
        error = self.picam.Picam_StopAcquisition(self.gan_camera)
        logger.info("Picam_StopAcquisition(): => %i" % error)

        self.set_ttl_out(0)

        readout_time_out = c_int(1500) # miliseconds
        self.gan_available_data = py_PicamAvailableData()
        status = py_PicamAcquisitionStatus()

        error = self.picam.Picam_WaitForAcquisitionUpdate(
            self.gan_camera,
            readout_time_out,
            byref(self.gan_available_data),
            byref(status)
        )

        logger.info("Picam_WaitForAcquisitionUpdate(): => %i [readout_count = %i, running = %i]" % \
            (error, self.gan_available_data.readout_count, status.running))

        ## OUTPUT SIGNAL
        #PicamParameter_OutputSignal = c_int(50593824)
        #PicamOutputSignal_AlwaysLow = c_int(4)

        #error = self.picam.Picam_SetParameterIntegerValue(
        #    self.gan_camera,
        #    PicamParameter_OutputSignal,
        #    PicamOutputSignal_AlwaysLow
        #)
        #logger.info("PicamParameter_OutputSignal => %i" % error)

        ## COMMIT PARAMETERS
        #committed = c_int()
        #error = self.picam.Picam_AreParametersCommitted(self.gan_camera, byref(committed))
        #logger.info("Picam_AreParametersCommitted(): committed = %i => %i" % (committed.value, error))

        #failed_parameters = c_void_p()
        #failed_parameters_count = c_int()

        #error = self.picam.Picam_CommitParameters(
        #    self.gan_camera,
        #    byref(failed_parameters),
        #    byref(failed_parameters_count)
        #)
        #logger.info("Picam_CommitParameters(): => %i [failed_parameters_count = %i]" % \
        #    (error, failed_parameters_count.value))

        #self.picam.Picam_DestroyParameters(failed_parameters)
        #return error
        return 0

class GandalfServerFunctions:
    def __init__(self, ccd):
        self.ccd = ccd

    def ccd_init(self):
        logger.info("RPC ccd_init()")
        try:
            return self.ccd.init()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_uninit(self):
        logger.info("RPC ccd_uninit()")
        try:
            return self.ccd.uninit()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_expose_init(self, exptime, readout_speed, shutter, gain):
        logger.info("RPC ccd_expose_init()")
        try:
            return self.ccd.expose_init(exptime, readout_speed, shutter, gain)
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_expose_start(self):
        logger.info("RPC ccd_expose_start()")
        try:
            return self.ccd.expose_start()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_expose(self):
        logger.info("RPC ccd_expose()")
        try:
            return self.ccd.expose()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_set_ttl_out(self, value):
        logger.info("RPC ccd_set_ttl_out()")
        try:
            return self.ccd.set_ttl_out(value)
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_readout(self):
        logger.info("RPC ccd_readout()")
        try:
            return self.ccd.readout()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_save_raw_image(self):
        logger.info("RPC ccd_save_raw_image()")
        try:
            return self.ccd.save_raw_image()
        except:
            logger.exception("PylonCCD exception")
            return ""

    def ccd_get_data(self):
        logger.info("RPC ccd_get_data()")
        try:
            return self.ccd.get_data()
        except:
            logger.exception("PylonCCD exception")
            return ""

    def ccd_expose_end(self):
        logger.info("RPC ccd_expose_end()")
        try:
            return self.ccd.expose_end()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_expose_uninit(self):
        logger.info("RPC ccd_expose_uninit()")
        try:
            return self.ccd.expose_uninit()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_get_temp(self):
        logger.info("RPC ccd_get_temp()")
        try:
            return self.ccd.get_temp()
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_set_temp(self, temp):
        logger.info("RPC ccd_set_temp()")
        try:
            return self.ccd.set_temp(temp)
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_set_exptime(self, exptime):
        logger.info("RPC ccd_set_exptime()")
        try:
            return self.ccd.set_exptime(exptime)
        except:
            logger.exception("PylonCCD exception")
            return -1

    def ccd_expose_stop(self):
        logger.info("RPC ccd_expose_stop()")
        try:
            return self.ccd.expose_stop()
        except:
            logger.exception("PylonCCD exception")
            return -1

def print_flush(text):
    print text
    sys.stdout.flush()

def main():
    print os.getpid()
    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - %(message)s")
    fh = TimedRotatingFileHandler("./gandalf_xmlrpc_server.log", when='D', interval=1, backupCount=30)
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)

    logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)

    server = SimpleXMLRPCServer(("0.0.0.0", 5000), requestHandler=RequestHandler)
    server.register_introspection_functions()
    server.register_instance(GandalfServerFunctions(PylonCCD()))
    server.serve_forever()

if __name__ == '__main__':
    main()
