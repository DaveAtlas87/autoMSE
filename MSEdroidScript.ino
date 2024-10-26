#include <DFRobotDFPlayerMini.h>
#include <NewPing.h>               // For ultrasonic sensor
#include <SoftwareSerial.h>        // For MP3 player communication
#include <Wire.h>                  // For I2C communication with MPU6050
#include <MPU6050.h>               // MPU6050 library

// Define motor PWM pins
#define MOTOR1_CW_PIN 3
#define MOTOR1_CCW_PIN 5
#define MOTOR2_CW_PIN 6
#define MOTOR2_CCW_PIN 9
#define MOTOR3_CW_PIN 10
#define MOTOR3_CCW_PIN 11
#define MOTOR4_CW_PIN 12
#define MOTOR4_CCW_PIN 13

// Define RGB LED array pins
const int LED_R_PINS[5] = {14, 15, 16, 17, 18};  // Red pins for each LED
const int LED_G_PINS[5] = {19, 20, 21, 22, 23};  // Green pins for each LED
const int LED_B_PINS[5] = {24, 25, 26, 27, 28};  // Blue pins for each LED

// MP3 player setup
SoftwareSerial mp3Serial(7, 8);  // RX, TX
DFRobotDFPlayerMini mp3Player;

// Ultrasonic sensor setup
#define TRIG_PIN 2
#define ECHO_PIN 4
NewPing sonar(TRIG_PIN, ECHO_PIN, 200);

// Light sensor, microphone, and IR sensor pins
#define LIGHT_SENSOR_PIN A0
#define MICROPHONE_PIN A1
#define IR_SENSOR_PIN A2  // IR sensor for detecting companion

// MPU6050 setup
MPU6050 mpu;
int16_t ax, ay, az, gx, gy, gz;

// Mood and state variables
enum Mood { HAPPY, CURIOUS, ANNOYED, SLEEPY, STEALTH };
Mood currentMood = HAPPY;
bool stealthMode = false;
int obstacleCount = 0;  // Track the number of obstacles encountered
unsigned long lastObstacleTime = 0;
const int obstacleThreshold = 3;  // Number of obstacles before becoming "annoyed"
const unsigned long obstacleTimeout = 10000;  // Time frame to reset the counter

// Idle timing variables
unsigned long lastIdleTime = 0;
unsigned long nextIdleInterval = random(5000, 15000);  // Set initial random interval for idle state
unsigned long idleDuration = 0;
bool inIdleState = false;

unsigned long lastMoodChange;  // Timestamp of last mood change
unsigned long moodDuration = 10000; // Change mood every 10 seconds

// Additional timing and event counters
unsigned long lastPlayfulTime = 0;
unsigned long nextPlayfulInterval = random(20000, 40000); // Random interval for playful behavior
unsigned long lastEventTime = 0;
int obstacleCountMemory = 0;
int loudSoundCountMemory = 0;

void setup() {
    Serial.begin(9600);
    mp3Serial.begin(9600);

    // Initialize MP3 player
    if (!mp3Player.begin(mp3Serial)) {
        Serial.println("MP3 Player not detected!");
    }
    mp3Player.volume(20);

    // Initialize MPU6050
    Wire.begin();
    mpu.initialize();
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed!");
    }

    // Set up motor pins
    pinMode(MOTOR1_CW_PIN, OUTPUT);
    pinMode(MOTOR1_CCW_PIN, OUTPUT);
    pinMode(MOTOR2_CW_PIN, OUTPUT);
    pinMode(MOTOR2_CCW_PIN, OUTPUT);
    pinMode(MOTOR3_CW_PIN, OUTPUT);
    pinMode(MOTOR3_CCW_PIN, OUTPUT);
    pinMode(MOTOR4_CW_PIN, OUTPUT);
    pinMode(MOTOR4_CCW_PIN, OUTPUT);

    // Set up each RGB LED pin as output
    for (int i = 0; i < 5; i++) {
        pinMode(LED_R_PINS[i], OUTPUT);
        pinMode(LED_G_PINS[i], OUTPUT);
        pinMode(LED_B_PINS[i], OUTPUT);
    }

    setMood(HAPPY);
    lastMoodChange = millis();  // Initialize lastMoodChange with the current time
    lastIdleTime = millis();    // Initialize lastIdleTime
}

