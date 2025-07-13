//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "pc_serial_com.h"
#include "user_interface.h"

#include "code.h"
#include "siren.h"
#include "smart_home_system.h"
#include "fire_alarm.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "matrix_keypad.h"
#include "display.h"

//=====[Declaration of private defines]========================================

#define DISPLAY_REFRESH_TIME_MS 3000

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromUserInterface[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static bool incorrectCodeState = OFF;
static bool systemBlockedState = OFF;

static bool codeComplete = false;
static int numberOfCodeChars = 0;
static bool FlagTemperature= false;
static bool FlagGas =false;

//=====[Declarations (prototypes) of private functions]========================

static void userInterfaceMatrixKeypadUpdate();
static void incorrectCodeIndicatorUpdate();
static void systemBlockedIndicatorUpdate();

static void userInterfaceDisplayInit();
static void userInterfaceDisplayUpdate();

//=====[Implementations of public functions]===================================

void userInterfaceInit()
{
    incorrectCodeLed = OFF;
    systemBlockedLed  = OFF;
    matrixKeypadInit(SYSTEM_TIME_INCREMENT_MS);
    displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
    // LCD prompt for initial code entry
    displayCharPositionWrite(0, 3);
    displayStringWrite("Enter code: ___");

    // Now block until user enters 3 digits + '#'
    char newCode[CODE_NUMBER_OF_KEYS] = {0};
    int  idx = 0;
    while (true) {
        char key = matrixKeypadUpdate();
        if (key >= '0' && key <= '9' && idx < CODE_NUMBER_OF_KEYS) {
            newCode[idx] = key;
            // update the LCD at positions 12,13,14
            displayCharPositionWrite(11 + idx, 3);
            displayStringWrite(key+" ");
            idx++;
        }
        if (key == '#' && idx == CODE_NUMBER_OF_KEYS) {
            break;  // code entry complete
        }
    }

    // save into the global sequence
    codeWrite(newCode);

    // confirm on LCD
    displayCharPositionWrite(0, 3);
    displayStringWrite("Code set.        ");    
    userInterfaceDisplayInit();

}

void userInterfaceUpdate()
{
    userInterfaceMatrixKeypadUpdate();
    incorrectCodeIndicatorUpdate();
    systemBlockedIndicatorUpdate();
    userInterfaceDisplayUpdate();
}

bool incorrectCodeStateRead()
{
    return incorrectCodeState;
}

void incorrectCodeStateWrite( bool state )
{
    incorrectCodeState = state;
}

bool systemBlockedStateRead()
{
    return systemBlockedState;
}

void systemBlockedStateWrite( bool state )
{
    systemBlockedState = state;
}

bool userInterfaceCodeCompleteRead()
{
    return codeComplete;
}

void userInterfaceCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

static void userInterfaceMatrixKeypadUpdate()
{
    static int numberOfHashKeyReleased = 0;
    char keyReleased = matrixKeypadUpdate();

    if (keyReleased == '\0') return;

    // ── ALARM ACTIVE: collect deactivation code ──
    if( sirenStateRead() && !systemBlockedStateRead() ) {
        if( !incorrectCodeStateRead() ) {
            codeSequenceFromUserInterface[numberOfCodeChars] = keyReleased;
            numberOfCodeChars++;
            if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
                codeComplete = true;
                numberOfCodeChars = 0;
            }
        } else {
            if( keyReleased == '#' ) {
                numberOfHashKeyReleased++;
                if( numberOfHashKeyReleased >= 2 ) {
                    numberOfHashKeyReleased = 0;
                    numberOfCodeChars = 0;
                    codeComplete = false;
                    incorrectCodeState = OFF;
                }
            }
        }
    }
    // ── ALARM INACTIVE: handle '1' and '9' for status on the LCD ──
    else {
        if (keyReleased == '1') {
            // gas status on row 1
            FlagGas = true;
        }
        else if (keyReleased == '9') {
            // temperature on row 0
            FlagTemperature =true;
        }
    }
}


static void userInterfaceDisplayInit()
{
    displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
     
    displayCharPositionWrite ( 0,0 );
    displayStringWrite( "Temperature:" );

    displayCharPositionWrite ( 0,1 );
    displayStringWrite( "Gas:" );
    
    displayCharPositionWrite ( 0,2 );
    displayStringWrite( "Alarm:" );
}

static void userInterfaceDisplayUpdate()
{
    static int accumulatedDisplayTime = 0;
    char temperatureString[3] = "";
    
    if( accumulatedDisplayTime >=
        DISPLAY_REFRESH_TIME_MS ) {

        accumulatedDisplayTime = 0;


        if(FlagTemperature){
            displayCharPositionWrite ( 12,0 );
             if ( overTemperatureDetectorStateRead() ) {
                displayStringWrite( "ON        " );
            } else {
                displayStringWrite( "OFF       " );
            }
            FlagTemperature = false;
        }
        else{
            sprintf(temperatureString, "%.0f", temperatureSensorReadCelsius());
            displayCharPositionWrite ( 12,0 );
            displayStringWrite( temperatureString );
            displayCharPositionWrite ( 14,0 );
            displayStringWrite( "'C" );
        }        

        displayCharPositionWrite ( 4,1 );

        if(FlagGas){
            if ( gasDetectorStateRead() ) {
                displayStringWrite( "ON               " );
            } else {
                displayStringWrite( "OFF              " );
            }
            FlagGas = false;
        }
        else{
            if ( gasDetectedRead() ) {
                displayStringWrite( "Detected    " );
            } else {
                displayStringWrite( "Not Detected" );
            }
        }

        displayCharPositionWrite ( 6,2 );
        
        if ( sirenStateRead() ) {
            displayStringWrite( "ON " );
        } else {
            displayStringWrite( "OFF" );
        }

    } else {
        accumulatedDisplayTime =
            accumulatedDisplayTime + SYSTEM_TIME_INCREMENT_MS;        
    } 
}

static void incorrectCodeIndicatorUpdate()
{
    incorrectCodeLed = incorrectCodeStateRead();
}

static void systemBlockedIndicatorUpdate()
{
    systemBlockedLed = systemBlockedState;
}