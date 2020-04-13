#include "mbed.h"
#include "mbed_events.h" 
#include "fsl_port.h"
#include "fsl_gpio.h"
#define UINT14_MAX        16383

// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS870fasdfasdfadsfadsf0CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1

// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01    
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7


DigitalOut redLED(LED1);
InterruptIn sw2(SW2);

EventQueue tilt_queue(32 * EVENTS_EVENT_SIZE);
Timer timer;

I2C i2c( PTD9,PTD8);
Serial pc(USBTX, USBRX);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;

Thread thread1;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);
float* FXOS8700CQ();


//global variable;

uint8_t who_am_i, data[2], res[6];
int16_t acc16;
float t[3];


void FXOS8700CQ(){
    pc.baud(115200);

    // Enable the FXOS8700Q

    FXOS8700CQ_readRegs(FXOS8700Q_CTRL_REG1, &data[1], 1);
    data[1] |= 0x01;
    data[0] = FXOS8700Q_CTRL_REG1;
    FXOS8700CQ_writeRegs(data, 2);

    // Get the slave address
    FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);

    pc.printf("Here is %x\r\n", who_am_i);


    FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

    acc16 = (res[0] << 6) | (res[1] >> 2);
    if (acc16 > UINT14_MAX/2)
        acc16 -= UINT14_MAX;
    t[0] = ((float)acc16) / 4096.0f;

    acc16 = (res[2] << 6) | (res[3] >> 2);
    if (acc16 > UINT14_MAX/2)
        acc16 -= UINT14_MAX;
    t[1] = ((float)acc16) / 4096.0f;

    acc16 = (res[4] << 6) | (res[5] >> 2);
    if (acc16 > UINT14_MAX/2)
        acc16 -= UINT14_MAX;
    t[2] = ((float)acc16) / 4096.0f;

    printf("FXOS8700Q ACC: X=%1.4f(%x%x) Y=%1.4f(%x%x) Z=%1.4f(%x%x)\r\n",\
        t[0], res[0], res[1],\
        t[1], res[2], res[3],\
        t[2], res[4], res[5]\
    );
   
}

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {

   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);

}


void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);

}



void start_record(float origin[]){
    timer.start();
    while(t<10){
        FXOS8700CQ();
        wait(0.1);
        if(((origin[0]*t[0]+origin[1]*t[1]+origin[2]*t[2])/(sqrt(pow(origin[0], 2) + pow(origin[1], 2)+ pow(origin[2], 2))))<=0.707){
            pc.printf("over 45 degree!!!!\r\n");
        }else{
            pc.printf(" not thing happend?\r\n");
        }
    }
    timer.stop();
}


void sw_fall_irq(float origin[]){
    redLED = !redLED;
    tilt_queue.call(&start_record(origin));

}



int main(){
    // new
    float origin[3];
    float tile_cap[3];

    //init
    FXOS8700CQ();
    origin[0]=t[0];
    origin[1]=t[1];
    origin[2]=t[2];
    wait(0.1);

    //thread start
    thread1.start(callback(&tilt_queue, &EventQueue::dispatch_forever));
    sw2.fall(&sw_fall_irq(origin));
    
    while(1){wait(0.1);}
    
}


