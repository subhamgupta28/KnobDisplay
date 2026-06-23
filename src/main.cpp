#include "lcd/lv_fs_sd.h"
#include "lcd/lcd_bsp.h"
#include "lcd/cst816.h"
#include "lcd/lcd_bl_pwm_bsp.h"
#include "lcd/lcd_config.h"
#include "ui.h"
#include "lcd/bidi_switch_knob.h"
#include "Automata.h"
#include "ArduinoJson.h"
#include <WiFi.h>
#include "lcd/drv2605.c"
#include <SPI.h>
#include <Arduino.h>
#include <time.h>
#include "lcd/sd_card_bsp.h"
#include "AutomataAddOn.h"
#include <HardwareSerial.h>
#include "SDWebServer.h"

const char *HOST = "automata.realsubhamgupta.in";
int PORT = 443;
const char *MQTT_HOST = "mqtt.realsubhamgupta.in";
// const char *HOST = "raspberry.local";
// int PORT = 8010;

Preferences preferences;
Automata automata("KNOB", "DISPLAY", HOST, PORT, MQTT_HOST, PORT);
AutomataAddOn automataAddOn(HOST, PORT);
SDWebServer *sdWebServer;

// HardwareSerial SerialPort(1);
long d = 8000;
long st = millis();
unsigned long startMillis;
int ch = millis();
long start = millis();
JsonDocument doc;

#define SENSOR_SDA 11
#define SENSOR_SCL 12

uint8_t effect = 1;

static lv_obj_t *meter;
lv_meter_indicator_t *needle;
static lv_obj_t *gif;

int bright = 60;
#define EXAMPLE_ENCODER_ECA_PIN 8
#define EXAMPLE_ENCODER_ECB_PIN 7
unsigned long lastEncoderMove = 0;
bool encoderMoving = false;
const unsigned long ENCODER_IDLE_TIMEOUT = 300; // ms
#define SET_BIT(reg, bit) (reg |= ((uint32_t)0x01 << bit))
#define CLEAR_BIT(reg, bit) (reg &= (~((uint32_t)0x01 << bit)))
#define READ_BIT(reg, bit) (((uint32_t)reg >> bit) & 0x01)
#define BIT_EVEN_ALL (0x00ffffff)

// --- Display power management ---
unsigned long lastActivity = 0;              // tracks last user action time
const unsigned long DISPLAY_TIMEOUT = 60000; // 60 seconds idle timeout
bool displayOn = true;
bool showAnimation = false;
bool animationActive = false; // tracks current state

void action(const Action action)
{
  if (action.data.containsKey("bright"))
  {
    bright = action.data["bright"];
    lcd_bl_pwm_bsp_init(bright);
  }

  if (action.data.containsKey("animation"))
  {
    bool val = action.data["animation"];
    showAnimation = val;

  }

  String jsonString;
  serializeJson(action.data, jsonString);
  Serial.println(jsonString);
}
void sendData()
{
  automata.sendData(doc);
}

EventGroupHandle_t knob_even_ = NULL;
int selectedScreen = 1;
static knob_handle_t s_knob = 0;
int encPos = 0;
SemaphoreHandle_t mutex;
int actionNum = 0;
String actionVal = "";
bool actionSend = false;
String selectedAutomation = "";
bool changeDetected = false;
bool masterSendLive = false;
String masterOption = "";
static lv_obj_t *clock_meter;
static lv_meter_indicator_t *hour_hand;
static lv_meter_indicator_t *minute_hand;
static lv_meter_indicator_t *second_hand;
void turnDisplayOff()
{
  if (displayOn)
  {
    lcd_bl_pwm_bsp_init(0); // backlight off
    // lv_disp_load_scr(ui_Screen5);
    displayOn = false;
    Serial.println("[Display] Turned OFF due to inactivity");
  }
}

void turnDisplayOn()
{
  if (!displayOn)
  {
    lcd_bl_pwm_bsp_init(bright); // restore brightness
    drv2605_play_effect(47);     // short vibration
    // lv_disp_load_scr(ui_Screen5);
    displayOn = true;
    Serial.println("[Display] Woke up");
  }
}

// Call this whenever user interacts (touch or knob)
void markActivity()
{
  lastActivity = millis();
  if (!displayOn)
  {
    turnDisplayOn();
  }
}

