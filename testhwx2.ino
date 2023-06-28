#include <NTPClient.h>
#include <WiFiUdp.h>

#include <IRremoteESP8266.h>//8266的IRremote库，如果是arduinoIDE可以在“管理库”里下载(IRremoteESP8266,不是IRremote哈)
#include <IRrecv.h>//接收库，IRremoteESP8266里的，不用下载
#include <IRutils.h>//这个很重要，一定要include一下(resultToTimingInfo就是这个库的)。也是IRremoteESP8266里的,不用下载
#include <IRsend.h>//发射库，还是IRremoteESP8266里的,不用下载
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MD5HASH.h>

MD5HASH a();

const char *WIFI_SSID = "lin_feng";     // WIFI名称   必须2.4G  可以手机开热点
const char *WIFI_PASS = "20020416"; // WIFI密码  

const char *MQTT_BROKER = "2c2g.akasaki.space"; // MQTT服务器地址
const int MQTT_PORT = 1883;             // MQTT服务端口  服务器1883端口记得打开，不然连不进去
const char *CLIENT_ID = "hwx01";         //客户端ID
const char *USERNAME="lin01";
const char *PASSWORD="123456";
const char *PUBLISH_TOPIC01 = "pub01";    //发布的topic
const char *PUBLISH_TOPIC02 = "pub02";    //发布的topic
const char *SUBSCRIBE_TOPIC = "sub01";  //订阅的topic


//下面是一些设置，大概是一些容错率之类的参数，直接复制即可，在示例->第三方库示例->IRremoteESP8266->IRrecvDumpV2可以看到官方详细的解释，我们直接复制即可
const uint16_t kCaptureBufferSize = 1024;
#if DECODE_AC
const uint8_t kTimeout = 50;
#else   // DECODE_AC
const uint8_t kTimeout = 15;
#endif  // DECODE_AC
const uint16_t kMinUnknownSize = 12;
const uint8_t kTolerancePercentage = kTolerance;
#define LEGACY_TIMING_INFO false


const uint16_t kRecvPin = 14;//接收管的引脚（这里是D5脚(就是GPIO14，可百度引脚定义，这里填GPIO号)）

IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);//传进所有的参数，固定写上即可（没有kCaptureBufferSize, kTimeout可以不填，只填kRecvPin也ok）
decode_results results;//固定写上即可，红外接收数据的results变量

//const uint16_t kIrLed = 4;//发射的引脚(这里是D2脚(就是GPIO4，最好用这个引脚接发射管))
//IRsend irsend(kIrLed);//传进发射引脚参数

void receiveCallback(char *topic, uint8_t *message, unsigned int length); //回调函数声明,用于传入mqtt客户端构造函数作为参数

