#include <TuyaWifi.h>
#include <SoftwareSerial.h>


// Simple demonstration on using an input device to trigger changes on your
// NeoPixels. Wire a momentary push button to connect from ground to a
// digital IO pin. When the button is pressed it will change to a new pixel
// animation. Initial state has all pixels off -- press the button once to
// start the first animation. As written, the button does not interrupt an
// animation in-progress, it works only when idle.

#include <FastLED.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Digital IO pin connected to the button. This will be driven with a
// pull-up resistor so the switch pulls the pin to ground momentarily.
// On a high -> low transition the button press logic will execute.
#define BUTTON_PIN   2

#define PIXEL_PIN    6  // Digital IO pin connected to the NeoPixels.

#define PIXEL_COUNT 256  // Number of NeoPixels

boolean oldState = HIGH;
int     mode     = 0;    // Currently-active animation mode, 0-9

//LED自定义显示模式
#define MODE_NONE           0   //不显示
#define MODE_RAINBOW_FLOW   1   //彩虹流动
#define MODE_RAINBOW_BLINK  2   //彩虹闪烁
#define MODE_STR_UP         3   //字符上移
#define MODE_STR_DOWN       4   //字符下移
#define MODE_STR_LEFT       5   //字符左移
#define MODE_STR_RIGHT      6   //字符右移
#define MODE_MUSIC          7   //音乐闪烁

//矩阵参数
#define LED_MATRIX_WIDTH    16
#define LED_MATRIX_HEIGHT   16

//矩阵设置方式
#define LED_MATRIX_UP       0
#define LED_MATRIX_DOWN     1
#define LED_MATRIX_LEFT     2
#define LED_MATRIX_RIGHT    3

TuyaWifi my_device;

unsigned char led_state = 0;
/* Connect network button pin */

//彩虹起始变量
unsigned char firstPixelHue = 0;
unsigned char blinkcnt = 0;

int key_pin = 7;

