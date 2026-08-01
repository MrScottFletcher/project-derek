EESchema Schematic File Version 4
LIBS:power
LIBS:device
LIBS:Connector
EELAYER 29 0
EELAYER END
$Descr A4 11693 8268
Sheet 1 1
Title "Derek Can Interface Board Rev 4"
Date "2026-08-01"
Rev "4"
Comp "Halloween Eye Clusters"
Comment1 "RJ45 pinout copied from Cluster Controller Rev G"
Comment2 "All components through-hole; board 50 mm x 25 mm"
Comment3 "RJ45 is NOT Ethernet"
Comment4 "LED chain enters J4 and returns through J5 to RJ45 pin 8"
$EndDescr
$Comp
L Connector:8P8C J1
U 1 1 61000001
P 1900 3050
F 0 "J1" H 1957 3717 50  0000 C CNN
F 1 "RJ45_FROM_CLUSTER_NOT_ETHERNET" H 1957 3626 50 0000 C CNN
F 2 "Connector_RJ:RJ45_Amphenol_54602-x08_Horizontal" V 1900 3075 50 0001 C CNN
	1    1900 3050
	-1 0 0 -1
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J2
U 1 1 61000002
P 6900 2350
F 0 "J2" H 6980 2392 50 0000 L CNN
F 1 "PAN_SERVO" H 6980 2301 50 0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 6900 2350 50 0001 C CNN
	1    6900 2350
	1 0 0 -1
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J3
U 1 1 61000003
P 6900 3200
F 0 "J3" H 6980 3242 50 0000 L CNN
F 1 "LIFT_SERVO" H 6980 3151 50 0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 6900 3200 50 0001 C CNN
	1    6900 3200
	1 0 0 -1
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J4
U 1 1 61000004
P 6900 4200
F 0 "J4" H 6980 4242 50 0000 L CNN
F 1 "LED_IN_TO_FIRST_LED" H 6980 4151 50 0000 L CNN
F 2 "Connector_JST:JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical" H 6900 4200 50 0001 C CNN
	1    6900 4200
	1 0 0 -1
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J5
U 1 1 61000005
P 6900 5050
F 0 "J5" H 6980 5092 50 0000 L CNN
F 1 "LED_OUT_FROM_LAST_LED" H 6980 5001 50 0000 L CNN
F 2 "Connector_JST:JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical" H 6900 5050 50 0001 C CNN
	1    6900 5050
	1 0 0 -1
$EndComp
$Comp
L Device:R R1
U 1 1 61000006
P 4700 4100
F 0 "R1" V 4493 4100 50 0000 C CNN
F 1 "330R" V 4584 4100 50 0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 4630 4100 50 0001 C CNN
	1    4700 4100
	0 1 1 0
$EndComp
$Comp
L Device:C_Polarized C1
U 1 1 61000007
P 4100 2350
F 0 "C1" H 4218 2396 50 0000 L CNN
F 1 "1000uF 16V 105C" H 4218 2305 50 0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm" H 4138 2200 50 0001 C CNN
	1    4100 2350
	1 0 0 -1
$EndComp
$Comp
L Device:C C2
U 1 1 61000008
P 5300 2350
F 0 "C2" H 5415 2396 50 0000 L CNN
F 1 "100nF 50V X7R" H 5415 2305 50 0000 L CNN
F 2 "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm" H 5338 2200 50 0001 C CNN
	1    5300 2350
	1 0 0 -1
$EndComp
Text Label 2600 2750 0 50 ~ 0
5V
Text Label 2600 2850 0 50 ~ 0
5V
Text Label 2600 2950 0 50 ~ 0
GND
Text Label 2600 3050 0 50 ~ 0
GND
Text Label 2600 3150 0 50 ~ 0
PAN_PWM
Text Label 2600 3250 0 50 ~ 0
LIFT_PWM
Text Label 2600 3350 0 50 ~ 0
LED_FROM_CLUSTER
Text Label 2600 3450 0 50 ~ 0
LED_RETURN_CLUSTER
Wire Wire Line
	2300 2750 3300 2750
Wire Wire Line
	2300 2850 3300 2850
Wire Wire Line
	2300 2950 3300 2950
Wire Wire Line
	2300 3050 3300 3050
Wire Wire Line
	2300 3150 3300 3150
Wire Wire Line
	2300 3250 3300 3250
Wire Wire Line
	2300 3350 4550 3350
Wire Wire Line
	2300 3450 5900 3450
Wire Wire Line
	3300 2750 3300 2850
Connection ~ 3300 2850
Wire Wire Line
	3300 2950 3300 3050
Connection ~ 3300 3050
Wire Wire Line
	3300 2850 4100 2850
Wire Wire Line
	4100 2500 4100 2850
Wire Wire Line
	4100 2200 4100 2000
Wire Wire Line
	4100 2000 5300 2000
Wire Wire Line
	5300 2000 5300 2200
Wire Wire Line
	5300 2500 5300 2850
Wire Wire Line
	5300 2850 4100 2850
Text Label 4450 2000 0 50 ~ 0
5V
Text Label 4450 2850 0 50 ~ 0
GND
Wire Wire Line
	4550 3350 4550 4100
Wire Wire Line
	4850 4100 6700 4100
Text Label 5300 4100 0 50 ~ 0
LED_TO_FIRST
Wire Wire Line
	5900 3450 5900 4950
Wire Wire Line
	5900 4950 6700 4950
Text Label 5950 4950 0 50 ~ 0
LED_RETURN_CLUSTER
Wire Wire Line
	6700 2250 6100 2250
Wire Wire Line
	6700 2350 6100 2350
Wire Wire Line
	6700 2450 6100 2450
Text Label 6100 2250 0 50 ~ 0
GND
Text Label 6100 2350 0 50 ~ 0
5V
Text Label 6100 2450 0 50 ~ 0
PAN_PWM
Wire Wire Line
	6700 3100 6100 3100
Wire Wire Line
	6700 3200 6100 3200
Wire Wire Line
	6700 3300 6100 3300
Text Label 6100 3100 0 50 ~ 0
GND
Text Label 6100 3200 0 50 ~ 0
5V
Text Label 6100 3300 0 50 ~ 0
LIFT_PWM
Wire Wire Line
	6700 4200 6100 4200
Wire Wire Line
	6700 4300 6100 4300
Text Label 6100 4200 0 50 ~ 0
5V
Text Label 6100 4300 0 50 ~ 0
GND
Wire Wire Line
	6700 5050 6100 5050
Wire Wire Line
	6700 5150 6100 5150
Text Label 6100 5050 0 50 ~ 0
5V
Text Label 6100 5150 0 50 ~ 0
GND
Text Notes 1150 3900 0 60 ~ 12
RJ45 PINOUT - NOT ETHERNET:\n1=5V, 2=5V, 3=GND, 4=GND,\n5=PAN PWM, 6=LIFT PWM,\n7=LED DATA IN, 8=LED DATA RETURN
Text Notes 6000 5500 0 50 ~ 0
Servo headers: pin 1 GND, pin 2 +5V, pin 3 PWM\nJST LED connectors: pin 1 DATA, pin 2 +5V, pin 3 GND
Text Notes 3650 3250 0 50 ~ 0
R1 is in series only with incoming LED data.
$EndSCHEMATC
