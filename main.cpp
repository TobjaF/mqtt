/** MQTT Publish von Sensordaten */
#include "mbed.h"
#include "OLEDDisplay.h"
/*
#if MBED_CONF_IOTKIT_HTS221_SENSOR == true
#include "HTS221Sensor.h"
#endif
#if MBED_CONF_IOTKIT_BMP180_SENSOR == true
#include "BMP180Wrapper.h"
#endif
*/


#include "QEI.h"



#include <MQTTClientMbedOs.h>
#include <MQTTNetwork.h>
#include <MQTTClient.h>
#include <MQTTmbed.h> // Countdown

//Threads
#include "platform/mbed_thread.h"

///////////////////////////////////////////////////////////////////////
//////////////////// GLOBALE VARIABLEN START //////////////////////////
///////////////////////////////////////////////////////////////////////

// Topic's publish
char* topicLED = (char*) "iotkit_tobja/actors/led"
// Topic's subscribe
char* topicActors = (char*) "iotkit_tobja/actors/#";
// MQTT Brocker
char* hostname = (char*) "cloud.tbz.ch";
int port = 1883;
// MQTT Message
MQTT::Message message;
// I/O Buffer
char buf[100];




int global_state, wheel_oldvalue, wheel_newvalue = 0;

// UI
OLEDDisplay oled( MBED_CONF_IOTKIT_OLED_RST, MBED_CONF_IOTKIT_OLED_SDA, MBED_CONF_IOTKIT_OLED_SCL );


//Use X2 encoding by default.
QEI wheel (MBED_CONF_IOTKIT_BUTTON2, MBED_CONF_IOTKIT_BUTTON3, NC, 624);


DigitalOut led1( MBED_CONF_IOTKIT_LED1 );
DigitalOut led2( MBED_CONF_IOTKIT_LED2 );
DigitalOut led3( MBED_CONF_IOTKIT_LED3 );
DigitalOut led4( MBED_CONF_IOTKIT_LED4 );

//Test
Thread thread_mosfetled;
Thread thread_spiled;
Thread thread_wheel_watch;

///////////////////////////////////////////////////////////////////////
//////////////////// GLOBALE VARIABLEN ENDE ///////////////////////////
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
//////////////////// MQTT-FUNKTIONEN START ////////////////////////////
///////////////////////////////////////////////////////////////////////

/** Hilfsfunktion zum Publizieren auf MQTT Broker */
void publish( MQTTNetwork &mqttNetwork, MQTT::Client<MQTTNetwork, Countdown> &client, char* topic )
{
    MQTT::Message message;    
    oled.cursor( 2, 0 );
    oled.printf( "Topi: %s\n", topic );
    oled.cursor( 3, 0 );    
    oled.printf( "Push: %s\n", buf );
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buf;
    message.payloadlen = strlen(buf)+1;
    client.publish( topic, message);  
}

/** Daten empfangen von MQTT Broker */
void messageArrived( MQTT::MessageData& md )
{
    float value;
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Topic %.*s, ", md.topicName.lenstring.len, (char*) md.topicName.lenstring.data );
    printf("Payload %.*s\n", message.payloadlen, (char*) message.payload);
    
    if  ( strncmp( (char*) md.topicName.lenstring.data + md.topicName.lenstring.len - 3, "led", 3) == 0  
        && message.payloadlen >= 2) {

        DigitalOut * ledN;
        switch(((char*) message.payload)[0]) {
            case '0': global_state = 0; printf("Status 0\n"); break;
            case '1': global_state = 1; printf("Status 1\n"); break;
            case '2': global_state = 2; printf("Status 2\n"); break;
            case '3': global_state = 3; printf("Status 3\n"); break;
            case '4': global_state = 4; printf("Status 4\n"); break;
            case '5': global_state = 5; printf("Status 5\n"); break;
            case '6': global_state = 6; printf("Status 6\n"); break;
            case '7': global_state = 7; printf("Status 7\n"); break;
            default: break;
            
        }

        if(((char*) message.payload)[1] == '1') {
            *ledN = 1;
        }else{
            *ledN = 0;
        }
    }

}

///////////////////////////////////////////////////////////////////////
//////////////////// MQTT-FUNKTIONEN ENDE /////////////////////////////
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
//////////////////// LED - STEUERUNG START ////////////////////////////
///////////////////////////////////////////////////////////////////////


void off()
{
    printf( "off \n" );
    red = 0;
    green = 0;
    blue = 0;
    thread_sleep_for( 1000 );
}

/** 3 x 3 Werte */
unsigned int strip[9];

void writeLED()
{
    for ( int p = 0; p < 9; p++ )
        spi.write( strip[p] );
}

void clearLED()
{
    for ( int p = 0; p < 9; p++ ) 
    {
        strip[p] = 0;
        spi.write( strip[p] );
    }
}

void dim( PwmOut& pin )
{
    printf( "dim\n" );
    for ( float i = 0.0f; i < 1.0f; i += .01f )
    {
        pin = i;
        thread_sleep_for( 200 );
    }
    thread_sleep_for( 1000 );
    
}

void thread_mosfetled_func() {
    while(true) {
        dim( red );
        off();
        dim( green );
        off();
        dim( blue );
        off();
        
        red = 1;
        thread_sleep_for( 1000 );
        off();
        
        green = 1;
        thread_sleep_for( 1000 );
        off();
        
        blue = 1;
        thread_sleep_for( 1000 );
        off();
        
        red = 1;
        blue = 1;
        green = 1;
        thread_sleep_for( 1000 );
        off();
    }
}

