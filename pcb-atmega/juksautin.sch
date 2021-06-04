EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Pumpunjuksautin"
Date "2021-05-28"
Rev "0.7"
Comp "Oy Pula-ajan Puhelin Ab"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L power:GND #PWR0101
U 1 1 60C357A5
P 3750 2250
F 0 "#PWR0101" H 3750 2000 50  0001 C CNN
F 1 "GND" H 3755 2077 50  0000 C CNN
F 2 "" H 3750 2250 50  0001 C CNN
F 3 "" H 3750 2250 50  0001 C CNN
	1    3750 2250
	1    0    0    -1  
$EndComp
Text Notes 4800 6150 0    50   ~ 0
LOW/HIGH/HI-Z\nemulates NTC
Wire Wire Line
	3350 1950 3300 1950
Wire Wire Line
	3350 1750 3300 1750
Wire Wire Line
	3300 1750 3300 1850
Wire Wire Line
	5650 5850 5650 5950
$Comp
L Device:CP C1
U 1 1 60C357A6
P 5650 6100
F 0 "C1" H 5765 6146 50  0000 L CNN
F 1 "1000u" H 5765 6055 50  0000 L CNN
F 2 "Capacitor_SMD:CP_Elec_8x10" H 5688 5950 50  0001 C CNN
F 3 "~" H 5650 6100 50  0001 C CNN
F 4 "C134755" H 5650 6100 50  0001 C CNN "LCSC"
	1    5650 6100
	1    0    0    -1  
$EndComp
Text GLabel 6050 5850 2    50   UnSpc ~ 0
K9
$Comp
L Device:R R4
U 1 1 60C357A7
P 5350 5850
F 0 "R4" V 5557 5850 50  0000 C CNN
F 1 "200" V 5466 5850 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 5280 5850 50  0001 C CNN
F 3 "~" H 5350 5850 50  0001 C CNN
F 4 "C8218" V 5350 5850 50  0001 C CNN "LCSC"
	1    5350 5850
	0    -1   -1   0   
$EndComp
Connection ~ 5650 5850
Wire Wire Line
	5650 6250 5650 6350
$Comp
L power:GND #PWR0103
U 1 1 613447E6
P 5650 6350
F 0 "#PWR0103" H 5650 6100 50  0001 C CNN
F 1 "GND" H 5655 6177 50  0000 C CNN
F 2 "" H 5650 6350 50  0001 C CNN
F 3 "" H 5650 6350 50  0001 C CNN
	1    5650 6350
	1    0    0    -1  
$EndComp
Wire Wire Line
	3750 1450 3750 1300
$Comp
L power:VCC #PWR0107
U 1 1 60C357A9
P 3750 1300
F 0 "#PWR0107" H 3750 1150 50  0001 C CNN
F 1 "VCC" H 3767 1473 50  0000 C CNN
F 2 "" H 3750 1300 50  0001 C CNN
F 3 "" H 3750 1300 50  0001 C CNN
	1    3750 1300
	1    0    0    -1  
$EndComp
Text GLabel 1700 3900 3    50   3State ~ 0
A
Text GLabel 1800 3900 3    50   3State ~ 0
B
Text GLabel 1600 3900 3    50   UnSpc ~ 0
K9
Text GLabel 1500 3900 3    50   UnSpc ~ 0
K9
Text GLabel 4150 1750 2    50   3State ~ 0
A
Text GLabel 4150 1950 2    50   3State ~ 0
B
Wire Wire Line
	5500 5850 5650 5850
Text GLabel 1400 3900 3    50   3State ~ 0
IO2
Wire Wire Line
	5500 5500 5650 5500
Wire Wire Line
	5650 5500 5650 5850
Wire Notes Line
	2300 5100 2300 6700
Wire Notes Line
	850  6700 850  5100
Wire Notes Line
	6500 5100 6500 6700
Wire Notes Line
	6500 6700 4550 6700
Wire Notes Line
	4550 6700 4550 5100
Wire Notes Line
	4550 5100 6500 5100
