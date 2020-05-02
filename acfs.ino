// WHOCODE

// ---------- IMPORT LIBRARY ---------- //
#ifdef ESP32 #include <WiFi.h>
#else #include <ESP8266WiFi.h>
#endif

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>

// ---------- DEFINE ---------- //
#define ph_sensor A5 // _PH SENSOR PIN .
#define ONE_WIRE_BUS 32 // _WATER-TEMP SENSOR PIN .
#define servo_pin 13 // _SERVO PIN .
#define period 1000 // _1 SEC .

// ---------- INTERNET ---------- //
const char* wifi_id  = "HOME-WIFI01";
const char* wifi_pwd = "0841396061";
String macAddr;
String storeMacAddr;
char macAddr16Bit[15];
IPAddress server_addr(192, 168, 1, 4); // _HOST IP
WiFiClient client; // WIFI CLIENT INSTANCE
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// ---------- MYSQL ---------- //
char user[] = "whocode_s1";  // _MySQL USER NAME
char password[] = "codetooez_1234"; // MySQL USER PASSWORD
MySQL_Connection conn((Client *)&client); // _MYSQL CONNECTER
MySQL_Cursor cur = MySQL_Cursor(&conn); // _MYSQL CURSOR

// ---------- SQL ---------- //
char query[150]; // USE TO STORE SQL .
String sch_feed_time[10]; // SCHEDULE FEED TIME .
char sql[] = "SELECT * FROM acfs_beta.board_info ORDER BY created_at DESC LIMIT 1";

// ---------- SENSOR VAL ---------- //
int ph_coll[10] ; // _PH VALUE COLLECTION .
float ph_val; // _PH VALUE .
float temp_val; // _TEMP VAL .
unsigned long int avg_val; // _AVERAGE PH VALUE .

// ---------- DEVICES WORKING FREQ/TIME ---------- //
unsigned long latest_time = 0;
unsigned long previous_time = 0;
int sensor_config_freq = 5 * 60;
int previous_sensor_work_freq = 0;
int feeder_config_freq = 15;
int previous_feeder_work_freq = 0;
int prevBoardConfigUpdateTime = 0;
int boardConfigUpdateFreq = 5 * 60;
int sensorWorkingFreq = 5 ;
String feedSchedule;

// ---------- DATETIME ---------- //
String datetime; // DATETIME .
String date_now; // CURRENT DATE .
String time_now; // CURRENT TIME .
char timestamp[50]; // TIMESTMAP .
int hour_now, min_now, sche_hour[3], sche_min[3];

// ---------- VARIABLE ---------- //
char* acfsUserID = "Toey";
bool isAcfsUserRegister = false;
bool isAcfsBoardRegister = false;

char splitT = 'T'; // SPLIT CHARACTER 'T'.
char splitZ = 'Z'; // SPLIT CHARACTER 'Z'.
int splitTindex , splitZindex; // SPLIT INDEX OF CHAR 'T' & 'Z'.
int value = 0; // STORE QUERY DATA.

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo mServo;

void setup()
{
  // _BEGIN SERIAL
  Serial.begin(115200);

  // _GET MAC ADDR
  macAddr = WiFi.macAddress();
  char macAddrBuff[macAddr.length() + 1];
  String strBuff;
  macAddr.toCharArray(macAddrBuff, macAddr.length() + 1);
  macAddr.toCharArray(macAddr16Bit, macAddr.length() + 1);

  // _SETUP PIN
  pinMode(ph_sensor, INPUT);
  pinMode(ONE_WIRE_BUS, INPUT);
  mServo.attach(servo_pin);

  // _START CONNECT WIFI .
  Serial.print("\nCONNECTING WIFI TO : ");
  Serial.println(wifi_id);
  WiFi.begin(wifi_id, wifi_pwd); // BEGIN CONNECTION .
  while (WiFi.status() != WL_CONNECTED) {
    // LOOP CHECK WIFI CONNECTION .
    delay(500);
    Serial.print(".");
  }

  // _GET TIMES FROM SERVER
  timeClient.begin();
  timeClient.setTimeOffset(25200); // SETUP TIME OFFSET (EX. 3600 * X | X => GMT + -> X <-)
  sensors.begin(); // START WATER PROOF SENSOR .

  // _IF WIFI CONNECTED .
  Serial.println("\nWIFI CONNECTED .");
  Serial.print("IP ADDR : ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC ADDR : ");
  Serial.println(macAddr);

  // _START CONNECTING DATABASE .
  Serial.println("Connecting...");
  if (conn.connect(server_addr, 3306, user, password)) {
    delay(1000);
  }
  else
    Serial.println("CONNECTING SERVER FAILED .");
  //  conn.close(); // CLOSE DATABASE CONNECTION .
  mServo.write(90); // _STOP MOTOR
}

