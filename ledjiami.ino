
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MD5HASH.h>

#define LED D0 // GPIO引脚别名 仅使用于NodeMCU开发板

//默认使用'$'符号作为调试信息输出的标识
//默认使用'#'符号作为控制信息的标识开头
MD5HASH a();

const char *WIFI_SSID = "lin_feng";     // WIFI名称   必须2.4G  可以手机开热点
const char *WIFI_PASS = "20020416"; // WIFI密码  

const char *MQTT_BROKER = "2c2g.akasaki.space"; // MQTT服务器地址
const int MQTT_PORT = 1883;             // MQTT服务端口  服务器1883端口记得打开，不然连不进去
const char *CLIENT_ID = "led02";         //客户端ID
const char *USERNAME="lin01";
const char *PASSWORD="123456";
const char *PUBLISH_TOPIC = "sub01";    //发布的topic
const char *SUBSCRIBE_TOPIC = "pub02";  //订阅的topic


void callback(char *topic, byte *message, unsigned int length); //回调函数声明,用于传入mqtt客户端构造函数作为参数

WiFiClient wifiClient;
//参数: MQTT服务器地址,端口号,回调函数名,承载的连接(WIFI)
PubSubClient mqttClient(MQTT_BROKER, MQTT_PORT, callback, wifiClient);

//回调函数
void callback(char *topic, byte *message, unsigned int length) {
    Serial.println("$Received message:");
    //String sTopic = topic; //将topic转换为String 可以加以处理
    
    String sMessage = (char *)message;        //将消息转换为String
    sMessage = sMessage.substring(0, length); //取合法长度 避免提取到旧消息
    Serial.println(sMessage); //输出消息 用于调试 可以注释掉

    String str1="";
    String str2="";
    //64位数据 2+dasd +时间加密 取后32位加上2/5与前32位比较
    for(int i=0;i<64;i++)
    {
      if(i<32)
      {
        str1+=sMessage[i];//2+dasd加密结果
      }else
      {
        str2+=sMessage[i];//时间加密结果
      }
    }
    Serial.println(str1);
    Serial.println(str2);
    String str3="2";
    str3.concat(str2);//拼接进行加密
    
    char* str_ver=const_cast<char*>(str3.c_str());//转换
    Serial.println(str_ver);
    
    unsigned char* hash1=MD5HASH::make_hash(str_ver);//加密时间
    char *md5str1 = MD5HASH::make_digest(hash1, 16);

    Serial.println(md5str1);//加密结果
    
    //比较str1和md5str1是否相同
    bool isMatch = str1.equals(md5str1);
    if (isMatch) {
      // 字符串匹配
      digitalWrite(LED, 0);  //输出低电平 2关灯？
    } else {
      // 字符串不匹配
      digitalWrite(LED, 1); //初始化输出高电平 5开灯
    }
    
//    //按字符判断mqtt消息中命令的作用 可以自行定义
//    if (sMessage.charAt(0) == '2')
//    {         //第一位#
//             digitalWrite(LED, 0);  //输出低电平
//             //digitalWrite(LED_BUILTIN, LOW);  
//    }
//    else if (sMessage.charAt(0) == '5') 
//            {
//                //处理#D0
//                digitalWrite(LED, 1); //初始化输出高电平
//                //digitalWrite(LED_BUILTIN, HIGH);  
//            }
    }

// MQTT发布函数 需要使用可以解除注释 传入一个字符串 发送到默认配置的发布topic
void MQTT_doPublishOnDefaultTopic(String payload) {
    //参数: 发布的话题,发布的内容
    mqttClient.publish(PUBLISH_TOPIC, payload.c_str()); // String的c_str()方法将一个字符串转换为字符数组
}

//所有Arduino程序都具备的配置函数 系统初始化后只执行一次
void setup() {
    pinMode(LED, OUTPUT); //初始化引脚
    //pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED, 0); //初始化输出高电平
    //digitalWrite(LED_BUILTIN, LOW);  

    //启用串口
    Serial.begin(115200);
    // Serial.println("$Hello world!"); //串口输出消息

    //连接WIFI
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    //连接未成功时 循环等待
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("$Still waiting...");
    }

    Serial.println("$Wi-Fi connected");
    Serial.print("$IP address: ");
    Serial.println(WiFi.localIP());

    //进行MQTT连接
    //当MQTT服务器连接不成功时
    while (!mqttClient.connected()) {
        Serial.println("$Waiting for MQTT");
        //执行(重试)MQTT连接
        if (mqttClient.connect(CLIENT_ID,USERNAME, PASSWORD)) {
            //连接成功后 订阅指定的topic
            if (mqttClient.subscribe(SUBSCRIBE_TOPIC, 0)) { // QoS: 0/1/2 [|0: 只发送一次(常用)|1: 至少发送一次|2: 一定收到一次]
                Serial.println("$Subscribed."); //订阅成功 发送调试消息
            } else {
                Serial.println("$Subscribe failed."); //订阅不成功 发送调试消息
                //如有需要 可以加死循环阻止程序继续运行
            }
        }
        delay(1000);
    } 

    
}

//所有Arduino程序都具备的循环函数 setup()函数执行后循环执行
void loop() {
    delay(500);
    mqttClient.loop(); //保证MQTT客户端持续运行和接收消息
}
