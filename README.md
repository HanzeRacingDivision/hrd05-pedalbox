# Overview
The pedalbox of the HRD05 reads the sensors installed on the accelerator and brake pedals. There are 2 sensors for each pedal, which are checked for signal plausiblity (see APPS regulations).

A CAN bus network is used to share digital messages among the various systems within the car. 

# Important notes
A proper safe state is a torque request to 0. The inverter **must remain active** via the 'enable' bit `DMC_EnableRq`! If the inverter would be switched off, a huge back-EMF regenerating torque would be created at high speeds. Refer to `docs/Brusa/DMC5_ControlConcept_0.3.pdf`, section 6.1. 

# FSG Rules
The implementation of the rules is described below. Additionally, `@rule` annotations can be found in the source code itself. 

## T 11.8 - APPS signals and plausibility
Two sensors are used for each pedal. The raw signal is software-filtered with a moving average. After comparing the sensor values with the offset given during the startup phase, the APPS signal is generated by taking the average of the 2 sensor values. 

## T 11.8.8 - APPS implausibility safe state
If an implausibility occurs, the requested torque and brake torque are both set to zero. 

## T 11.9.2 - System Critical Signals (SCS)
The maximum delay of digital transmission can at most be 500ms. The safe state is activated if no valid CAN has been received for the duration of the timeout, set by the `CAN_TIMEOUT` constant.

Regarding `T 11.9.2.d`, handling data corruption is provided by the CAN protocol, as it implements a cyclic redundancy check (CRC) / checksum. 

## EV 4.11.6 - Ready to drive mode
The car will respond to APPS when the following conditions are met:
1. The tractive system (TS) status is active, as received from a CAN message from the dashboard.
2. The inverter notified that it's ready to be operated, via CAN bit `DMC_Ready`.
3. The driver has applied pressure to the brake pedal, read using the position sensors.
4. The driver requested to start the car with the button on the dashboard, this status is received via the dash CAN message. 

## EV 4.12 - Ready to drive sound
A buzzer is activated by setting a digital output pin high for 2 seconds. 

## EV 4.11.7 - Exiting ready to drive mode
`@todo`