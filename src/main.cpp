#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <cctype>

// Edit these pins to match your actual wiring before upload.
constexpr uint8_t LED_PIN = 48;
constexpr uint8_t NUM_LEDS = 1;
constexpr uint16_t HTTP_PORT = 80;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;
constexpr uint32_t WIFI_POLL_INTERVAL_MS = 500;
constexpr bool KEEP_AP_ACTIVE_WITH_STA = true;

constexpr uint8_t FRONT_TRIG_PIN = 4;
constexpr uint8_t FRONT_ECHO_PIN = 5;
constexpr uint8_t LEFT_TRIG_PIN = 6;
constexpr uint8_t LEFT_ECHO_PIN = 7;
constexpr uint8_t RIGHT_TRIG_PIN = 8;
constexpr uint8_t RIGHT_ECHO_PIN = 9;

constexpr uint8_t FRONT_LEFT_EN_PIN = 10;
constexpr uint8_t FRONT_LEFT_IN1_PIN = 11;
constexpr uint8_t FRONT_LEFT_IN2_PIN = 12;
constexpr uint8_t REAR_LEFT_EN_PIN = 13;
constexpr uint8_t REAR_LEFT_IN1_PIN = 14;
constexpr uint8_t REAR_LEFT_IN2_PIN = 15;
constexpr uint8_t FRONT_RIGHT_EN_PIN = 16;
constexpr uint8_t FRONT_RIGHT_IN1_PIN = 17;
constexpr uint8_t FRONT_RIGHT_IN2_PIN = 18;
constexpr uint8_t REAR_RIGHT_EN_PIN = 21;
// On ESP32-S3 N16R8 boards, GPIO35/36/37 are used internally by flash/PSRAM.
// Keep motor control off those pins or the board can become unstable during boot.
constexpr uint8_t REAR_RIGHT_IN1_PIN = 38;
constexpr uint8_t REAR_RIGHT_IN2_PIN = 39;

constexpr uint8_t FRONT_LEFT_PWM_CHANNEL = 0;
constexpr uint8_t REAR_LEFT_PWM_CHANNEL = 1;
constexpr uint8_t FRONT_RIGHT_PWM_CHANNEL = 2;
constexpr uint8_t REAR_RIGHT_PWM_CHANNEL = 3;
constexpr uint16_t MOTOR_PWM_FREQ = 2000;
constexpr uint8_t MOTOR_PWM_BITS = 8;

constexpr uint32_t MANUAL_COMMAND_TIMEOUT_MS = 900;
constexpr uint32_t SENSOR_POLL_INTERVAL_MS = 160;
constexpr uint32_t POLICE_LIGHT_INTERVAL_MS = 140;
constexpr uint32_t AUTO_REVERSE_MS = 350;
constexpr uint32_t AUTO_TURN_MS = 450;
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
constexpr uint32_t MQTT_STATUS_PUBLISH_INTERVAL_MS = 1500;
constexpr uint8_t DEFAULT_MANUAL_SPEED = 180;
constexpr uint8_t DEFAULT_AUTO_SPEED = 150;
constexpr uint8_t DEFAULT_TURN_SPEED = 170;
constexpr uint8_t CURVE_INNER_SPEED_PERCENT = 55;
constexpr uint16_t DEFAULT_MQTT_TLS_PORT = 8883;
constexpr uint16_t DEFAULT_MQTT_WS_PORT = 8884;
constexpr uint16_t MQTT_PACKET_BUFFER_SIZE = 1536;
constexpr int OBSTACLE_STOP_CM = 28;
constexpr int SENSOR_VALID_MIN_CM = 2;
constexpr int SENSOR_VALID_MAX_CM = 400;
constexpr unsigned long SONAR_TIMEOUT_US = 25000;

const char* AP_SSID = "ESP32-ROVER";
const char* AP_PASSWORD = "12345678";
const char* DEFAULT_PROJECT_MQTT_TOPIC = "rover/yanzoo-car-1";
const char* PREFS_NAMESPACE = "wifi";
const char* PREFS_SSID_KEY = "ssid";
const char* PREFS_PASSWORD_KEY = "password";
const char* PREFS_MQTT_ENABLED_KEY = "mqtt_en";
const char* PREFS_MQTT_HOST_KEY = "mqtt_host";
const char* PREFS_MQTT_PORT_KEY = "mqtt_port";
const char* PREFS_MQTT_USER_KEY = "mqtt_user";
const char* PREFS_MQTT_PASSWORD_KEY = "mqtt_pass";
const char* PREFS_MQTT_TOPIC_KEY = "mqtt_topic";
const char* PREFS_MQTT_WS_PORT_KEY = "mqtt_wsp";
const char* PREFS_MQTT_WS_PATH_KEY = "mqtt_path";
const char* DEFAULT_MQTT_WS_PATH = "/mqtt";

struct ColorRgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct MotorChannel {
  const char* name;
  uint8_t enPin;
  uint8_t in1Pin;
  uint8_t in2Pin;
  uint8_t pwmChannel;
};

struct UltrasonicSensor {
  const char* name;
  uint8_t trigPin;
  uint8_t echoPin;
  int distanceCm;
};

struct MqttConfig {
  bool enabled;
  String host;
  uint16_t port;
  String username;
  String password;
  String baseTopic;
  uint16_t wsPort;
  String wsPath;
};

enum class ControlMode {
  Manual,
  Autonomous
};

enum class DriveCommand {
  Stop,
  Forward,
  ForwardLeft,
  ForwardRight,
  Reverse,
  ReverseLeft,
  ReverseRight,
  Left,
  Right
};

enum class LedBehavior {
  StaticColor,
  PoliceMove
};

enum class AutoStage {
  Cruise,
  Reverse,
  Turn
};

const ColorRgb COLOR_RED = {255, 0, 0};
const ColorRgb COLOR_GREEN = {0, 255, 0};
const ColorRgb COLOR_BLUE = {0, 0, 255};
const ColorRgb COLOR_WHITE = {255, 255, 255};
const ColorRgb COLOR_PURPLE = {180, 0, 255};
const ColorRgb COLOR_BLACK = {0, 0, 0};
const ColorRgb POLICE_SEQUENCE[] = {
  {255, 0, 0},
  {0, 0, 0},
  {0, 0, 255},
  {0, 0, 0},
  {255, 255, 255},
  {0, 0, 0},
  {0, 255, 0},
  {0, 0, 0},
  {180, 0, 255},
  {0, 0, 0}
};

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(HTTP_PORT);
Preferences preferences;
WiFiClientSecure mqttSecureClient;
PubSubClient mqttClient(mqttSecureClient);

MotorChannel frontLeftMotor = {"Front Left", FRONT_LEFT_EN_PIN, FRONT_LEFT_IN1_PIN, FRONT_LEFT_IN2_PIN, FRONT_LEFT_PWM_CHANNEL};
MotorChannel rearLeftMotor = {"Rear Left", REAR_LEFT_EN_PIN, REAR_LEFT_IN1_PIN, REAR_LEFT_IN2_PIN, REAR_LEFT_PWM_CHANNEL};
MotorChannel frontRightMotor = {"Front Right", FRONT_RIGHT_EN_PIN, FRONT_RIGHT_IN1_PIN, FRONT_RIGHT_IN2_PIN, FRONT_RIGHT_PWM_CHANNEL};
MotorChannel rearRightMotor = {"Rear Right", REAR_RIGHT_EN_PIN, REAR_RIGHT_IN1_PIN, REAR_RIGHT_IN2_PIN, REAR_RIGHT_PWM_CHANNEL};

UltrasonicSensor frontSensor = {"Front", FRONT_TRIG_PIN, FRONT_ECHO_PIN, 999};
UltrasonicSensor leftSensor = {"Left", LEFT_TRIG_PIN, LEFT_ECHO_PIN, 999};
UltrasonicSensor rightSensor = {"Right", RIGHT_TRIG_PIN, RIGHT_ECHO_PIN, 999};

bool apModeActive = false;
bool lastStationAttempted = false;
bool lastStationSucceeded = false;
wl_status_t lastStationStatus = WL_DISCONNECTED;
String lastAttemptedSsid;
String lastConnectionMessage = "Booting...";

ControlMode controlMode = ControlMode::Manual;
DriveCommand currentDriveCommand = DriveCommand::Stop;
LedBehavior ledBehavior = LedBehavior::StaticColor;
ColorRgb selectedStaticColor = COLOR_GREEN;
ColorRgb appliedLedColor = COLOR_BLACK;
uint8_t manualSpeed = DEFAULT_MANUAL_SPEED;
uint8_t autoSpeed = DEFAULT_AUTO_SPEED;
uint8_t turnSpeed = DEFAULT_TURN_SPEED;
unsigned long lastManualCommandAt = 0;
unsigned long lastSensorPollAt = 0;
unsigned long lastLedAnimationAt = 0;
size_t policeSequenceIndex = 0;
AutoStage autoStage = AutoStage::Cruise;
DriveCommand queuedAutoTurn = DriveCommand::Left;
unsigned long autoStageUntil = 0;
MqttConfig mqttConfig = {false, "", DEFAULT_MQTT_TLS_PORT, "", "", "", DEFAULT_MQTT_WS_PORT, DEFAULT_MQTT_WS_PATH};
String mqttClientId;
String lastMqttMessage = "MQTT cloud disabled.";
unsigned long lastMqttReconnectAt = 0;
unsigned long lastMqttStatusPublishAt = 0;

void setConnectionMessage(const String& message) {
  if (lastConnectionMessage != message) {
    lastConnectionMessage = message;
  }
}

void setMqttMessage(const String& message) {
  if (lastMqttMessage != message) {
    lastMqttMessage = message;
  }
}

String jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);

    switch (c) {
      case '\\':
        escaped += "\\\\";
        break;
      case '\"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += c;
        break;
    }
  }

  return escaped;
}

String wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
    default:
      return "UNKNOWN_STATUS";
  }
}

String controlModeToString(ControlMode mode) {
  return mode == ControlMode::Autonomous ? "autonomous" : "manual";
}

String driveCommandToString(DriveCommand command) {
  switch (command) {
    case DriveCommand::Forward:
      return "forward";
    case DriveCommand::ForwardLeft:
      return "forward-left";
    case DriveCommand::ForwardRight:
      return "forward-right";
    case DriveCommand::Reverse:
      return "reverse";
    case DriveCommand::ReverseLeft:
      return "reverse-left";
    case DriveCommand::ReverseRight:
      return "reverse-right";
    case DriveCommand::Left:
      return "left";
    case DriveCommand::Right:
      return "right";
    case DriveCommand::Stop:
    default:
      return "stop";
  }
}

