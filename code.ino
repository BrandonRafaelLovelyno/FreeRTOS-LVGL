#include "lvgl.h"
#include "SPI.h"
#include "DHT.h"
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LOOP_INTERVAL 500

// Display
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 320
TFT_eSPI tft = TFT_eSPI();  
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];
static lv_disp_drv_t disp_drv;

// DHT
#define DHT_PIN 13
DHT dht(DHT_PIN, DHT22);
const int numReadings = 100;
int currentDHTRead = 0;
float readings[numReadings];
int readIndex = 0;
float total = 0;
float mean = 0;
float currentTemp = 0.0;


// label
lv_obj_t *label_mean;
lv_obj_t *label_deviation;
lv_obj_t *label_median;
lv_obj_t *label_mode;
lv_obj_t *label_min;
lv_obj_t *label_max;
lv_obj_t *label_current;

void lv_disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  int32_t x, y;
  for (y = area->y1; y <= area->y2; y++) {
    for (x = area->x1; x <= area->x2; x++) {
      uint16_t color = color_p->full; // Get the color from the buffer
      tft.drawPixel(x, y, color); // Draw the pixel on the display
      color_p++; // Move to the next color
    }
  }
  lv_disp_flush_ready(drv); // Notify LVGL that the flushing is done
}

void initializeDisplay() {
  Serial.println("Initializing display...");
  tft.begin(); // Initialize the TFT display
  lv_init(); // Initialize LVGL

  static lv_disp_draw_buf_t draw_buf;
  static lv_color_t buf[LV_HOR_RES_MAX * 10]; // Define buffer size
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 10); // Initialize buffer

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv); // Initialize display driver
  disp_drv.hor_res = 240; // Horizontal resolution
  disp_drv.ver_res = 320; // Vertical resolution
  disp_drv.flush_cb = lv_disp_flush_cb; // Set flush callback
  disp_drv.draw_buf = &draw_buf; // Set the draw buffer

  lv_disp_t *disp = lv_disp_drv_register(&disp_drv); // Register display driver
  if (!disp) {
    Serial.println("Display driver registration failed!"); // Error message
    return;
  }

  createLabel(); // Create a label (example)
  Serial.println("Display initialized successfully!"); // Success message
}