Text GLabel 1000 3900 3    50   Output ~ 0
VIN
Text Notes 850  5050 0    50   ~ 0
LED sensing and generic I/O
Text Notes 4550 5050 0    50   ~ 0
Temperature faker
Wire Notes Line
	850  3050 850  4650
Wire Notes Line
	850  4650 1950 4650
Wire Notes Line
	1950 4650 1950 3050
Wire Notes Line
	1950 3050 850  3050
Text Notes 850  3000 0    50   ~ 0
Screw terminal
Connection ~ 3300 1850
Wire Wire Line
	3300 1850 3300 1950
Wire Notes Line
	2800 1000 4450 1000
Wire Notes Line
	4450 1000 4450 2600
Wire Notes Line
	4450 2600 2800 2600
Wire Notes Line
	2800 2600 2800 1000
Text Notes 2800 950  0    50   ~ 0
RS-485
$Comp
L Connector:Screw_Terminal_01x09 J1
U 1 1 60C3579D
P 1400 3700
F 0 "J1" V 1617 3696 50  0000 C CNN
F 1 "Screw_Terminal_01x09" V 1526 3696 50  0000 C CNN
F 2 "Connector_Phoenix_MC:PhoenixContact_MC_1,5_9-G-3.81_1x09_P3.81mm_Horizontal" H 1400 3700 50  0001 C CNN
F 3 "~" H 1400 3700 50  0001 C CNN
	1    1400 3700
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0105
U 1 1 60C3579E
P 1150 6150
F 0 "#PWR0105" H 1150 5900 50  0001 C CNN
F 1 "GND" H 1150 5950 50  0000 C CNN
F 2 "" H 1150 6150 50  0001 C CNN
F 3 "" H 1150 6150 50  0001 C CNN
	1    1150 6150
	1    0    0    -1  
$EndComp
$Comp
L Device:R R7
U 1 1 60A868A1
P 1150 5950
F 0 "R7" H 1080 5904 50  0000 R CNN
F 1 "20k" H 1080 5995 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 1080 5950 50  0001 C CNN
F 3 "~" H 1150 5950 50  0001 C CNN
F 4 "C4184" H 1150 5950 50  0001 C CNN "LCSC"
	1    1150 5950
	-1   0    0    1   
$EndComp
Text GLabel 1200 3900 3    50   Output ~ 0
PUMPFAIL
Text GLabel 2000 6350 2    50   3State ~ 0
IO1
Text GLabel 1300 3900 3    50   3State ~ 0
IO1
Wire Notes Line
	850  5100 2300 5100
Wire Notes Line
	850  6700 2300 6700
Text GLabel 1350 5750 2    50   Input ~ 0
PUMPFAIL
$Comp
L Device:R R5
U 1 1 60C357A4
P 5350 5500
F 0 "R5" V 5557 5500 50  0000 C CNN
F 1 "200" V 5466 5500 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 5280 5500 50  0001 C CNN
F 3 "~" H 5350 5500 50  0001 C CNN
F 4 "C8218" V 5350 5500 50  0001 C CNN "LCSC"
	1    5350 5500
	0    -1   -1   0   
$EndComp
$Comp
L Device:D D5
U 1 1 60BAEA1A
P 3200 3450
F 0 "D5" H 3200 3233 50  0000 C CNN
F 1 "SM4007PL" H 3200 3324 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 3200 3450 50  0001 C CNN
F 3 "~" H 3200 3450 50  0001 C CNN
F 4 "C64898" H 3200 3450 50  0001 C CNN "LCSC"
	1    3200 3450
	-1   0    0    1   
$EndComp
Wire Wire Line
	2900 3450 3050 3450
Wire Wire Line
	3350 3450 3500 3450
$Comp
L power:VCC #PWR08
U 1 1 60BC8E2F
P 8850 850
F 0 "#PWR08" H 8850 700 50  0001 C CNN
F 1 "VCC" H 8865 1023 50  0000 C CNN
F 2 "" H 8850 850 50  0001 C CNN
F 3 "" H 8850 850 50  0001 C CNN
	1    8850 850 
	1    0    0    -1  
