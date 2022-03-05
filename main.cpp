#include "mbed.h"
#include <cstdio>

// The earth's magnetic field varies according to its location.
// Add or subtract a constant to get the right value
// of the magnetic field using the following site
// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION 9.9 // declination (in degrees) in YOUR_LOCATION.

//Functions
void CMPS2_read_XYZ();
void CMPS2_init();
void CMPS2_set(char reset);
float CMPS2_getHeading();
void CMPS2_decodeHeading(float measured_angle);


I2C i2compass(PB_14, PB_13);
// I2C slave address
//const char compasSlaveAddr = 0x30;
int RWcompasSlaveAddr = 0x60;

char buf[2];
//float Max[2], Mid[2], Min[2], X, Y;     // index 1 : x , index 2 : y or 

float data[3];
unsigned int raw[3];
float offset[3];

int main() {


    i2compass.frequency(400000);
    // Pmod Compass is init. here
    ThisThread::sleep_for(500ms);
    CMPS2_init(); //initialize the compass
    ThisThread::sleep_for(500ms);
    printf("sleep ended\n");
    while (1) {

        printf("%d\n", i2compass.write(RWcompasSlaveAddr, buf, 1));
        float measured_angle = CMPS2_getHeading();
        printf("Heading = %f°\t\n", measured_angle);
        CMPS2_decodeHeading(measured_angle);    //get direction
        ThisThread::sleep_for(200ms);
    }
}

/***********************************
** Pmod Compass functions defined here **
************************************/

//reads measurements in mG
void CMPS2_read_XYZ(){
    
    //command internal control register 0 bit 0 (measure)
    buf[0] = 0x07;
    buf[1] = 0x01;
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(8ms);

    //wait for measurement to be completed
    bool flag = false;
    while (!flag) {
        //jump to status register
        buf[0] = 0x06;
        i2compass.write(RWcompasSlaveAddr, buf, 1);

        //read its value
        i2compass.read(RWcompasSlaveAddr, buf, 1);
        int temporal = buf[0];
        //ThisThread::sleep_for(15s);
        //if the last bit is 1, data is ready
        temporal &= 1;
        if (temporal != 0) {
            flag = true;
        }
    }
    //move address pointer to first address
    buf[0] = 0x00;
    i2compass.write(RWcompasSlaveAddr, buf,1);
    char tmpData[] = {0, 0, 0, 0, 0, 0};
    i2compass.read(RWcompasSlaveAddr, tmpData, 6);

    //initialize array for data

    raw[0] = tmpData[1] << 8 | tmpData[0];
    raw[1] = tmpData[3] << 8 | tmpData[2];
    raw[2] = tmpData[5] << 8 | tmpData[4];
    debug("raw measured data x: %d\n", raw[0]);
    debug("raw measured data x: %d\n", raw[1]);
    debug("raw measured data y: %d\n", raw[2]);
    //reconstruct raw data
    //measured_data[0] = 1.0 * (int) (tmpData[0] << 8 | tmpData[1]); //x
    //measured_data[1] = 1.0 * (int) (tmpData[2] << 8 | tmpData[3]); //y
    //measured_data[3] = 1.0 * (int) (tmpData[4] << 8 | tmpData[5]); //y    // if Z axis is also included

    // convert raw data to mG
    for (int i = 0; i < 3; i++) {
        data[i] = 0.48828125 * (float) raw[i];
    }

    /*X = measured_data[0];
    Y = measured_data[1];

    //correct minimum, mid and maximum values
    if (measured_data[0] > Max[0]) { //X max
        Max[0] = measured_data[0];
    }
    if (measured_data[0] < Min[0]) { //X min
        Min[0] = measured_data[0];
    }
    if (measured_data[1] > Max[1]) { //Y max
        Max[1] = measured_data[1];
    }
    if (measured_data[1] < Min[1]) { //Y min
        Min[1] = measured_data[1];
    }*/
    /*if (measured_data[2] > Max[2]) { //Z max
        Max[2] = measured_data[2];
    }
    if (measured_data[2] > Max[2]) { //Z max
        Min[2] = measured_data[2];
    }*/
    
    /*for (int i = 0; i < 2; i++) { //mid
        Mid[i] = (Max[i] + Min[i]) / 2;
    }*/
    
    // if Z axis is included
    /*for (int i = 0; i < 3; i++) { //mid
        Mid[i] = (Max[i] + Min[i]) / 3;
    }*/
}


//initialize the compass
//Update: this should follow Calibration steps
void CMPS2_init(){
    float out1[3];
    float out2[3];
    int i;

    //calibration: SET
    buf[0] = 0x07;
    buf[1] = 0x80;
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(60ms);


    buf[0] = 0x07;
    buf[1] = 0x20;
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(10ms);

    CMPS2_read_XYZ();
    for (int i = 0; i < 3; i++) { //mid
        out1[i] = data[i];
    }
    printf("Raw SET = %d \t%d \t%d\n",raw[0], raw[1], raw[2]);

    //calibration: RESET
    buf[0] = 0x07;
    buf[1] = 0x80;
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(60ms);


    buf[0] = 0x07;
    buf[1] = 0x40;
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(10ms);

    CMPS2_read_XYZ();
    for (int i = 0; i < 3; i++) { //mid
        out2[i] = data[i];
    }
    printf("Raw RESET = %d \t%d \t%d\n",raw[0], raw[1], raw[2]);

    //offset calculation
    for (int i = 0; i < 3; i++) {
        offset[i] = (out1[i] + out2[i])*0.5;
    }

    //command internal control register 0 for set operation
    buf[0] = 0x07;
    buf[1] = 0x40;  //SET
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(10ms);


    buf[0] = 0x08;
    buf[1] = 0x00;
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(10ms);

}

