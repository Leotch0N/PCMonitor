#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define LVGL_TICK_PERIOD 10

#define LV_COLOR_MY_GRAY LV_COLOR_MAKE(0xD0, 0xD0, 0xD0)

const char *ssid = "YOUR_WIFI_SSID";      // wifi ssid
const char *password = "YOUR_WIFI_PASSWORD";   // wifi password
const char *monitoring_data_url = "http://192.168.1.66:5000/monitoring-data";  //should match the url scheme on server side

TFT_eSPI tft = TFT_eSPI();
static lv_disp_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 10];

// basic variables
static uint8_t test_data = 0;

// lv_objs
static lv_obj_t *chart_cpu_ram;
static lv_obj_t *chart_network;

static lv_chart_series_t * cpu_ser;
static lv_chart_series_t * ram_ser;
static lv_chart_series_t * netup_ser;
static lv_chart_series_t * netdn_ser;

// lv_coord_t cpu_usage_series[30] = {0};
// lv_coord_t ram_usage_series[30] = {0};
// lv_coord_t netup_usage_series[30] = {0};
// lv_coord_t netdn_usage_series[30] = {0};

static lv_obj_t *cpu_ram_value_label;
static lv_obj_t *network_value_label;


void connectWiFi()
{
    WiFi.begin(ssid, password);     
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println(" ..."); 

    int i = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);        // check WiFi.status() every 1s
        Serial.print(i++);
        Serial.print(' '); 
    }

    // print IP address of this mcu
    Serial.println(""); 
    Serial.println("Connection established!");         
    Serial.print("IP address:    ");                   
    Serial.println(WiFi.localIP().toString().c_str()); 
}

void lv_tick_handler() {
    lv_tick_inc(LVGL_TICK_PERIOD);
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    
    lv_disp_flush_ready(disp);
}