$EndComp
Text Label 8200 1550 2    50   ~ 0
AREF
$Comp
L power:GND #PWR09
U 1 1 60BDE19F
P 8850 4250
F 0 "#PWR09" H 8850 4000 50  0001 C CNN
F 1 "GND" H 8855 4077 50  0000 C CNN
F 2 "" H 8850 4250 50  0001 C CNN
F 3 "" H 8850 4250 50  0001 C CNN
	1    8850 4250
	1    0    0    -1  
$EndComp
$Comp
L Device:R R10
U 1 1 60C798A5
P 4850 3550
F 0 "R10" H 4920 3596 50  0000 L CNN
F 1 "10k" H 4920 3505 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4780 3550 50  0001 C CNN
F 3 "~" H 4850 3550 50  0001 C CNN
F 4 "C25804" H 4850 3550 50  0001 C CNN "LCSC"
	1    4850 3550
	1    0    0    -1  
$EndComp
$Comp
L power:VCC #PWR07
U 1 1 60C7D8C2
P 4850 3400
F 0 "#PWR07" H 4850 3250 50  0001 C CNN
F 1 "VCC" H 4865 3573 50  0000 C CNN
F 2 "" H 4850 3400 50  0001 C CNN
F 3 "" H 4850 3400 50  0001 C CNN
	1    4850 3400
	1    0    0    -1  
$EndComp
Text Label 2900 3450 0    50   ~ 0
VBUS
Wire Wire Line
	4850 3800 4850 3700
Text Label 4200 3800 0    50   ~ 0
RESET_IN
Text Label 9450 1850 0    50   ~ 0
MOSI
Text Label 9450 1950 0    50   ~ 0
MISO
Text Label 9450 2050 0    50   ~ 0
SCK
$Comp
L Device:C_Small C7
U 1 1 60B7073A
P 7600 1700
F 0 "C7" H 7692 1746 50  0000 L CNN
F 1 "100n" H 7692 1655 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 7600 1700 50  0001 C CNN
F 3 "~" H 7600 1700 50  0001 C CNN
F 4 "C1525" H 7600 1700 50  0001 C CNN "LCSC"
	1    7600 1700
	1    0    0    -1  
$EndComp
$Comp
L Interface_UART:SP3485EN U1
U 1 1 60BB9B57
P 3750 1850
F 0 "U1" H 3650 2400 50  0000 C CNN
F 1 "SP3485EN" H 3500 2300 50  0000 C CNN
F 2 "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm" H 4800 1500 50  0001 C CIN
F 3 "http://www.icbase.com/pdf/SPX/SPX00480106.pdf" H 3750 1850 50  0001 C CNN
F 4 "C8963" H 3750 1850 50  0001 C CNN "LCSC"
	1    3750 1850
	1    0    0    -1  
$EndComp
Wire Wire Line
	10850 1800 10950 1800
$Comp
L Device:C_Small C4
U 1 1 60B5C443
P 10750 1800
F 0 "C4" V 10521 1800 50  0000 C CNN
F 1 "22p" V 10612 1800 50  0000 C CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 10750 1800 50  0001 C CNN
F 3 "~" H 10750 1800 50  0001 C CNN
F 4 "C1555" V 10750 1800 50  0001 C CNN "LCSC"
	1    10750 1800
	0    -1   1    0   
$EndComp
Wire Wire Line
	10950 1800 10950 2250
$Comp
L power:GND #PWR02
U 1 1 60B9E570
P 10300 3550
F 0 "#PWR02" H 10300 3300 50  0001 C CNN
F 1 "GND" H 10305 3377 50  0000 C CNN
F 2 "" H 10300 3550 50  0001 C CNN
F 3 "" H 10300 3550 50  0001 C CNN
	1    10300 3550
	-1   0    0    -1  
$EndComp
Wire Wire Line
	10850 2250 10950 2250
$Comp
L Device:C_Small C5
U 1 1 60B5D62C
P 10750 2250
F 0 "C5" V 10521 2250 50  0000 C CNN
F 1 "22p" V 10612 2250 50  0000 C CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 10750 2250 50  0001 C CNN
F 3 "~" H 10750 2250 50  0001 C CNN
F 4 "C1555" V 10750 2250 50  0001 C CNN "LCSC"
	1    10750 2250
	0    -1   1    0   