void loop()
{
  //   _GET LETEST WORK MILLISEC
  latest_time = millis();

  //  // _WORK EVERY SEC
  //  if (latest_time - previous_time >= period) {
  //    // _COMPARITION WORKING TIME
  //    Serial.print("TIME NOW | PREV TIME -> ");
  //    Serial.print(latest_time);
  //    Serial.print(" | ");
  //    Serial.println(previous_time);
  //
  //    // _INITIAL MYSQL CURSOR .
  //    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  //    row_values *row = NULL;
  //    delay(100);
  //
  //    // _EXECUTE THE SQL
  //    cur_mem->execute(sql);
  //    column_names *cols = cur_mem->get_columns(); // Fetch the columns (required) but we don't use them.
  //    for (int f = 0; f < cols->num_fields; f++) {
  //      Serial.print(cols->fields[f]->name);
  //      if (f < cols->num_fields - 1) {
  //        Serial.print(" | ");
  //      }
  //    } Serial.println();
  //
  //    do {
  //      row = cur_mem->get_next_row();
  //      if (row != NULL) {
  //        for (int f = 0; f < cols->num_fields; f++) {
  //          sch_feed_time[f] = row->values[f];
  //          Serial.print(row -> values[f]);
  //          if (f < cols->num_fields - 1) {
  //            Serial.print(" | ");
  //          }
  //        } Serial.println();
  //      }
  //    } while (row != NULL);
  //
  //    sche_hour[0] = sch_feed_time[0].substring(0, 2).toInt();
  //    sche_min[0] = sch_feed_time[0].substring(3, 5).toInt();
  //    sche_hour[1] = sch_feed_time[1].substring(0, 2).toInt();
  //    sche_min[1] = sch_feed_time[1].substring(3, 5).toInt();
  //    sche_hour[2] = sch_feed_time[2].substring(0, 2).toInt();
  //    sche_min[2] = sch_feed_time[2].substring(3, 5).toInt();
  //
  //    Serial.print("SCHEDULE : ");
  //    Serial.print(sch_feed_time[0]);
  //    Serial.print("| ");
  //    Serial.print(sch_feed_time[1]);
  //    Serial.print("| ");
  //    Serial.println(sch_feed_time[2]);
  //    Serial.print("Feed Time : ");
  //    Serial.println(sch_feed_time[3]);
  //    Serial.print("Sensor Time : ");
  //    Serial.println(sch_feed_time[4]);
  //
  //    sensor_config_freq = sch_feed_time[4].toInt() * 60;
  //
  //    datetimeManager();
  //    if (latest_time - previous_sensor_work_freq >= sensor_config_freq * period)
  //    {
  //      calcualateTempValue();
  //      calculateAvgPhValue();
  //      previous_sensor_work_freq = latest_time;
  //      insertSQL(temp_val, ph_val);
  //      cur_mem->execute(query); // _INSERT SENSOR LOG TO DATABASE .
  //      avg_val = 0;
  //    }
  //    delete cur_mem;
  //
  //    handleSchedule();
  //    Serial.println();
  //  }

  if (latest_time - previous_time >= period) {
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    row_values *row = NULL;
    delay(100);

    datetimeManager(true);

    isUserRegister(cur_mem, row);

    if (!isAcfsBoardRegister) {
      isBoardRegister(cur_mem, row);
    }

    if (latest_time - prevBoardConfigUpdateTime >= boardConfigUpdateFreq * period) {
      getBoardConfig(cur_mem, row);
    }

    if (isAcfsUserRegister == true && isAcfsBoardRegister == true) {
      calcualateTempValue();
      calculateAvgPhValue();
      recordSensorValue(cur_mem, row, temp_val, ph_val);
    }

    previous_time = millis();
  }
}