WiFiClient wifiClient;
//参数: MQTT服务器地址,端口号,回调函数名,承载的连接(WIFI)
PubSubClient mqttClient(MQTT_BROKER, MQTT_PORT, receiveCallback, wifiClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com",60*60*8, 30*60*1000);


//mqttClient.publish(PUBLISH_TOPIC, payload.c_str());"hello emqx"
void setup() {
  Serial.begin(115200);               // 启动串口通讯
  //设置ESP8266工作模式为无线终端模式
  WiFi.mode(WIFI_STA);
  irrecv.enableIRIn();     // 初始化irrecv接收的这个库，必须要有
  timeClient.begin();
  
  while (!Serial)          //等待初始化完成，防止莫名其妙的bug   
  delay(55);
  // 连接WiFi
  connectWifi();
  
  // 设置MQTT服务器和端口号
  mqttClient.setServer(MQTT_BROKER, 1883);
  // 设置MQTT订阅回调函数
  mqttClient.setCallback(receiveCallback);
 
  // 连接MQTT服务器
  connectMQTTserver();
}
 

void loop() {
   timeClient.update();
   
   if (mqttClient.connected()) { // 如果开发板成功连接服务器
    mqttClient.loop();          // 处理信息以及心跳
  } else {                      // 如果开发板未能成功连接服务器
    connectMQTTserver();        // 则尝试连接服务器
  }
  
  if (irrecv.decode(&results)) {          //判断是否接收到了红外的信号
    IRdisplay(results.value);//调用IRdisplay函数
    irrecv.resume(); // 等待接收下一组信号
}
}


// 收到信息后的回调函数
void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message Received [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
  Serial.print("Message Length(Bytes) ");
  Serial.println(length);
 
  if ((char)payload[0] == '1') {     // 如果收到的信息以“1”为开始
    digitalWrite(BUILTIN_LED, LOW);  // 则点亮LED。
  } else {                           
    digitalWrite(BUILTIN_LED, HIGH); // 否则熄灭LED。
  }
}



// 连接MQTT服务器并订阅信息
void connectMQTTserver(){
  // 根据ESP8266的MAC地址生成客户端ID（避免与其它ESP8266的客户端ID重名）
  //String clientId = "esp8266-" + WiFi.macAddress();
 
  // 连接MQTT服务器
  if (mqttClient.connect(CLIENT_ID,USERNAME, PASSWORD)) { 
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address:");
    Serial.println(MQTT_BROKER);
    Serial.println("ClientId: ");
    Serial.println(CLIENT_ID);
    subscribeTopic(); // 订阅指定主题
  } else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    delay(5000);
  }   
}
 

 
// 订阅指定主题
void subscribeTopic(){
 
  // 建立订阅主题1。主题名称以Taichi-Maker-Sub为前缀，后面添加设备的MAC地址。
  // 这么做是为确保不同设备使用同一个MQTT服务器测试消息订阅时，所订阅的主题名称不同
//  String topicString = "Sub-" + WiFi.macAddress();
//  char subTopic[topicString.length() + 1];  
//  strcpy(subTopic, topicString.c_str());
  
  // 通过串口监视器输出是否成功订阅主题1以及订阅的主题1名称
  if(mqttClient.subscribe(SUBSCRIBE_TOPIC)){
    Serial.println("Subscrib Topic:");
    Serial.println(SUBSCRIBE_TOPIC);
  } else {
    Serial.print("Subscribe Fail...");
  }  
    
}

void IRdisplay(unsigned long value){//传过来的results.value 是unsigned long型
   if(value == 0x1FEE01F)
  {
    mqttClient.publish(PUBLISH_TOPIC01,"0");//不用转 传到pub01
    return;
  }
  else if(value == 0x1FE50AF)
  {
    mqttClient.publish(PUBLISH_TOPIC01,"1");//不用转 传到pub01
    return;
  }
  
  String str1 = String(timeClient.getMinutes()) + String(timeClient.getSeconds());
  char* shuju1=const_cast<char*>(str1.c_str());//转换
  Serial.println(shuju1);
  
  unsigned char* hash1=MD5HASH::make_hash(shuju1);//加密时间
  char *md5str1 = MD5HASH::make_digest(hash1, 16);
  //free(hash1);
  Serial.println(md5str1);
  String strjm1=md5str1;  //时间加密1转换
  String str2;
  if(value == 0x1FED827)
  {    
    str2="2";
  }
  else if(value == 0x1FEB04F)
  {
    str2="5";
  }
  else 
  {
    return;
  }
  str2.concat(strjm1);//2dfsdfsdf
  char* shuju2=const_cast<char*>(str2.c_str());//转换
  
  unsigned char* hash2=MD5HASH::make_hash(shuju2);//加密时间+0
  char *md5str2 = MD5HASH::make_digest(hash2, 16);
  //free(hash2);
  Serial.println(md5str2);
  //free(md5str2);
  String strjm2=md5str2;//2+时间的
  strjm2.concat(strjm1);//连接到strjm1  的加密结果
  
  char* jm=const_cast<char*>(strjm2.c_str());//转换
  mqttClient.publish(PUBLISH_TOPIC02, jm);//不用转 传到pub02
}

// ESP8266连接wifi
void connectWifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  //等待WiFi连接,成功连接后输出成功信息
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected!");  
  Serial.println(""); 
}
   