$EndComp
$Comp
L power:GND #PWR0104
U 1 1 60C90861
P 7600 1800
F 0 "#PWR0104" H 7600 1550 50  0001 C CNN
F 1 "GND" H 7605 1627 50  0000 C CNN
F 2 "" H 7600 1800 50  0001 C CNN
F 3 "" H 7600 1800 50  0001 C CNN
	1    7600 1800
	1    0    0    -1  
$EndComp
$Comp
L Power_Protection:SRV05-4 U2
U 1 1 60CDD1DA
P 3300 5900
F 0 "U2" H 3150 6550 50  0000 R CNN
F 1 "SRV05-4" H 3150 6450 50  0000 R CNN
F 2 "Package_TO_SOT_SMD:SOT-23-6" H 4000 5450 50  0001 C CNN
F 3 "http://www.onsemi.com/pub/Collateral/SRV05-4-D.PDF" H 3300 5900 50  0001 C CNN
F 4 "C85364" H 3300 5900 50  0001 C CNN "LCSC"
	1    3300 5900
	1    0    0    -1  
$EndComp
Text GLabel 2800 5800 0    50   3State ~ 0
IO1
Text GLabel 2800 6000 0    50   3State ~ 0
IO2
Text GLabel 3800 6000 2    50   Input ~ 0
PUMPFAIL
Text GLabel 3800 5800 2    50   UnSpc ~ 0
K9
$Comp
L power:GND #PWR011
U 1 1 60CF9B5F
P 3300 6400
F 0 "#PWR011" H 3300 6150 50  0001 C CNN
F 1 "GND" H 3305 6227 50  0000 C CNN
F 2 "" H 3300 6400 50  0001 C CNN
F 3 "" H 3300 6400 50  0001 C CNN
	1    3300 6400
	1    0    0    -1  
$EndComp
$Comp
L power:VCC #PWR010
U 1 1 60CFC8E7
P 3300 5400
F 0 "#PWR010" H 3300 5250 50  0001 C CNN
F 1 "VCC" H 3315 5573 50  0000 C CNN
F 2 "" H 3300 5400 50  0001 C CNN
F 3 "" H 3300 5400 50  0001 C CNN
	1    3300 5400
	1    0    0    -1  
$EndComp
Wire Wire Line
	1150 6100 1150 6150
Text GLabel 2000 6250 2    50   3State ~ 0
IO2
$Comp
L Device:R R3
U 1 1 60B69E15
P 1850 6250
F 0 "R3" V 2057 6250 50  0000 C CNN
F 1 "200" V 1966 6250 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 1780 6250 50  0001 C CNN
F 3 "~" H 1850 6250 50  0001 C CNN
F 4 "C8218" V 1850 6250 50  0001 C CNN "LCSC"
	1    1850 6250
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1700 6250 1400 6250
$Comp
L Device:R R2
U 1 1 60D6EBF4
P 1850 6350
F 0 "R2" V 1735 6350 50  0000 C CNN
F 1 "200" V 1644 6350 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 1780 6350 50  0001 C CNN
F 3 "~" H 1850 6350 50  0001 C CNN
F 4 "C8218" V 1850 6350 50  0001 C CNN "LCSC"
	1    1850 6350
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1400 6350 1700 6350
Wire Wire Line
	1150 5800 1150 5750
Wire Wire Line
	5650 5850 6050 5850
Text Label 9450 2850 0    50   ~ 0
IO2_M
Text Label 9450 2750 0    50   ~ 0
IO1_M
Text Label 1400 6250 0    50   ~ 0
IO2_M
Text Label 1400 6350 0    50   ~ 0
IO1_M
Text Label 9450 2450 0    50   ~ 0
FB
Text Label 9450 2550 0    50   ~ 0
DRIVE
Text Label 5000 5500 0    50   ~ 0
FB
Text Label 5000 5850 0    50   ~ 0
DRIVE
Wire Wire Line
	5000 5500 5200 5500