String ledBehaviorToString(LedBehavior behavior) {
  return behavior == LedBehavior::PoliceMove ? "police" : "static";
}

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }

  if (value > maxValue) {
    return maxValue;
  }

  return value;
}

String trimSlashes(const String& value) {
  int start = 0;
  int end = value.length();

  while (start < end && value.charAt(start) == '/') {
    ++start;
  }

  while (end > start && value.charAt(end - 1) == '/') {
    --end;
  }

  return value.substring(start, end);
}

String getDeviceSuffix() {
  uint64_t chipId = ESP.getEfuseMac();
  char buffer[13];
  snprintf(buffer, sizeof(buffer), "%08llX", static_cast<unsigned long long>(chipId & 0xFFFFFFFFULL));
  String suffix(buffer);
  suffix.toLowerCase();
  return suffix;
}

String getLegacyDeviceTopic() {
  return "rover/" + getDeviceSuffix();
}

String getDefaultMqttBaseTopic() {
  return DEFAULT_PROJECT_MQTT_TOPIC;
}

String readStoredString(const char* key, const String& defaultValue = "") {
  return preferences.isKey(key) ? preferences.getString(key, defaultValue) : defaultValue;
}

bool isMqttConfigured() {
  return mqttConfig.enabled &&
         mqttConfig.host.length() > 0 &&
         mqttConfig.baseTopic.length() > 0;
}

bool isMqttConnected() {
  return mqttClient.connected();
}

String mqttTopic(const String& leaf) {
  String base = trimSlashes(mqttConfig.baseTopic.length() ? mqttConfig.baseTopic : getDefaultMqttBaseTopic());
  return base + "/" + trimSlashes(leaf);
}

void loadMqttConfig() {
  mqttConfig.enabled = preferences.getBool(PREFS_MQTT_ENABLED_KEY, false);
  mqttConfig.host = readStoredString(PREFS_MQTT_HOST_KEY, "");
  mqttConfig.port = static_cast<uint16_t>(preferences.getUInt(PREFS_MQTT_PORT_KEY, DEFAULT_MQTT_TLS_PORT));
  mqttConfig.username = readStoredString(PREFS_MQTT_USER_KEY, "");
  mqttConfig.password = readStoredString(PREFS_MQTT_PASSWORD_KEY, "");
  mqttConfig.baseTopic = trimSlashes(readStoredString(PREFS_MQTT_TOPIC_KEY, ""));
  if (mqttConfig.baseTopic.length() == 0 || mqttConfig.baseTopic == trimSlashes(getLegacyDeviceTopic())) {
    mqttConfig.baseTopic = trimSlashes(getDefaultMqttBaseTopic());
  }
  mqttConfig.wsPort = static_cast<uint16_t>(preferences.getUInt(PREFS_MQTT_WS_PORT_KEY, DEFAULT_MQTT_WS_PORT));
  mqttConfig.wsPath = readStoredString(PREFS_MQTT_WS_PATH_KEY, DEFAULT_MQTT_WS_PATH);

  if (mqttConfig.wsPath.length() == 0 || mqttConfig.wsPath.charAt(0) != '/') {
    mqttConfig.wsPath = "/" + trimSlashes(mqttConfig.wsPath);
  }

  mqttClientId = "rover-" + getDeviceSuffix();
}

void saveMqttConfig(const MqttConfig& config) {
  preferences.putBool(PREFS_MQTT_ENABLED_KEY, config.enabled);
  preferences.putString(PREFS_MQTT_HOST_KEY, config.host);
  preferences.putUInt(PREFS_MQTT_PORT_KEY, config.port);
  preferences.putString(PREFS_MQTT_USER_KEY, config.username);
  preferences.putString(PREFS_MQTT_PASSWORD_KEY, config.password);
  preferences.putString(PREFS_MQTT_TOPIC_KEY, trimSlashes(config.baseTopic));
  preferences.putUInt(PREFS_MQTT_WS_PORT_KEY, config.wsPort);
  preferences.putString(PREFS_MQTT_WS_PATH_KEY, config.wsPath);
}

void clearStoredMqttConfig() {
  preferences.remove(PREFS_MQTT_ENABLED_KEY);
  preferences.remove(PREFS_MQTT_HOST_KEY);
  preferences.remove(PREFS_MQTT_PORT_KEY);
  preferences.remove(PREFS_MQTT_USER_KEY);
  preferences.remove(PREFS_MQTT_PASSWORD_KEY);
  preferences.remove(PREFS_MQTT_TOPIC_KEY);
  preferences.remove(PREFS_MQTT_WS_PORT_KEY);
  preferences.remove(PREFS_MQTT_WS_PATH_KEY);
}

String mqttConnectionLabel() {
  if (!mqttConfig.enabled) {
    return "disabled";
  }

  if (!isMqttConfigured()) {
    return "not configured";
  }

  if (mqttClient.connected()) {
    return "connected";
  }

  if (WiFi.status() != WL_CONNECTED) {
    return "waiting for Wi-Fi";
  }

  return "disconnected";
}

String getSavedSsid() {
  return readStoredString(PREFS_SSID_KEY, "");
}

String getSavedPassword() {
  return readStoredString(PREFS_PASSWORD_KEY, "");
}

bool hasSavedCredentials() {
  return getSavedSsid().length() > 0;
}

String getStaIpAddress() {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0";
}

String getApIpAddress() {
  return apModeActive ? WiFi.softAPIP().toString() : "0.0.0.0";
}

String getActiveIpAddress() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }

  if (apModeActive) {
    return WiFi.softAPIP().toString();
  }

  return "0.0.0.0";
}

String getCurrentModeLabel() {
  if (WiFi.status() == WL_CONNECTED) {
    return "STA connected";
  }

  if (apModeActive) {
    return "AP fallback";
  }

  if (lastStationAttempted && !lastStationSucceeded) {
    return "STA failed";
  }

  return "AP fallback";
}

String getStationResultLabel() {
  if (WiFi.status() == WL_CONNECTED || lastStationSucceeded) {
    return "STA connected";
  }

  if (lastStationAttempted) {
    return "STA failed";
  }

  return hasSavedCredentials() ? "STA idle" : "STA not configured";
}

String getCurrentSsidLabel() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.SSID();
  }

  return getSavedSsid();
}

bool isVehicleMoving() {
  return currentDriveCommand != DriveCommand::Stop;
}

bool isValidDistanceReading(int distanceCm) {
  return distanceCm >= SENSOR_VALID_MIN_CM && distanceCm <= SENSOR_VALID_MAX_CM;
}

bool hasValidSensorData() {
  return isValidDistanceReading(frontSensor.distanceCm) &&
         isValidDistanceReading(leftSensor.distanceCm) &&
         isValidDistanceReading(rightSensor.distanceCm);
}

String colorNameFromRgb(const ColorRgb& color) {
  if (color.r == COLOR_RED.r && color.g == COLOR_RED.g && color.b == COLOR_RED.b) {
    return "red";
  }

  if (color.r == COLOR_GREEN.r && color.g == COLOR_GREEN.g && color.b == COLOR_GREEN.b) {
    return "green";
  }

  if (color.r == COLOR_BLUE.r && color.g == COLOR_BLUE.g && color.b == COLOR_BLUE.b) {
    return "blue";
  }

  if (color.r == COLOR_WHITE.r && color.g == COLOR_WHITE.g && color.b == COLOR_WHITE.b) {
    return "white";
  }

  if (color.r == COLOR_PURPLE.r && color.g == COLOR_PURPLE.g && color.b == COLOR_PURPLE.b) {
    return "purple";
  }

  return "black";
}

ColorRgb colorFromName(const String& name, bool& found) {
  found = true;

  if (name == "red") {
    return COLOR_RED;
  }

  if (name == "green") {
    return COLOR_GREEN;
  }

  if (name == "blue") {
    return COLOR_BLUE;
  }

  if (name == "white") {
    return COLOR_WHITE;
  }

  if (name == "purple") {
    return COLOR_PURPLE;
  }

  if (name == "black" || name == "off") {
    return COLOR_BLACK;
  }

  found = false;
  return COLOR_BLACK;
}

DriveCommand driveCommandFromString(const String& commandName, bool& found) {
  found = true;

  if (commandName == "forward") {
    return DriveCommand::Forward;
  }

  if (commandName == "forward-left") {
    return DriveCommand::ForwardLeft;
  }

  if (commandName == "forward-right") {
    return DriveCommand::ForwardRight;
  }

  if (commandName == "reverse") {
    return DriveCommand::Reverse;
  }

  if (commandName == "reverse-left") {
    return DriveCommand::ReverseLeft;
  }

  if (commandName == "reverse-right") {
    return DriveCommand::ReverseRight;
  }

  if (commandName == "left") {
    return DriveCommand::Left;
  }

  if (commandName == "right") {
    return DriveCommand::Right;
  }

  if (commandName == "stop") {
    return DriveCommand::Stop;
  }

  found = false;
  return DriveCommand::Stop;
}