// ---------- DATETIME FORMAT ---------- //
void datetimeManager(bool isPrint) {
  // FORCE UDPATE TIME IF DIDN'T UPDATE .
  if (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  datetime = timeClient.getFormattedDate(); // GET DATETIME .
  splitTindex = datetime.indexOf(splitT); // GET INDEX OF CHAR 'T' .
  splitZindex = datetime.indexOf(splitZ); // GET INDEX OF CHAR 'Z' .
  date_now = datetime.substring(0, splitTindex); // SPLIT DATETIME TO GET DATE .
  time_now = datetime.substring(splitTindex + 1, splitZindex); // SPLIT DATETIME TO GET TIME .
  hour_now = timeClient.getHours();
  min_now = timeClient.getMinutes();

  if (isPrint) {
    sprintf(timestamp, "%s %s", date_now, time_now); // FORMATTING timestamp .
    Serial.print("\nTIMESTAMP : ");
    Serial.println(timestamp);
    Serial.print("HOUR | MIN : ");
    Serial.print(hour_now);
    Serial.print(" | ");
    Serial.println(min_now);

    Serial.print("SCHEDULE HOUR | MIN : ");
    Serial.print(sche_hour[0]);
    Serial.print(" | ");
    Serial.println(sche_min[0]);
  }
}

// ---------- INSERT SQL FUNCTION ---------- //
void insertSQL(float temp_val, float ph_val) {
  sprintf(query, "INSERT INTO acfs.sensor_log(temp_val, ph_val, created_at, updated_at) VALUES (%.2f, %.2f, '%s', '%s');", temp_val, ph_val, timestamp, timestamp);
}

void recordSensorValue(MySQL_Cursor *cur_mem, row_values *row, float tempValue, float phValue) {

  char* recordSensorValueSql = "INSERT INTO acfs_beta.board_log (temp_value, ph_value, board_id, u_id, created_at) VALUES (%.2f, %.2f, '%s', '%s', '%s');";
  sprintf(query, recordSensorValueSql, tempValue, phValue, macAddr16Bit, acfsUserID, timestamp);

  // Execute sql
  cur_mem->execute(query);
  delay(100);
}

// ---------- SELECT SQL FUNCTION ---------- //
void getBoardID(String board_id) {
  char sql[] = "SELECT * FROM acfs_beta.board_info WHERE board_id = %s ORDER BY created_at DESC LIMIT 1";
  sprintf(query, sql, board_id);
}

void getBoardConfig(MySQL_Cursor *cur_mem, row_values *row) {

  char getBoardConfigSql[] = "SELECT * FROM acfs_beta.board_config WHERE board_id = '%s';";
  sprintf(query, getBoardConfigSql, macAddr16Bit);

  // Execute sql
  cur_mem->execute(query);
  column_names *cols = cur_mem->get_columns();
  for (int f = 0; f < cols->num_fields; f++) {
    Serial.print(cols->fields[f]->name);
    if (f < cols->num_fields - 1) {
      Serial.print(" | ");
    }
  } Serial.println();

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      for (int f = 0; f < cols->num_fields; f++) {
        switch (f) {
          case 0 : {
              sensorWorkingFreq = (int)(row -> values[f]);
              break;
            }
          case 1 : {
              feedSchedule = row -> values[f];
              break;
            }
        }
        Serial.print(row -> values[f]);
        if (f < cols->num_fields - 1) {
          Serial.print(" | ");
        }
      } Serial.println();
    }
  } while (row != NULL);

  prevBoardConfigUpdateTime = latest_time; // _Update board config time to latest
  delay(100);
}

