#include <Arduino.h>

#define DEBUG true

#include <AsyncStream.h>
AsyncStream<255> serial(&Serial, '\n');

#include <TimerMs.h>
// настройка (период, мс), (не запущен/запущен), (период/таймер)
TimerMs blink_tmr(2000, false, 1);

#include <GyverOS.h>
GyverOS<3> OS;

#define TELEGRAM_TASK 0
#define PORTAL_TASK 1
#define SERIAL_MONITOR_TASK 2

#include <ESP8266WiFi.h>

#define WIFI_SSID ""
#define WIFI_PASS ""

#include <FastBot.h>

#define BOT_TOKEN ""
#define CHAT_ID ""

FastBot bot(BOT_TOKEN);

#include <GyverPortal.h>
GyverPortal portal;

bool waiting_for_target= false;

bool isNumber(String s)
{
    for (unsigned int i = 0; i < s.length(); i++)
        if (isdigit(s[i]) == false)
            return false;
 
    return true;
}

void serial_controller()
{
  if (serial.available())
  {
    switch (String(serial.buf).toInt())
    {
    case 0:
      break;
    case 1:
      break;

    default:
      if (DEBUG)
      {
        Serial.println(serial.buf);
        bot.sendMessage(serial.buf, CHAT_ID);
      }
    }
  }
}

void bot_controller()
{
  bot.tick();
}

void portal_controller()
{
  portal.tick();
  if (portal.click("move"))
  {
    portal.log.println("Command move"); Serial.println("1");
  }
  else if (portal.click("stop"))
  {
    portal.log.println("Command stop"); Serial.println("0");
  }
  if (portal.form()) {
  if (portal.form("/target")) {
    String target = portal.getString("txt");
    Serial.println(target.toInt()+2); 
    portal.log.println("New Target: #" + target);
  }
}
}

void blink()
{
  digitalWrite(16, !digitalRead(16));
  blink_tmr.start();
}

// создать свою функцию вида имя(FB_msg& сообщение)
void newMsg(FB_msg &msg)
{
  blink_tmr.start();
  if (msg.text == "start" || msg.text == "help")
  {
    bot.sendMessage("Hello, " + msg.username + ", try /move or /stop!");
  }
  else if (msg.text == "move")
  {
    bot.sendMessage("Bot is moving!"); Serial.println("1");
  }
  else if (msg.text == "stop")
  {
    bot.sendMessage("Bot is idle!"); Serial.println("0");
  }
  else if (msg.text == "setnewtarget")
  {
    bot.sendMessage("Send Target Number!"); waiting_for_target = true;
  }
  else if (waiting_for_target && isNumber(msg.text))
  {
    bot.sendMessage("Just wait!"); waiting_for_target = false; Serial.println(msg.text.toInt()+2);
  }
}

void build()
{
  String s;
  BUILD_BEGIN(s);
  add.THEME(GP_DARK);
  add.TITLE("Робот доставщик");
  add.BUTTON("move", "Move");
  add.BUTTON("stop", "Stop");
  add.BREAK(); 
  add.FORM_BEGIN("/target");     // начать форму, передать имя
  add.TEXT("txt", "Target", "1");
  add.BREAK();                  // перенос строки
  add.SUBMIT("Submit");         
  add.FORM_END(); 
  add.BREAK();

  add.AREA_LOG(5);

  BUILD_END();
}

void setup()
{
  Serial.begin(115200);

  blink_tmr.attach(blink);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (millis() > 15000)
      ESP.restart();
  }

  blink_tmr.start();

  portal.attachBuild(build);
  portal.start();
  portal.log.start(30);

  bot.setChatID(CHAT_ID);
  bot.showMenu("move \t stop \t setnewtarget"); 
  bot.attach(newMsg); 
  bot.sendMessage(WiFi.localIP().toString(), CHAT_ID);

  OS.attach(TELEGRAM_TASK, bot_controller, 0);
  OS.attach(PORTAL_TASK, portal_controller, 0);
  OS.attach(SERIAL_MONITOR_TASK, serial_controller, 0);
}

void loop()
{
  OS.tick();
}
