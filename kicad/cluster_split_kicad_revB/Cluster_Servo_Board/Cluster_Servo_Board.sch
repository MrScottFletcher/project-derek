EESchema Schematic File Version 4
LIBS:power
LIBS:device
LIBS:Connector_Generic
EELAYER 29 0
EELAYER END
$Descr A3 16535 11693
Sheet 1 1
Title "Cluster Servo Board"
Date "2026-08-07"
Rev "B"
Comp "Derek Eyes-in-Cans Cluster Controller"
Comment1 "PCA9685 module, eight Derek RJ45s, 5V high-current plane, LED level shifter"
Comment2 "RJ45 CONNECTORS ARE NOT ETHERNET"
$EndDescr
Text Notes 700 550 0 140 ~ 28
CLUSTER SERVO BOARD - REV B
Text Notes 700 800 0 80 ~ 16
High-current board. 8 Derek ports = 16 servos. Separate 5V input. 4-layer recommended.
Text Notes 750 1200 0 80 ~ 16
J1 - JST-XH-8 FROM LOGIC BOARD
$Comp
L Connector_Generic:Conn_01x08 J1
U 1 1 1
P 1900 1900
F 0 "J1" H 1818 2417 50 0000 C CNN
F 1 "LOGIC INTERCONNECT" H 1818 2326 50 0000 C CNN
	1    1900 1900
	-1 0 0 -1
$EndComp
Text Label 2500 1600 0 50 ~ 0
GND
Text Label 2500 1700 0 50 ~ 0
GND
Text Label 2500 1800 0 50 ~ 0
+3V3_LOGIC
Text Label 2500 1900 0 50 ~ 0
I2C_SDA
Text Label 2500 2000 0 50 ~ 0
I2C_SCL
Text Label 2500 2100 0 50 ~ 0
LED_DATA_3V3
Text Label 2500 2200 0 50 ~ 0
PCA_OE_N
Text Label 2500 2300 0 50 ~ 0
SPARE
Text Notes 4300 1200 0 80 ~ 16
U1 - ADAFRUIT PCA9685 16-CHANNEL SERVO MODULE
Text Notes 4300 1450 0 60 ~ 12
VCC = +3V3_LOGIC; V+ = +5V_SERVO; GND common; SDA/SCL from J1; OE from J1
$Comp
L Connector_Generic:Conn_01x16 U1
U 1 1 2
P 5900 2300
F 0 "U1" H 5980 2292 50 0000 L CNN
F 1 "Adafruit PCA9685 PWM CH0..CH15" H 5980 2201 50 0000 L CNN
	1    5900 2300
	1 0 0 -1
$EndComp
Text Notes 8300 1200 0 80 ~ 16
U2 - 74AHCT125N LED LEVEL SHIFTER
$Comp
L 74xx:74AHCT125 U2
U 1 1 3
P 9000 2000
F 0 "U2" H 9000 2317 50 0000 C CNN
F 1 "74AHCT125" H 9000 2226 50 0000 C CNN
	1    9000 2000
	1 0 0 -1
$EndComp
Text Label 8450 2000 2 50 ~ 0
LED_DATA_3V3
Text Label 9550 2000 0 50 ~ 0
LED_5V
Text Notes 8200 2600 0 50 ~ 10
Power U2 from +5V_SERVO; 100nF directly at VCC/GND. Tie gate OE low.
Text Notes 11300 1200 0 80 ~ 16
HIGH-CURRENT 5V INPUT
$Comp
L Connector_Generic:Conn_01x02 J10
U 1 1 4
P 12100 1750
F 0 "J10" H 12180 1742 50 0000 L CNN
F 1 "5V/GND INPUT A" H 12180 1651 50 0000 L CNN
	1    12100 1750
	1 0 0 -1
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J11
U 1 1 5
P 12100 2250
F 0 "J11" H 12180 2242 50 0000 L CNN
F 1 "5V/GND INPUT B" H 12180 2151 50 0000 L CNN
	1    12100 2250
	1 0 0 -1
$EndComp
Text Notes 11300 2750 0 50 ~ 10
J10/J11 are parallel. Use high-current terminals and heavy conductors. Board ceiling ~15A.
Text Notes 700 3500 0 90 ~ 18
DEREK RJ45 PORTS - NOT ETHERNET
Text Notes 700 3750 0 60 ~ 12
Every port: 1 +5V, 2 +5V, 3 GND, 4 GND, 5 PAN PWM, 6 LIFT PWM, 7 LED DATA IN, 8 LED DATA OUT
Text Notes 1200 4100 0 55 ~ 11
DEREK 1 / J2\nPAN=PWM0 LIFT=PWM1
$Comp
L Connector_Generic:Conn_01x08 J2
U 1 1 21
P 1200 5000
F 0 "J2" H 1280 4992 50 0000 L CNN
F 1 "RJ45 DEREK 1" H 1280 4901 50 0000 L CNN
	1    1200 5000
	1 0 0 -1
