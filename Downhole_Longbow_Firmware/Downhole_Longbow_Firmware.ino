// #include <MS5803.h>
// #include <SoftI2CMaster.h> 
//
#include <Wire.h>
//#define SDA_PORT PORTA
//#define SDA_PIN 6 // = A4
//#define SCL_PORT PORTA
//#define SCL_PIN 4 // = A5

// SoftI2CMaster Wire = SoftI2CMaster( 4, 6, 0);
#define CMD_RESET 0x1E // reset command
#define CMD_ADC_READ 0x00 // ADC read command
#define CMD_ADC_CONV 0x40 // ADC conversion command

#define CMD_PROM 0xA0 // Coefficient location

#define COEF0 18
#define COEF1 5
#define COEF2 17
#define COEF3 7
#define COEF4 10000
#define COEF5 3
#define COEF6 33
#define COEF7 3
#define COEF8 3
#define COEF9 7
#define COEF10 3
#define COEF11 0
#define COEF12 3
#define COEF13 0
#define COEF14 0
#define COEF15 0

#define PresADR 0x77
#define ADC_ADR 0x68

#define ADR_Print 13

#define SOF '['
#define EOF ']'

uint16_t coefficient[8];// Coefficients;
uint16_t ConvCoef[] = {18, 5, 17, 7, 10000, 3, 33, 3, 3, 7, 3, 0, 3, 0, 0, 0}; //Coefficients for data conversion

int32_t _temperature_actual;
int32_t _pressure_actual;
// MS5803 Pres;

// uint8_t PresADR = 0x77;
// uint8_t ADC_ADR = 0x68;

float Pressure = 0; //Global pressure value
float Temp0 = 0; //Global tempurature from themristor
float Temp1 = 0; //Global tempurature from on board temp

char Val = 0; //FIX!

uint8_t DecFigs = 0; //Used to specify print formating of numbers

// uint8_t ADR_Print = 13; //FIX make this less bad!
char ADR[2] = {'1', '3'}; //Used to read in and check ADR
char Reg[2] = {0}; //Used to read in register ID
char Format = 'f';

void setup() {
	// pinMode(10, OUTPUT); //DEBUG!
	// digitalWrite(10, HIGH); //DEBUG!
	Wire.begin();
	// Pres.begin(0x76, 2);  //Inialize pressure sensor
	// Pres.reset();

	InitADC();

	InitPressure();

	GetValues(); //Update on power on
	Serial.begin(4800); //Start serial comunication over RS-485
//	Serial.println("Vals = ");
//	Serial.println(Pressure);
	// delay(1000); //DEBUG!
	// digitalWrite(10, LOW); //DEBUG!
}

void loop() {

	static char Index = 0;
    char TempADR[2] = {0};
    uint8_t RegID = 0; //Used to store int converted register value
    // char Val;
    while(Serial.read() != SOF); //Wait for SOF character //DEBUG!
    // delay(10);
    // Serial.print(Serial.available()); //DEBUG!
    while(Serial.available() < 4) { //FIX add timeout!  //DEBUG!
        delay(1); //Must remain! I know it seems unessicary, but Scouts Honor, it's not! 
    }; 
    TempADR[0] = Serial.read(); //Read in address
    TempADR[1] = Serial.read();
    // Serial.print(TempADR[0]); //DEBUG!
    // Serial.print(TempADR[1]); //DEBUG!
    if(TempADR[0] == ADR[0] && TempADR[1] == ADR[1]) //Proceed only if address matches 
    {
        Reg[0] = Serial.read();
        Reg[1] = Serial.read();
        // RegID = String(Reg).toInt();
        RegID = (Reg[0] - 48)*10 + (Reg[1] - 48); //Convert Reg to int because frak strings! 
        // sscanf(Reg, "%d", &RegID); //Convert the pair of chars to int
        // while(Serial.read() != EOF); //FIX add timeout! //DEBUG!
        float DataTemp = GetData(RegID);
        // String Msg = String(DataTemp);
        char Msg[10] = {0}; // 9 digits + null termanator? 
        // sprintf(Msg, "%ld", 0x10);
        // dtostrf(DataTemp, 6, 2, Msg);
        // Msg.toCharArray(MsgAry, Msg.length());
        long Temp = long((DataTemp*float(1000.0))); //shift 3 digits left to make it a whole number
        uint8_t Len = log10(Temp); //Get length of number to know when to stop
        // Serial.println(Temp); //DEBUG!
        // Serial.print(Len); //DEBUG!
        for(int i = Len + 1; i > Len - 2; i--) {
            Msg[i] = (Temp % 10) + 48; //Take lowest digit, convert to ASCII char
            Temp /= 10; //Shift to the right one
        } 
        Msg[Len - 2] = '.'; //Add decimal
        for(int p = Len - 3; p >= 0; p--) {
            Msg[p] = (Temp % 10) + 48; //Take lowest digit, convert to ASCII char
            Temp /= 10;
        }
        Serial.print(Msg[0]);
        Serial.print(SOF);
        if(ADR_Print < 10) Serial.print('0'); //Zero pad if needed
        Serial.print(ADR_Print);
        if(RegID < 10) Serial.print('0');  //Zero pad if needed
        Serial.print(RegID);
        // Serial.print(DataTemp, DecFigs);
        Serial.print(Msg); //DEBUG!
        Serial.print(Format);
        Serial.print(GetCRC(Msg, Len + 2));
        Serial.print(EOF);
        Serial.print("\n\r"); //Print formatter
    }
	// if(Serial.available() > 0) { //Check buffer
	// 	Val = Serial.read(); //Read value in
	// }
// 	if(Val != '\r') {
// 		Index = Val;  //Copy desied index
// 	}
// 	if(Val == '\r') { //End of transmission 
// 	    switch(Index) {
// 	    	case '0':
// //          Serial.print("\rf");
// 	    		Serial.print(Pressure);
// //          Serial.print("f\r");
//           digitalWrite(10, HIGH); //DEBUG!
//           delay(200); //DEBUG!
//           digitalWrite(10, LOW); //DEBUG!
// 	    		break;

//     		case '1':
//     			Serial.println(Temp0);
//     			break;

// 			case '2':
// 				Serial.println(Temp1);
// 				break;

// 			case '5':
// 				//FIX! Print Self test
// 				GetValues();
// 				break;
// 	    }
//      Val = NULL; //clear value?? 
//   	}
}