Wire Wire Line
	5000 5850 5200 5850
Text Label 9450 2650 0    50   ~ 0
PUMPFAIL_M
$Comp
L Device:R R1
U 1 1 60C357A1
P 1150 5550
F 0 "R1" H 1220 5596 50  0000 L CNN
F 1 "200" H 1220 5505 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 1080 5550 50  0001 C CNN
F 3 "~" H 1150 5550 50  0001 C CNN
F 4 "C8218" H 1150 5550 50  0001 C CNN "LCSC"
	1    1150 5550
	1    0    0    -1  
$EndComp
Text Label 3050 1650 0    50   ~ 0
RX
Wire Wire Line
	3050 1650 3350 1650
Text Label 3050 2050 0    50   ~ 0
TX
Wire Wire Line
	3050 2050 3350 2050
Text Label 9450 3250 0    50   ~ 0
RX
Text Label 9450 3350 0    50   ~ 0
TX
Text Label 9450 3450 0    50   ~ 0
TX_EN
Text Label 3050 1850 0    50   ~ 0
TX_EN
Wire Wire Line
	3050 1850 3300 1850
Text Label 1150 5400 0    50   ~ 0
PUMPFAIL_M
Wire Wire Line
	1350 5750 1150 5750
Connection ~ 1150 5750
Wire Wire Line
	1150 5750 1150 5700
Text Label 9450 3050 0    50   ~ 0
RESET
$Comp
L power:GND #PWR0102
U 1 1 60EDAA2A
P 4850 4150
F 0 "#PWR0102" H 4850 3900 50  0001 C CNN
F 1 "GND" H 4855 3977 50  0000 C CNN
F 2 "" H 4850 4150 50  0001 C CNN
F 3 "" H 4850 4150 50  0001 C CNN
	1    4850 4150
	1    0    0    -1  
$EndComp
$Comp
L Jumper:SolderJumper_2_Open JP1
U 1 1 60EE7989
P 4850 4000
F 0 "JP1" V 4804 4068 50  0000 L CNN
F 1 "SolderJumper_2_Open" V 4895 4068 50  0000 L CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm" H 4850 4000 50  0001 C CNN
F 3 "~" H 4850 4000 50  0001 C CNN
	1    4850 4000
	0    1    1    0   
$EndComp
Wire Wire Line
	9450 2150 9700 2150
Wire Wire Line
	9700 2150 9700 1800
Wire Wire Line
	7600 1550 8250 1550
Wire Wire Line
	7600 1550 7600 1600
$Comp
L power:VCC #PWR05
U 1 1 60BAC777
P 3500 3450
F 0 "#PWR05" H 3500 3300 50  0001 C CNN
F 1 "VCC" H 3517 3623 50  0000 C CNN
F 2 "" H 3500 3450 50  0001 C CNN
F 3 "" H 3500 3450 50  0001 C CNN
	1    3500 3450
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0108
U 1 1 61456AC1
P 1100 3900
F 0 "#PWR0108" H 1100 3650 50  0001 C CNN
F 1 "GND" V 1100 3700 50  0000 C CNN
F 2 "" H 1100 3900 50  0001 C CNN
F 3 "" H 1100 3900 50  0001 C CNN
	1    1100 3900
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0109
U 1 1 60C41985
P 1550 2100
F 0 "#PWR0109" H 1550 1850 50  0001 C CNN
F 1 "GND" H 1555 1927 50  0000 C CNN
F 2 "" H 1550 2100 50  0001 C CNN
F 3 "" H 1550 2100 50  0001 C CNN
	1    1550 2100
	1    0    0    -1  
$EndComp
Text GLabel 2250 1750 2    50   Input ~ 0
VIN
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 60C4198C
P 2000 1650
F 0 "#FLG0101" H 2000 1725 50  0001 C CNN
F 1 "PWR_FLAG" H 2000 1823 50  0000 C CNN
F 2 "" H 2000 1650 50  0001 C CNN
F 3 "~" H 2000 1650 50  0001 C CNN
	1    2000 1650
	1    0    0    -1  