/* Timer callback to update clock every second */
static void clock_update_cb(lv_timer_t *timer)
{
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int hour = t->tm_hour % 12;
  int minute = t->tm_min;
  int second = t->tm_sec;

  // Convert to degrees:
  // Scale is 0..59 mapped to 360°
  int hour_value = (hour * 5) + (minute / 12); // every hour = 5 ticks
  int minute_value = minute;
  int second_value = second;

  lv_meter_set_indicator_value(clock_meter, hour_hand, hour_value);
  lv_meter_set_indicator_value(clock_meter, minute_hand, minute_value);
  lv_meter_set_indicator_value(clock_meter, second_hand, second_value);
}

void lv_example_clock(void)
{
  extern lv_obj_t *ui_Screen5;
  clock_meter = lv_meter_create(ui_Screen5);
  lv_obj_set_size(clock_meter, 360, 360);
  lv_obj_center(clock_meter);
  lv_obj_set_style_bg_opa(clock_meter, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(clock_meter, 0, LV_PART_MAIN);

  /* Add scale: 0..59 (like minutes/seconds) */
  lv_meter_scale_t *scale = lv_meter_add_scale(clock_meter);
  lv_meter_set_scale_range(clock_meter, scale, 0, 59, 360, 270);
  lv_meter_set_scale_ticks(clock_meter, scale, 61, 2, 15, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(clock_meter, scale, 5, 4, 25, lv_color_hex(0xF5F527), 15);
  lv_obj_set_style_text_opa(clock_meter, LV_OPA_TRANSP, LV_PART_TICKS);
  hour_hand = lv_meter_add_needle_line(clock_meter, scale, 8, lv_color_hex(0xF5F527), -100);
  minute_hand = lv_meter_add_needle_line(clock_meter, scale, 5, lv_color_hex(0xFFFFFF), -60);
  second_hand = lv_meter_add_needle_line(clock_meter, scale, 2, lv_palette_main(LV_PALETTE_RED), -40);

  /* Start updating every 1000 ms */
  lv_timer_create(clock_update_cb, 1000, NULL);

  /* Initial update */
  clock_update_cb(NULL);
}

void lv_example_gif_1(String path)
{
  // lv_obj_t *img = lv_img_create(ui_Screen5);
  // lv_img_set_src(img, "S:/sdcard/test0826.png");
  // lv_obj_center(img);
  gif = lv_gif_create(ui_Screen5);
  lv_obj_center(gif);
  // lv_obj_set_pos(gif, 0, -75);
  lv_gif_set_src(gif, path.c_str());
  // lv_obj_move_foreground(gif);
}

void sendAction1Click(lv_event_t *e)
{
  // Your code here
  actionNum = 1;
  actionSend = true;
}
// void sendAction2Click(lv_event_t *e)
// {
//   // Your code here
//   actionNum = 30;
//   actionSend = true;
// }
// void sendAction4Click(lv_event_t *e)
// {
//   actionNum = 40;
//   actionSend = true;
// }
void sendAction9Click(lv_event_t *e)
{
  actionNum = 5;
  actionSend = true;
}
// void sendAction8Click(lv_event_t *e)
// {
//   actionNum = 80;
//   actionSend = true;
// }
void sendAction7Click(lv_event_t *e)
{
  actionNum = 4;
  actionSend = true;
}
void sendAction3Click(lv_event_t *e)
{
  actionNum = 2;
  actionSend = true;
}
void sendOptionSelect(lv_event_t *e)
{
  // Get the object that triggered the event
  lv_obj_t *dropdown = lv_event_get_target(e);

  // Get the selected option index
  uint16_t selected = lv_dropdown_get_selected(dropdown);

  // Get the selected option text
  char buf[64];
  lv_dropdown_get_selected_str(dropdown, buf, sizeof(buf));

  Serial.println(selected);
  Serial.println(buf);
  selectedAutomation = String(buf);
}
void runbtnClick(lv_event_t *e)
{
  JsonDocument doc;
  doc["id"] = automataAddOn.getAutomationId(selectedAutomation);
  doc["key"] = "id";
  doc["automation"] = true;

  automata.sendAction(doc);
}
void sendAction6Click(lv_event_t *e)
{
  actionNum = 60;
  actionSend = true;
}
void sendAction5Click(lv_event_t *e)
{
  actionNum = 0;
  actionVal = "OK";
  actionSend = true;
}
void screen1btn_cb(lv_event_t *e)
{

  lv_obj_t *sw = lv_event_get_target(e);

  if (lv_obj_has_state(sw, LV_STATE_CHECKED))
  {
    masterSendLive = true;
    Serial.printf("Switch turned ON\n");
  }
  else
  {
    masterSendLive = false;
    Serial.printf("Switch turned OFF\n");
  }
}

void roller1_cb(lv_event_t *e)
{
  lv_obj_t *roller = lv_event_get_target(e);

  // Get the selected option index
  uint16_t selected = lv_roller_get_selected(roller);

  // Get the selected option text
  char buf[64];
  lv_roller_get_selected_str(roller, buf, sizeof(buf));

  Serial.println(selected);
  masterOption = String(buf);
  Serial.println(buf);
}
byte value[4] = {25, 25, 25, 25}; // values of each meter 0= power, 1 =green , 2 red, 3 blue
int chosen = 0;

void selectedScreen1_cb(lv_event_t *e)
{
  selectedScreen = 1;
  changeDetected = true;
}

void selectedScreen2_cb(lv_event_t *e)
{
  selectedScreen = 2;
  changeDetected = true;
}

void selectedScreen4_cb(lv_event_t *e)
{
  selectedScreen = 4;
  changeDetected = true;
}
void selectedScreen5_cb(lv_event_t *e)
{
  selectedScreen = 5;
  changeDetected = true;
}
static void anim_set_meter_value(void *obj, int32_t v)
{
  lv_meter_set_indicator_value(meter, needle, v);
}
void sendActionUpClick(lv_event_t *e)
{
  actionNum = 0;
  actionVal = "UP";
  actionSend = true;
}

void sendActionLeftClick(lv_event_t *e)
{
  actionNum = 0;
  actionVal = "LEFT";
  actionSend = true;
}

void sendActionRightClick(lv_event_t *e)
{
  actionNum = 0;
  actionVal = "RIGHT";
  actionSend = true;
}

void sendActionDownClick(lv_event_t *e)
{
  actionNum = 0;
  actionVal = "DOWN";
  actionSend = true;
}
void add_dropdown_options(const char *new_data)
{
  // Convert server response (comma-separated) → newline-separated
  static char buffer[256];
  int j = 0;
  for (int i = 0; new_data[i] != '\0'; i++)
  {
    if (new_data[i] == ',')
      buffer[j++] = '\n';
    else
      buffer[j++] = new_data[i];
  }
  buffer[j] = '\0';
}

static void _knob_left_cb(void *arg, void *data)
{
  uint8_t eventBits_ = 0;
  SET_BIT(eventBits_, 0);
  xEventGroupSetBits(knob_even_, eventBits_);
}
static void _knob_right_cb(void *arg, void *data)
{
  uint8_t eventBits_ = 0;
  SET_BIT(eventBits_, 1);
  xEventGroupSetBits(knob_even_, eventBits_);
}

static void user_encoder_loop_task(void *arg)
{
  for (;;)
  {
    EventBits_t even = xEventGroupWaitBits(knob_even_, BIT_EVEN_ALL, pdTRUE, pdFALSE, pdMS_TO_TICKS(20));

    if (READ_BIT(even, 0)) // left
    {
      if (xSemaphoreTake(mutex, portMAX_DELAY))
      {
        if (encPos > 0)
          encPos -= 5;
        if (encPos < 0)
          encPos = 0;
        changeDetected = true;
        drv2605_play_effect(1);
        markActivity();
        lastEncoderMove = millis();
        encoderMoving = true;

        xSemaphoreGive(mutex);
      }
    }

    if (READ_BIT(even, 1)) // right
    {
      if (xSemaphoreTake(mutex, portMAX_DELAY))
      {
        markActivity();
        encPos += 5;
        if (encPos > 255)
          encPos = 255;
        changeDetected = true;
        drv2605_play_effect(1);

        lastEncoderMove = millis();
        encoderMoving = true;

        xSemaphoreGive(mutex);
      }
    }

    vTaskDelay(10);
  }
}

void add_rolloer_data()
{
  MasterDataList list = automataAddOn.getMasterDataList();
  String rollerOptions = "";
  int j = 0;
  for (auto item : list)
  {
    rollerOptions += item.name;
    if (j < list.size() - 1)
      rollerOptions += "\n"; // newline-separated options
    j++;
  }

  lv_roller_set_options(ui_Roller1, rollerOptions.c_str(), LV_ROLLER_MODE_NORMAL);
}

static void example_lvgl_port_task(void *arg)
{

  for (;;)
  {

    lv_timer_handler();

    if (xSemaphoreTake(mutex, portMAX_DELAY))
    {

      lv_arc_set_value(ui_Arc1, encPos);
      lv_label_set_text(uic_arcLabel, String(encPos).c_str());
      if ((millis() - st) > 60000)
      {
        add_rolloer_data();
        // automataAddOn.getAutomationsList();
        // add_dropdown_options(automataAddOn.getAutomations().c_str());
        st = millis();
        // Start vibration
      }
      if (showAnimation && !animationActive)
      {
        if (gif == NULL)
        {
          lv_example_gif_1("S:/sdcard/loading_anim.gif");
        }
        else
        {
          lv_obj_clear_flag(gif, LV_OBJ_FLAG_HIDDEN);
        }

        animationActive = true;
      }
      else if (!showAnimation && animationActive)
      {
        if (gif != NULL)
        {
          lv_obj_add_flag(gif, LV_OBJ_FLAG_HIDDEN);
        }

        animationActive = false;
      }
      // lv_color_t color = lv_color_make(map(value[2], 0, 100, 0, 255), map(value[1], 0, 100, 0, 255), map(value[3], 0, 100, 0, 255));
      // lv_obj_set_style_bg_color(ui_colorPNL, color, LV_PART_MAIN);

      xSemaphoreGive(mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void show_message(const char *msg)
{
  // Create a label on the active screen
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, msg);

  // Align to bottom center with some padding (e.g., 20 px above bottom)
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);

  // Optionally, make sure it redraws right away
  lv_task_handler();

  // Keep message visible for 2 seconds
  // lv_timer_t *timer = lv_timer_create(
  //     (lv_timer_cb_t)lv_obj_del, 200, label);
  // // after 2000 ms, lv_obj_del(label) will be called automatically
  vTaskDelay(pdMS_TO_TICKS(200));
  lv_obj_del(label);
}

#define FRAME_INTERVAL 200 // 100 ms per frame = 10 FPS
#define NUM_FRAMES 144     // Number of frames you have

static lv_obj_t *frame_obj;
static int current_frame = 0;
static esp_timer_handle_t frame_timer;

// Example: frames named frame_000.png, frame_001.png, ...
static char path[64];

static void show_next_frame(void *arg)
{
  // frame_113_delay-0.04s
  snprintf(path, sizeof(path), "S:/sdcard/frame_%03d_delay-0.04s.png", current_frame);
  printf("Loading frame: %s\n", path);

  lv_img_set_src(frame_obj, path);
  lv_obj_center(frame_obj);

  current_frame++;
  if (current_frame >= NUM_FRAMES)
  {
    current_frame = 0; // loop
  }
}

void lv_example_video_play(void)
{
  // Create image object
  frame_obj = lv_img_create(ui_Screen5);
  lv_obj_center(frame_obj);

  // Start timer
  const esp_timer_create_args_t frame_timer_args = {
      .callback = &show_next_frame,
      .name = "frame_timer"};
  ESP_ERROR_CHECK(esp_timer_create(&frame_timer_args, &frame_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(frame_timer, FRAME_INTERVAL * 1000));
}
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // 5h 30m in seconds
const int daylightOffset_sec = 0; // No DST
void setup()
{
  mutex = xSemaphoreCreateMutex();
  Serial.begin(115200);
  Touch_Init();
  lcd_lvgl_Init();
  sd_card_Init(); // Mounts at /sdcard
  lv_fs_sd_init();

  // lv_example_meter_1();
  // lv_example_meter_2();
  // lv_example_meter_3();
  // lv_example_meter_4();
  // lv_example_clock();

  add_dropdown_options("A1,A2,A3");
  Serial.println("starting");
  // set_active_meter(chosen);
  lcd_bl_pwm_bsp_init(bright);
  if (drv2605_init() == ESP_OK)
  {
    // Vibrate with effect 1 (strong click)
    drv2605_play_effect(47); // Long strong buzz
  }

  xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);
  show_message("Starting...");
  preferences.begin("bat", false);
  automata.begin();
  automata.useHTTPS();
  automata.useWSS();
  sdWebServer = new SDWebServer(automata.getWebserver());
  sdWebServer->begin();
  automata.addAttribute("encoder1", "Encoder 1", "", "DATA|MAIN");
  automata.addAttribute("screen", "Screen", "", "DATA|MAIN");
  automata.addAttribute("action", "Action", "", "ACTION|IN");
    automata.addAttribute("animation", "Animation", "", "ACTION|MENU|SWITCH");
  JsonDocument doc;
  doc["max"] = 255;
  doc["min"] = 0;
  automata.addAttribute("bright", "Brightness", "", "ACTION|MENU|SLIDER", doc);
  automata.addAttribute("battery_volt", "Battery", "V", "DATA|AUX");
  show_message("Setting attributes...");
  automata.registerDevice();
  automata.onActionReceived(action);
  automata.delayedUpdate(sendData);
  show_message("Registering device...");
  knob_even_ = xEventGroupCreate();
  // create knob
  knob_config_t cfg =
      {
          .gpio_encoder_a = EXAMPLE_ENCODER_ECA_PIN,
          .gpio_encoder_b = EXAMPLE_ENCODER_ECB_PIN,
      };
  s_knob = iot_knob_create(&cfg);

  iot_knob_register_cb(s_knob, KNOB_LEFT, _knob_left_cb, NULL);
  iot_knob_register_cb(s_knob, KNOB_RIGHT, _knob_right_cb, NULL);
  xTaskCreate(user_encoder_loop_task, "user_encoder_loop_task", 3000, NULL, 2, NULL);

  lv_disp_load_scr(ui_Screen5);

  show_message("Welcome...");
  add_dropdown_options(automataAddOn.getAutomations().c_str());
  // sd_card_list_files("/", 1);
  // lv_example_video_play();
  Serial.println(ESP.getPsramSize());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // lv_example_gif_1("S:/sdcard/loading_anim_h.gif");
  add_rolloer_data();
  lastActivity = millis();
  // SerialPort.begin(9600, SERIAL_8N1, ESP32S3_RX, ESP32S3_TX);
}
bool alreadySet = false;

void updateClock()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }

  char timeString[10];
  char dateString[20];

  // Format strings
  strftime(timeString, sizeof(timeString), "%I:%M %p", &timeinfo);  // 11:50 PM
  strftime(dateString, sizeof(dateString), "%a, %b %d", &timeinfo); // Thu, Aug 28

  // Update LVGL labels
  lv_label_set_text(ui_timeText, timeString);
  lv_label_set_text(ui_dateField, dateString);

  // Debug print
  // Serial.print("Time: ");
  // Serial.println(timeString);
  // Serial.print("Date: ");
  // Serial.println(dateString);
}

int msd = millis();
void sendMasterAction()
{
  String id, key;
  if (automataAddOn.getMasterDeviceByName(masterOption.c_str(), id, key))
  {
    JsonDocument doc;
    doc["screen"] = selectedScreen;
    doc["type"] = "master";
    doc["value"] = encPos;
    doc["deviceId"] = id;
    doc["key"] = key;
    drv2605_play_effect(1);
    automata.sendAction(doc);
  }
}
void screen1_btn_send_cb(lv_event_t *e)
{
  sendMasterAction();
}
void loop()
{
  doc["encoder1"] = encPos;
  doc["screen"] = selectedScreen;
  doc["bright"] = bright;
  float bt = ((analogRead(1) * 2 * 3.3 * 1000) / 4096) / 1000;

  doc["battery_volt"] = String(bt, 2);
  // automata.loop();

  if (masterSendLive && encoderMoving && (millis() - lastEncoderMove > ENCODER_IDLE_TIMEOUT))
  {
    sendMasterAction(); // call your function
    Serial.println("live data sent");
    drv2605_play_effect(47);
    encoderMoving = false;
    // vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (changeDetected || (millis() - start) > 10000)
  {

    automata.sendLive(doc);
    // String json;
    // serializeJson(doc, json);
    // SerialPort.println(json);
    // automata.getMasterList();
    updateClock();
    // drv2605_play_effect(1); // Strong click
    // drv2605_play_effect(2);  // Sharp click
    // drv2605_play_effect(47); // Long strong buzz
    // drv2605_play_effect(12); // Double strong click

    start = millis();
    // Start vibration
    changeDetected = false;
  }

  if (actionSend)
  {
    JsonDocument doc;
    if (actionNum == 0)
    {
      doc["action"] = actionVal;
      Serial.println(actionVal);
    }
    else
    {
      doc["action"] = actionNum;
      Serial.println(actionNum);
    }

    doc["key"] = "action";
    drv2605_play_effect(1);
    automata.sendAction(doc);
    Serial.print("action: ");

    actionSend = false;
  }
  // --- Auto display off logic ---
  // if (displayOn && (millis() - lastActivity > DISPLAY_TIMEOUT))
  // {
  //   turnDisplayOff();
  // }
  vTaskDelay(pdMS_TO_TICKS(30));
}