void loop() {
    unsigned long currentTime = millis();

    // Check for random idle state
    if (!inIdleState && currentTime - lastIdleTime > nextIdleInterval) {
        inIdleState = true;
        idleDuration = random(2000, 5000); // Random idle duration between 2-5 seconds
        motorStop();
        idleBehavior();
        lastIdleTime = currentTime;
        nextIdleInterval = random(5000, 15000); // Set the next random idle interval
    }
    if (inIdleState && currentTime - lastIdleTime > idleDuration) {
        inIdleState = false;
        lastIdleTime = currentTime;
    }

    if (inIdleState) return;

    // Original mood and movement control
    if (currentTime - lastMoodChange > moodDuration) {
        cycleMood();
        lastMoodChange = currentTime;
    }
    switch (currentMood) {
        case HAPPY:
            smoothForwardMovement();
            break;
        case CURIOUS:
            exploratoryMovement();
            break;
        case ANNOYED:
            jitteryMovement();
            break;
        case SLEEPY:
            slowRelaxedMovement();
            break;
        case STEALTH:
            slowStealthMovement();
            break;
    }

    // New Feature Functions
    adjustSpeedBasedOnMood();
    if (currentTime - lastPlayfulTime > nextPlayfulInterval) {
        playfulBehavior();
        lastPlayfulTime = currentTime;
        nextPlayfulInterval = random(20000, 40000);
    }
    if (sonar.ping_cm() < 5) proximityReaction();
    adjustMoodBasedOnLight();
    detectTiltReaction();
    adjustDirectionBasedOnLight();
    detectSoundPatterns();
    updateMoodBasedOnMemory();
    periodicScanningMode();
    adjustAlertnessBasedOnIncline();

    // Existing interactive reactions
    if (analogRead(MICROPHONE_PIN) > 500) {
        reactToSound();
    }
    if (analogRead(LIGHT_SENSOR_PIN) < 300) {
        activateStealthMode();
    } else if (stealthMode) {
        deactivateStealthMode();
    }
    if (sonar.ping_cm() < 15) {
        avoidObstacle();
    }
    if (analogRead(IR_SENSOR_PIN) > 512) {
        detectCompanion();
    }
    checkShake();
}

// Additional Functions

void adjustSpeedBasedOnMood() {
    int speed;
    switch (currentMood) {
        case HAPPY: speed = 255; break;
        case CURIOUS: speed = 200; break;
        case ANNOYED: speed = 225; break;
        case SLEEPY: speed = 100; break;
        case STEALTH: speed = 80; break;
    }
    moveForward(speed);  // Adjust forward movement speed based on mood
}

void playfulBehavior() {
    Serial.println("Performing playful behavior...");
    for (int i = 0; i < 3; i++) {
        randomTurn();
        ledChaseAnimation(255, 255, 0);  // Yellow LEDs for playfulness
    }
}

void proximityReaction() {
    Serial.println("Reacting to close proximity!");
    playSoundRandomly((const int[]){4, 6}, 2); // Play a cautious or curious sound
    ledBlinkingAnimation(0, 255, 255);  // Blink cyan LEDs
    moveBackward(100);  // Move back briefly
    delay(300);
    motorStop();
}

void adjustMoodBasedOnLight() {
    int lightLevel = analogRead(LIGHT_SENSOR_PIN);
    if (lightLevel < 200) {
        setMood(STEALTH);
    } else if (lightLevel > 800) {
        setMood(HAPPY);
    } else {
        setMood(CURIOUS);
    }
}

void detectTiltReaction() {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    if (abs(ax) > 2000 || abs(ay) > 2000) {
        Serial.println("Tilt detected! Reacting nervously.");
        ledBlinkingAnimation(255, 0, 0);  // Flash red LEDs
        playSoundRandomly((const int[]){6, 7}, 2); // Annoyed sound
        moveBackward(100);  // Brief back movement
        delay(300);
        motorStop();
    }
}