void taskIncrementTick(void *parameter){
    while(1){
        lv_tick_inc(LOOP_INTERVAL);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void initializeTask() {
    xTaskCreate(taskUpdateDHT, "TaskUpdate", 1500, NULL, 3, NULL);
    xTaskCreate(taskUpdateMeanLabel, "TaskMean", 3000, NULL, 3, NULL);
    xTaskCreate(taskUpdateDeviationLabel, "TaskDeviation", 3000, NULL, 3, NULL);
    xTaskCreate(taskUpdateMedianLabel, "TaskMedian", 3000, NULL, 3, NULL);
    xTaskCreate(taskUpdateModeLabel, "TaskMode", 3000, NULL, 3, NULL);
    xTaskCreate(taskUpdateMinLabel, "TaskMin", 3000, NULL, 3, NULL);
    xTaskCreate(taskUpdateMaxLabel, "TaskMax", 3000, NULL, 3, NULL);
    xTaskCreate(taskIncrementTick, "TaskTick", 3500, NULL, 1, NULL);
    xTaskCreate(taskUpdateCurrentLabel, "TaskCurrent", 3500, NULL, 2, NULL);
}

void initializeDHT() {
    dht.begin();
    for(int i = 0; i < numReadings; i++) {
        readings[i] = 0;
    }
    Serial.println("DHT initialized successfully!");
}

void taskUpdateCurrentLabel(void *parameter) {
    while (1) {
        char buffer[20]; 
        snprintf(buffer, sizeof(buffer), "Current: %.2f", currentTemp);
        lv_label_set_text(label_current, buffer);
        Serial.println("Task update current label");
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void taskUpdateMeanLabel(void *parameter) {
    while (1) {
        char buffer[20]; 
        snprintf(buffer, sizeof(buffer), "Mean: %.2f", getDHTMean());
        lv_label_set_text(label_mean, buffer);
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void taskUpdateDeviationLabel(void *parameter) {
    while (1) {
        char buffer[20]; 
        snprintf(buffer, sizeof(buffer), "Deviation: %.2f", getDHTDeviation());
        lv_label_set_text(label_deviation, buffer);
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void taskUpdateMedianLabel(void *parameter) {
    while (1) {
        char buffer[20]; 
        snprintf(buffer, sizeof(buffer), "Median: %.2f", getDHTMedian());
        lv_label_set_text(label_median, buffer);
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void taskUpdateModeLabel(void *parameter) {
    while (1) {
        char buffer[20]; 
        snprintf(buffer, sizeof(buffer), "Mode: %.2f", getDHTMode());
        lv_label_set_text(label_mode, buffer);
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void taskUpdateMinLabel(void *parameter) {
    while (1) {
        char buffer[20]; 
        snprintf(buffer, sizeof(buffer), "Min: %.2f", getDHTMin());
        lv_label_set_text(label_min, buffer);
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void taskUpdateMaxLabel(void *parameter) {
    while (1) {
        char buffer[20]; 
        snprintf(buffer, sizeof(buffer), "Max: %.2f", getDHTMax());
        lv_label_set_text(label_max, buffer);
        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

void createLabel(){
    label_mean = lv_label_create(lv_scr_act());
    lv_label_set_text(label_mean, "Mean: ");
    lv_obj_align(label_mean, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(label_mean, lv_color_black(), 0);

    label_deviation = lv_label_create(lv_scr_act());
    lv_label_set_text(label_deviation, "Deviation: ");
    lv_obj_align(label_deviation, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_set_style_text_color(label_deviation, lv_color_black(), 0);

    label_median = lv_label_create(lv_scr_act());
    lv_label_set_text(label_median, "Median: ");
    lv_obj_align(label_median, LV_ALIGN_TOP_LEFT, 10, 70);
    lv_obj_set_style_text_color(label_median, lv_color_black(), 0);

    label_mode = lv_label_create(lv_scr_act());
    lv_label_set_text(label_mode, "Mode: ");
    lv_obj_align(label_mode, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_obj_set_style_text_color(label_mode, lv_color_black(), 0);

    label_min = lv_label_create(lv_scr_act());
    lv_label_set_text(label_min, "Min: ");
    lv_obj_align(label_min, LV_ALIGN_TOP_LEFT, 10, 130);
    lv_obj_set_style_text_color(label_min, lv_color_black(), 0);

    label_max = lv_label_create(lv_scr_act());
    lv_label_set_text(label_max, "Max: ");
    lv_obj_align(label_max, LV_ALIGN_TOP_LEFT, 10, 160);
    lv_obj_set_style_text_color(label_max, lv_color_black(), 0);

    label_current = lv_label_create(lv_scr_act());
    lv_label_set_text(label_current, "Current: ");
    lv_obj_align(label_current, LV_ALIGN_TOP_LEFT, 10, 190);
    lv_obj_set_style_text_color(label_current, lv_color_black(), 0);

    lv_obj_t *label_identitas=lv_label_create(lv_scr_act());
    lv_label_set_text(label_identitas, "Brandon Rafael Lovelyno\n 22/500359/TK/54847");
    lv_obj_align(label_identitas, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_text_color(label_identitas, lv_color_black(), 0);
}

void taskUpdateDHT(void *parameter) {
    while (1) {
    total = total - readings[readIndex];
    currentTemp = dht.readTemperature();
    readings[readIndex] = currentTemp;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;

    // Increment the number of actual readings
    if (currentDHTRead < numReadings) {
        currentDHTRead++;
    }    
    vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

float getDHTMean() {
    if (currentDHTRead == 0) return 0; // Avoid division by zero
    mean = total / currentDHTRead;
    return mean;
}

float getDHTDeviation() {
    float deviation = 0;
    for (int i = 0; i < currentDHTRead; i++) {
        deviation += pow(readings[i] - mean, 2);
    }
    deviation = sqrt(deviation / currentDHTRead);
    return deviation;
}

float getDHTMedian() {
    float sortedReadings[numReadings];
    memcpy(sortedReadings, readings, sizeof(readings)); // Copy readings array

    // Sort the array
    for (int i = 0; i < currentDHTRead; i++) {
        for (int j = i + 1; j < currentDHTRead; j++) {
            if (sortedReadings[i] > sortedReadings[j]) {
                float temp = sortedReadings[i];
                sortedReadings[i] = sortedReadings[j];
                sortedReadings[j] = temp;
            }
        }
    }

    // Calculate the median
    if (currentDHTRead % 2 == 0) {
        return (sortedReadings[currentDHTRead / 2 - 1] + sortedReadings[currentDHTRead / 2]) / 2;
    } else {
        return sortedReadings[currentDHTRead / 2];
    }
}

float getDHTMode() {
    int maxCount = 0;
    float mode = 0;
    for (int i = 0; i < currentDHTRead; i++) {
        int count = 0;
        for (int j = 0; j < currentDHTRead; j++) {
            if (readings[j] == readings[i]) {
                count++;
            }
        }
        if (count > maxCount) {
            maxCount = count;
            mode = readings[i];
        }
    }
    return mode;
}

float getDHTMin() {
    float min = 100; // Initialize to a very high value to ensure correct min calculation
    for (int i = 0; i < currentDHTRead; i++) {
        if (readings[i] < min) {
            min = readings[i];
        }
    }
    return min;
}

float getDHTMax() {
    float max = -100; // Initialize to a very low value to ensure correct max calculation
    for (int i = 0; i < currentDHTRead; i++) {
        if (readings[i] > max) {
            max = readings[i];
        }
    }
    return max;
}


// Setup function
void setup() {
    Serial.begin(115200);
    Serial.println("Setup started");
    initializeDHT();
    initializeDisplay();
    initializeTask();
}

// Loop function
void loop() {}
