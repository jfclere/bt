# BLE environment

The source files are located in the src/ directory.

pkg.yml contains the base definition of the app.

syscfg.yml contains setting definitions and overrides.

## The target is like the following:
```
jfclere@0:~/dev/try$ newt target show thingy_my_sensor
targets/thingy_my_sensor
    app=apps/my_sensor_app
    bsp=@apache-mynewt-core/hw/bsp/nordic_pca10056
    build_profile=debug
    syscfg=BME280=1:BUS_DRIVER_PRESENT=1:CONSOLE_RTT=0:CONSOLE_UART=1:FLOAT_USER=1:I2C_0=1:I2C_0_FREQ_KHZ=10:I2C_0_PIN_SCL=27:I2C_0_PIN_SDA=26:SENSOR_OIC=0:SHELL_TASK=0:FLOAT_USER=1
```

## DK configuration:
```
I2C_0_PIN_SCL=27:I2C_0_PIN_SDA=26
```

## Adafruit nRF52 fether pro configuration:
```
I2C_0_PIN_SCL=26:I2C_0_PIN_SDA=25
```

## For each BLE device use a different Device ID for example:
```
BLE_LL_PUBLIC_DEV_ADDR=0x1122aabb33cd
```