void adjustDirectionBasedOnLight() {
    int lightLevel = analogRead(LIGHT_SENSOR_PIN);
    if (currentMood == CURIOUS && lightLevel > 600) {
        moveForward(200);  // Move towards brighter areas
    } else if (currentMood == STEALTH && lightLevel > 600) {
        moveBackward(100); // Avoid bright areas if in stealth
        delay(300);
        motorStop();
    }
}

void detectSoundPatterns() {
    int soundLevel = analogRead(MICROPHONE_PIN);
    if (soundLevel > 500) {
        unsigned long currentEventTime = millis();
        if (currentEventTime - lastEventTime < 1000) { // Detect repeated sound within 1 second
            Serial.println("Reacting to rhythmic sound!");
            playSoundRandomly((const int[]){2, 3}, 2);  // Play happy or curious sound
            ledChaseAnimation(0, 0, 255); // Show blue LED pattern
        }
        lastEventTime = currentEventTime;
    }
}

void updateMoodBasedOnMemory() {
    if (obstacleCountMemory >= 5) {
        setMood(ANNOYED);  // Change to annoyed if obstacles are frequently encountered
        obstacleCountMemory = 0; // Reset memory
    }
    if (loudSoundCountMemory >= 5) {
        setMood(SLEEPY);  // Become sleepy if loud sounds are frequent
        loudSoundCountMemory = 0; // Reset memory
    }
}

void periodicScanningMode() {
    unsigned long currentTime = millis();
    if (currentTime - lastPlayfulTime > 30000) {  // Every 30 seconds
        Serial.println("Performing environmental scan...");
        smallLookAround();
    }
}

void adjustAlertnessBasedOnIncline() {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    if (abs(ax) > 2000 || abs(az) > 2000) {  // Detect incline
        Serial.println("Incline detected! Moving cautiously.");
        moveForward(100);  // Move slowly on inclines
        ledBlinkingAnimation(255, 128, 0);  // Blink orange LEDs for caution
    }
}


// Function to play a random sound from a list
void playSoundRandomly(const int sounds[], int soundCount) {
    int randomIndex = random(0, soundCount);
    int soundID = sounds[randomIndex];
    mp3Player.play(soundID);
}

// Function to set a new mood
void setMood(Mood newMood) {
    currentMood = newMood;
    obstacleCount = 0;  // Reset obstacle counter when mood changes
    Serial.print("Mood set to: ");
    switch (newMood) {
        case HAPPY: Serial.println("HAPPY"); break;
        case CURIOUS: Serial.println("CURIOUS"); break;
        case ANNOYED: Serial.println("ANNOYED"); break;
        case SLEEPY: Serial.println("SLEEPY"); break;
        case STEALTH: Serial.println("STEALTH"); break;
    }
}

// Function to cycle through moods
void cycleMood() {
    currentMood = static_cast<Mood>((currentMood + 1) % 5); // Cycle through moods
    Serial.print("Mood changed to: ");
    switch (currentMood) {
        case HAPPY: Serial.println("HAPPY"); break;
        case CURIOUS: Serial.println("CURIOUS"); break;
        case ANNOYED: Serial.println("ANNOYED"); break;
        case SLEEPY: Serial.println("SLEEPY"); break;
        case STEALTH: Serial.println("STEALTH"); break;
    }
}

// Function to activate stealth mode
void activateStealthMode() {
    Serial.println("Entering stealth mode...");
    stealthMode = true;
    setMood(STEALTH);  // Set to STEALTH mood
    ledChaseAnimation(10, 10, 10);  // Dim white LED animation
}

// Function to deactivate stealth mode
void deactivateStealthMode() {
    Serial.println("Exiting stealth mode...");
    stealthMode = false;
    setMood(HAPPY);  // Return to a default mood
}

// Function to avoid obstacle
void avoidObstacle() {
    Serial.println("Avoiding obstacle!");
    moveBackward(150); // Move back slightly
    delay(500);
    randomTurn();  // Make a random turn
    motorStop();
}

// Function to detect companion with IR sensor
void detectCompanion() {
    int companionAction = random(0, 4);  // Randomize companion interaction
    switch (companionAction) {
        case 0: approachAndInspect(); break;
        case 1: playfulDance(); break;
        case 2: synchronizeLights(); break;
        case 3: backAwayRespectfully(); break;
    }
}