void thread_spiled_func() {
    while(true) {
        //for ( int i = 0; i < 9; i++ )
            //strip[1] = 1;//rand() % 64 + 1; // the function rand() was replace by wheel.getPulses() to control the brightness with the wheel encoder/switch
       // if (status % 7 == 0 ){
            //brightness = 7 / status;
       // }
        if (status == 0)
        {
            for ( int i = 0; i < 9; i++ )
                strip[i] = 0;
        }
        if (status == 1)
        {
            for ( int i = 0; i < 3; i++ )
                strip[i] = brightness;

            for ( int i = 3; i < 9; i++ )
                strip[i] = 0;
        }
        if (status == 2)
        {
            for ( int i = 3; i < 6; i++ )
                strip[i] = brightness;

            for ( int i = 0; i < 3; i++ )
                strip[i] = 0;

            for ( int i = 6; i < 9; i++ )
                strip[i] = 0;
        }
        if (status == 3)
        {
            for ( int i = 6; i < 9; i++ )
                strip[i] = brightness;

            for ( int i = 0; i < 6; i++ )
                strip[i] = 0;
        }
        if (status == 4)
        {
            for ( int i = 0; i < 6; i++ )
                strip[i] = brightness;

            for ( int i = 6; i < 9; i++ )
                strip[i] = 0;
        }
        if (status == 5)
        {
            for ( int i = 0; i < 3; i++ )
                strip[i] = 0;

            for ( int i = 3; i < 9; i++ )
                strip[i] = brightness;
        }
        if (status == 6)
        {
            for ( int i = 0; i < 3; i++ )
                strip[i] = brightness;

            for ( int i = 6; i < 9; i++ )
                strip[i] = brightness;

            for ( int i = 3; i < 6; i++ )
                strip[i] = 0;    
        }
        if (status == 7)
        {
            for ( int i = 0; i < 9; i++ )
                strip[i] = brightness;
        }
        //else{for ( int i = 0; i < 9; i++ )
         //       strip[i] = 0;}
        //strip[3] = 1;
        //strip[2] = 1;
        //strip[0] = 64;

        writeLED();
        thread_sleep_for( 200 );
    }
}
    
    
    
    
    
    while(true) {
        for ( int i = 0; i < 9; i++ )
            strip[i] = rand() % 64 + 1;
            
        writeLED();
        thread_sleep_for( 200 );
    }
}

//////////////////////////////////////////////////////////////////////
//////////////////// LED - STEUERUNG ENDE ////////////////////////////
//////////////////////////////////////////////////////////////////////


void thread_wheel_watch_func() {
    oled.cursor( 1, 0 );
        wheel_newvalue = wheel.getPulses(); 
        if  ( wheel_newvalue != wheel_oldvalue ){
            if ( wheel_newvalue > wheel_oldvalue) global_status =  (global_status + 1)%8
            }
            else {
             global_status =  (global_status- 1)%8
            }
        oled.printf("Pulses: %6i\n", wheel_newvalue );
        wheel_oldvalue = wheel_newvalue;
        thread_sleep_for( 100 );    
}




//////////////////////////////////////////////////////////////////////
//////////////////// MAIN FUNCTION START /////////////////////////////
//////////////////////////////////////////////////////////////////////
int main()
{
    uint8_t id;
    float temp, hum;
    int encoder;
    
    oled.clear();
    oled.printf( "MQTTPublish\r\n" );
    oled.printf( "host: %s:%s\r\n", hostname, port );

    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    oled.printf( "SSID: %s\r\n", MBED_CONF_APP_WIFI_SSID );
    
    // Connect to the network with the default networking interface
    // if you use WiFi: see mbed_app.json for the credentials
    WiFiInterface *wifi = WiFiInterface::get_default_instance();
    if ( !wifi ) 
    {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }
    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect( MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2 );
    if ( ret != 0 ) 
    {
        printf("\nConnection error: %d\n", ret);
        return -1;
    }    

    // TCP/IP und MQTT initialisieren (muss in main erfolgen)
    MQTTNetwork mqttNetwork( wifi );
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    printf("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc); 

    // Zugangsdaten - der Mosquitto Broker ignoriert diese
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char*) wifi->get_mac_address(); // muss Eindeutig sein, ansonsten ist nur 1ne Connection moeglich
    data.username.cstring = (char*) wifi->get_mac_address(); // User und Password ohne Funktion
    data.password.cstring = (char*) "password";
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);           

    // MQTT Subscribe!
    client.subscribe( topicActors, MQTT::QOS0, messageArrived );
    printf("MQTT subscribe %s\n", topicActors );
    

    led1 = 0;led2 = 0;led3 = 0;led4 = 0;
    
    
    thread_spiled.start(thread_spiled_func);
    thread_spiled.start(thread_spiled_func);
    thread_wheel_watch.start(thread_wheel_watch_func);

    while   ( 1 ) 
    {
        // Encoder
        encoder = wheel.getRevolutions();
        // wheel.getRevolutions();
        sprintf( buf, "%d", encoder );
        publish( mqttNetwork, client, topicENCODER );
        
        //publish( mqttNetwork, client, topicLED );

        client.yield    ( 1000 );                   // MQTT Client darf empfangen
        thread_sleep_for( 500 );
    }

    // Verbindung beenden
    if ((rc = client.disconnect()) != 0)
        printf("rc from disconnect was %d\r\n", rc);

    mqttNetwork.disconnect();    
}

//////////////////////////////////////////////////////////////////////
//////////////////// MAIN FUNCTION ENDE //////////////////////////////
//////////////////////////////////////////////////////////////////////