void create_label_cpu_ram(void){
    cpu_ram_value_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(cpu_ram_value_label, "CPU:    RAM:");
    lv_obj_set_style_local_text_color(cpu_ram_value_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_pos(cpu_ram_value_label, 20, 100);
}

void create_label_network(void){
    network_value_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(network_value_label, "UP:    DN:");
    lv_obj_set_style_local_text_color(network_value_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_pos(network_value_label, 20, 220);
}

void create_chart_cpu_ram(void)
{
    /*Create a chart*/
    chart_cpu_ram = lv_chart_create(lv_scr_act(), NULL);
    // chart_cpu_ram = lv_chart_create(main_page, NULL);
    lv_obj_set_size(chart_cpu_ram, 320, 120);
    lv_obj_align(chart_cpu_ram, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_chart_set_type(chart_cpu_ram, LV_CHART_TYPE_LINE);   // Show lines and points too 
    lv_chart_set_range(chart_cpu_ram, 0, 100);
    lv_chart_set_point_count(chart_cpu_ram, 30); // 30 points

    /*Add a faded are effect*/
    lv_obj_set_style_local_bg_opa(chart_cpu_ram, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, LV_OPA_50); /*Max. opa.*/
    lv_obj_set_style_local_bg_grad_dir(chart_cpu_ram, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
    lv_obj_set_style_local_bg_main_stop(chart_cpu_ram, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, 255);    /*Max opa on the top*/
    lv_obj_set_style_local_bg_grad_stop(chart_cpu_ram, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, 0);      /*Transparent on the bottom*/

    lv_obj_set_style_local_bg_color(chart_cpu_ram, LV_CHART_PART_BG, LV_STATE_DEFAULT, LV_COLOR_MY_GRAY);
    
    /*Add two data series*/
    cpu_ser = lv_chart_add_series(chart_cpu_ram, LV_COLOR_RED);
    ram_ser = lv_chart_add_series(chart_cpu_ram, LV_COLOR_GREEN);

    /*Directly set points on 'ram_ser'*/
    // lv_chart_set_points(chart_cpu_ram, cpu_ser, cpu_usage_series);
    // lv_chart_set_points(chart_cpu_ram, ram_ser, ram_usage_series);

    // lv_chart_refresh(chart_cpu_ram); /*Required after direct set*/
}

void create_chart_network(void)
{
    /*Create a chart*/
    chart_network = lv_chart_create(lv_scr_act(), NULL);
    lv_obj_set_size(chart_network, 320, 120);
    lv_obj_align(chart_network, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 121);
    lv_chart_set_type(chart_network, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart_network, 0, 10240);
    lv_chart_set_point_count(chart_network, 30);

    /*Add a faded are effect*/
    lv_obj_set_style_local_bg_opa(chart_network, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, LV_OPA_COVER); /*Max. opa.*/
    lv_obj_set_style_local_bg_grad_dir(chart_network, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
    lv_obj_set_style_local_bg_main_stop(chart_network, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, 255);    /*Max opa on the top*/
    lv_obj_set_style_local_bg_grad_stop(chart_network, LV_CHART_PART_SERIES, LV_STATE_DEFAULT, 0);      /*Transparent on the bottom*/

    lv_obj_set_style_local_bg_color(chart_network, LV_CHART_PART_BG, LV_STATE_DEFAULT, LV_COLOR_MY_GRAY);
    
    /*Add two data series*/
    netup_ser = lv_chart_add_series(chart_network, LV_COLOR_BLUE);
    netdn_ser = lv_chart_add_series(chart_network, LV_COLOR_YELLOW);

    /*Directly set points on 'ram_ser'*/
    // lv_chart_set_points(chart_network, netup_ser, netup_usage_series);
    // lv_chart_set_points(chart_network, netdn_ser, netdn_usage_series);

    // lv_chart_refresh(chart_network); /*Required after direct set*/
}

void update_monitoring_data(void){
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.addHeader("User-Agent", "ESP8266");
        http.addHeader("Content-Type", "application/json");      
        http.begin(client, monitoring_data_url);

        int httpCode = http.GET();
        Serial.printf("Http Code %d\n", httpCode);
        if (httpCode == 200) {
            // decode json response 
            String payload = http.getString();
            // Serial.println("===========================");
            // Serial.println("收到的JSON数据:");
            // Serial.println(payload);
            
            DynamicJsonDocument json(2048);
            DeserializationError error = deserializeJson(json, payload);
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            // get the newest data and update chart
            String value = json[4];
            int cpu_value = json[4]["CPU"];
            int ram_value = json[4]["RAM"];
            int received = json[4]["RECEIVED"];
            int sent = json[4]["SENT"];

            // Serial.println(value);
            // Serial.println(cpu_value);
            // Serial.println(ram_value);
            
            int cpu_point = int(float(cpu_value) / 150 * 100);
            int ram_point = int(100 - float(ram_value) / (1024 * 8) * 100);
            
            // Update CPU RAM chart
            lv_chart_set_next(chart_cpu_ram, cpu_ser, cpu_point);
            lv_chart_set_next(chart_cpu_ram, ram_ser, ram_point);

            // Update Network chart
            lv_chart_set_next(chart_network, netup_ser, sent);
            lv_chart_set_next(chart_network, netdn_ser, received);

            // Update label values
            String cpu_ram_string = "CPU:  " + String(cpu_value) + "%   " + "RAM:  " + String(ram_point) + "%";
            lv_label_set_text(cpu_ram_value_label, cpu_ram_string.c_str());
            String network_string = "UP:  " + String(sent) +"KB/s   " + "DN:  " + String(received) + "KB/s";
            lv_label_set_text(network_value_label, network_string.c_str());

        } else {
            Serial.printf("HTTP请求失败，错误码: %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("WiFi未连接");
    }
}


// Task callback 
static void task_cb(lv_task_t *task)
{
    update_monitoring_data();

    // // 测试内存泄漏
    Serial.print("⚠ Left Memory:");
    Serial.println(ESP.getFreeHeap());
}

void setup() {   
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWiFi();
    }
    // Set up the TFT and display driver
    Serial.begin(115200);
    tft.begin();
    tft.setRotation(1);
    lv_init();

    lv_disp_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // set background as black
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_local_bg_color(scr, LV_OBJ_PART_MAIN,LV_STATE_DEFAULT, LV_COLOR_BLACK);

    // Create charts
    create_chart_cpu_ram();
    create_chart_network();

    create_label_cpu_ram();
    create_label_network();

    // task callback, refresh UI every 2000 seconds
    lv_task_t *t = lv_task_create(task_cb, 2000, LV_TASK_PRIO_MID, &test_data);

}

void loop() {
    lv_task_handler();
    delay(10);
}
