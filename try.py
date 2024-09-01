import pygatt

# Purpose basic tests for https://mynewt.apache.org/latest/tutorials/ble/bleprph/bleprph-sections/bleprph-app.html
YOUR_DEVICE_ADDRESS = "D6:31:7F:6F:AC:DB"
# Many devices, e.g. Fitbit, use random addressing - this is required to
# connect.
ADDRESS_TYPE = pygatt.BLEAddressType.random

adapter = pygatt.GATTToolBackend()

try:
    adapter.start()
    #device = adapter.connect('C8:2C:42:87:30:83')
    #device = adapter.connect('nimble-bleprph')
    device = adapter.connect(YOUR_DEVICE_ADDRESS, address_type=ADDRESS_TYPE)
    value = device.char_read("5c3a659e-897e-45e1-b016-007107c96df7")
    # The random value... need some security... so won't work for the moment :-(
    # value = device.char_read("5c3a659e-897e-45e1-b016-007107c96df6");
    # the uid value... 
    # BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC (protected too)
    # value = device.char_read("59462f12-9543-9999-12c8-58b459a2712d");
    print("Data: {}".format(value.hex()))
finally:
    adapter.stop()
