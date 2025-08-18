#include "lcd/lcd_bsp.h"
#include "lcd/cst816.h"
#include "lcd/lcd_bl_pwm_bsp.h"
#include "lcd/lcd_config.h"
#include "ui.h"
#include "lcd/bidi_switch_knob.h"
#include "Automata.h"
#include "ArduinoJson.h"
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include "SensorDRV2605.hpp"
#include <time.h>
// const char* HOST = "192.168.1.7";
// int PORT = 8080;

const char *HOST = "raspberry.local";
int PORT = 8010;
Preferences preferences;
Automata automata("KNOB", HOST, PORT);
long d = 8000;
long st = millis();
unsigned long startMillis;
int ch = millis();
long start = millis();
JsonDocument doc;
static const char *TAG = "encoder";

#define SENSOR_SDA 11
#define SENSOR_SCL 12

SensorDRV2605 drv;

uint8_t effect = 1;

static lv_obj_t *meter;
lv_meter_indicator_t *needle;

static lv_obj_t *meter2;
lv_meter_indicator_t *needle2;

static lv_obj_t *meter3;
lv_meter_indicator_t *needle3;

static lv_obj_t *meter4;
static lv_obj_t *meter5;
lv_meter_indicator_t *needle4;

typedef struct struct_message
{
  byte power;
  byte red;
  byte green;
  byte blue;
} struct_message;

struct_message myData;

#define EXAMPLE_ENCODER_ECA_PIN 8
#define EXAMPLE_ENCODER_ECB_PIN 7

#define SET_BIT(reg, bit) (reg |= ((uint32_t)0x01 << bit))
#define CLEAR_BIT(reg, bit) (reg &= (~((uint32_t)0x01 << bit)))
#define READ_BIT(reg, bit) (((uint32_t)reg >> bit) & 0x01)
#define BIT_EVEN_ALL (0x00ffffff)