// _Check exists board : register board when don't have
void isBoardRegister(MySQL_Cursor *cur_mem, row_values *row) {

  bool boardRegisterState = false;
  char getAcfsBoardSql[] = "SELECT * FROM acfs_beta.board_info WHERE board_id = '%s';";

  Serial.print("Board MAC Address : ");
  Serial.print(macAddr);
  Serial.println();

  sprintf(query, getAcfsBoardSql, macAddr16Bit);

  cur_mem->execute(query); // _Execute the sql
  column_names *cols = cur_mem->get_columns();
  for (int f = 0; f < cols->num_fields; f++) {
    Serial.print(cols->fields[f]->name);
    if (f < cols->num_fields - 1) {
      Serial.print(" | ");
    }
  } Serial.println();

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      for (int f = 0; f < cols->num_fields; f++) {
        if (row->values[f] != "") {
          isAcfsBoardRegister = true;
        }
        Serial.print(row -> values[f]);
        if (f < cols->num_fields - 1) {
          Serial.print(" | ");
        }
      } Serial.println();
    }
  } while (row != NULL);

  if (isAcfsBoardRegister) {
    Serial.println("Board already registered\n");
  } else {
    Serial.print("Register board number : ");
    Serial.println(macAddr16Bit);
    char* registerAcfsBoardSql = "INSERT INTO acfs_beta.board_info VALUES ('%s', '%s', '%s');";
    sprintf(query, registerAcfsBoardSql, macAddr16Bit, acfsUserID, timestamp);
    cur_mem->execute(query);
  }
  delay(100);
}

// _Check exist user : don't start main function if don't found user account
void isUserRegister(MySQL_Cursor *cur_mem, row_values *row) {

  bool userRegisterState = false;
  char getAcfsUserSql[] = "SELECT * FROM acfs_beta.user_info WHERE u_id = '%s';";
  sprintf(query, getAcfsUserSql, acfsUserID);

  // Execute sql
  cur_mem->execute(query);
  column_names *cols = cur_mem->get_columns();
  for (int f = 0; f < cols->num_fields; f++) {
    Serial.print(cols->fields[f]->name);
    if (f < cols->num_fields - 1) {
      Serial.print(" | ");
    }
  } Serial.println();

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      for (int f = 0; f < cols->num_fields; f++) {
        if (row->values[f] != "") {
          isAcfsUserRegister = true;
        }
        Serial.print(row -> values[f]);
        if (f < cols->num_fields - 1) {
          Serial.print(" | ");
        }
      } Serial.println();
    }
  } while (row != NULL);
  if (isAcfsUserRegister == true) {
    Serial.println("User already registered\n");
  } else
    Serial.println("User didn't register\n");
  delay(100);
}

// ---------- HANDLE SENSOR ---------- //

// _CALCULATE AVERAGE PH VALUE
void calculateAvgPhValue() {

  // _Reset to default
  avg_val = 0;

  // _STORE PH VAL TO PH VAL COLLECTION .
  for (int idx = 0 ; idx < 10; idx++) {
    ph_coll[idx] = map(analogRead(ph_sensor), 0, 4095, 0, 1024);
    delay(30);
  }
  for (int idx = 2 ; idx < 8; idx++) {
    avg_val += ph_coll[idx];
  }

  ph_val = (float)avg_val * 5.0 / 1024 / 6;
  ph_val = 3.5 * ph_val + 0.00;

  Serial.print("AVG OF PH VALUE : ");
  Serial.println(ph_val);
}

// Calculate water temperature value
void calcualateTempValue() {
  sensors.requestTemperatures(); // REQUEST TO USE TEMP SENSOR .
  temp_val = sensors.getTempCByIndex(0); // GET TEMP VALUE .
  Serial.print("TEMPERATURE ->  ");
  Serial.println(sensors.getTempCByIndex(0));
}

// >> HANDLE SERVO
void handleServoMotor() {
  while (latest_time - previous_feeder_work_freq >= feeder_config_freq * period) {

    Serial.print("TIME NOW | PREV TIME | FEED TIME CONFIG -> ");
    Serial.print(latest_time);
    Serial.print(" | ");
    Serial.print(previous_feeder_work_freq);
    Serial.print(" | ");
    Serial.println(feeder_config_freq);

    Serial.println("STATE : TRUE");
    mServo.write(0);
    Serial.println("STATE LED : ON");
    Serial.println("WORKING ...");
    previous_feeder_work_freq += 250;
    delay(250);
  }
  previous_feeder_work_freq = latest_time;
  Serial.println("STATE LED : OFF");
  Serial.println("STATE : FALSE");
  mServo.write(90);
  previous_time = millis();
}

void handleSchedule() {
  if (sche_hour[0] - hour_now == 0 && sche_min[0] - min_now == 0) {
    handleServoMotor();
  }
  if (sche_hour[1] - hour_now == 0 && sche_min[1] - min_now == 0) {
    handleServoMotor();
  }
  if (sche_hour[2] - hour_now == 0 && sche_min[2] - min_now == 0) {
    handleServoMotor();
  }
}
