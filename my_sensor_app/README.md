# BME280 Sample App Definition on nRF52840DK
It uses:
```
   P0.27 = SCL
   P0.26 = SDA
```
## The target is the following:
```
jfclere@0:~/dev/try$ newt target show thingy_my_sensor
targets/thingy_my_sensor
    app=apps/my_sensor_app
    bsp=@apache-mynewt-core/hw/bsp/nordic_pca10056
    build_profile=debug
    syscfg=BME280=1:BUS_DRIVER_PRESENT=1:CONSOLE_RTT=0:CONSOLE_UART=1:FLOAT_USER=1:I2C_0=1:I2C_0_FREQ_KHZ=10:I2C_0_PIN_SCL=27:I2C_0_PIN_SDA=26:SENSOR_OIC=0:SHELL_TASK=0
```
## To test use something like FT232H on P0.08/P0.06 (D0/D1) (TX/RX) and minicom
Once the nRF52840DK is resetted with the thingy_my_sensor app you should get:
```
000000 my_sensor_app starting!!!                                                                                                                   
000000 my_sensor_app rc = 0                                                                                                                        
000001 my_sensor_app dev YES!!!                                                                                                                    
000173 my_sensor_app bme280_sensor_configure DONE                                                                                                  
000174 sensor_register_listener 0                                                                                                                  
000175 sensor_register_listener 0                                                                                                                  
000176 sensor_register_listener 0                                                                                                                  
000176 my_sensor_app 0                                                                                                                             
000177 my_sensor_app waiting....                                                                                                                   
000436 read_bme280!!!                                                                                                                              
000436 Temperature=23.859 (valid 1)                                                                                                                
000437 Pressure=95200.632 (valid 1)                                                                                                                
000438 Humidity=48.644 (valid 1)                                                                                                                   
000699 read_bme280!!!                                                                                                                              
...
```

## Pullup resistors
I have used 2 4.7kOhm pull up resistors to VDD and power the BME280 with an external 3.3V power supply.