$EndComp
$Comp
L Regulator_Switching:R-78E5.0-0.5 U3
U 1 1 60C41992
P 1550 1750
F 0 "U3" H 1550 1992 50  0000 C CNN
F 1 "R-78E5.0-0.5" H 1550 1901 50  0000 C CNN
F 2 "Converter_DCDC:Converter_DCDC_RECOM_R-78E-0.5_THT" H 1600 1500 50  0001 L CIN
F 3 "https://www.recom-power.com/pdf/Innoline/R-78Exx-0.5.pdf" H 1550 1750 50  0001 C CNN
	1    1550 1750
	-1   0    0    -1  
$EndComp
Wire Wire Line
	1850 1750 2000 1750
Wire Wire Line
	2000 1650 2000 1750
Connection ~ 2000 1750
Wire Wire Line
	2000 1750 2250 1750
Wire Wire Line
	1550 2050 1550 2100
Connection ~ 1550 2100
Wire Wire Line
	2000 2050 2000 2100
Wire Wire Line
	2000 2100 1550 2100
$Comp
L power:VCC #PWR0111
U 1 1 60C419A0
P 1050 1650
F 0 "#PWR0111" H 1050 1500 50  0001 C CNN
F 1 "VCC" H 1065 1823 50  0000 C CNN
F 2 "" H 1050 1650 50  0001 C CNN
F 3 "" H 1050 1650 50  0001 C CNN
	1    1050 1650
	1    0    0    -1  
$EndComp
Wire Wire Line
	1250 1750 1050 1750
Wire Notes Line
	850  1000 2600 1000
Wire Notes Line
	2600 1000 2600 2600
Wire Notes Line
	850  2600 850  1000
Text Notes 850  950  0    50   ~ 0
Voltage supply
$Comp
L Device:C C2
U 1 1 60C419AC
P 1050 1900
F 0 "C2" H 1165 1946 50  0000 L CNN
F 1 "10u" H 1165 1855 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 1088 1750 50  0001 C CNN
F 3 "~" H 1050 1900 50  0001 C CNN
F 4 "C15525" H 1050 1900 50  0001 C CNN "LCSC"
	1    1050 1900
	1    0    0    -1  
$EndComp
$Comp
L Device:C C3
U 1 1 60C419B2
P 2000 1900
F 0 "C3" H 2115 1946 50  0000 L CNN
F 1 "10u" H 2115 1855 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric" H 2038 1750 50  0001 C CNN
F 3 "~" H 2000 1900 50  0001 C CNN
F 4 "C13585" H 2000 1900 50  0001 C CNN "LCSC"
	1    2000 1900
	1    0    0    -1  
$EndComp
Connection ~ 1050 1750
Wire Wire Line
	1550 2100 1050 2100
Wire Wire Line
	1050 2100 1050 2050
Wire Wire Line
	1050 1650 1050 1750
Wire Notes Line
	850  2600 2600 2600
$Comp
L MCU_Microchip_ATmega:ATmega328P-AU U4
U 1 1 60CFDBF2
P 8850 2750
F 0 "U4" H 8950 1250 50  0000 L CNN
F 1 "ATmega328P-AU" H 8950 1150 50  0000 L CNN
F 2 "Package_QFP:TQFP-32_7x7mm_P0.8mm" H 8850 2750 50  0001 C CIN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/ATmega328_P%20AVR%20MCU%20with%20picoPower%20Technology%20Data%20Sheet%2040001984A.pdf" H 8850 2750 50  0001 C CNN
	1    8850 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	8850 1200 8950 1200
Wire Wire Line
	8950 1200 8950 1250
Connection ~ 8850 1200
Wire Wire Line
	8850 1200 8850 1250
$Comp
L Connector:AVR-ISP-6 J3
U 1 1 60D6CDE9
P 3000 3950
F 0 "J3" H 2671 4046 50  0000 R CNN
F 1 "AVR-ISP-6" H 2671 3955 50  0000 R CNN
F 2 "Connector_IDC:IDC-Header_2x03_P2.54mm_Horizontal" V 2750 4000 50  0001 C CNN
F 3 " ~" H 1725 3400 50  0001 C CNN
	1    3000 3950
	1    0    0    -1  