uint8_t GetValues() 
{
	//Update global values
	// Pressure = Pres.getPressure(ADC_4096);
	Pressure = GetPressure();
	Temp0 = GetTemp();
	Temp1 = GetTempDH(); 
    return 0xFF; //FIX to give status indication!
}

void InitADC() 
{
	Wire.beginTransmission(ADC_ADR);
	Wire.write(0x9C); //Set ADC to continuious conversion, 18 bits, 1V/V gain
	Wire.endTransmission();
}

void InitPressure()
{
	uint8_t i;
    uint8_t highByte = 0;
    uint8_t lowByte = 0;

	for(i = 0; i <= 7; i++){
        sendCommand(CMD_PROM + (i * 2));
        Wire.requestFrom(PresADR, 2);
        highByte = Wire.read();
        lowByte = Wire.read();
        coefficient[i] = (highByte << 8)|lowByte;
    }
	sendCommand(CMD_RESET);
	delay(3);
}

float GetPressure()
{
	getMeasurements();
    float pressure_reported;
    pressure_reported = _pressure_actual;
    pressure_reported = pressure_reported / (float(COEF4)/100.0); //05BA model!
    return pressure_reported;
}

float GetTempDH()
{
    getMeasurements();
    float temperature_reported;
    temperature_reported = _temperature_actual / 100.0f;
    return temperature_reported;
}

void getMeasurements()
// Gets resuts from ADC and stores them into internal variables
{
    //Retrieve ADC result
    int32_t temperature_raw = getADCconversion(0x10);
    int32_t pressure_raw = getADCconversion(0x00);

    //Create Variables for calculations
    int32_t temp_calc;
    int32_t pressure_calc;

    int32_t dT;

    //Now that we have a raw temperature, let's compute our actual.
    dT = temperature_raw - ((int32_t)coefficient[5] << 8);
    temp_calc = (((int64_t)dT * coefficient[6]) >> 23) + 2000;

    // TODO TESTING  _temperature_actual = temp_calc;

    //Now we have our first order Temperature, let's calculate the second order.
    int64_t T2, OFF2, SENS2, OFF, SENS; //working variables

    if (temp_calc < 2000)
    // If temp_calc is below 20.0C
        //LOW TEMP
    {
        T2 = ConvCoef[5]* (((int64_t)dT * dT) >> ConvCoef[6]);
        OFF2 = ConvCoef[7] * ((temp_calc - 2000) * (temp_calc - 2000)) / (pow(2,ConvCoef[8]));
        SENS2 = ConvCoef[9] * ((temp_calc - 2000) * (temp_calc - 2000)) / (pow(2,ConvCoef[10]));

        // if(temp_calc < -1500)
        // // If temp_calc is below -15.0C
        //     //VERY LOW TEMP
        // {
        //     OFF2 = OFF2 + ConvCoef[11] * ((temp_calc + 1500) * (temp_calc + 1500));
        //     SENS2 = SENS2 + ConvCoef[12] * ((temp_calc + 1500) * (temp_calc + 1500));
        // }
    }
    else
    // If temp_calc is above 20.0C
        //HIGH TEMP
    {
        T2 = ConvCoef[13] * ((uint64_t)dT * dT)/pow(2,ConvCoef[14]);
        OFF2 = ConvCoef[15]*((temp_calc - 2000) * (temp_calc - 2000)) / 16;
        SENS2 = 0;

        // if(temp_calc > 4500 && Model == 1) SENS2 = SENS2 - ((temp_calc + 1500) * (temp_calc + 1500))/8; //NOTE: this condition is only used for the 01BA model!
    }

    // Now bring it all together to apply offsets


    OFF = ((int64_t)coefficient[2] << ConvCoef[0]) + (((coefficient[4] * (int64_t)dT)) >> ConvCoef[1]);    //05BA model!
    SENS = ((int64_t)coefficient[1] << ConvCoef[2]) + (((coefficient[3] * (int64_t)dT)) >> ConvCoef[3]); //05BA model!
    // OFF = ((int64_t)coefficient[2] << 16) + (((coefficient[4] * (int64_t)dT)) >> 7);
    // SENS = ((int64_t)coefficient[1] << 15) + (((coefficient[3] * (int64_t)dT)) >> 8);

    temp_calc = temp_calc - T2;
    OFF = OFF - OFF2;
    SENS = SENS - SENS2;

    // Now lets calculate the pressure
    pressure_calc = (((SENS * pressure_raw) / 2097152 ) - OFF) / 32768;

    _temperature_actual = temp_calc ;
    _pressure_actual = pressure_calc ; // 10;// pressure_calc;
}