// Companion interaction behaviors
void approachAndInspect() {
    Serial.println("Approaching and inspecting companion...");
    moveForward(150);
    delay(500);
    smallCircle();
    delay(500);
    motorStop();
}

void playfulDance() {
    Serial.println("Performing playful dance with companion...");
    ledBlinkingAnimation(0, 255, 255);  // Flash cyan LEDs
    for (int i = 0; i < 3; i++) {
        randomTurn();
    }
    motorStop();
}

void synchronizeLights() {
    Serial.println("Synchronizing lights with companion...");
    for (int i = 0; i < 3; i++) {
        ledBreathingAnimation(255, 255, 0);  // Yellow "communication" lights
    }
    motorStop();
}

void backAwayRespectfully() {
    Serial.println("Backing away respectfully...");
    moveForward(100);
    delay(500);
    moveBackward(100);
    delay(500);
    motorStop();
}

// Small circle motion for inspection
void smallCircle() {
    analogWrite(MOTOR1_CW_PIN, 150);
    analogWrite(MOTOR1_CCW_PIN, 0);
    analogWrite(MOTOR2_CW_PIN, 0);
    analogWrite(MOTOR2_CCW_PIN, 150);
    delay(1000);
    motorStop();
}

// Function for idle behavior based on mood
void idleBehavior() {
    switch (currentMood) {
        case HAPPY:
            wiggleMotion();  // A friendly wiggle
            break;
        case CURIOUS:
            smallLookAround();  // Look around in curiosity
            break;
        case ANNOYED:
            ledBlinkingAnimation(255, 0, 0);  // Flash red LEDs in frustration
            playSoundRandomly((const int[]){6, 7}, 2);  // Play annoyed sound
            break;
        case SLEEPY:
            ledBreathingAnimation(128, 0, 128);  // Soft purple glow
            break;
        case STEALTH:
            ledChaseAnimation(10, 10, 10);  // Dim white chase effect
            break;
    }
}


// Mood-specific motor behaviors
void smoothForwardMovement() {
    moveForward(255);  // Smooth, full-speed movement
}

void exploratoryMovement() {
    moveForward(200);
    delay(200);
    randomTurn();
}

void jitteryMovement() {
    for (int i = 0; i < 3; i++) {
        moveForward(255);
        delay(100);
        moveBackward(255);
        delay(100);
    }
    randomTurn();
}

void slowRelaxedMovement() {
    moveForward(100);  // Slow, relaxed movement
    delay(500);
    motorStop();
    delay(500);
}

void slowStealthMovement() {
    moveForward(80);  // Very slow movement in stealth mode
    delay(300);
}

// Function to react to sound detected by the microphone
void reactToSound() {
    Serial.println("Reacting to sound!");
    ledBlinkingAnimation(255, 0, 0); // Red alert LED
    moveBackward(150);  // Move back slightly
    delay(200);
    motorStop();
}

// Function to detect and react to a shake using MPU6050 accelerometer
void checkShake() {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    if (abs(ax) > 2000 || abs(ay) > 2000 || abs(az) > 2000) {  // Adjust threshold as needed
        Serial.println("Shake detected!");
        ledBlinkingAnimation(255, 0, 0);  // Flash red LED to indicate reaction
        playSoundRandomly((const int[]){6, 7}, 2);  // Play annoyed sound if shaken
        moveBackward(100);  // Move back slightly
        delay(300);
        motorStop();
    }
}

// Short motor movements to simulate "wiggling" for happy mood
void wiggleMotion() {
    moveForward(150);
    delay(100);
    moveBackward(150);
    delay(100);
    motorStop();
}

// "Look around" by briefly turning left and right
void smallLookAround() {
    int turnSpeed = 100;
    analogWrite(MOTOR1_CW_PIN, 0);
    analogWrite(MOTOR1_CCW_PIN, turnSpeed);
    analogWrite(MOTOR2_CW_PIN, turnSpeed);
    analogWrite(MOTOR2_CCW_PIN, 0);
    delay(200);
    motorStop();
    delay(200);
    analogWrite(MOTOR1_CW_PIN, turnSpeed);
    analogWrite(MOTOR1_CCW_PIN, 0);
    analogWrite(MOTOR2_CW_PIN, 0);
    analogWrite(MOTOR2_CCW_PIN, turnSpeed);
    delay(200);
    motorStop();
}