//开关(可下发可上报)
//备注:
#define DPID_SWITCH_LED 20
//模式(可下发可上报)
//备注:
#define DPID_WORK_MODE 21
//亮度值(可下发可上报)
//备注:
#define DPID_BRIGHT_VALUE 22
//冷暖值(可下发可上报)
//备注:
#define DPID_TEMP_VALUE 23
//彩光(可下发可上报)
//备注:类型：字符；
//Value: 000011112222；
//0000：H（色度：0-360，0X0000-0X0168）；
//1111：S (饱和：0-1000, 0X0000-0X03E8）；
//2222：V (明度：0-1000，0X0000-0X03E8)
#define DPID_COLOUR_DATA 24
//场景(可下发可上报)
//备注:类型：字符;
//Value: 0011223344445555666677778888;
//00：情景号;
//11：单元切换间隔时间（0-100）;
//22：单元变化时间（0-100）;
//33：单元变化模式（0静态1跳变2渐变）;
//4444：H（色度：0-360，0X0000-0X0168）;
//5555：S (饱和：0-1000, 0X0000-0X03E8);
//6666：V (明度：0-1000，0X0000-0X03E8);
//7777：白光亮度（0-1000）;
//8888：色温值（0-1000）;
//注：数字1-8的标号对应有多少单元就有多少组
#define DPID_SCENE_DATA 25
//倒计时剩余时间(可下发可上报)
//备注:
#define DPID_COUNTDOWN 26
//音乐灯(只下发)
//备注:类型：字符串；
//Value: 011112222333344445555；
//0：   变化方式，0表示直接输出，1表示渐变；
//1111：H（色度：0-360，0X0000-0X0168）；
//2222：S (饱和：0-1000, 0X0000-0X03E8）；
//3333：V (明度：0-1000，0X0000-0X03E8）；
//4444：白光亮度（0-1000）；
//5555：色温值（0-1000）
#define DPID_MUSIC_DATA 27
//调节(只下发)
//备注:类型：字符串 ;
//Value: 011112222333344445555  ;
//0：   变化方式，0表示直接输出，1表示渐变;
//1111：H（色度：0-360，0X0000-0X0168）;
//2222：S (饱和：0-1000, 0X0000-0X03E8);
//3333：V (明度：0-1000，0X0000-0X03E8);
//4444：白光亮度（0-1000）;
//5555：色温值（0-1000）
#define DPID_CONTROL_DATA 28
//入睡(可下发可上报)
//备注:灯光按设定的时间淡出直至熄灭
#define DPID_SLEEP_MODE 31
//唤醒(可下发可上报)
//备注:灯光按设定的时间逐渐淡入直至设定的亮度
#define DPID_WAKEUP_MODE 32
//断电记忆(可下发可上报)
//备注:通电后，灯亮起的状态
#define DPID_POWER_MEMORY 33
//勿扰模式(可下发可上报)
//备注:适用经常停电区域，开启通电勿扰，通过APP关灯需连续两次上电才会亮灯
//Value：ABCCDDEEFFGG
//A：版本，初始版本0x00；
//B：模式，0x00初始默认值、0x01恢复记忆值、0x02用户定制；
//CC：色相 H，0~360；
//DD：饱和度 S，0~1000；
//EE：明度 V，0~1000；
//FF：亮度，0~1000；
//GG：色温，0~1000；
#define DPID_DO_NOT_DISTURB 34
//麦克风音乐律动(可下发可上报)
//备注:类型：  字符串
//Value：  AABBCCDDEEFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNN
//AA  版本
//BB  0-关闭，1-打开
//CC  模式编号，自定义从201开始
//DD  变换方式：0 - 呼吸模式，1 -跳变模式 ， 2 - 经典模式
//EE  变化速度
//FF  灵敏度
//GGGG  颜色1-色相饱和度
//HHHH  颜色2-色相饱和度
//......
//NNNN  颜色8-色相饱和度
#define DPID_MIC_MUSIC_DATA 42
//炫彩情景(可下发可上报)
//备注:专门用于幻彩灯带场景
//Value：ABCDEFGHIJJKLLM...
//A：版本号；
//B：情景模式编号；
//C：变化方式（0-静态、1-渐变、2跳变、3呼吸、4-闪烁、10-流水、11-彩虹）
//D：单元切换间隔时间（0-100）;
//E：单元变化时间（0-100）；
//FGH：设置项；
//I：亮度（亮度V：0~100）；
//JJ：颜色1（色度H：0-360）；
//K：饱和度1 (饱和度S：0-100)；
//LL：颜色2（色度H：0-360）；
//M：饱和度2（饱和度S：0~100）；
//注：有多少个颜色单元就有多少组，最多支持20组；
//每个字母代表一个字节
#define DPID_DREAMLIGHT_SCENE_MODE 51
//炫彩本地音乐律动(可下发可上报)
//备注:专门用于幻彩灯带本地音乐
//Value：ABCDEFGHIJKKLMMN...
//A：版本号；
//B：本地麦克风开关（0-关、1-开）；
//C：音乐模式编号；
//D：变化方式；
//E：变化速度（1-100）;
//F：灵敏度(1-100)；
//GHI：设置项；
//J：亮度（亮度V：0~100）；
//KK：颜色1（色度H：0-360）；
//L：饱和度1 (饱和度S：0-100)；
//MM：颜色2（色度H：0-360）；
//N：饱和度2（饱和度S：0~100）；
//注：有多少个颜色单元就有多少组，最多支持8组；
//每个字母代表一个字节
#define DPID_DREAMLIGHTMIC_MUSIC_DATA 52
//点数/长度设置(可下发可上报)
//备注:幻彩灯带裁剪之后重新设置长度
#define DPID_LIGHTPIXEL_NUMBER_SET 53



///* Current device DP values */
unsigned char dp_bool_value = 0;
long dp_value_value = 0;
unsigned char dp_enum_value = 0;
unsigned char dp_string_value[21] = {"0"};
uint16_t Hue = 0; //HSV
uint8_t Sat = 0;
uint8_t Val = 0;
uint8_t scene_mode = 0;
unsigned char hex[10] = {"0"};
//unsigned char dp_raw_value[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
//int dp_fault_value = 0x01;