String buildStatusJson(const String& messageOverride = "") {
  String json;
  json.reserve(1024);
  json = "{";
  json += "\"mode\":\"" + jsonEscape(WiFi.status() == WL_CONNECTED ? "STA+AP ready" : getCurrentModeLabel()) + "\",";
  json += "\"stationResult\":\"" + jsonEscape(getStationResultLabel()) + "\",";
  json += "\"ip\":\"" + jsonEscape(getActiveIpAddress()) + "\",";
  json += "\"stationIp\":\"" + jsonEscape(getStaIpAddress()) + "\",";
  json += "\"apIp\":\"" + jsonEscape(getApIpAddress()) + "\",";
  json += "\"ssid\":\"" + jsonEscape(getCurrentSsidLabel()) + "\",";
  json += "\"savedSsid\":\"" + jsonEscape(getSavedSsid()) + "\",";
  json += "\"savedPassword\":\"" + jsonEscape(getSavedPassword()) + "\",";
  json += "\"apSsid\":\"" + jsonEscape(String(AP_SSID)) + "\",";
  json += "\"hasSavedCredentials\":";
  json += (hasSavedCredentials() ? "true" : "false");
  json += ",";
  json += "\"wifiStatus\":\"" + jsonEscape(wifiStatusToString(lastStationStatus)) + "\",";
  json += "\"controlMode\":\"" + jsonEscape(controlModeToString(controlMode)) + "\",";
  json += "\"driveCommand\":\"" + jsonEscape(driveCommandToString(currentDriveCommand)) + "\",";
  json += "\"manualSpeed\":" + String(manualSpeed) + ",";
  json += "\"autoSpeed\":" + String(autoSpeed) + ",";
  json += "\"frontDistanceCm\":" + String(frontSensor.distanceCm) + ",";
  json += "\"leftDistanceCm\":" + String(leftSensor.distanceCm) + ",";
  json += "\"rightDistanceCm\":" + String(rightSensor.distanceCm) + ",";
  json += "\"ledBehavior\":\"" + jsonEscape(ledBehaviorToString(ledBehavior)) + "\",";
  json += "\"selectedColor\":\"" + jsonEscape(colorNameFromRgb(selectedStaticColor)) + "\",";
  json += "\"mqttEnabled\":";
  json += (mqttConfig.enabled ? "true" : "false");
  json += ",";
  json += "\"mqttConnected\":";
  json += (mqttClient.connected() ? "true" : "false");
  json += ",";
  json += "\"mqttHost\":\"" + jsonEscape(mqttConfig.host) + "\",";
  json += "\"mqttPort\":" + String(mqttConfig.port) + ",";
  json += "\"mqttWsPort\":" + String(mqttConfig.wsPort) + ",";
  json += "\"mqttWsPath\":\"" + jsonEscape(mqttConfig.wsPath) + "\",";
  json += "\"mqttUsername\":\"" + jsonEscape(mqttConfig.username) + "\",";
  json += "\"mqttTopic\":\"" + jsonEscape(mqttConfig.baseTopic) + "\",";
  json += "\"mqttClientId\":\"" + jsonEscape(mqttClientId) + "\",";
  json += "\"mqttStatus\":\"" + jsonEscape(mqttConnectionLabel()) + "\",";
  json += "\"mqttMessage\":\"" + jsonEscape(lastMqttMessage) + "\",";
  json += "\"remoteHint\":\"" + jsonEscape("Internet control works through MQTT cloud when broker settings are saved and connected.") + "\",";
  json += "\"lastAttemptedSsid\":\"" + jsonEscape(lastAttemptedSsid) + "\",";
  json += "\"message\":\"" + jsonEscape(messageOverride.length() ? messageOverride : lastConnectionMessage) + "\"";
  json += "}";
  return json;
}