void action(const Action action)
{
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
bool actionSend = false;
String selectedAutomation = "";

static lv_obj_t *clock_meter;
static lv_meter_indicator_t *hour_hand;
static lv_meter_indicator_t *minute_hand;
static lv_meter_indicator_t *second_hand;

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

void sendAction1Click(lv_event_t *e)
{
  // Your code here
  actionNum = 1;
  actionSend = true;
}
void sendAction2Click(lv_event_t *e)
{
  // Your code here
  actionNum = 2;
  actionSend = true;
}
void sendAction4Click(lv_event_t *e)
{
  actionNum = 4;
  actionSend = true;
}
void sendAction9Click(lv_event_t *e)
{
  actionNum = 9;
  actionSend = true;
}
void sendAction8Click(lv_event_t *e)
{
  actionNum = 8;
  actionSend = true;
}
void sendAction7Click(lv_event_t *e)
{
  actionNum = 7;
  actionSend = true;
}
void sendAction3Click(lv_event_t *e)
{
  actionNum = 3;
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
  lv_label_set_text(ui_Label26, String(selectedAutomation).c_str());
}
void runbtnClick(lv_event_t *e)
{
  JsonDocument doc;
  doc["id"] = automata.getAutomationId(selectedAutomation);
  doc["key"] = "id";
  doc["automation"] = true;

  automata.sendAction(doc);
  lv_label_set_text(ui_Label19, String("Command Sent").c_str());
}
void sendAction6Click(lv_event_t *e)
{
  actionNum = 6;
  actionSend = true;
}
void sendAction5Click(lv_event_t *e)
{
  actionNum = 5;
  actionSend = true;
}
void vibrateStrong2Sec()
{
  // Select a strong continuous buzz effect
  for (int i = 0; i < 10; i++)
  {
    drv.setWaveform(0, 118); // Long buzz (100%) effect
    drv.setWaveform(1, 0);   // End waveform

    drv.run(); // Start vibration
    vTaskDelay(pdMS_TO_TICKS(200));

    drv.stop(); // Stop the vibration
  }
}
byte value[4] = {25, 25, 25, 25}; // values of each meter 0= power, 1 =green , 2 red, 3 blue
int chosen = 0;
void set_active_meter(int index)
{
  // Resetiraj svim metrima border (sakrij)
  lv_obj_set_style_border_width(meter, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter2, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter3, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter4, 0, LV_PART_MAIN);

  // Postavi aktivnom metru border
  switch (index)
  {
  case 0:
    lv_obj_set_style_border_width(meter, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(meter, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    break;
  case 1:
    lv_obj_set_style_border_width(meter2, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(meter2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    break;
  case 3:
    lv_obj_set_style_border_width(meter4, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(meter4, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    break;
  case 2:
    lv_obj_set_style_border_width(meter3, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(meter3, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    break;
  }
}
void meter_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    chosen = 0;
    set_active_meter(chosen);
  }
}
void meter2_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    chosen = 1;
    set_active_meter(chosen);
  }
}
void meter3_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    chosen = 2;
    set_active_meter(chosen);
  }
}
void meter4_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    chosen = 3;
    set_active_meter(chosen);
  }
}

void selectedScreen1_cb(lv_event_t *e)
{
  selectedScreen = 1;
}

void selectedScreen2_cb(lv_event_t *e)
{
  selectedScreen = 2;
}

void selectedScreen4_cb(lv_event_t *e)
{
  selectedScreen = 4;
}
static void anim_set_meter_value(void *obj, int32_t v)
{
  lv_meter_set_indicator_value(meter, needle, v);
}
void lv_example_meter_1(void)
{
  extern lv_obj_t *ui_Screen1;
  meter = lv_meter_create(ui_Screen1);
  lv_obj_add_event_cb(meter, meter_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_center(meter);
  lv_obj_set_size(meter, 200, 200);
  lv_obj_set_pos(meter, -71, 0);
  lv_obj_set_style_bg_opa(meter, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter, 0, LV_PART_MAIN);

  /*Add a scale first*/
  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_range(meter, scale, 0, 100, 270, 135); // Set range from 0 to 10
  lv_meter_set_scale_ticks(meter, scale, 31, 3, 10, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter, scale, 6, 5, 15, lv_color_white(), 10);

  lv_meter_indicator_t *indic;

  /*Add a blue arc to the start*/
  // indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_LIME), 0);
  // lv_meter_set_indicator_start_value(meter, indic, 0);
  // lv_meter_set_indicator_end_value(meter, indic, 20);

  // /*Make the tick lines blue at the start of the scale*/
  // indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_LIME), lv_palette_main(LV_PALETTE_LIME), false, 0);
  // lv_meter_set_indicator_start_value(meter, indic, 0);
  // lv_meter_set_indicator_end_value(meter, indic, 20);

  // /*Add a red arc to the end*/
  // indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  // lv_meter_set_indicator_start_value(meter, indic, 80);
  // lv_meter_set_indicator_end_value(meter, indic, 100);

  // /*Make the tick lines red at the end of the scale*/
  // indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  // lv_meter_set_indicator_start_value(meter, indic, 80);
  // lv_meter_set_indicator_end_value(meter, indic, 100);

  /*Add a needle line indicator*/
  needle = lv_meter_add_needle_line(meter, scale, 4, lv_color_hex(0xFFFFFF), -10);
}

void lv_example_meter_2(void)
{
  extern lv_obj_t *ui_Screen1;
  meter2 = lv_meter_create(ui_Screen1);
  lv_obj_add_event_cb(meter2, meter2_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_center(meter2);
  lv_obj_set_size(meter2, 146, 146);
  lv_obj_set_pos(meter2, 109, 0);
  lv_obj_set_style_bg_opa(meter2, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter2, 0, LV_PART_MAIN);

  /*Add a scale first*/
  lv_meter_scale_t *scale = lv_meter_add_scale(meter2);
  lv_meter_set_scale_range(meter2, scale, 0, 100, 270, 135); // Set range from 0 to 10
  lv_meter_set_scale_ticks(meter2, scale, 31, 2, 8, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter2, scale, 6, 3, 11, lv_color_white(), 10);

  lv_meter_indicator_t *indic;

  /*Add a blue arc to the start*/
  // indic = lv_meter_add_arc(meter2, scale, 3, lv_palette_main(LV_PALETTE_LIME), 0);
  // lv_meter_set_indicator_start_value(meter2, indic, 0);
  // lv_meter_set_indicator_end_value(meter2, indic, 20);

  // /*Make the tick lines blue at the start of the scale*/
  // indic = lv_meter_add_scale_lines(meter2, scale, lv_palette_main(LV_PALETTE_LIME), lv_palette_main(LV_PALETTE_LIME), false, 0);
  // lv_meter_set_indicator_start_value(meter2, indic, 0);
  // lv_meter_set_indicator_end_value(meter2, indic, 20);

  // /*Add a red arc to the end*/
  // indic = lv_meter_add_arc(meter2, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  // lv_meter_set_indicator_start_value(meter2, indic, 80);
  // lv_meter_set_indicator_end_value(meter2, indic, 100);

  // /*Make the tick lines red at the end of the scale*/
  // indic = lv_meter_add_scale_lines(meter2, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  // lv_meter_set_indicator_start_value(meter2, indic, 80);
  // lv_meter_set_indicator_end_value(meter2, indic, 100);

  /*Add a needle line indicator*/
  needle2 = lv_meter_add_needle_line(meter2, scale, 3, lv_color_hex(0xFFFFFF), -10);
}

void lv_example_meter_3(void)
{
  extern lv_obj_t *ui_Screen1;
  meter3 = lv_meter_create(ui_Screen1);
  lv_obj_add_event_cb(meter3, meter3_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_center(meter3);
  lv_obj_set_size(meter3, 128, 128);
  lv_obj_set_pos(meter3, 43, -107);
  lv_obj_set_style_bg_opa(meter3, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter3, 0, LV_PART_MAIN);

  /*Add a scale first*/
  lv_meter_scale_t *scale = lv_meter_add_scale(meter3);
  lv_meter_set_scale_range(meter3, scale, 0, 100, 270, 135); // Set range from 0 to 10
  lv_meter_set_scale_ticks(meter3, scale, 31, 1, 6, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter3, scale, 6, 2, 10, lv_color_white(), 10);

  lv_meter_indicator_t *indic;

  /*Add a blue arc to the start*/
  // indic = lv_meter_add_arc(meter3, scale, 3, lv_palette_main(LV_PALETTE_LIME), 0);
  // lv_meter_set_indicator_start_value(meter3, indic, 0);
  // lv_meter_set_indicator_end_value(meter3, indic, 20);

  // /*Make the tick lines blue at the start of the scale*/
  // indic = lv_meter_add_scale_lines(meter3, scale, lv_palette_main(LV_PALETTE_LIME), lv_palette_main(LV_PALETTE_LIME), false, 0);
  // lv_meter_set_indicator_start_value(meter3, indic, 0);
  // lv_meter_set_indicator_end_value(meter3, indic, 20);

  // /*Add a red arc to the end*/
  // indic = lv_meter_add_arc(meter3, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  // lv_meter_set_indicator_start_value(meter3, indic, 80);
  // lv_meter_set_indicator_end_value(meter3, indic, 100);

  // /*Make the tick lines red at the end of the scale*/
  // indic = lv_meter_add_scale_lines(meter3, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  // lv_meter_set_indicator_start_value(meter3, indic, 80);
  // lv_meter_set_indicator_end_value(meter3, indic, 100);

  /*Add a needle line indicator*/
  needle3 = lv_meter_add_needle_line(meter3, scale, 3, lv_color_hex(0xFFFFFF), -10);
}

void lv_example_meter_4(void)
{
  extern lv_obj_t *ui_Screen1;
  meter4 = lv_meter_create(ui_Screen1);
  lv_obj_add_event_cb(meter4, meter4_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_center(meter4);
  lv_obj_set_size(meter4, 128, 128);
  lv_obj_set_pos(meter4, 43, 107);
  lv_obj_set_style_bg_opa(meter4, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter4, 0, LV_PART_MAIN);

  /*Add a scale first*/
  lv_meter_scale_t *scale = lv_meter_add_scale(meter4);
  lv_meter_set_scale_range(meter4, scale, 0, 100, 270, 135); // Set range from 0 to 10
  lv_meter_set_scale_ticks(meter4, scale, 31, 1, 6, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter4, scale, 6, 2, 10, lv_color_white(), 10);

  lv_meter_indicator_t *indic;

  /*Add a blue arc to the start*/
  // indic = lv_meter_add_arc(meter4, scale, 3, lv_palette_main(LV_PALETTE_LIME), 0);
  // lv_meter_set_indicator_start_value(meter4, indic, 0);
  // lv_meter_set_indicator_end_value(meter4, indic, 20);

  // /*Make the tick lines blue at the start of the scale*/
  // indic = lv_meter_add_scale_lines(meter4, scale, lv_palette_main(LV_PALETTE_LIME), lv_palette_main(LV_PALETTE_LIME), false, 0);
  // lv_meter_set_indicator_start_value(meter4, indic, 0);
  // lv_meter_set_indicator_end_value(meter4, indic, 20);

  // /*Add a red arc to the end*/
  // indic = lv_meter_add_arc(meter4, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  // lv_meter_set_indicator_start_value(meter4, indic, 80);
  // lv_meter_set_indicator_end_value(meter4, indic, 100);

  // /*Make the tick lines red at the end of the scale*/
  // indic = lv_meter_add_scale_lines(meter4, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  // lv_meter_set_indicator_start_value(meter4, indic, 80);
  // lv_meter_set_indicator_end_value(meter4, indic, 100);

  /*Add a needle line indicator*/
  needle4 = lv_meter_add_needle_line(meter4, scale, 3, lv_color_hex(0xFFFFFF), -10);
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

  // Update dropdown options
  lv_dropdown_set_options(ui_Dropdown1, buffer);
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

bool shoot = false;
int pos = -96;

static void user_encoder_loop_task(void *arg)
{

  for (;;)
  {
    EventBits_t even = xEventGroupWaitBits(knob_even_, BIT_EVEN_ALL, pdTRUE, pdFALSE, pdMS_TO_TICKS(50));
    if (READ_BIT(even, 0))
    {
      if (xSemaphoreTake(mutex, portMAX_DELAY))
      {
        if (value[chosen] > 0)
          value[chosen] = value[chosen] - 2;

        myData.power = value[0];
        myData.red = map(value[2], 0, 100, 0, 255);
        myData.blue = map(value[3], 0, 100, 0, 255);
        myData.green = map(value[1], 0, 100, 0, 255);

        encPos = value[chosen];

        // doc["encoder1"] = myData.power;
        // doc["encoder2"] = myData.green;
        // doc["encoder3"] = myData.blue;
        // doc["encoder4"] = myData.red;
        // doc["chosen"] = chosen;
        // automata.sendLive(doc);

        xSemaphoreGive(mutex);
      }
      // vibrateStrong2Sec();
    }
    if (READ_BIT(even, 1))
    {
      if (xSemaphoreTake(mutex, portMAX_DELAY))
      {
        value[chosen] = value[chosen] + 2;
        if (value[chosen] > 100)
          value[chosen] = 100;
        myData.power = value[0];
        myData.red = map(value[2], 0, 100, 0, 255);
        myData.blue = map(value[3], 0, 100, 0, 255);
        myData.green = map(value[1], 0, 100, 0, 255);
        encPos = value[chosen];
        // doc["encoder1"] = myData.power;
        // doc["encoder2"] = myData.green;
        // doc["encoder3"] = myData.blue;
        // doc["encoder4"] = myData.red;
        // doc["chosen"] = chosen;
        // automata.sendLive(doc);

        xSemaphoreGive(mutex);
      }
      // vibrateStrong2Sec();
    }
    vTaskDelay(100);
  }
}

static void example_lvgl_port_task(void *arg)
{

  for (;;)
  {
    lv_timer_handler();

    if (xSemaphoreTake(mutex, portMAX_DELAY))
    {

      lv_label_set_text(ui_power, String(value[0]).c_str());
      lv_meter_set_indicator_value(meter, needle, value[0]);
      lv_arc_set_value(ui_Arc1, value[0]);

      lv_label_set_text(ui_power1, String(value[1]).c_str());
      lv_meter_set_indicator_value(meter2, needle2, value[1]);
      lv_arc_set_value(ui_Arc2, value[1]);

      lv_label_set_text(ui_power3, String(value[2]).c_str());
      lv_meter_set_indicator_value(meter3, needle3, value[2]);
      lv_arc_set_value(ui_Arc3, value[2]);

      lv_label_set_text(ui_power2, String(value[3]).c_str());
      lv_meter_set_indicator_value(meter4, needle4, value[3]);
      lv_arc_set_value(ui_Arc4, value[3]);

      // lv_label_set_text(ui_power2, String(value[3]).c_str());
      // lv_meter_set_indicator_value(meter5, needle4, value[3]);

      lv_label_set_text(ui_Label25, String(encPos).c_str());
      lv_arc_set_value(ui_Arc5, encPos);

      if ((millis() - st) > 60000)
      {
        add_dropdown_options(automata.getAutomations().c_str());
        st = millis();
        // Start vibration
      }
      lv_color_t color = lv_color_make(map(value[2], 0, 100, 0, 255), map(value[1], 0, 100, 0, 255), map(value[3], 0, 100, 0, 255));
      lv_obj_set_style_bg_color(ui_colorPNL, color, LV_PART_MAIN);

      xSemaphoreGive(mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void initVib()
{
  Wire.begin(SENSOR_SDA, SENSOR_SCL, 400000);
  if (!drv.begin(Wire))
  {
    Serial.println("Failed to find DRV2605 - check your wiring!");
    while (1)
    {
      Serial.println("Failed to find DRV2605 - check your wiring!");
      delay(1000);
    }
  }
  drv.selectLibrary(1);

  // I2C trigger by sending 'run' command
  // default, internal trigger when sending RUN command
  drv.setMode(SensorDRV2605::MODE_INTTRIG);
  Serial.println("Init DRV2605 Sensor success!");
  vibrateStrong2Sec();
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

void setup()
{
  mutex = xSemaphoreCreateMutex();
  Serial.begin(115200);

  initVib();
  Touch_Init();
  lcd_lvgl_Init();

  lv_example_meter_1();
  lv_example_meter_2();
  lv_example_meter_3();
  lv_example_meter_4();
  lv_example_clock();
  add_dropdown_options("A1,A2,A3");
  Serial.println("starting");
  set_active_meter(chosen);
  lcd_bl_pwm_bsp_init(80);

  xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);
  show_message("Starting...");
  preferences.begin("bat", false);
  automata.begin();
  automata.addAttribute("encoder1", "Encoder 1", "", "DATA|MAIN");
  automata.addAttribute("encoder2", "Encoder 2", "", "DATA|MAIN");
  automata.addAttribute("encoder3", "Encoder 3", "", "DATA|MAIN");
  automata.addAttribute("encoder4", "Encoder 4", "", "DATA|MAIN");
  automata.addAttribute("screen", "Screen", "", "DATA|AUX");
  automata.addAttribute("chosen", "Chosen", "", "DATA|MAIN");
  automata.addAttribute("action", "Action", "", "ACTION|IN");
  automata.addAttribute("battery_volt", "Battery", "V", "DATA|MAIN");
  // automata.addAttribute("upTime", "Up Time", "Hours", "DATA|MAIN");
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
  add_dropdown_options(automata.getAutomations().c_str());
}
bool alreadySet = false;
void loop()
{
  doc["encoder1"] = myData.power;
  doc["encoder2"] = myData.green;
  doc["encoder3"] = myData.blue;
  doc["encoder4"] = myData.red;
  doc["chosen"] = chosen;
  doc["screen"] = selectedScreen;
  float bt = ((analogRead(1) * 2 * 3.3 * 1000) / 4096) / 1000;

  doc["battery_volt"] = String(bt, 2);
  // Serial.println("loop");
  automata.loop();

  if ((millis() - start) > 500)
  {
    automata.sendLive(doc);
    start = millis();
    // Start vibration
  }

  if (actionSend)
  {
    JsonDocument doc;
    doc["action"] = actionNum;
    doc["key"] = "button";
    automata.sendAction(doc);
    Serial.print("action: ");
    Serial.println(actionNum);
    actionSend = false;
  }
}