/* Stores all DPs and their types. PS: array[][0]:dpid, array[][1]:dp type.
                                       dp type(TuyaDefs.h) : DP_TYPE_RAW, DP_TYPE_BOOL, DP_TYPE_VALUE, DP_TYPE_STRING, DP_TYPE_ENUM, DP_TYPE_BITMAP
*/
unsigned char dp_array[][2] = {
  {DPID_SWITCH_LED, DP_TYPE_BOOL},
  {DPID_WORK_MODE, DP_TYPE_ENUM},
  {DPID_BRIGHT_VALUE, DP_TYPE_VALUE},
  {DPID_TEMP_VALUE, DP_TYPE_VALUE},
  {DPID_COLOUR_DATA, DP_TYPE_STRING},
  {DPID_SCENE_DATA, DP_TYPE_STRING},
  {DPID_COUNTDOWN, DP_TYPE_VALUE},
  {DPID_MUSIC_DATA, DP_TYPE_STRING},
  {DPID_CONTROL_DATA, DP_TYPE_STRING},
  {DPID_SLEEP_MODE, DP_TYPE_RAW},
  {DPID_WAKEUP_MODE, DP_TYPE_RAW},
  {DPID_POWER_MEMORY, DP_TYPE_RAW},
  {DPID_DO_NOT_DISTURB, DP_TYPE_BOOL},
  {DPID_MIC_MUSIC_DATA, DP_TYPE_STRING},
  {DPID_DREAMLIGHT_SCENE_MODE, DP_TYPE_RAW},
  {DPID_DREAMLIGHTMIC_MUSIC_DATA, DP_TYPE_RAW},
  {DPID_LIGHTPIXEL_NUMBER_SET, DP_TYPE_VALUE},
};

unsigned char pid[] = {"wbt3vyglw7h3odtm"};//*********处替换成涂鸦IoT平台自己创建的产品的PID
unsigned char mcu_ver[] = {"1.0.0"};

//字符串字模
const unsigned char textCode[] =
{
  0x00, 0x00, 0xC4, 0x00, 0x04, 0x07, 0x44, 0x3E, 0x44, 0x34, 0x44, 0x04, 0x44, 0x04, 0xFC, 0x7F,
  0xFC, 0x7F, 0x44, 0x04, 0x44, 0x04, 0xC4, 0x0C, 0x4C, 0x0C, 0x0C, 0x04, 0x04, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x3F, 0x88, 0x10, 0x88, 0x10, 0x88, 0x10, 0x88, 0x10,
  0x88, 0x10, 0x88, 0x10, 0xFC, 0x3F, 0xFC, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x01, 0x00, 0x07, 0xFC, 0x7F, 0xFC, 0x7F, 0x80, 0x0C, 0x80, 0x0E, 0x8C, 0x08, 0xF8, 0x08,
  0xF0, 0x7F, 0x80, 0x78, 0xB8, 0x08, 0x8C, 0x1F, 0x84, 0x1F, 0x84, 0x01, 0x80, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x0C, 0x00, 0x98, 0x3F, 0x30, 0x3F, 0x64, 0x11, 0x24, 0x11, 0x04, 0x11, 0xFC, 0x3F,
  0xFC, 0x3F, 0x40, 0x21, 0x60, 0x61, 0x30, 0x23, 0x1C, 0x23, 0x0C, 0x03, 0x00, 0x00, 0x00, 0x00
};

CRGB leds[PIXEL_COUNT];
//LED显示模式
unsigned char ledMode = MODE_NONE;
//LED显示缓存，用于音乐律动
unsigned short ledBuffer[16];
//LED显示缓存索引
unsigned char ledBufIndex = 0;
//LED颜色缓存
CRGB ledColor[16];
//LED颜色缓存索引
unsigned char ledColorIndex = 0;
//字符显示行数
unsigned char textLine = 0;
//是否有新的音乐数据
unsigned char isMusicNew = 0;
//是否处于音乐模式，用于初始化
unsigned char isInMusicMode = 0;

unsigned long last_time = 0;
SoftwareSerial DebugSerial(8, 9);
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  LEDS.addLeds<WS2812, PIXEL_PIN, GRB>(leds, PIXEL_COUNT);
  FastLED.setBrightness(15);
  FastLED.showColor(CRGB(0, 0, 0));

  DebugSerial.begin(9600);

  Serial.begin(9600);
  //Initialize led port, turn off led.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  //Initialize networking keys.
  pinMode(key_pin, INPUT_PULLUP);

  //incoming all DPs and their types array, DP numbers
  //Enter the PID and MCU software version
  my_device.init(pid, mcu_ver);
  my_device.set_dp_cmd_total(dp_array, 17);
  //register DP download processing callback function
  my_device.dp_process_func_register(dp_process);
  //register upload all DP callback function
  my_device.dp_update_all_func_register(dp_update_all);

  last_time = millis();

}