$EndComp
Text Notes 2950 4100 0 55 ~ 11
DEREK 2 / J3\nPAN=PWM2 LIFT=PWM3
$Comp
L Connector_Generic:Conn_01x08 J3
U 1 1 22
P 2950 5000
F 0 "J3" H 3030 4992 50 0000 L CNN
F 1 "RJ45 DEREK 2" H 3030 4901 50 0000 L CNN
	1    2950 5000
	1 0 0 -1
$EndComp
Text Notes 4700 4100 0 55 ~ 11
DEREK 3 / J4\nPAN=PWM4 LIFT=PWM5
$Comp
L Connector_Generic:Conn_01x08 J4
U 1 1 23
P 4700 5000
F 0 "J4" H 4780 4992 50 0000 L CNN
F 1 "RJ45 DEREK 3" H 4780 4901 50 0000 L CNN
	1    4700 5000
	1 0 0 -1
$EndComp
Text Notes 6450 4100 0 55 ~ 11
DEREK 4 / J5\nPAN=PWM6 LIFT=PWM7
$Comp
L Connector_Generic:Conn_01x08 J5
U 1 1 24
P 6450 5000
F 0 "J5" H 6530 4992 50 0000 L CNN
F 1 "RJ45 DEREK 4" H 6530 4901 50 0000 L CNN
	1    6450 5000
	1 0 0 -1
$EndComp
Text Notes 8200 4100 0 55 ~ 11
DEREK 5 / J6\nPAN=PWM8 LIFT=PWM9
$Comp
L Connector_Generic:Conn_01x08 J6
U 1 1 25
P 8200 5000
F 0 "J6" H 8280 4992 50 0000 L CNN
F 1 "RJ45 DEREK 5" H 8280 4901 50 0000 L CNN
	1    8200 5000
	1 0 0 -1
$EndComp
Text Notes 9950 4100 0 55 ~ 11
DEREK 6 / J7\nPAN=PWM10 LIFT=PWM11
$Comp
L Connector_Generic:Conn_01x08 J7
U 1 1 26
P 9950 5000
F 0 "J7" H 10030 4992 50 0000 L CNN
F 1 "RJ45 DEREK 6" H 10030 4901 50 0000 L CNN
	1    9950 5000
	1 0 0 -1
$EndComp
Text Notes 11700 4100 0 55 ~ 11
DEREK 7 / J8\nPAN=PWM12 LIFT=PWM13
$Comp
L Connector_Generic:Conn_01x08 J8
U 1 1 27
P 11700 5000
F 0 "J8" H 11780 4992 50 0000 L CNN
F 1 "RJ45 DEREK 7" H 11780 4901 50 0000 L CNN
	1    11700 5000
	1 0 0 -1
$EndComp
Text Notes 13450 4100 0 55 ~ 11
DEREK 8 / J9\nPAN=PWM14 LIFT=PWM15
$Comp
L Connector_Generic:Conn_01x08 J9
U 1 1 28
P 13450 5000
F 0 "J9" H 13530 4992 50 0000 L CNN
F 1 "RJ45 DEREK 8" H 13530 4901 50 0000 L CNN
	1    13450 5000
	1 0 0 -1
$EndComp
Text Notes 700 6100 0 80 ~ 16
LED DATA CHAIN / BYPASS JUMPERS
Text Notes 700 6400 0 60 ~ 12
LED_5V -> J2 pin7. Each Derek returns data on pin8. JP1..JP7 selects NORMAL(returned output) or BYPASS(current input) for the next Derek.
Text Notes 700 6900 0 60 ~ 12
JP1: J2 OUT / J3 IN / J2 IN     JP2: J3 OUT / J4 IN / J3 IN     JP3: J4 OUT / J5 IN / J4 IN
Text Notes 700 7200 0 60 ~ 12
JP4: J5 OUT / J6 IN / J5 IN     JP5: J6 OUT / J7 IN / J6 IN     JP6: J7 OUT / J8 IN / J7 IN     JP7: J8 OUT / J9 IN / J8 IN
Text Notes 700 7900 0 80 ~ 16
POWER DECOUPLING
Text Notes 700 8200 0 60 ~ 12
C1-C4: 1000uF/10V low-ESR, one per two Derek ports. C5-C12: 100nF branch bypass. C13: 100nF at U2. C14: 10uF local logic bulk.
Text Notes 700 8800 0 70 ~ 14
PCB STACKUP: 4 layers recommended — F.Cu signals/pours, In1 +5V plane, In2 GND plane, B.Cu signals/pours.\nUse 2 oz outer copper if practical, generous via stitching at every RJ45 power pad, and verify connector current ratings.
Text Notes 700 9700 0 90 ~ 18
WARNING: THESE RJ45 CONNECTORS CARRY 5V SERVO/LED POWER. THEY ARE NOT ETHERNET PORTS.
$EndSCHEMATC