void sendJson(int statusCode, const String& payload) {
  server.send(statusCode, "application/json", payload);
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  if (appliedLedColor.r == r &&
      appliedLedColor.g == g &&
      appliedLedColor.b == b) {
    return;
  }

  appliedLedColor = {r, g, b};
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

void setColor(const ColorRgb& color) {
  setColor(color.r, color.g, color.b);
}

void setupMotorChannel(const MotorChannel& motor) {
  pinMode(motor.in1Pin, OUTPUT);
  pinMode(motor.in2Pin, OUTPUT);
  digitalWrite(motor.in1Pin, LOW);
  digitalWrite(motor.in2Pin, LOW);
  ledcSetup(motor.pwmChannel, MOTOR_PWM_FREQ, MOTOR_PWM_BITS);
  ledcAttachPin(motor.enPin, motor.pwmChannel);
  ledcWrite(motor.pwmChannel, 0);
}

void applyMotorOutput(const MotorChannel& motor, int power) {
  int clamped = clampInt(power, -255, 255);

  if (clamped > 0) {
    digitalWrite(motor.in1Pin, HIGH);
    digitalWrite(motor.in2Pin, LOW);
    ledcWrite(motor.pwmChannel, clamped);
    return;
  }

  if (clamped < 0) {
    digitalWrite(motor.in1Pin, LOW);
    digitalWrite(motor.in2Pin, HIGH);
    ledcWrite(motor.pwmChannel, -clamped);
    return;
  }

  digitalWrite(motor.in1Pin, LOW);
  digitalWrite(motor.in2Pin, LOW);
  ledcWrite(motor.pwmChannel, 0);
}

void applyDriveCommand(DriveCommand command, uint8_t speed) {
  int leftPower = 0;
  int rightPower = 0;
  const int curveSpeed = (static_cast<int>(speed) * CURVE_INNER_SPEED_PERCENT) / 100;

  switch (command) {
    case DriveCommand::Forward:
      leftPower = speed;
      rightPower = speed;
      break;
    case DriveCommand::ForwardLeft:
      leftPower = curveSpeed;
      rightPower = speed;
      break;
    case DriveCommand::ForwardRight:
      leftPower = speed;
      rightPower = curveSpeed;
      break;
    case DriveCommand::Reverse:
      leftPower = -speed;
      rightPower = -speed;
      break;
    case DriveCommand::ReverseLeft:
      leftPower = -curveSpeed;
      rightPower = -speed;
      break;
    case DriveCommand::ReverseRight:
      leftPower = -speed;
      rightPower = -curveSpeed;
      break;
    case DriveCommand::Left:
      leftPower = -speed;
      rightPower = speed;
      break;
    case DriveCommand::Right:
      leftPower = speed;
      rightPower = -speed;
      break;
    case DriveCommand::Stop:
    default:
      leftPower = 0;
      rightPower = 0;
      break;
  }

  currentDriveCommand = command;
  applyMotorOutput(frontLeftMotor, leftPower);
  applyMotorOutput(rearLeftMotor, leftPower);
  applyMotorOutput(frontRightMotor, rightPower);
  applyMotorOutput(rearRightMotor, rightPower);
}

void stopVehicle() {
  applyDriveCommand(DriveCommand::Stop, 0);
}

void setupSensors() {
  pinMode(frontSensor.trigPin, OUTPUT);
  pinMode(frontSensor.echoPin, INPUT_PULLDOWN);
  pinMode(leftSensor.trigPin, OUTPUT);
  pinMode(leftSensor.echoPin, INPUT_PULLDOWN);
  pinMode(rightSensor.trigPin, OUTPUT);
  pinMode(rightSensor.echoPin, INPUT_PULLDOWN);
  digitalWrite(frontSensor.trigPin, LOW);
  digitalWrite(leftSensor.trigPin, LOW);
  digitalWrite(rightSensor.trigPin, LOW);
}

int readDistanceCm(UltrasonicSensor& sensor) {
  digitalWrite(sensor.trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(sensor.trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sensor.trigPin, LOW);

  unsigned long duration = pulseIn(sensor.echoPin, HIGH, SONAR_TIMEOUT_US);
  sensor.distanceCm = duration == 0 ? 999 : static_cast<int>(duration / 58UL);
  return sensor.distanceCm;
}

void updateSensors() {
  if (millis() - lastSensorPollAt < SENSOR_POLL_INTERVAL_MS) {
    return;
  }

  lastSensorPollAt = millis();
  readDistanceCm(frontSensor);
  delay(4);
  readDistanceCm(leftSensor);
  delay(4);
  readDistanceCm(rightSensor);
}

void resetAutoStage() {
  autoStage = AutoStage::Cruise;
  autoStageUntil = 0;
  queuedAutoTurn = DriveCommand::Left;
}

void updateAutonomousMode() {
  if (controlMode != ControlMode::Autonomous) {
    return;
  }

  if (!hasValidSensorData()) {
    controlMode = ControlMode::Manual;
    resetAutoStage();
    stopVehicle();
    setConnectionMessage("Auto mode stopped because ultrasonic sensor data is not valid.");
    return;
  }

  unsigned long now = millis();

  if (autoStage == AutoStage::Reverse && now < autoStageUntil) {
    return;
  }

  if (autoStage == AutoStage::Reverse && now >= autoStageUntil) {
    autoStage = AutoStage::Turn;
    autoStageUntil = now + AUTO_TURN_MS;
    applyDriveCommand(queuedAutoTurn, turnSpeed);
    setConnectionMessage("Autonomous turn: " + driveCommandToString(queuedAutoTurn));
    return;
  }

  if (autoStage == AutoStage::Turn && now < autoStageUntil) {
    return;
  }

  if (frontSensor.distanceCm <= OBSTACLE_STOP_CM) {
    queuedAutoTurn = leftSensor.distanceCm >= rightSensor.distanceCm
      ? DriveCommand::Left
      : DriveCommand::Right;
    autoStage = AutoStage::Reverse;
    autoStageUntil = now + AUTO_REVERSE_MS;
    applyDriveCommand(DriveCommand::Reverse, autoSpeed);
    setConnectionMessage("Obstacle detected. Reversing before turning.");
    return;
  }

  autoStage = AutoStage::Cruise;
  applyDriveCommand(DriveCommand::Forward, autoSpeed);
  setConnectionMessage("Autonomous cruise active.");
}

void enforceManualSafetyStop() {
  if (controlMode != ControlMode::Manual) {
    return;
  }

  if (!isVehicleMoving()) {
    return;
  }

  if (millis() - lastManualCommandAt > MANUAL_COMMAND_TIMEOUT_MS) {
    stopVehicle();
    setConnectionMessage("Manual command timeout safety stop.");
  }
}

void updateLedState() {
  if (controlMode == ControlMode::Autonomous) {
    setColor(COLOR_RED);
    return;
  }

  if (ledBehavior == LedBehavior::PoliceMove) {
    if (isVehicleMoving() && millis() - lastLedAnimationAt >= POLICE_LIGHT_INTERVAL_MS) {
      lastLedAnimationAt = millis();
      policeSequenceIndex = (policeSequenceIndex + 1) %
                            (sizeof(POLICE_SEQUENCE) / sizeof(POLICE_SEQUENCE[0]));
    }
    setColor(POLICE_SEQUENCE[policeSequenceIndex]);
    return;
  }

  setColor(selectedStaticColor);
}

bool isValidHexColor(const String& hex) {
  if (hex.length() != 6) {
    return false;
  }

  for (size_t i = 0; i < hex.length(); ++i) {
    if (!std::isxdigit(static_cast<unsigned char>(hex.charAt(i)))) {
      return false;
    }
  }

  return true;
}

bool setColorFromHex(const String& hex) {
  if (!isValidHexColor(hex)) {
    return false;
  }

  long value = strtol(hex.c_str(), NULL, 16);
  selectedStaticColor = {
    static_cast<uint8_t>((value >> 16) & 0xFF),
    static_cast<uint8_t>((value >> 8) & 0xFF),
    static_cast<uint8_t>(value & 0xFF)
  };
  ledBehavior = LedBehavior::StaticColor;
  if (controlMode != ControlMode::Autonomous) {
    setColor(selectedStaticColor);
  }
  return true;
}

bool setNamedColor(const String& color) {
  bool found = false;
  selectedStaticColor = colorFromName(color, found);
  if (found) {
    ledBehavior = LedBehavior::StaticColor;
    if (controlMode != ControlMode::Autonomous) {
      setColor(selectedStaticColor);
    }
  }
  return found;
}

void publishMqttStatus(const String& messageOverride = "");

bool applyDriveRequest(DriveCommand command, int requestedSpeed, const String& sourceLabel, String& responseMessage) {
  manualSpeed = command == DriveCommand::Stop
    ? 0
    : static_cast<uint8_t>(clampInt(requestedSpeed, 80, 255));
  controlMode = ControlMode::Manual;
  resetAutoStage();
  lastManualCommandAt = millis();
  applyDriveCommand(command, manualSpeed);
  responseMessage = sourceLabel + " drive command: " + driveCommandToString(command);
  setConnectionMessage(responseMessage);
  return true;
}

bool applyModeRequest(const String& mode, int requestedSpeed, const String& sourceLabel, String& responseMessage) {
  if (mode == "manual") {
    controlMode = ControlMode::Manual;
    resetAutoStage();
    stopVehicle();
    responseMessage = sourceLabel + " manual mode enabled.";
    setConnectionMessage(responseMessage);
    return true;
  }

  if (mode == "auto") {
    if (!hasValidSensorData()) {
      controlMode = ControlMode::Manual;
      resetAutoStage();
      stopVehicle();
      responseMessage = "Auto mode is blocked until all three ultrasonic sensors return valid data.";
      setConnectionMessage(responseMessage);
      return false;
    }

    autoSpeed = static_cast<uint8_t>(clampInt(requestedSpeed, 90, 255));
    turnSpeed = static_cast<uint8_t>(clampInt(autoSpeed + 20, 120, 255));
    controlMode = ControlMode::Autonomous;
    resetAutoStage();
    responseMessage = sourceLabel + " autonomous avoid mode enabled at speed " + String(autoSpeed) + ".";
    setConnectionMessage(responseMessage);
    return true;
  }

  responseMessage = "Unsupported mode.";
  return false;
}

bool applyLedColorRequest(const String& color, const String& sourceLabel, String& responseMessage) {
  if (!setNamedColor(color)) {
    responseMessage = "Unsupported LED color.";
    return false;
  }

  responseMessage = sourceLabel + " static LED color set to " + colorNameFromRgb(selectedStaticColor);
  setConnectionMessage(responseMessage);
  return true;
}

bool applyLedBehaviorRequest(const String& mode, const String& sourceLabel, String& responseMessage) {
  if (mode == "police") {
    ledBehavior = LedBehavior::PoliceMove;
    policeSequenceIndex = 0;
    lastLedAnimationAt = millis();
    if (controlMode != ControlMode::Autonomous) {
      setColor(POLICE_SEQUENCE[policeSequenceIndex]);
    }
    responseMessage = sourceLabel + " police moving lights enabled.";
    setConnectionMessage(responseMessage);
    return true;
  }

  if (mode == "static") {
    ledBehavior = LedBehavior::StaticColor;
    if (controlMode != ControlMode::Autonomous) {
      setColor(selectedStaticColor);
    }
    responseMessage = sourceLabel + " static LED mode enabled.";
    setConnectionMessage(responseMessage);
    return true;
  }

  responseMessage = "Unsupported LED mode.";
  return false;
}

bool parseCommandPayload(const String& payload, String& command, int& value) {
  String trimmed = payload;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return false;
  }

  int separator = trimmed.indexOf('|');
  if (separator < 0) {
    separator = trimmed.indexOf(':');
  }
  if (separator < 0) {
    separator = trimmed.indexOf(',');
  }

  if (separator < 0) {
    command = trimmed;
    value = -1;
    return true;
  }

  command = trimmed.substring(0, separator);
  command.trim();
  String numeric = trimmed.substring(separator + 1);
  numeric.trim();
  value = numeric.toInt();
  return command.length() > 0;
}

void publishMqttAvailability(const char* state) {
  if (!mqttClient.connected()) {
    return;
  }

  String topic = mqttTopic("state/availability");
  mqttClient.publish(topic.c_str(), state, true);
}

void publishMqttStatus(const String& messageOverride) {
  if (!mqttClient.connected()) {
    return;
  }

  String topic = mqttTopic("state/status");
  String payload = buildStatusJson(messageOverride);
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  lastMqttStatusPublishAt = millis();
}

void handleMqttMessage(char* topic, byte* payload, unsigned int length) {
  String topicName(topic);
  String body;
  body.reserve(length + 1);
  for (unsigned int i = 0; i < length; ++i) {
    body += static_cast<char>(payload[i]);
  }
  body.trim();

  String responseMessage;
  bool handled = false;

  if (topicName == mqttTopic("cmd/drive")) {
    String commandName;
    int requestedSpeed = -1;
    bool parsed = parseCommandPayload(body, commandName, requestedSpeed);
    bool found = false;
    DriveCommand command = parsed ? driveCommandFromString(commandName, found) : DriveCommand::Stop;
    if (parsed && found) {
      handled = applyDriveRequest(command,
                                  requestedSpeed > 0 ? requestedSpeed : DEFAULT_MANUAL_SPEED,
                                  "MQTT",
                                  responseMessage);
    } else {
      responseMessage = "Unsupported MQTT drive command.";
    }
  } else if (topicName == mqttTopic("cmd/mode")) {
    String modeName;
    int requestedSpeed = -1;
    bool parsed = parseCommandPayload(body, modeName, requestedSpeed);
    handled = parsed && applyModeRequest(modeName,
                                         requestedSpeed > 0 ? requestedSpeed : autoSpeed,
                                         "MQTT",
                                         responseMessage);
  } else if (topicName == mqttTopic("cmd/led/color")) {
    handled = applyLedColorRequest(body, "MQTT", responseMessage);
  } else if (topicName == mqttTopic("cmd/led/behavior")) {
    handled = applyLedBehaviorRequest(body, "MQTT", responseMessage);
  } else if (topicName == mqttTopic("cmd/status")) {
    responseMessage = "MQTT status refresh requested.";
    handled = true;
  } else {
    responseMessage = "Unsupported MQTT topic.";
  }

  setMqttMessage(responseMessage);
  Serial.printf("[MQTT] %s => %s\n", topicName.c_str(), responseMessage.c_str());

  if (mqttClient.connected()) {
    publishMqttStatus(responseMessage);
  }
}

bool connectToMqttBroker() {
  if (!isMqttConfigured()) {
    setMqttMessage(mqttConfig.enabled
      ? "MQTT enabled but broker settings are incomplete."
      : "MQTT cloud disabled.");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    setMqttMessage("Waiting for Wi-Fi before MQTT connect.");
    return false;
  }

  mqttSecureClient.setInsecure();
  mqttClient.setServer(mqttConfig.host.c_str(), mqttConfig.port);
  mqttClient.setBufferSize(MQTT_PACKET_BUFFER_SIZE);

  String willTopic = mqttTopic("state/availability");
  bool connected = mqttClient.connect(
    mqttClientId.c_str(),
    mqttConfig.username.length() ? mqttConfig.username.c_str() : nullptr,
    mqttConfig.password.length() ? mqttConfig.password.c_str() : nullptr,
    willTopic.c_str(),
    0,
    true,
    "offline"
  );

  if (!connected) {
    setMqttMessage("MQTT connect failed. rc=" + String(mqttClient.state()));
    Serial.printf("[MQTT] Connect failed. rc=%d\n", mqttClient.state());
    return false;
  }

  mqttClient.subscribe(mqttTopic("cmd/drive").c_str());
  mqttClient.subscribe(mqttTopic("cmd/mode").c_str());
  mqttClient.subscribe(mqttTopic("cmd/led/color").c_str());
  mqttClient.subscribe(mqttTopic("cmd/led/behavior").c_str());
  mqttClient.subscribe(mqttTopic("cmd/status").c_str());
  publishMqttAvailability("online");
  setMqttMessage("MQTT connected to " + mqttConfig.host + ".");
  Serial.printf("[MQTT] Connected to %s:%u\n", mqttConfig.host.c_str(), mqttConfig.port);
  publishMqttStatus(lastMqttMessage);
  return true;
}

void updateMqttConnection() {
  if (!mqttConfig.enabled) {
    if (mqttClient.connected()) {
      mqttClient.disconnect();
    }
    setMqttMessage("MQTT cloud disabled.");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (mqttClient.connected()) {
      mqttClient.disconnect();
    }
    setMqttMessage("MQTT waiting for Wi-Fi.");
    return;
  }

  if (mqttClient.connected()) {
    mqttClient.loop();

    if (millis() - lastMqttStatusPublishAt >= MQTT_STATUS_PUBLISH_INTERVAL_MS) {
      publishMqttStatus();
    }
    return;
  }

  if (millis() - lastMqttReconnectAt < MQTT_RECONNECT_INTERVAL_MS) {
    return;
  }

  lastMqttReconnectAt = millis();
  connectToMqttBroker();
}

bool startAccessPoint(bool keepStation) {
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.mode(keepStation ? WIFI_AP_STA : WIFI_AP);

  bool started = WiFi.softAP(AP_SSID, AP_PASSWORD);
  apModeActive = started;

  if (started) {
    Serial.printf("[WiFi] AP active. SSID: %s, IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("[WiFi] Failed to start AP fallback.");
  }

  return started;
}

void ensureApFallback(const String& reason) {
  setConnectionMessage(reason);
  Serial.printf("[WiFi] %s\n", reason.c_str());

  if (!apModeActive) {
    startAccessPoint(false);
  } else {
    Serial.printf("[WiFi] AP fallback already active. IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

void saveCredentials(const String& ssid, const String& password) {
  preferences.putString(PREFS_SSID_KEY, ssid);
  preferences.putString(PREFS_PASSWORD_KEY, password);
}

void clearCredentials() {
  preferences.remove(PREFS_SSID_KEY);
  preferences.remove(PREFS_PASSWORD_KEY);
}

bool connectToWifi(const String& ssid, const String& password, bool keepApAlive) {
  lastAttemptedSsid = ssid;
  lastStationAttempted = true;
  lastStationSucceeded = false;
  lastStationStatus = WL_IDLE_STATUS;

  Serial.printf("[WiFi] Attempting STA connection to SSID: %s\n", ssid.c_str());
  Serial.printf("[WiFi] Boot mode: %s\n", keepApAlive ? "AP+STA" : "STA only");

  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(keepApAlive ? WIFI_AP_STA : WIFI_STA);
  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startTime = millis();
  while ((lastStationStatus = WiFi.status()) != WL_CONNECTED &&
         millis() - startTime < WIFI_CONNECT_TIMEOUT_MS) {
    delay(WIFI_POLL_INTERVAL_MS);
    Serial.print(".");
  }
  Serial.println();

  lastStationStatus = WiFi.status();

  if (lastStationStatus == WL_CONNECTED) {
    lastStationSucceeded = true;
    setConnectionMessage("Connected to " + ssid + " at " + WiFi.localIP().toString());
    Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    if (KEEP_AP_ACTIVE_WITH_STA) {
      startAccessPoint(true);
      setConnectionMessage("Connected to " + ssid + " and local hotspot is also active.");
    }
    return true;
  }

  Serial.printf("[WiFi] Connection timeout reached. Final status: %s (%d)\n",
                wifiStatusToString(lastStationStatus).c_str(),
                static_cast<int>(lastStationStatus));
  setConnectionMessage("Failed to connect to " + ssid + ". Final status: " +
                       wifiStatusToString(lastStationStatus));

  if (!keepApAlive) {
    WiFi.disconnect(true, true);
  } else {
    WiFi.disconnect(false, true);
  }

  return false;
}

const char* webpage = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta name=viewport content="width=device-width,initial-scale=1">
  <title>ESP32 Rover</title>
  <style>
    :root{color-scheme:dark;--bg:#07080b;--panel:#170c10;--panelEdge:#5a1f27;--text:#fff2f2;--muted:#f4bbbb;--accent:#ff4439;--accentDark:#b71317;--steel:#47525b;--ok:#40da7b}
    *{box-sizing:border-box}
    body{margin:0;padding:14px;font-family:'Trebuchet MS',Verdana,sans-serif;color:var(--text);background:radial-gradient(circle at top,#4a1219 0,#18070b 42%,#06070a 100%)}
    section,details{background:linear-gradient(180deg,rgba(29,13,17,.96),rgba(13,10,11,.98));border:1px solid var(--panelEdge);border-radius:18px;padding:14px;margin-bottom:12px;box-shadow:0 18px 40px rgba(0,0,0,.28)}
    .shell{max-width:980px;margin:0 auto}
    h1,h2,h3{margin:0 0 10px}
    .hero{position:relative;overflow:hidden}
    .hero::after{content:'';position:absolute;left:-10%;right:-10%;bottom:-70px;height:170px;background:radial-gradient(circle,rgba(255,68,57,.24),rgba(255,68,57,0) 70%)}
    .dashTop{display:flex;justify-content:space-between;align-items:center;gap:12px;position:relative;z-index:1}
    .eyebrow{font-size:12px;letter-spacing:2px;color:#ff9e96;text-transform:uppercase}
    .carMark{width:122px;height:72px;position:relative;flex:0 0 auto}
    .carBody{position:absolute;left:10px;right:10px;bottom:12px;height:32px;border-radius:22px 22px 16px 16px;background:linear-gradient(180deg,#ff7164,#c11618);box-shadow:0 10px 24px rgba(255,68,57,.35)}
    .carBody::before{content:'';position:absolute;left:22px;right:22px;top:-15px;height:18px;border-radius:18px 18px 10px 10px;background:linear-gradient(180deg,#ff8973,#e33328)}
    .carBody::after{content:'';position:absolute;left:14px;right:14px;bottom:6px;height:4px;border-radius:999px;background:rgba(255,255,255,.25)}
    p,small{color:var(--muted)}
    #feedback,.feedback{min-height:22px;font-weight:700;margin-top:8px;position:relative;z-index:1}
    .chipRow,.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center}
    .chip{display:inline-flex;align-items:center;padding:8px 12px;border-radius:999px;background:rgba(255,68,57,.12);border:1px solid rgba(255,68,57,.28);font-size:13px;position:relative;z-index:1}
    .telemetry{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;position:relative;z-index:1}
    .tile{padding:11px 12px;border-radius:14px;background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.06);min-height:54px}
    button,input,select{font-size:15px;border-radius:12px}
    button{border:0;background:linear-gradient(180deg,var(--accent),var(--accentDark));color:#fff;padding:12px 14px;min-width:88px;box-shadow:0 10px 22px rgba(255,68,57,.22);position:relative;overflow:hidden;cursor:pointer;transition:transform .14s ease,filter .14s ease,box-shadow .18s ease}
    button::after{content:'';position:absolute;top:-10%;bottom:-10%;left:-42%;width:32%;background:linear-gradient(90deg,rgba(255,255,255,0),rgba(255,255,255,.34),rgba(255,255,255,0));transform:skewX(-22deg) translateX(0);opacity:0;pointer-events:none;transition:transform .28s ease,opacity .18s ease}
    button:hover{filter:brightness(1.08) saturate(1.04);transform:translateY(-1px)}
    button:hover::after{opacity:1;transform:skewX(-22deg) translateX(300%)}
    button.gray{background:linear-gradient(180deg,#5f6a75,#323841)}
    button.stop{background:linear-gradient(180deg,#ff5d72,#bf1e34)}
    button.active{outline:2px solid rgba(255,255,255,.28);box-shadow:0 0 0 2px rgba(255,68,57,.18),0 12px 24px rgba(255,68,57,.28)}
    input[type=text],input[type=password],input[type=number],select{width:100%;padding:12px 14px;border:1px solid #5c252c;background:#0d1014;color:var(--text)}
    select,input[type=range],details summary{cursor:pointer}
    input[type=range]{accent-color:var(--accent);flex:1;min-width:150px}
    .speedRow{display:flex;align-items:center;gap:10px;flex-wrap:wrap}
    .toggleRow{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-top:12px}
    .toggleRow button{margin:0;width:100%}
    .dpadShell{display:flex;justify-content:center;padding:12px 0 6px}
    .dpadStage{width:336px;height:336px;border-radius:40%;position:relative;background:radial-gradient(circle at 35% 28%,#48515d 0,#242a32 34%,#0b0d11 74%,#050608 100%);border:1px solid rgba(255,255,255,.08);box-shadow:inset 0 18px 28px rgba(255,255,255,.08),inset 0 -16px 24px rgba(0,0,0,.55),0 24px 46px rgba(0,0,0,.38),0 0 40px rgba(74,190,255,.08);display:flex;align-items:center;justify-content:center;overflow:hidden}
    .dpadStage::before{content:'';position:absolute;inset:24px;border-radius:38%;border:1px solid rgba(140,226,255,.16);box-shadow:inset 0 0 28px rgba(140,226,255,.08),0 0 22px rgba(140,226,255,.06)}
    .dpadStage::after{content:'';position:absolute;left:50%;top:50%;width:188px;height:188px;transform:translate(-50%,-50%);border-radius:50%;background:radial-gradient(circle,rgba(120,200,255,.08),rgba(120,200,255,0) 72%)}
    .dpadGrid{display:grid;grid-template-columns:repeat(3,88px);grid-template-rows:repeat(3,88px);gap:12px;justify-content:center;align-content:center;position:relative;z-index:1}
    .dpadBtn{border:1px solid rgba(255,255,255,.12);background:linear-gradient(180deg,#3b434f 0,#1a1f26 52%,#0b0d11 100%);color:#eaf7ff;padding:0;min-width:0;width:88px;height:88px;border-radius:30px;font-size:38px;line-height:1;display:flex;align-items:center;justify-content:center;text-shadow:0 0 10px rgba(132,224,255,.28);box-shadow:inset 0 12px 18px rgba(255,255,255,.12),inset 0 -12px 18px rgba(0,0,0,.45),0 12px 20px rgba(0,0,0,.34),0 0 18px rgba(95,212,255,.08)}
    .dpadBtn.stop{background:radial-gradient(circle at 50% 38%,#ffd0d8 0,#ff7c8d 24%,#ff4f63 42%,#b91832 100%);border-radius:50%;font-size:32px;text-shadow:none;box-shadow:0 0 0 6px rgba(255,255,255,.04),0 0 18px rgba(255,95,130,.35),inset 0 8px 14px rgba(255,255,255,.16),0 14px 24px rgba(255,68,57,.28)}
    .dpadBtn:hover{box-shadow:inset 0 12px 18px rgba(255,255,255,.12),inset 0 -12px 18px rgba(0,0,0,.45),0 12px 20px rgba(0,0,0,.34),0 0 24px rgba(95,212,255,.18)}
    .dpadBtn:active{transform:translateY(1px);box-shadow:inset 0 6px 14px rgba(0,0,0,.32),0 6px 12px rgba(0,0,0,.22),0 0 12px rgba(95,212,255,.12)}
    .dpadBtn.gray{background:linear-gradient(180deg,#242a31 0,#15191f 52%,#090b0e 100%);color:#afbcc8;opacity:.62;box-shadow:inset 0 8px 14px rgba(255,255,255,.06),inset 0 -10px 16px rgba(0,0,0,.48),0 8px 16px rgba(0,0,0,.22)}
    .dpadBtn.active{background:linear-gradient(180deg,#7fe3ff 0,#3fb7ff 28%,#1e79e8 68%,#0d2b7f 100%);color:#f6fdff;box-shadow:0 0 0 2px rgba(142,231,255,.2),0 0 22px rgba(92,204,255,.34),inset 0 12px 18px rgba(255,255,255,.2),0 16px 28px rgba(0,0,0,.36)}
    .dpadBtn.stop.gray{opacity:.72}
    .dpadBtn.stop.active{box-shadow:0 0 0 2px rgba(255,255,255,.12),0 0 24px rgba(255,95,130,.42),inset 0 8px 14px rgba(255,255,255,.16),0 14px 24px rgba(255,68,57,.32)}
    .split{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
    .checkRow{display:flex;align-items:center;gap:10px}
    .checkRow input{width:auto}
    .controlShell{margin-top:12px}
    .hidden{display:none!important}
    #joystickWrap{display:flex;justify-content:center;padding:10px 0 6px}
    #joystick{width:220px;height:220px;border-radius:50%;position:relative;overflow:hidden;touch-action:none;user-select:none;background:radial-gradient(circle at 50% 50%,#35171b 0,#211015 55%,#0e0b0c 100%);border:1px solid #7b2731;box-shadow:inset 0 0 28px rgba(255,68,57,.18)}
    #joystick::before,#joystick::after{content:'';position:absolute;background:rgba(255,68,57,.18)}
    #joystick::before{left:50%;top:18px;bottom:18px;width:2px;transform:translateX(-50%)}
    #joystick::after{top:50%;left:18px;right:18px;height:2px;transform:translateY(-50%)}
    #stick{width:76px;height:76px;border-radius:50%;position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);background:linear-gradient(180deg,#ff8d80,#ff4a40 52%,#ba1518);box-shadow:0 14px 28px rgba(255,68,57,.36)}
    details summary{cursor:pointer;font-weight:700;list-style:none}
    details summary::-webkit-details-marker{display:none}
    details[open] summary{margin-bottom:12px}
    .okText{color:var(--ok)}
    @media (max-width:520px){.telemetry{grid-template-columns:1fr}.dpadStage{width:292px;height:292px}.dpadGrid{grid-template-columns:repeat(3,76px);grid-template-rows:repeat(3,76px)}.dpadBtn{width:76px;height:76px;font-size:32px;border-radius:24px}.carMark{width:96px;height:60px}}
  </style>
</head>
<body>
  <main class=shell>
  <section class=hero>
    <div class=dashTop>
      <div>
        <div class=eyebrow>Local Rover Control</div>
        <h1>ESP32-S3 4WD Remote</h1>
        <p>Use this page while your phone is connected to the ESP hotspot or the same Wi-Fi router as the rover. Save Wi-Fi and MQTT here, then use the GitHub Pages remote app from anywhere.</p>
      </div>
      <div class=carMark><div class=carBody></div></div>
    </div>
    <div id=feedback class=feedback></div>
    <div class=chipRow>
      <div class=chip id=modeChip>Loading network...</div>
      <div class=chip id=mqttChip>Cloud: disabled</div>
      <div class=chip id=controlChip>Control: manual</div>
      <div class=chip id=driveChip>Drive: stop</div>
      <div class=chip id=speedChip>Manual 180 | Auto 150</div>
    </div>
    <div class=telemetry>
      <div class=tile id=networkTile>Router -- | Hotspot --</div>
      <div class=tile id=sensorTile>Front -- cm | Left -- cm | Right -- cm</div>
      <div class=tile id=messageTile>Booting...</div>
      <div class=tile id=remoteTile>Project broker defaults can be saved here first, then reused in the GitHub Pages remote app later.</div>
      <div class=tile id=mqttTile>MQTT host -- | topic --</div>
    </div>
  </section>

  <section>
    <h2>Drive Console</h2>
    <div class=speedRow>
      <span>Speed</span>
      <input id=speed type=range min=90 max=255 value=180>
      <span id=speedv>180</span>
    </div>
    <div class=toggleRow>
      <button id=showButtons type=button class=active>Buttons</button>
      <button id=showJoystick type=button class=gray>Joystick</button>
    </div>
    <div id=buttonsPanel class=controlShell>
      <div class=dpadShell>
        <div class=dpadStage>
          <div class=dpadGrid>
            <button id=forwardLeft type=button class=dpadBtn title="Forward left" aria-label="Forward left">&#8598;</button>
            <button id=forward type=button class=dpadBtn title="Forward" aria-label="Forward">&#8593;</button>
            <button id=forwardRight type=button class=dpadBtn title="Forward right" aria-label="Forward right">&#8599;</button>
            <button id=left type=button class=dpadBtn title="Left" aria-label="Left">&#8592;</button>
            <button id=stop type=button class="dpadBtn stop" title="Stop" aria-label="Stop">&#9679;</button>
            <button id=right type=button class=dpadBtn title="Right" aria-label="Right">&#8594;</button>
            <button id=reverseLeft type=button class=dpadBtn title="Reverse left" aria-label="Reverse left">&#8601;</button>
            <button id=reverse type=button class=dpadBtn title="Reverse" aria-label="Reverse">&#8595;</button>
            <button id=reverseRight type=button class=dpadBtn title="Reverse right" aria-label="Reverse right">&#8600;</button>
          </div>
        </div>
      </div>
      <small>Hold an arrow to move. Diagonal arrows act like pressing two directions together.</small>
    </div>
    <div id=joystickPanel class="controlShell hidden">
      <div id=joystickWrap>
        <div id=joystick>
          <div id=stick></div>
        </div>
      </div>
      <small>Drag the joystick in any direction. Release to stop. It supports forward, reverse, curves, and turns.</small>
    </div>
  </section>

  <section>
    <h2>Auto Drive</h2>
    <div class=row>
      <button id=autoModeBtn type=button onclick="startAuto()">Start Auto Avoid</button>
      <button id=manualModeBtn type=button class=gray onclick="mode('manual')">Manual Mode</button>
      <button id=stopAutoBtn type=button class=gray onclick="stopAuto()">Stop Auto</button>
    </div>
    <small>Auto mode uses the three ultrasonic sensors to detect obstacles, reverse, and turn away before impact. While auto mode is active, the rover LED is forced red.</small>
  </section>

  <details>
    <summary>RGB Lights And Effects</summary>
    <div class=row>
      <select id=ledColor>
        <option value=green>Green</option>
        <option value=red>Red</option>
        <option value=white>White</option>
        <option value=blue>Blue</option>
        <option value=purple>Purple</option>
        <option value=black>Off</option>
      </select>
      <button id=applyLed type=button>Apply Color</button>
    </div>
    <div class=row>
      <button id=policeBtn type=button onclick="ledmode('police')">Police Move</button>
      <button id=staticBtn type=button class=gray onclick="ledmode('static')">Static</button>
    </div>
    <small>The default power-on light is green. If the rover power switch is off, the LED is off because the board has no power.</small>
  </details>

  <section>
    <h2>Wi-Fi And Hotspot</h2>
    <form id=w>
      <input id=ssid type=text placeholder="Wi-Fi SSID">
      <br><br>
      <input id=pwd type=password placeholder="Wi-Fi Password">
      <br><br>
      <div class=row>
        <button type=submit>Save And Connect</button>
        <button type=button class=gray onclick="clearWifi()">Clear Wi-Fi</button>
      </div>
    </form>
    <small>If your router Wi-Fi is available, the ESP uses it. If not, the ESP creates its own hotspot so you can still connect your phone directly and control the rover.</small>
  </section>

  <section>
    <h2>MQTT Broker</h2>
    <form id=mqttForm>
      <div class=checkRow>
        <input id=mqttEnabled type=checkbox checked>
        <label for=mqttEnabled>Enable cloud MQTT control</label>
      </div>
      <br>
      <input id=mqttHost type=text placeholder="Broker host, for example yourcluster.s1.eu.hivemq.cloud">
      <br><br>
      <div class=split>
        <input id=mqttPort type=number min=1 max=65535 placeholder="ESP TLS port, usually 8883">
        <input id=mqttWsPort type=number min=1 max=65535 placeholder="WebSocket port, usually 8884">
      </div>
      <br>
      <input id=mqttWsPath type=text placeholder="/mqtt">
      <br><br>
      <input id=mqttUser type=text placeholder="MQTT username">
      <br><br>
      <input id=mqttPass type=password placeholder="MQTT password. Leave blank to keep the saved one.">
      <br><br>
      <input id=mqttTopic type=text placeholder="Topic base, for example rover/abcd1234">
      <br><br>
      <div class=row>
        <button type=submit>Save Cloud MQTT</button>
        <button type=button class=gray onclick="clearMqtt()">Clear Cloud MQTT</button>
      </div>
    </form>
    <small>Use the TLS TCP port for the ESP32 and the secure WebSocket port/path for the GitHub Pages remote app. Host, topic base, username, and WebSocket settings can be prefilled here, but the broker password stays manual.</small>
  </section>

  <script>
    let holds={};
    let driveUi='buttons';
    let joystickActive=false;
    let joystickTimer=null;
    let joystickCommand='stop';
    let joystickSpeed=0;
    let mqttFormPrimed=false;
    let ledSelectionDirty=false;
    let wifiFormDirty=false;
    let statusPauseUntil=0;
    const commandRepeatMs=120;
    const statusRefreshMs=1500;
    const statusPauseMs=450;
    const mqttDefaults={
      host:'8038be31051d4e368ce62a5753aaf95d.s1.eu.hivemq.cloud',
      port:'8883',
      wsPort:'8884',
      wsPath:'/mqtt',
      username:'Yanzoo4wd',
      topic:'rover/yanzoo-car-1'
    };
    const driveButtons={
      forwardLeft:'forward-left',
      forward:'forward',
      forwardRight:'forward-right',
      left:'left',
      stop:'stop',
      right:'right',
      reverseLeft:'reverse-left',
      reverse:'reverse',
      reverseRight:'reverse-right'
    };
    const joystick=document.getElementById('joystick');
    const stick=document.getElementById('stick');
    const ledSelect=document.getElementById('ledColor');
    const ssidInput=document.getElementById('ssid');
    const pwdInput=document.getElementById('pwd');
    const driveButtonIds=Object.keys(driveButtons);

    function syncModeButtons(mode){
      const autoBtn=document.getElementById('autoModeBtn');
      const manualBtn=document.getElementById('manualModeBtn');
      const autoSelected=mode==='autonomous';
      autoBtn.classList.toggle('active',autoSelected);
      autoBtn.classList.toggle('gray',!autoSelected);
      manualBtn.classList.toggle('active',!autoSelected);
      manualBtn.classList.toggle('gray',autoSelected);
    }

    function syncDriveButtons(command){
      const selected=command||'stop';
      driveButtonIds.forEach(id=>{
        const button=document.getElementById(id);
        const active=driveButtons[id]===selected;
        button.classList.toggle('active',active);
        button.classList.toggle('gray',!active);
      });
    }

    function syncLedButtons(behavior){
      const policeBtn=document.getElementById('policeBtn');
      const staticBtn=document.getElementById('staticBtn');
      const policeSelected=behavior==='police';
      policeBtn.classList.toggle('active',policeSelected);
      policeBtn.classList.toggle('gray',!policeSelected);
      staticBtn.classList.toggle('active',!policeSelected);
      staticBtn.classList.toggle('gray',policeSelected);
    }

    function markUiBusy(duration=statusPauseMs){
      statusPauseUntil=Date.now()+duration;
    }

    function renderStatus(d){
      document.getElementById('modeChip').textContent=`Network: ${d.mode}`;
      document.getElementById('mqttChip').textContent=`Cloud: ${d.mqttStatus}`;
      document.getElementById('controlChip').textContent=`Control: ${d.controlMode}`;
      document.getElementById('driveChip').textContent=`Drive: ${d.driveCommand}`;
      document.getElementById('speedChip').textContent=`Manual ${d.manualSpeed} | Auto ${d.autoSpeed}`;
      syncModeButtons(d.controlMode);
      syncDriveButtons(d.driveCommand);
      syncLedButtons(d.ledBehavior);
      document.getElementById('sensorTile').textContent=`Front ${d.frontDistanceCm} cm | Left ${d.leftDistanceCm} cm | Right ${d.rightDistanceCm} cm`;
      document.getElementById('networkTile').textContent=`Router ${d.ssid||'not set'} @ ${d.stationIp} | Hotspot ${d.apSsid} @ ${d.apIp}`;
      document.getElementById('mqttTile').textContent=`MQTT ${d.mqttHost||'not set'}:${d.mqttPort} | Topic ${d.mqttTopic||'not set'} | ${d.mqttMessage||''}`;
      document.getElementById('messageTile').textContent=d.message||'';
      document.getElementById('remoteTile').textContent=d.remoteHint||'';
      if(d.selectedColor && !ledSelectionDirty && document.activeElement!==ledSelect) ledSelect.value=d.selectedColor;
      if(!wifiFormDirty && document.activeElement!==ssidInput && document.activeElement!==pwdInput){
        ssidInput.value=d.savedSsid||d.ssid||'';
        pwdInput.value=d.savedPassword||'';
      }
      if(!mqttFormPrimed){
        document.getElementById('mqttEnabled').checked=!!d.mqttEnabled;
        document.getElementById('mqttHost').value=d.mqttHost||mqttDefaults.host;
        document.getElementById('mqttPort').value=d.mqttPort||mqttDefaults.port;
        document.getElementById('mqttWsPort').value=d.mqttWsPort||mqttDefaults.wsPort;
        document.getElementById('mqttWsPath').value=d.mqttWsPath||mqttDefaults.wsPath;
        document.getElementById('mqttUser').value=d.mqttUsername||mqttDefaults.username;
        document.getElementById('mqttTopic').value=d.mqttTopic||mqttDefaults.topic;
        mqttFormPrimed=true;
      }
    }

    function fb(message,isError){
      const el=document.getElementById('feedback');
      el.textContent=message||'';
      el.style.color=isError?'#ff8f8f':'#84f5a8';
    }

    async function json(url){
      const response=await fetch(url,{cache:'no-store'});
      const text=await response.text();
      let data={};
      try{
        data=text?JSON.parse(text):{};
      }catch(error){
        if(!response.ok) throw new Error(text||'Request failed');
        data={ok:response.ok,message:text};
      }
      if(typeof data.ok==='undefined') data.ok=response.ok;
      return data;
    }

    async function hit(url){
      const response=await fetch(url,{cache:'no-store'});
      const text=await response.text();
      if(!response.ok) throw new Error(text||'Request failed');
      return text;
    }

    async function status(force){
      if(!force && (Date.now()<statusPauseUntil || Object.keys(holds).length || joystickActive)) return;
      try{
        const d=await json('/status');
        renderStatus(d);
      }catch(error){
        fb('Status read failed',true);
      }
    }

    async function drive(cmd,speed){
      const chosenSpeed=speed===undefined?document.getElementById('speed').value:speed;
      markUiBusy();
      syncModeButtons('manual');
      syncDriveButtons(cmd);
      document.getElementById('controlChip').textContent='Control: manual';
      document.getElementById('driveChip').textContent=`Drive: ${cmd}`;
      try{
        await hit(`/drive?cmd=${cmd}&speed=${chosenSpeed}`);
      }catch(error){
        fb(error.message,true);
        throw error;
      }
    }

    function bind(id,cmd){
      const button=document.getElementById(id);
      const start=event=>{
        event.preventDefault();
        if(holds[id]) return;
        markUiBusy();
        drive(cmd).catch(()=>{});
        holds[id]=setInterval(()=>{
          markUiBusy();
          drive(cmd).catch(()=>{});
        },commandRepeatMs);
      };
      const stop=event=>{
        if(event) event.preventDefault();
        if(holds[id]){
          clearInterval(holds[id]);
          delete holds[id];
        }
        markUiBusy();
        drive('stop',0).catch(()=>{});
      };
      ['mousedown','touchstart'].forEach(name=>button.addEventListener(name,start,{passive:false}));
      ['mouseup','mouseleave','touchend','touchcancel'].forEach(name=>button.addEventListener(name,stop,{passive:false}));
    }

    async function mode(value){
      markUiBusy();
      try{
        const url=value==='auto'
          ? `/mode?m=auto&speed=${document.getElementById('speed').value}`
          : `/mode?m=${value}`;
        const data=await json(url);
        fb(data.message,!data.ok);
        if(value!=='auto'){
          syncDriveButtons('stop');
          document.getElementById('driveChip').textContent='Drive: stop';
        }
        if(typeof data.controlMode!=='undefined') renderStatus(data);
        else await status(true);
      }catch(error){
        fb(error.message,true);
      }
    }

    async function startAuto(){
      stopJoystick(true);
      await mode('auto');
    }

    async function stopAuto(){
      await mode('manual');
      await drive('stop',0).catch(()=>{});
    }

    async function led(value){
      markUiBusy();
      syncLedButtons('static');
      try{
        const data=await json(`/led?c=${value}`);
        fb(data.message,!data.ok);
        ledSelectionDirty=false;
        if(typeof data.controlMode!=='undefined') renderStatus(data);
        else await status(true);
      }catch(error){
        fb(error.message,true);
      }
    }

    async function ledmode(value){
      markUiBusy();
      syncLedButtons(value);
      try{
        const data=await json(`/ledmode?m=${value}`);
        fb(data.message,!data.ok);
        if(typeof data.controlMode!=='undefined') renderStatus(data);
        else await status(true);
      }catch(error){
        fb(error.message,true);
      }
    }

    async function clearWifi(){
      markUiBusy(900);
      try{
        const data=await json('/wifi/clear');
        fb(data.message,!data.ok);
        renderStatus(data);
      }catch(error){
        fb(error.message,true);
      }
    }

    async function clearMqtt(){
      markUiBusy(900);
      try{
        const data=await json('/mqtt/clear');
        fb(data.message,!data.ok);
        document.getElementById('mqttPass').value='';
        mqttFormPrimed=false;
        renderStatus(data);
      }catch(error){
        fb(error.message,true);
      }
    }

    function resetStick(){
      stick.style.left='50%';
      stick.style.top='50%';
    }

    function setDriveUi(kind){
      driveUi=kind;
      const buttonsMode=kind==='buttons';
      document.getElementById('buttonsPanel').classList.toggle('hidden',!buttonsMode);
      document.getElementById('joystickPanel').classList.toggle('hidden',buttonsMode);
      document.getElementById('showButtons').classList.toggle('active',buttonsMode);
      document.getElementById('showButtons').classList.toggle('gray',!buttonsMode);
      document.getElementById('showJoystick').classList.toggle('active',!buttonsMode);
      document.getElementById('showJoystick').classList.toggle('gray',buttonsMode);
      if(buttonsMode) stopJoystick(true);
    }

    function classifyJoystick(nx,ny){
      const magnitude=Math.min(1,Math.hypot(nx,ny));
      if(magnitude<0.18) return {cmd:'stop',speed:0};
      const maxSpeed=Number(document.getElementById('speed').value);
      const speed=Math.max(90,Math.round(maxSpeed*Math.max(0.45,magnitude)));
      if(ny<-0.35){
        if(nx<-0.28) return {cmd:'forward-left',speed};
        if(nx>0.28) return {cmd:'forward-right',speed};
        return {cmd:'forward',speed};
      }
      if(ny>0.35){
        if(nx<-0.28) return {cmd:'reverse-left',speed};
        if(nx>0.28) return {cmd:'reverse-right',speed};
        return {cmd:'reverse',speed};
      }
      if(nx<0) return {cmd:'left',speed};
      return {cmd:'right',speed};
    }

    function updateJoystickCommand(cmd,speed){
      if(cmd===joystickCommand&&Math.abs(speed-joystickSpeed)<8) return;
      joystickCommand=cmd;
      joystickSpeed=speed;
      drive(cmd,speed).catch(()=>{});
    }

    function updateJoystickFromPoint(clientX,clientY){
      const rect=joystick.getBoundingClientRect();
      const centerX=rect.left+(rect.width/2);
      const centerY=rect.top+(rect.height/2);
      let dx=clientX-centerX;
      let dy=clientY-centerY;
      const limit=72;
      const distance=Math.hypot(dx,dy);
      if(distance>limit){
        const ratio=limit/distance;
        dx*=ratio;
        dy*=ratio;
      }
      stick.style.left=`calc(50% + ${dx}px)`;
      stick.style.top=`calc(50% + ${dy}px)`;
      const next=classifyJoystick(dx/limit,dy/limit);
      updateJoystickCommand(next.cmd,next.speed);
    }

    function startJoystick(event){
      if(driveUi!=='joystick') return;
      event.preventDefault();
      joystickActive=true;
      if(joystick.setPointerCapture) joystick.setPointerCapture(event.pointerId);
      if(joystickTimer) clearInterval(joystickTimer);
      joystickTimer=setInterval(()=>{
        if(joystickActive){
          markUiBusy();
          drive(joystickCommand,joystickSpeed).catch(()=>{});
        }
      },commandRepeatMs);
      updateJoystickFromPoint(event.clientX,event.clientY);
    }

    function moveJoystick(event){
      if(!joystickActive||driveUi!=='joystick') return;
      event.preventDefault();
      updateJoystickFromPoint(event.clientX,event.clientY);
    }

    function stopJoystick(sendStop){
      const hadCommand=joystickActive||joystickCommand!=='stop';
      joystickActive=false;
      if(joystickTimer){
        clearInterval(joystickTimer);
        joystickTimer=null;
      }
      joystickCommand='stop';
      joystickSpeed=0;
      resetStick();
      if(sendStop&&hadCommand){
        drive('stop',0).catch(()=>{});
      }
    }

    joystick.addEventListener('pointerdown',startJoystick);
    joystick.addEventListener('pointermove',moveJoystick);
    joystick.addEventListener('pointerup',()=>stopJoystick(true));
    joystick.addEventListener('pointercancel',()=>stopJoystick(true));
    joystick.addEventListener('pointerleave',event=>{if(joystickActive&&event.pointerType==='mouse') stopJoystick(true)});

    document.getElementById('showButtons').addEventListener('click',()=>setDriveUi('buttons'));
    document.getElementById('showJoystick').addEventListener('click',()=>setDriveUi('joystick'));
    document.getElementById('speed').addEventListener('input',event=>document.getElementById('speedv').textContent=event.target.value);
    document.getElementById('stop').addEventListener('click',()=>drive('stop',0).catch(()=>{}));
    ssidInput.addEventListener('input',()=>{wifiFormDirty=true;});
    ssidInput.addEventListener('focus',()=>{wifiFormDirty=true;});
    pwdInput.addEventListener('input',()=>{wifiFormDirty=true;});
    pwdInput.addEventListener('focus',()=>{wifiFormDirty=true;});
    ledSelect.addEventListener('change',()=>{ledSelectionDirty=true;});
    ledSelect.addEventListener('focus',()=>{ledSelectionDirty=true;});
    document.getElementById('applyLed').addEventListener('click',()=>led(ledSelect.value));
    document.getElementById('w').addEventListener('submit',async event=>{
      event.preventDefault();
      const ssid=document.getElementById('ssid').value.trim();
      const password=document.getElementById('pwd').value;
      if(!ssid){
        fb('SSID is required',true);
        return;
      }
      try{
        const data=await json(`/wifi/save?ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`);
        fb(data.message,!data.ok);
        wifiFormDirty=false;
        renderStatus(data);
      }catch(error){
        fb(error.message,true);
      }
    });

    document.getElementById('mqttForm').addEventListener('submit',async event=>{
      event.preventDefault();
      markUiBusy(900);
      const host=document.getElementById('mqttHost').value.trim();
      if(!host){
        fb('MQTT host is required',true);
        return;
      }
      const params=new URLSearchParams({
        enabled:document.getElementById('mqttEnabled').checked?'1':'0',
        host,
        port:document.getElementById('mqttPort').value||'8883',
        wsPort:document.getElementById('mqttWsPort').value||'8884',
        wsPath:document.getElementById('mqttWsPath').value.trim()||'/mqtt',
        username:document.getElementById('mqttUser').value.trim(),
        password:document.getElementById('mqttPass').value,
        topic:document.getElementById('mqttTopic').value.trim()
      });
      try{
        const data=await json(`/mqtt/save?${params.toString()}`);
        fb(data.message,!data.ok);
        document.getElementById('mqttPass').value='';
        mqttFormPrimed=false;
        renderStatus(data);
      }catch(error){
        fb(error.message,true);
      }
    });

    bind('forwardLeft','forward-left');
    bind('forward','forward');
    bind('forwardRight','forward-right');
    bind('left','left');
    bind('right','right');
    bind('reverseLeft','reverse-left');
    bind('reverse','reverse');
    bind('reverseRight','reverse-right');
    setDriveUi('buttons');
    syncModeButtons('manual');
    syncDriveButtons('stop');
    syncLedButtons('static');
    resetStick();
    status(true);
    setInterval(()=>status(false),statusRefreshMs);
  </script>
  </main>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleStatus() {
  sendJson(200, buildStatusJson());
}

void handleSet() {
  String color = server.arg("c");

  if (!setNamedColor(color)) {
    server.send(400, "text/plain", "Unsupported color");
    return;
  }

  setConnectionMessage("Static LED color set to " + colorNameFromRgb(selectedStaticColor));
  server.send(200, "text/plain", "OK");
}

void handleSetColor() {
  String hex = server.arg("hex");

  if (!setColorFromHex(hex)) {
    server.send(400, "text/plain", "Hex color must be exactly 6 hexadecimal characters");
    return;
  }

  setConnectionMessage("Custom LED color updated.");
  server.send(200, "text/plain", "OK");
}

void handleBlink() {
  String responseMessage;
  applyLedBehaviorRequest("police", "HTTP", responseMessage);
  server.send(200, "text/plain", "OK");
}

void handleDrive() {
  bool found = false;
  DriveCommand command = driveCommandFromString(server.arg("cmd"), found);

  if (!found) {
    sendJson(400, "{\"ok\":false,\"message\":\"Unsupported drive command.\"}");
    return;
  }

  String responseMessage;
  applyDriveRequest(command, server.arg("speed").toInt(), "HTTP", responseMessage);
  sendJson(200, "{\"ok\":true,\"message\":\"" + jsonEscape(responseMessage) + "\"}");
}

void handleMode() {
  String mode = server.arg("m");
  String responseMessage;
  if (!applyModeRequest(mode, server.arg("speed").toInt(), "HTTP", responseMessage)) {
    sendJson(400, buildStatusJson(responseMessage));
    return;
  }

  sendJson(200, buildStatusJson(responseMessage));
}

void handleLed() {
  String responseMessage;
  if (!applyLedColorRequest(server.arg("c"), "HTTP", responseMessage)) {
    sendJson(400, buildStatusJson(responseMessage));
    return;
  }

  sendJson(200, buildStatusJson(responseMessage));
}

void handleLedMode() {
  String responseMessage;
  if (!applyLedBehaviorRequest(server.arg("m"), "HTTP", responseMessage)) {
    sendJson(400, buildStatusJson(responseMessage));
    return;
  }

  sendJson(200, buildStatusJson(responseMessage));
}

void handleWifiSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  ssid.trim();

  if (ssid.length() == 0) {
    sendJson(400, "{\"message\":\"SSID is required.\"}");
    return;
  }

  Serial.printf("[WiFi] Saving credentials for SSID: %s\n", ssid.c_str());
  saveCredentials(ssid, password);

  bool connected = connectToWifi(ssid, password, false);

  if (connected) {
    String responseMessage = "Connected to " + ssid + ". STA " + getStaIpAddress() + " | AP " + getApIpAddress();
    setConnectionMessage(responseMessage);
    sendJson(200, buildStatusJson(responseMessage));
    return;
  }

  ensureApFallback("Saved Wi-Fi failed. Staying in hotspot mode.");
  sendJson(500, buildStatusJson(lastConnectionMessage));
}

void handleWifiClear() {
  Serial.println("[WiFi] Clearing saved Wi-Fi credentials.");
  clearCredentials();
  WiFi.disconnect(true, true);
  lastStationAttempted = false;
  lastStationSucceeded = false;
  lastStationStatus = WL_DISCONNECTED;
  lastAttemptedSsid = "";
  startAccessPoint(false);
  setConnectionMessage("Saved Wi-Fi cleared. Local hotspot mode only.");
  sendJson(200, buildStatusJson(lastConnectionMessage));
}

void handleMqttSave() {
  MqttConfig newConfig = mqttConfig;
  String host = server.arg("host");
  String password = server.arg("password");
  String topic = server.arg("topic");
  String wsPath = server.arg("wsPath");
  int mqttPortValue = server.arg("port").length() ? server.arg("port").toInt() : DEFAULT_MQTT_TLS_PORT;
  int mqttWsPortValue = server.arg("wsPort").length() ? server.arg("wsPort").toInt() : DEFAULT_MQTT_WS_PORT;

  host.trim();
  topic.trim();
  wsPath.trim();

  if (host.length() == 0) {
    sendJson(400, "{\"ok\":false,\"message\":\"MQTT host is required.\"}");
    return;
  }

  newConfig.enabled = server.arg("enabled") != "0";
  newConfig.host = host;
  newConfig.port = static_cast<uint16_t>(clampInt(mqttPortValue, 1, 65535));
  newConfig.username = server.arg("username");
  newConfig.baseTopic = trimSlashes(topic.length() ? topic : getDefaultMqttBaseTopic());
  newConfig.wsPort = static_cast<uint16_t>(clampInt(mqttWsPortValue, 1, 65535));
  newConfig.wsPath = wsPath.length() ? wsPath : DEFAULT_MQTT_WS_PATH;

  if (newConfig.wsPath.charAt(0) != '/') {
    newConfig.wsPath = "/" + newConfig.wsPath;
  }

  if (password.length() > 0) {
    newConfig.password = password;
  }

  saveMqttConfig(newConfig);
  mqttConfig = newConfig;
  mqttClient.disconnect();
  lastMqttReconnectAt = 0;
  lastMqttStatusPublishAt = 0;

  String responseMessage;
  if (!mqttConfig.enabled) {
    setMqttMessage("MQTT cloud disabled.");
    responseMessage = "MQTT settings saved. Cloud control is disabled.";
    sendJson(200, buildStatusJson(responseMessage));
    return;
  }

  if (WiFi.status() == WL_CONNECTED && connectToMqttBroker()) {
    responseMessage = "MQTT settings saved and connected.";
  } else if (WiFi.status() == WL_CONNECTED) {
    responseMessage = "MQTT settings saved. " + lastMqttMessage;
  } else {
    setMqttMessage("MQTT settings saved. Waiting for Wi-Fi.");
    responseMessage = lastMqttMessage;
  }

  sendJson(200, buildStatusJson(responseMessage));
}

void handleMqttClear() {
  clearStoredMqttConfig();
  mqttClient.disconnect();
  mqttConfig = {false, "", DEFAULT_MQTT_TLS_PORT, "", "", getDefaultMqttBaseTopic(), DEFAULT_MQTT_WS_PORT, DEFAULT_MQTT_WS_PATH};
  setMqttMessage("MQTT settings cleared.");
  sendJson(200, buildStatusJson(lastMqttMessage));
}

void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/drive", HTTP_GET, handleDrive);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/led", HTTP_GET, handleLed);
  server.on("/ledmode", HTTP_GET, handleLedMode);
  server.on("/set", HTTP_GET, handleSet);
  server.on("/setcolor", HTTP_GET, handleSetColor);
  server.on("/blink", HTTP_GET, handleBlink);
  server.on("/wifi/save", HTTP_GET, handleWifiSave);
  server.on("/wifi/clear", HTTP_GET, handleWifiClear);
  server.on("/wifi/save", HTTP_POST, handleWifiSave);
  server.on("/wifi/clear", HTTP_POST, handleWifiClear);
  server.on("/mqtt/save", HTTP_GET, handleMqttSave);
  server.on("/mqtt/clear", HTTP_GET, handleMqttClear);
  server.on("/mqtt/save", HTTP_POST, handleMqttSave);
  server.on("/mqtt/clear", HTTP_POST, handleMqttClear);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
}

void initializeNetwork() {
  Serial.println("[WiFi] Boot mode: provisioning-aware startup");

  if (hasSavedCredentials()) {
    String savedSsid = getSavedSsid();
    String savedPassword = getSavedPassword();

    Serial.printf("[WiFi] Saved credentials found for SSID: %s\n", savedSsid.c_str());
    if (connectToWifi(savedSsid, savedPassword, false)) {
      Serial.println("[WiFi] Boot completed in STA connected mode.");
      return;
    }

    Serial.println("[WiFi] STA failed. Starting hotspot fallback.");
    ensureApFallback("Saved Wi-Fi failed. Starting hotspot fallback.");
    return;
  }

  lastStationAttempted = false;
  lastStationSucceeded = false;
  lastStationStatus = WL_DISCONNECTED;
  lastAttemptedSsid = "";
  ensureApFallback("No saved Wi-Fi credentials. Connect to hotspot to configure Wi-Fi.");
}

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.clear();
  pixels.show();

  setupMotorChannel(frontLeftMotor);
  setupMotorChannel(rearLeftMotor);
  setupMotorChannel(frontRightMotor);
  setupMotorChannel(rearRightMotor);
  stopVehicle();
  setupSensors();

  preferences.begin(PREFS_NAMESPACE, false);
  loadMqttConfig();
  mqttClient.setCallback(handleMqttMessage);
  initializeNetwork();
  setupRoutes();

  server.begin();
  Serial.println("[HTTP] Server started");
}

void loop() {
  updateSensors();
  updateAutonomousMode();
  enforceManualSafetyStop();
  updateLedState();
  updateMqttConnection();
  server.handleClient();
}