void loop() {

  //Enter the connection network mode when Pin7 is pressed.
  if (digitalRead(key_pin) == LOW) {
    delay(80);
    if (digitalRead(key_pin) == LOW) {
      my_device.mcu_set_wifi_mode(SMART_CONFIG);
    }
  }
  my_device.uart_service();


  /* LED blinks when network is being connected */
  if ((my_device.mcu_get_wifi_work_state() != WIFI_LOW_POWER) && (my_device.mcu_get_wifi_work_state() != WIFI_CONN_CLOUD) && (my_device.mcu_get_wifi_work_state() != WIFI_SATE_UNKNOW)) {
    if (millis() - last_time >= 500) {
      last_time = millis();

      if (led_state == LOW) {
        led_state = HIGH;
      } else {
        led_state = LOW;
      }
      digitalWrite(LED_BUILTIN, led_state);
    }
  }

  myLEDShow();

  // Get current button state.
  boolean newState = digitalRead(BUTTON_PIN);

  // Check if state changed from high to low (button press).
  if ((newState == LOW) && (oldState == HIGH)) {
    // Short delay to debounce button.
    delay(20);
    // Check if button is still low after debounce.
    newState = digitalRead(BUTTON_PIN);
    if (newState == LOW) {     // Yes, still low
      if (++mode > 8) mode = 0; // Advance to next mode, wrap around after #8
      switch (mode) {          // Start the new animation...
        case 0:
          colorWipe(CRGB(0,   0,   0), 2);    // Black/off
          break;
        case 1:
          colorWipe(CRGB(255,   0,   0), 2);    // Red
          break;
        case 2:

          //colorWipe(FastLED.Color(  0, 255,   0), 2);    // Green
          break;
        case 3:
          colorWipe(CRGB(  0,   0, 255), 2);    // Blue
          break;
        case 4:
          theaterChase(CRGB(127, 127, 127), 2); // White
          break;
        case 5:
          theaterChase(CRGB(127,   0,   0), 2); // Red
          break;
        case 6:
          theaterChase(CRGB(  0,   0, 127), 2); // Blue
          break;
        case 7:
          rainbow(2);
          break;
        case 8:
          theaterChaseRainbow(5);
          break;
      }
    }
  }

  // Set the last-read button state to the old state.
  oldState = newState;
}

