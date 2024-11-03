// Configuration
#define ENABLE_DEBUG_OUTPUT

// Dependent parameters
#define BIT_LOW_LEVEL_DURATION_MIN 1400  // value in microseconds
#define BIT_LOW_LEVEL_DURATION_MAX 2000  // value in microseconds

#ifdef ENABLE_DEBUG_OUTPUT
  #define DEBUG_PRINT(...) if (Serial) { Serial.print(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...)
#endif

// Constants
#define TYPE_IO_PIN_INPUT_MODE INPUT_PULLUP
#define TYPE_IO_PIN 3U
#define MIC_PIN 2U
#define RX_TIMEOUT_MS 12U
#define IN_BUFFER_SIZE 96U
#define NIBBLE_RESET_BIT_POS 0x08

// Struct Definitions
typedef struct {
  uint8_t target;
  uint8_t command;
  uint8_t data[];
} rxMessage_t;

typedef enum {
  Target_TapeDesk = 0x00,
  Target_Unknown = 0x01,
  Target_CDDesk = 0x03,
  Target_CDChangerExt = 0x05,
  Target_CDChangerUpper = 0x06,
  Target_MDDesk = 0x07,
  Target_BaseUnit = 0x08
} rxMessageTarget_t;

typedef enum {
  Command_Control = 0x01,
  Command_AnyBodyHome = 0x08,
  Command_WakeUp = 0x09
} rxMessageCommand_t;

typedef enum {
  SubCommand_Playback = 0x01,
  SubCommand_SeekTrack = 0x03,
  SubCommand_SetConfig = 0x04
} rxMessageSubCommand_t;

typedef enum {
  Playback_Play = 0x01,
  Playback_FF = 0x04,
  Playback_REW = 0x08,
  Playback_Stop = 0x60
} SubCommandPlayback_t;

typedef enum {
  SetConfig_RepeatMode = 0x01,
  SetConfig_RandomMode = 0x02,
  SetConfig_FastForwarding = 0x10,
  SetConfig_FastRewinding = 0x20
} SubCommandSetConfig_t;

typedef enum {
  Jack35_ShortPress,
  Jack35_LongPress,
  Jack35_DoublePress,
  Jack35_VolUp,
  Jack35_VolDown
} Jack35Control_t;

// Messages
const uint8_t TAPECMD_POWER_ON[] = {0x08, 0x08, 0x01, 0x02};
const uint8_t TAPECMD_STOPPED[] = {0x08, 0x09, 0x00, 0x0C, 0x0E};
const uint8_t TAPECMD_PLAYING[] = {0x08, 0x09, 0x04, 0x01, 0x05};
const uint8_t TAPECMD_SEEKING[] = {0x08, 0x09, 0x05, 0x01, 0x06};
const uint8_t TAPECMD_CASSETTE_PRESENT[] = {0x08, 0x0B, 0x09, 0x00, 0x04, 0x00, 0x00, 0x0C, 0x03};
const uint8_t TAPECMD_PLAYBACK[] = {0x08, 0x0B, 0x09, 0x00, 0x04, 0x00, 0x00, 0x01, 0x00};
const uint8_t TAPECMD_RANDOM_PLAY[] = {0x08, 0x0B, 0x09, 0x00, 0x06, 0x00, 0x00, 0x01, 0x0E};
const uint8_t TAPECMD_REPEAT_PLAY[] = {0x08, 0x0B, 0x09, 0x00, 0x05, 0x00, 0x00, 0x01, 0x0F};
const uint8_t TAPECMD_FAST_REWIND[] = {0x08, 0x0B, 0x09, 0x03, 0x04, 0x00, 0x01, 0x01, 0x0E};
const uint8_t TAPECMD_FAST_FORWARD[] = {0x08, 0x0B, 0x09, 0x02, 0x04, 0x00, 0x01, 0x01, 0x0D};

// Global Variables
static uint8_t inNibblesBuffer[IN_BUFFER_SIZE] = {0U};
static uint8_t nibblesReceived = 0;
static uint8_t biteShiftMask = NIBBLE_RESET_BIT_POS;
static uint32_t rx_time_us = 0;
static uint32_t rx_time_ms = 0;

// Function Prototypes
void bufferReset();
void collectInputData();
void send_nibble(const uint8_t nibble);
void send_message(const uint8_t *message, uint8_t length);
void process_radio_message(const rxMessage_t *message);
void Jack35Control(Jack35Control_t event);

// Setup Function
void setup() {
  pinMode(MIC_PIN, INPUT);
  pinMode(TYPE_IO_PIN, TYPE_IO_PIN_INPUT_MODE);
  attachInterrupt(digitalPinToInterrupt(TYPE_IO_PIN), collectInputData, CHANGE);

  #ifdef ENABLE_DEBUG_OUTPUT
    Serial.begin(9600);
  #endif

  DEBUG_PRINT("Init....\r\n");
}

// Main Loop
void loop() {
  if ((millis() - rx_time_ms) > RX_TIMEOUT_MS && nibblesReceived != 0U) {
    noInterrupts();
    DEBUG_PRINT("RX[");
    DEBUG_PRINT(nibblesReceived);
    DEBUG_PRINT("] ");

    for (int i = 0; i < nibblesReceived; i++) {
      DEBUG_PRINT(inNibblesBuffer[i], HEX);
    }
    DEBUG_PRINT("\r\n");

    process_radio_message(reinterpret_cast<rxMessage_t*>(inNibblesBuffer));
    bufferReset();
    rx_time_ms = millis();
    interrupts();
  }
}

// Function Definitions
void bufferReset() {
  memset(inNibblesBuffer, 0, sizeof(inNibblesBuffer));
  nibblesReceived = 0;
  biteShiftMask = NIBBLE_RESET_BIT_POS;
}