uint32_t getADCconversion(uint8_t _measurement)
// Retrieve ADC measurement from the device.
// Select measurement type and precision
// TODO:  Set up for asynchronous conversion? - ie, w/o waits
{
    uint32_t result;
    uint8_t highByte = 0;
    uint8_t midByte = 0;
    uint8_t lowByte = 0;

    sendCommand(CMD_ADC_CONV + _measurement + 0x08);
    // Wait for conversion to complete
    delay(10);

    sendCommand(CMD_ADC_READ);
    Wire.requestFrom(PresADR, 3);

   while(Wire.available())
   {
        highByte = Wire.read();
        midByte = Wire.read();
        lowByte = Wire.read();
   }

    result = ((uint32_t)highByte << 16) + ((uint32_t)midByte << 8) + lowByte;

    return result;
}

void sendCommand(uint8_t command)
{
    Wire.beginTransmission(PresADR);
    Wire.write(command);
    Wire.endTransmission();
}

float GetTemp()
{
	int Data[4];

	Wire.requestFrom(ADC_ADR, 4);

	if(Wire.available() == 4) //Get data bytes, 3 bytes of potential data and configuration register 
	{
		Data[0] = Wire.read();
		Data[1] = Wire.read();
		Data[2] = Wire.read();
		Data[3] = Wire.read();
	}

	return (((long(Data[0]) & 0x03) << 16) + (long(Data[1]) << 8) + Data[2])*(1.5625e-5);
}

// uint8_t GetCRC(char Data[], uint8_t Len)
// {
//     //FIX dummy function!
//     const unsigned char Poly = 0x8C;
//     unsigned char crc = 0;
//     // union {
//     //     float f;
//     //     uint32_t i;
//     // } Val;
//     // Val.f = Data;
//     // uint32_t Temp = Val.i; //Copy the "float" to a genaric 32 bit value

//     for(int i = 0; i < Len; i++) {
//         crc ^= Data[i];
//         for(int j = 0; j < 9; j++) {
//             if(crc & 1) crc ^= Poly;
//             crc >>= 1;
//         }
//     }
//     // return BitRead(Temp, 32) + BitRead(Temp, 26) + BitRead(Temp, 23) + BitRead(Temp, 22) + BitRead(Temp, 16) + BitRead(Temp, 12) + BitRead(Temp, 11) + BitRead(Temp, 10) \
//     // + BitRead(Temp, 8) + BitRead(Temp, 7) + BitRead(Temp, 5) + BitRead(Temp, 4) + BitRead(Temp, 2) + BitRead(Temp, 1) + 1; 
//     return crc; 
// }

unsigned char GetCRC(const unsigned char * data, const unsigned int size)
{
    unsigned char crc = 0;
    for ( unsigned int i = 0; i < size; ++i )
    {
        unsigned char inbyte = data[i];
        for ( unsigned char j = 0; j < 8; ++j )
        {
            unsigned char mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if ( mix ) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

// byte GetCRC(const byte *data, byte len) {
//   byte crc = 0x00;
//   while (len--) {
//     byte extract = *data++;
//     for (byte tempI = 8; tempI; tempI--) {
//       byte sum = (crc ^ extract) & 0x01;
//       crc >>= 1;
//       if (sum) {
//         crc ^= 0x8C;  //DEBUG! Return to 0x8C
//       }
//       extract >>= 1;
//     }
//   }
//   return crc;
// }

uint8_t BitRead(unsigned int Val, uint8_t Pos)  //macro for reading a bit at a position 
{
    return (Val >> Pos) & 1; 
}

float GetData(uint8_t RegID) 
{
switch(RegID) {
        case 0:
            Format = 'f'; 
            DecFigs = 3; 
            return Pressure;
            break;

        case 1:
            Format = 'f';
            DecFigs = 3; 
            return Temp0;
            break;

        case 2:
            Format = 'f';
            DecFigs = 3; 
            return Temp1;
            break;

        case 5:
            //FIX! Print Self test
            Format = 'b';
            DecFigs = 0; 
            return GetValues();
            break;
    }
}