// Fill FastLED pixels one after another with a color. FastLED is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// FastLED.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(CRGB rgb, int wait) {
  for (int i = 0; i < FastLED.size(); i++) { // For each pixel in FastLED...
    leds[i] = rgb;         //  Set pixel's color (in RAM)
    FastLED.show();                          //  Update FastLED to match
    //    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la FastLED.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(CRGB color, int wait) {
  for (int a = 0; a < 10; a++) { // Repeat 10 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      FastLED.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of FastLED in steps of 3...
      for (int c = b; c < FastLED.size(); c += 3) {
        leds[c] = color; // Set pixel 'c' to value 'color'
      }
      FastLED.show(); // Update FastLED with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole FastLED. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 3 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 3*65536. Adding 256 to firstPixelHue each time
  // means we'll make 3*65536/256 = 768 passes through this outer loop:
  for (long firstPixelHue = 0; firstPixelHue < 256; firstPixelHue += 1) {
    for (int i = 0; i < FastLED.size(); i++) { // For each pixel in FastLED...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the FastLED
      // (FastLED.numPixels() steps):
      unsigned char pixelHue = firstPixelHue + i;
      // FastLED.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through FastLED.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      leds[i].setHue(pixelHue);
    }
    FastLED.show(); // Update FastLED with new contents
    //    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  unsigned char firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for (int a = 0; a < 30; a++) { // Repeat 30 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      FastLED.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of FastLED in increments of 3...
      for (int c = b; c < FastLED.size(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the FastLED (FastLED.numPixels() steps):
        unsigned char      hue   = firstPixelHue + c;
        leds[c].setHue(hue);
      }
      FastLED.show();                // Update FastLED with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 1; // One cycle of color wheel over 90 frames
    }
  }
}


/**
   @description: DP download callback function.
   @param {unsigned char} dpid
   @param {const unsigned char} value
   @param {unsigned short} length
   @return {unsigned char}
*/
unsigned char dp_process(unsigned char dpid, const unsigned char value[], unsigned short length)
{
  switch (dpid) {
    case DPID_SWITCH_LED:
      ledMode = MODE_NONE;
      isInMusicMode = 0;
      dp_bool_value = my_device.mcu_get_dp_download_data(dpid, value, length); /* Get the value of the down DP command */
      if (dp_bool_value) {
        //Turn on
        colorfill (CRGB(  0, 255,  0)); //上一次状态
      } else {
        //Turn off
        colorfill (CRGB(  0, 0,   0));
      }
      //Status changes should be reported.
      my_device.mcu_dp_update(dpid, value, length);
      break;

    case DPID_WORK_MODE:
      isInMusicMode = 0;
      colorfill (CRGB( 255, 255,  0));
      dp_enum_value  = my_device.mcu_get_dp_download_data(dpid, value, length); /* Get the value of the down DP command */
      switch (dp_enum_value) {
        case 0: // white mode
          colorfill (CRGB(  255, 255,  255));
          break;
        case 1: // colour mode

          break;
        case 2: // scene mode

          break;
        case 3: // music mode

          break;

      }
      //Status changes should be reported.
      my_device.mcu_dp_update(dpid, value, length);
      break;

    case DPID_COUNTDOWN:  //倒计时
      colorfill (CRGB( 255, 0,  0));
      my_device.mcu_dp_update(dpid, value, length);
      break;

    case DPID_MUSIC_DATA: //音乐律动
      ledMode = MODE_MUSIC;
      if (isInMusicMode == 0)
      {
        isInMusicMode = 1;
        for (unsigned char i = 0; i < 16; i++)
          ledColor[i].setRGB(0, 0, 0);
        ledBufIndex = 0;
        ledColorIndex = 0;
      }
      my_device.mcu_dp_update(dpid, value, length);
      colour_data_control(value, length);

      break;

    case DPID_DREAMLIGHT_SCENE_MODE: //炫彩情景
      isInMusicMode = 0;
      my_device.mcu_dp_update(DPID_DREAMLIGHT_SCENE_MODE, value, length);

      scene_mode = value[1];

      switch (scene_mode) {
        case 0:
          ledMode = MODE_NONE;
          colorWipe(CRGB(  0,   0,   0), 2);    // Black/off
          break;
        case 1:
          ledMode = MODE_NONE;
          colorWipe(CRGB(255,   0,   0), 2);    // Red
          break;
        case 2:
          ledMode = MODE_NONE;
          colorWipe(CRGB(  0, 255,   0), 2);    // Blue
          break;
        case 3:
          ledMode = MODE_NONE;
          theaterChase(CRGB(0,   0,   255), 50); // 闪烁Green
          break;
        case 4:
          //上移
          firstPixelHue = 0;
          textLine = 0;
          ledColorIndex = 0;
          ledMode = MODE_STR_UP;
          last_time = millis();
          break;
        case 5:
          //下移
          firstPixelHue = 0;
          textLine = 0;
          ledColorIndex = 0;
          ledMode = MODE_STR_DOWN;
          last_time = millis();
          break;
        case 6:
          //左移
          firstPixelHue = 0;
          textLine = 0;
          ledColorIndex = 0;
          ledMode = MODE_STR_LEFT;
          last_time = millis();
          break;
        case 7:
          //右移
          firstPixelHue = 0;
          textLine = 0;
          ledColorIndex = 0;
          ledMode = MODE_STR_RIGHT;
          last_time = millis();
          break;
        case 8:
          // 彩虹
          firstPixelHue = 0;
          ledMode = MODE_RAINBOW_FLOW;
          last_time = millis();
          break;
        case 9:
          // 闪烁彩虹
          firstPixelHue = 0;
          blinkcnt = 0;
          ledMode = MODE_RAINBOW_BLINK;
          last_time = millis();
          break;
      }

      break;

    case DPID_LIGHTPIXEL_NUMBER_SET: //长度设置
      my_device.mcu_dp_update(dpid, value, length);
      break;
    default: break;
  }
  return SUCCESS;
}

/**
   @description: Upload all DP status of the current device.
   @param {*}
   @return {*}
*/
void dp_update_all(void)
{
  my_device.mcu_dp_update(DPID_SWITCH_LED, led_state, 1);
}



//拓展
void colorfill(CRGB color) {
  FastLED.showColor(color);
}

//LED矩阵设置某行
void maxtrixFillRow(CRGB color, unsigned char row, unsigned short data, unsigned char mode, unsigned char needupdate)
{
  unsigned char realrow;
  unsigned char realcol;
  unsigned short temp = data;

  switch (mode)
  {
    case LED_MATRIX_UP: //从上往下显示
      //起始行列计算
      realrow = row;
      //因为LED为蛇形排列，需要判断奇偶行
      if (row & 0x01)
        realcol = 0;
      else
        realcol = LED_MATRIX_WIDTH - 1;
      for (unsigned char i = 0; i < 16; i++)
      {
        //移位赋值，为1则显示颜色，为0则不显示
        if (temp & 0x01)
          leds[realrow * LED_MATRIX_WIDTH + realcol] = color;
        else
          leds[realrow * LED_MATRIX_WIDTH + realcol] = CRGB(0, 0, 0);
        temp = temp >> 1;
        //因为LED为蛇形排列，需要判断奇偶行
        if (row & 0x01)
          realcol++;
        else
          realcol--;
      }
      break;
    case LED_MATRIX_DOWN: //从下往上显示
      //起始行列计算
      realrow = LED_MATRIX_HEIGHT - 1 - row;
      //因为LED为蛇形排列，需要判断奇偶行
      if (row & 0x01)
        realcol = 0;
      else
        realcol = LED_MATRIX_WIDTH - 1;
      for (unsigned char i = 0; i < 16; i++)
      {
        //移位赋值，为1则显示颜色，为0则不显示
        if (temp & 0x01)
          leds[realrow * LED_MATRIX_WIDTH + realcol] = color;
        else
          leds[realrow * LED_MATRIX_WIDTH + realcol] = CRGB(0, 0, 0);
        temp = temp >> 1;
        //因为LED为蛇形排列，需要判断奇偶行
        if (row & 0x01)
          realcol++;
        else
          realcol--;
      }
      break;
    case LED_MATRIX_LEFT: //从左往右显示
      //起始行列计算
      realrow = LED_MATRIX_HEIGHT - 1;
      for (unsigned char i = 0; i < 16; i++)
      {
        //因为LED为蛇形排列，需要判断奇偶行
        if (realrow & 0x01)
          realcol = row;
        else
          realcol = LED_MATRIX_WIDTH - 1 - row;
        //移位赋值，为1则显示颜色，为0则不显示
        if (temp & 0x01)
          leds[realrow * LED_MATRIX_WIDTH + realcol] = color;
        else
          leds[realrow * LED_MATRIX_WIDTH + realcol] = CRGB(0, 0, 0);
        temp = temp >> 1;
        realrow--;
      }
      break;
    case LED_MATRIX_RIGHT:  //从右往左显示
      //起始行列计算
      realrow = 0;
      for (unsigned char i = 0; i < 16; i++)
      {
        //因为LED为蛇形排列，需要判断奇偶行
        if (realrow & 0x01)
          realcol = LED_MATRIX_WIDTH - 1 - row;
        else
          realcol = row;
        //移位赋值，为1则显示颜色，为0则不显示
        if (temp & 0x01)
          leds[realrow * LED_MATRIX_WIDTH + realcol] = color;
        else
          leds[realrow * LED_MATRIX_WIDTH + realcol] = CRGB(0, 0, 0);
        temp = temp >> 1;
        realrow++;
      }
      break;
  }
  if (needupdate)
    FastLED.show();
}

//显示字幕，增加16列空白显示，作为起始与结束间的间隔
void myLEDShowText(unsigned char mode)
{
  //有效显示区域为16列
  for (unsigned char i = 0; i < 16; i++)
  {
    //计算当前显示列数
    unsigned char row = textLine + i;
    //超界修正
    if (row >= 80)
      row -= 80;

    unsigned short tmp;
    //计算显示颜色
    unsigned char index = ledColorIndex + i;
    //超界修正
    if (index >= 16)
      index -= 16;
    if (row < 16)
    {
      //此时为无效列
      tmp = 0;
    }
    else
    {
      //有效列
      row -= 16;
      //更新显示内容
      tmp = *((unsigned short*)(textCode + 2 * row));
      if (i == 15)
      {
        //循环显示结束时，更新颜色
        ledColor[index].setHue(firstPixelHue);
        firstPixelHue ++;
      }
    }
    //显示当前列
    maxtrixFillRow(ledColor[index], i, tmp, mode, 0);
  }
  FastLED.show();
  //重新计算起始列数
  textLine++;
  //超界修正
  if (textLine >= 80)
    textLine = 0;
  ledColorIndex++;
  //超界修正
  if (ledColorIndex >= 16)
    ledColorIndex = 0;
}

//自定义显示函数
void myLEDShow()
{
  if (ledMode == MODE_NONE)
    return;
  switch (ledMode)
  {
    case MODE_RAINBOW_FLOW: //彩虹流水
      if (millis() - last_time >= 150)
      {
        last_time = millis();
        for (int i = 0; i < FastLED.size(); i++)
        {
          unsigned char pixelHue = firstPixelHue + i;
          leds[i].setHue(pixelHue);
        }
        FastLED.show();
        firstPixelHue += 1;
      }
      break;
    case MODE_RAINBOW_BLINK:  //彩虹闪烁
      if (millis() - last_time >= 150)
      {
        last_time = millis();
        for (int c = blinkcnt; c < FastLED.size(); c += 3) {
          unsigned char hue   = firstPixelHue + c;
          leds[c].setHue(hue);
        }
        FastLED.show();
        blinkcnt++;
        if (blinkcnt > 2)
          blinkcnt = 0;
        firstPixelHue += 65536 / 90;
        if (firstPixelHue > 65536)
          firstPixelHue = 0;
      }
      break;
    case MODE_STR_UP: //字幕上移
      if (millis() - last_time >= 150)
      {
        last_time = millis();
        myLEDShowText(LED_MATRIX_UP);
      }
      break;
    case MODE_STR_DOWN: //字幕下移
      if (millis() - last_time >= 150)
      {
        last_time = millis();
        myLEDShowText(LED_MATRIX_DOWN);
      }
      break;
    case MODE_STR_LEFT: //字幕左移
      if (millis() - last_time >= 150)
      {
        last_time = millis();
        myLEDShowText(LED_MATRIX_LEFT);
      }
      break;
    case MODE_STR_RIGHT:  //字幕右移
      if (millis() - last_time >= 150)
      {
        last_time = millis();
        myLEDShowText(LED_MATRIX_RIGHT);
      }
      break;
    case MODE_MUSIC:  //音乐律动
      if (isMusicNew)
      {
        //逐列显示
        for (unsigned char i = 0; i < 16; i++)
        {
          //计算起始索引
          unsigned char bufindex = ledBufIndex + (i + 1);
          //超界修正
          if (bufindex >= 16)
            bufindex -= 16;
            //计算起始索引
          unsigned char colorindex = ledColorIndex + (i + 1);
          //超界修正
          if (colorindex >= 16)
            colorindex -= 16;
            //显示
          maxtrixFillRow(ledColor[colorindex], i, ledBuffer[bufindex], LED_MATRIX_LEFT, 0);
        }
        FastLED.show();
        //更新起始索引
        ledColorIndex++;
        //超界修正
        if (ledColorIndex >= 16)
          ledColorIndex = 0;
          
        ledBufIndex++;
        //超界修正
        if (ledBufIndex >= 16)
          ledBufIndex = 0;
        isMusicNew = 0;
      }
      break;
  }
}

void colour_data_control( const unsigned char value[], u16 length)
{
  u8 string_data[13];
  u16 h, s, v, m, d;
  u8 r, g, b;
  u8 hue;
  u8 sat, val;

  u32 c = 0;

  string_data[0] = value[0]; //渐变、直接输出
  string_data[1] = value[1];
  string_data[2] = value[2];
  string_data[3] = value[3];
  string_data[4] = value[4];
  string_data[5] = value[5];
  string_data[6] = value[6];
  string_data[7] = value[7];
  string_data[8] = value[8];
  string_data[9] = value[9];
  string_data[10] = value[10];
  string_data[11] = value[11];
  string_data[12] = value[12];

  h = __str2short(__asc2hex(string_data[1]), __asc2hex(string_data[2]), __asc2hex(string_data[3]), __asc2hex(string_data[4]));
  s = __str2short(__asc2hex(string_data[5]), __asc2hex(string_data[6]), __asc2hex(string_data[7]), __asc2hex(string_data[8]));
  v = __str2short(__asc2hex(string_data[9]), __asc2hex(string_data[10]), __asc2hex(string_data[11]), __asc2hex(string_data[12]));

  // if (v <= 10) {
  //     v = 0;
  // } else {
  //     v = color_val_lmt_get(v);
  // }

  //hsv2rgb((float)h, (float)s / 1000.0, (float)v / 1000.0, &r , &g, &b);

  // c= r<<16|g<<8|b;
  hue = h * 255 / 360;
  sat = s * 255 / 1000;
  val = v * 255 / 1000;

  //根据数值生成颜色
  ledColor[ledColorIndex].setHue(hue);

  //将v值从0~1000重新映射到1~16
  unsigned short ledvalue = v / 62;
  if (ledvalue < 1)
    ledvalue = 1;

  //生成待显示数值，即二进制数有ledvalue个1
  ledBuffer[ledBufIndex] = (1 << ledvalue) - 1;

  isMusicNew = 1;
}

/**
   @brief  str to short
   @param[in] {a} Single Point
   @param[in] {b} Single Point
   @param[in] {c} Single Point
   @param[in] {d} Single Point
   @return Integrated value
   @note   Null
*/
u32 __str2short(u32 a, u32 b, u32 c, u32 d)
{
  return (a << 12) | (b << 8) | (c << 4) | (d & 0xf);
}

/**
    @brief ASCALL to Hex
    @param[in] {asccode} 当前ASCALL值
    @return Corresponding value
    @retval None
*/
u8 __asc2hex(u8 asccode)
{
  u8 ret;

  if ('0' <= asccode && asccode <= '9')
    ret = asccode - '0';
  else if ('a' <= asccode && asccode <= 'f')
    ret = asccode - 'a' + 10;
  else if ('A' <= asccode && asccode <= 'F')
    ret = asccode - 'A' + 10;
  else
    ret = 0;

  return ret;
}

/**
    @brief Normalized
    @param[in] {dp_val} dp value
    @return result
    @retval None
*/
u16 color_val_lmt_get(u16 dp_val)
{
  u16 max = 255 * 100 / 100;
  u16 min = 255 * 1 / 100;

  return ((dp_val - 10) * (max - min) / (1000 - 10) + min);
}

/**
    @brief hsv to rgb
    @param[in] {h} tone
    @param[in] {s} saturation
    @param[in] {v} Lightness
    @param[out] {color_r} red
    @param[out] {color_g} green
    @param[out] {color_b} blue
    @retval None
*/
void hsv2rgb(float h, float s, float v, u8 *color_r, u8 *color_g, u8 *color_b)
{
  float h60, f;
  u32 h60f, hi;

  h60 = h / 60.0;
  h60f = h / 60;

  hi = ( signed int)h60f % 6;
  f = h60 - h60f;

  float p, q, t;

  p = v * (1 - s);
  q = v * (1 - f * s);
  t = v * (1 - (1 - f) * s);

  float r, g, b;

  r = g = b = 0;
  if (hi == 0) {
    r = v;          g = t;        b = p;
  } else if (hi == 1) {
    r = q;          g = v;        b = p;
  } else if (hi == 2) {
    r = p;          g = v;        b = t;
  } else if (hi == 3) {
    r = p;          g = q;        b = v;
  } else if (hi == 4) {
    r = t;          g = p;        b = v;
  } else if (hi == 5) {
    r = v;          g = p;        b = q;
  }

  r = (r * (float)255);
  g = (g * (float)255);
  b = (b * (float)255);

  *color_r = r;
  *color_g = g;
  *color_b = b;
  // r *= 100;
  // g *= 100;
  // b *= 100;

  // *color_r = (r + 50) / 100;
  // *color_g = (g + 50) / 100;
  // *color_b = (b + 50) / 100;
}