// LED Animations
void ledChaseAnimation(int r, int g, int b) {
    for (int i = 0; i < 5; i++) {
        setLEDArrayColor(0, 0, 0);  // Clear LEDs
        setLEDColor(i, r, g, b);    // Light up one LED at a time
        delay(100);
    }
}

void ledBreathingAnimation(int r, int g, int b) {
    for (int brightness = 0; brightness <= 255; brightness += 5) {
        setLEDArrayColor((r * brightness) / 255, (g * brightness) / 255, (b * brightness) / 255);
        delay(20);
    }
    for (int brightness = 255; brightness >= 0; brightness -= 5) {
        setLEDArrayColor((r * brightness) / 255, (g * brightness) / 255, (b * brightness) / 255);
        delay(20);
    }
}

void ledBlinkingAnimation(int r, int g, int b) {
    for (int i = 0; i < 3; i++) {
        setLEDArrayColor(r, g, b);
        delay(200);
        setLEDArrayColor(0, 0, 0);
        delay(200);
    }
}

// Random turn movement for exploratory or jittery behaviors
void randomTurn() {
    int turnDirection = random(0, 2);
    int turnSpeed = 150;
    if (turnDirection == 0) {
        // Turn left
        analogWrite(MOTOR1_CW_PIN, 0);
        analogWrite(MOTOR1_CCW_PIN, turnSpeed);
        analogWrite(MOTOR2_CW_PIN, turnSpeed);
        analogWrite(MOTOR2_CCW_PIN, 0);
    } else {
        // Turn right
        analogWrite(MOTOR1_CW_PIN, turnSpeed);
        analogWrite(MOTOR1_CCW_PIN, 0);
        analogWrite(MOTOR2_CW_PIN, 0);
        analogWrite(MOTOR2_CCW_PIN, turnSpeed);
    }
    delay(300);
    motorStop();
}

// Set motor speed and direction functions
void moveForward(int speed) {
    analogWrite(MOTOR1_CW_PIN, speed);
    analogWrite(MOTOR1_CCW_PIN, 0);
    analogWrite(MOTOR2_CW_PIN, speed);
    analogWrite(MOTOR2_CCW_PIN, 0);
    analogWrite(MOTOR3_CW_PIN, speed);
    analogWrite(MOTOR3_CCW_PIN, 0);
    analogWrite(MOTOR4_CW_PIN, speed);
    analogWrite(MOTOR4_CCW_PIN, 0);
}

void moveBackward(int speed) {
    analogWrite(MOTOR1_CW_PIN, 0);
    analogWrite(MOTOR1_CCW_PIN, speed);
    analogWrite(MOTOR2_CW_PIN, 0);
    analogWrite(MOTOR2_CCW_PIN, speed);
    analogWrite(MOTOR3_CW_PIN, 0);
    analogWrite(MOTOR3_CCW_PIN, speed);
    analogWrite(MOTOR4_CW_PIN, 0);
    analogWrite(MOTOR4_CCW_PIN, speed);
}

void motorStop() {
    analogWrite(MOTOR1_CW_PIN, 0);
    analogWrite(MOTOR1_CCW_PIN, 0);
    analogWrite(MOTOR2_CW_PIN, 0);
    analogWrite(MOTOR2_CCW_PIN, 0);
    analogWrite(MOTOR3_CW_PIN, 0);
    analogWrite(MOTOR3_CCW_PIN, 0);
    analogWrite(MOTOR4_CW_PIN, 0);
    analogWrite(MOTOR4_CCW_PIN, 0);
}

// LED control functions
void setLEDArrayColor(int r, int g, int b) {
    for (int i = 0; i < 5; i++) {
        setLEDColor(i, r, g, b);
    }
}

void setLEDColor(int ledIndex, int r, int g, int b) {
    analogWrite(LED_R_PINS[ledIndex], r);
    analogWrite(LED_G_PINS[ledIndex], g);
    analogWrite(LED_B_PINS[ledIndex], b);
}