//sets/resets the sensor, changing the magnetic polarity of the sensing element
/*void CMPS2_set(char reset){
    
    //command internal control register 0 bit 7 (capacitor recharge)
    buf[0] = 0x07;
    buf[1] = 0x80;
    i2compass.write(RWcompasSlaveAddr, buf, 2);
    ThisThread::sleep_for(50ms);

    if (reset) {
        //command internal control register 0 bit 6 (reset)
        buf[0] = 0x07;
        buf[1] = 0x40;
        i2compass.write(RWcompasSlaveAddr, buf, 2);
        ThisThread::sleep_for(10ms);
    }else{
        //command internal control register 0 bit 5 (set)
        buf[0] = 0x07;
        buf[1] = 0x20;
        i2compass.write(RWcompasSlaveAddr, buf, 2);
        ThisThread::sleep_for(10ms);
    }
}*/


float CMPS2_getHeading(){
    CMPS2_read_XYZ();  //read X, Y, Z data of the magnetic field

    //eliminate offset before continuing
    for (int i=0;i<3;i++)
    {
    data[i] = data[i]-offset[i];
    }
    //variables for storing partial results
    float temp0 = 0;
    float temp1 = 0;
    //and for storing the final result
    float deg = 0;


    //calculate heading from data of the magnetic field
  //the formula is different in each quadrant
    if (data[0] < 0)
    {
        if (data[1] > 0)
        {
        //Quadrant 1
        temp0 = data[1];
        temp1 = -data[0];
        deg = 90 - atan(temp0 / temp1) * (180 / 3.14159);
        }
        else
        {
        //Quadrant 2
        temp0 = -data[1];
        temp1 = -data[0];
        deg = 90 + atan(temp0 / temp1) * (180 / 3.14159);
        }
    }
    else {
        if (data[1] < 0)
        {
        //Quadrant 3
        temp0 = -data[1];
        temp1 = data[0];
        deg = 270 - atan(temp0 / temp1) * (180 / 3.14159);
        }
        else
        {
        //Quadrant 4
        temp0 = data[1];
        temp1 = data[0];
        deg = 270 + atan(temp0 / temp1) * (180 / 3.14159);
        }
    }

        //correct heading
        deg += DECLINATION;
        if (DECLINATION > 0)
        {
        if (deg > 360) {
            deg -= 360;
        }
        }
        else{
            if (deg < 0) {
                deg += 360;
            }
        }

        return deg;


    /*float components[2];
    CMPS2_set(false);  // set the polarity to normal
    CMPS2_read_XYZ();  // read X, Y, Z components of the magnetic field
    components[0] = X; // save current results
    components[1] = Y;
    CMPS2_set(true);  // set the polarity to normal
    CMPS2_read_XYZ(); // read X, Y, Z components of the magnetic field

    // eliminate offset from all components
    components[0] = (components[0] - X) / 2.0;
    components[1] = (components[1] - Y) / 2.0;

    // variables for storing partial results
    float temp0 = 0;
    float temp1 = 0;
    // and for storing the final result
    float deg = 0;

    // calculate heading from components of the magnetic field
    // the formula is different in each quadrant
    if (components[0] < Mid[0]) {
        if (components[1] > Mid[1]) {
        // Quadrant 1
        temp0 = components[1] - Mid[1];
        temp1 = Mid[0] - components[0];
        deg = 90 - atan(temp0 / temp1) * (180 / 3.14159);
        }
        else {
        // Quadrant 2
        temp0 = Mid[1] - components[1];
        temp1 = Mid[0] - components[0];
        deg = 90 + atan(temp0 / temp1) * (180 / 3.14159);
        }
    }
    else{
        if (components[1] < Mid[1]) {
        // Quadrant 3
        temp0 = Mid[1] - components[1];
        temp1 = components[0] - Mid[0];
        deg = 270 - atan(temp0 / temp1) * (180 / 3.14159);
        }
        else{
        // Quadrant 4
        temp0 = components[1] - Mid[1];
        temp1 = components[0] - Mid[0];
        deg = 270 + atan(temp0 / temp1) * (180 / 3.14159);
        }
    }

    // correct heading
    deg += DECLINATION;
    if (DECLINATION > 0) {
      if (deg > 360) {
        deg -= 360;
      }
    } else {
      if (deg < 0) {
        deg += 360;
      }
    }

    return deg;*/
}


void CMPS2_decodeHeading(float measured_angle){
    
    if (measured_angle > 337.25 | measured_angle < 22.5) {
        printf("North\n");
    }
    else if (measured_angle > 292.5) {
        printf("North-West\n");
    }
    else if (measured_angle > 202.5) {
      printf("South-West\n");
    }
    else if (measured_angle > 157.5) {
      printf("South\n");
    }
    else if (measured_angle > 112.5) {
      printf("South-East\n");
    }
    else if (measured_angle > 67.5) {
      printf("East\n");
    }
    else {
      printf("North-East\n");
    }

}