void collectInputData() {
  uint32_t elapsed_time = micros() - rx_time_us;
  rx_time_us = micros();
  rx_time_ms = millis();

  if (digitalRead(TYPE_IO_PIN) == LOW) return;

  if (elapsed_time > BIT_LOW_LEVEL_DURATION_MIN && elapsed_time < BIT_LOW_LEVEL_DURATION_MAX) {
    inNibblesBuffer[nibblesReceived] |= biteShiftMask;
  }

  biteShiftMask >>= 1U;

  if (biteShiftMask == 0U) {
    biteShiftMask = NIBBLE_RESET_BIT_POS;
    ++nibblesReceived;
  }

  if (nibblesReceived >= IN_BUFFER_SIZE) {
    DEBUG_PRINT("Buffer overflow, reset!\r\n");
    bufferReset();
  }
}

void send_nibble(const uint8_t nibble) {
  uint8_t nibbleShiftMask = 0x08;

  while (nibbleShiftMask) {
    digitalWrite(TYPE_IO_PIN, LOW);
    bool bit_value = nibble & nibbleShiftMask;
    delayMicroseconds(bit_value ? 1780 : 600);
    digitalWrite(TYPE_IO_PIN, HIGH);
    delayMicroseconds(bit_value ? 1200 : 2380);
    nibbleShiftMask >>= 1U;
  }
}

void send_message(const uint8_t *message, uint8_t length) {
  DEBUG_PRINT("TX[");
  DEBUG_PRINT(length);
  DEBUG_PRINT("] ");

  for (int i = 0; i < length; i++) {
    DEBUG_PRINT(message[i], HEX);
  }
  DEBUG_PRINT("\r\n");

  noInterrupts();
  while (digitalRead(TYPE_IO_PIN) != HIGH) {
    delay(10);
  }

  detachInterrupt(digitalPinToInterrupt(TYPE_IO_PIN));
  digitalWrite(TYPE_IO_PIN, HIGH);
  pinMode(TYPE_IO_PIN, OUTPUT);

  for (uint8_t i = 0; i < length; i++) {
    send_nibble(message[i]);
  }

  pinMode(TYPE_IO_PIN, TYPE_IO_PIN_INPUT_MODE);
  attachInterrupt(digitalPinToInterrupt(TYPE_IO_PIN), collectInputData, CHANGE);
  interrupts();
}

void process_radio_message(const rxMessage_t *message) {
  if (message->target != Target_TapeDesk) return;

  switch (message->command) {
    case Command_AnyBodyHome:
      DEBUG_PRINT("Any body home msg\r\n");
      send_message(TAPECMD_POWER_ON, sizeof(TAPECMD_POWER_ON));
      send_message(TAPECMD_CASSETTE_PRESENT, sizeof(TAPECMD_CASSETTE_PRESENT));
      break;
    case Command_WakeUp:
      DEBUG_PRINT("Wake up msg\r\n");
      send_message(TAPECMD_CASSETTE_PRESENT, sizeof(TAPECMD_CASSETTE_PRESENT));
      send_message(TAPECMD_STOPPED, sizeof(TAPECMD_STOPPED));
      break;
    case Command_Control:
      if (message->data[0] == SubCommand_Playback) {
        handle_playback_command(message->data[1], message->data[2]);
      } else if (message->data[0] == SubCommand_SetConfig) {
        handle_config_command(message->data[1]);
      }
      break;
    default:
      DEBUG_PRINT("Unknown msg\r\n");
      break;
  }
}

// Helper Functions
void handle_playback_command(uint8_t action, uint8_t config) {
  DEBUG_PRINT("Playback command: ");
  DEBUG_PRINT(action, HEX);
  DEBUG_PRINT("\r\n");

  switch (action) {
    case Playback_Play:
      send_play_command(config);
      break;
    case Playback_FF:
      send_message(TAPECMD_FAST_FORWARD, sizeof(TAPECMD_FAST_FORWARD));
      break;
    case Playback_REW:
      send_message(TAPECMD_FAST_REWIND, sizeof(TAPECMD_FAST_REWIND));
      break;
    case Playback_Stop:
      send_message(TAPECMD_STOPPED, sizeof(TAPECMD_STOPPED));
      break;
    default:
      DEBUG_PRINT("Unknown playback action\r\n");
      break;
  }
}

void handle_config_command(uint8_t config) {
  DEBUG_PRINT("Config command: ");
  DEBUG_PRINT(config, HEX);
  DEBUG_PRINT("\r\n");

  if (config & SetConfig_RandomMode) {
    send_message(TAPECMD_RANDOM_PLAY, sizeof(TAPECMD_RANDOM_PLAY));
  } else if (config & SetConfig_RepeatMode) {
    send_message(TAPECMD_REPEAT_PLAY, sizeof(TAPECMD_REPEAT_PLAY));
  }
}

void send_play_command(uint8_t config) {
  if (config & SetConfig_RandomMode) {
    send_message(TAPECMD_RANDOM_PLAY, sizeof(TAPECMD_RANDOM_PLAY));
  } else if (config & SetConfig_RepeatMode) {
    send_message(TAPECMD_REPEAT_PLAY, sizeof(TAPECMD_REPEAT_PLAY));
  } else {
    send_message(TAPECMD_PLAYBACK, sizeof(TAPECMD_PLAYBACK));
  }
}

void Jack35Control(Jack35Control_t event) {
  DEBUG_PRINT("Jack 3.5mm control event: ");
  DEBUG_PRINT(event, DEC);
  DEBUG_PRINT("\r\n");
}