$EndComp
Text Label 3400 3850 0    50   ~ 0
MOSI
Text Label 3400 3750 0    50   ~ 0
MISO
Text Label 3400 3950 0    50   ~ 0
SCK
$Comp
L power:GND #PWR0112
U 1 1 60D75514
P 2900 4350
F 0 "#PWR0112" H 2900 4100 50  0001 C CNN
F 1 "GND" H 2905 4177 50  0000 C CNN
F 2 "" H 2900 4350 50  0001 C CNN
F 3 "" H 2900 4350 50  0001 C CNN
	1    2900 4350
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C8
U 1 1 60D999E8
P 9150 1000
F 0 "C8" H 9242 1046 50  0000 L CNN
F 1 "100n" H 9242 955 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 9150 1000 50  0001 C CNN
F 3 "~" H 9150 1000 50  0001 C CNN
F 4 "C1525" H 9150 1000 50  0001 C CNN "LCSC"
	1    9150 1000
	1    0    0    -1  
$EndComp
Wire Wire Line
	8850 850  8850 1200
$Comp
L power:GND #PWR0113
U 1 1 60D9F22E
P 9150 1100
F 0 "#PWR0113" H 9150 850 50  0001 C CNN
F 1 "GND" H 9155 927 50  0000 C CNN
F 2 "" H 9150 1100 50  0001 C CNN
F 3 "" H 9150 1100 50  0001 C CNN
	1    9150 1100
	-1   0    0    -1  
$EndComp
Wire Wire Line
	9150 900  9150 850 
Wire Wire Line
	9150 850  8850 850 
Connection ~ 8850 850 
$Comp
L Device:C_Small C6
U 1 1 60DBB221
P 4650 3800
F 0 "C6" V 4879 3800 50  0000 C CNN
F 1 "100n" V 4788 3800 50  0000 C CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 4650 3800 50  0001 C CNN
F 3 "~" H 4650 3800 50  0001 C CNN
F 4 "C1525" H 4650 3800 50  0001 C CNN "LCSC"
	1    4650 3800
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4750 3800 4850 3800
Connection ~ 4850 3800
Wire Wire Line
	4550 3800 4200 3800
Text Label 5150 3800 2    50   ~ 0
RESET
Wire Wire Line
	4850 3800 5150 3800
Text Label 3400 4050 0    50   ~ 0
RESET_IN
Wire Wire Line
	4850 3850 4850 3800
NoConn ~ 9450 3650
NoConn ~ 9450 3750
NoConn ~ 9450 2950
NoConn ~ 9450 3850
Wire Wire Line
	10500 1800 9700 1800
Connection ~ 10500 1800
Wire Wire Line
	10650 1800 10500 1800
Wire Wire Line
	10500 2250 9450 2250
Connection ~ 10500 2250
Wire Wire Line
	10650 2250 10500 2250
Wire Wire Line
	10500 2150 10500 2250
Wire Wire Line
	10500 1800 10500 1850
$Comp
L Device:Crystal Y1
U 1 1 60B59932
P 10500 2000
F 0 "Y1" V 10546 1869 50  0000 R CNN
F 1 "X322516MLB4SI" V 10455 1869 50  0000 R CNN
F 2 "Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm" H 10500 2000 50  0001 C CNN
F 3 "~" H 10500 2000 50  0001 C CNN
F 4 "C13738" V 10500 2000 50  0001 C CNN "LCSC"
	1    10500 2000
	0    1    -1   0   
$EndComp
Text Notes 2500 5050 0    50   ~ 0
ESD protection
Wire Notes Line
	4350 5100 4350 6700
Wire Notes Line
	4350 6700 2500 6700
Wire Notes Line
	2500 6700 2500 5100
Wire Notes Line
	2500 5100 4350 5100
Text Notes 2150 3000 0    50   ~ 0
AVR-ISP
Wire Notes Line
	2150 3050 2150 4650
Wire Notes Line
	2150 4650 3900 4650
Wire Notes Line
	3900 4650 3900 3050
Wire Notes Line
	3900 3050 2150 3050
Wire Notes Line
	4100 3050 4100 4650
Wire Notes Line
	4100 4650 5900 4650
Wire Notes Line
	5900 4650 5900 3050
Wire Notes Line
	5900 3050 4100 3050
Text Notes 4100 3000 0    50   ~ 0
Reset
Text Label 9350 5250 3    50   ~ 0
AREF
$Comp
L power:GND #PWR0114
U 1 1 60FD1429
P 9450 5550
F 0 "#PWR0114" H 9450 5300 50  0001 C CNN
F 1 "GND" H 9455 5377 50  0000 C CNN
F 2 "" H 9450 5550 50  0001 C CNN
F 3 "" H 9450 5550 50  0001 C CNN
	1    9450 5550
	-1   0    0    -1  
$EndComp
Text Label 9250 5250 3    50   ~ 0
INO_A6
Wire Wire Line
	9450 5550 9450 5250
Text Label 8250 1850 2    50   ~ 0
INO_A7
Text Label 9550 5250 3    50   ~ 0
INO_A7
Text Label 9450 1750 0    50   ~ 0
INO_D10
Text Label 9050 5250 3    50   ~ 0
INO_D10
$Comp
L Connector_Generic:Conn_01x09 J4
U 1 1 610D26CC
P 9150 5050
F 0 "J4" V 9367 5046 50  0000 C CNN
F 1 "Conn_01x09" V 9276 5046 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x09_P2.54mm_Vertical" H 9150 5050 50  0001 C CNN
F 3 "~" H 9150 5050 50  0001 C CNN
	1    9150 5050
	0    -1   -1   0   
$EndComp
$Comp
L power:VCC #PWR0115
U 1 1 610E1226
P 9150 5250
F 0 "#PWR0115" H 9150 5100 50  0001 C CNN
F 1 "VCC" H 9165 5423 50  0000 C CNN
F 2 "" H 9150 5250 50  0001 C CNN
F 3 "" H 9150 5250 50  0001 C CNN
	1    9150 5250
	-1   0    0    1   
$EndComp
Text Label 8950 5250 3    50   ~ 0
INO_D9
Text Label 8850 5250 3    50   ~ 0
INO_D8
Text Label 8750 5250 3    50   ~ 0
INO_D7
Text Label 8250 1750 2    50   ~ 0
INO_A6
Text Label 9450 1650 0    50   ~ 0
INO_D9
Text Label 9450 1550 0    50   ~ 0
INO_D8
Text Label 9450 3950 0    50   ~ 0
INO_D7
$Comp
L Device:LED D1
U 1 1 61105A7E
P 10150 3550
F 0 "D1" H 10143 3295 50  0000 C CNN
F 1 "LED" H 10143 3386 50  0000 C CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 10150 3550 50  0001 C CNN
F 3 "~" H 10150 3550 50  0001 C CNN
F 4 "C72043" H 10150 3550 50  0001 C CNN "LCSC"
	1    10150 3550
	-1   0    0    1   
$EndComp
$Comp
L Device:R R6
U 1 1 611065FB
P 9850 3550
F 0 "R6" V 9643 3550 50  0000 C CNN
F 1 "300" V 9734 3550 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 9780 3550 50  0001 C CNN
F 3 "~" H 9850 3550 50  0001 C CNN
F 4 "C23025" V 9850 3550 50  0001 C CNN "LCSC"
	1    9850 3550
	0    1    1    0   
$EndComp
Wire Wire Line
	9450 3550 9700 3550
$Comp
L power:GND #PWR?
U 1 1 6111F566
P 10950 2250
F 0 "#PWR?" H 10950 2000 50  0001 C CNN
F 1 "GND" H 10955 2077 50  0000 C CNN
F 2 "" H 10950 2250 50  0001 C CNN
F 3 "" H 10950 2250 50  0001 C CNN
	1    10950 2250
	-1   0    0    -1  
$EndComp
Connection ~ 10950 2250
$EndSCHEMATC
