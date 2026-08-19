#include <SoftwareSerial.h>
#include <ezButton.h>

//thanks to diyables and cefloide
//note: manual updates for fldr_track_nums

// ***** commands for the serial mp3 player module *****
#define CMD_PLAY_NEXT 0x01 //next track
#define CMD_PLAY_PREV 0x02 //previous track
#define CMD_PLAY_W_INDEX 0x03 //play specific n track
#define CMD_SET_VOLUME 0x06 // set num for volume
#define CMD_SEL_DEV 0x09 //select device
#define CMD_PLAY_W_VOL 0x22 // play n track at m volume
#define CMD_PLAY 0x0D // play current track
#define CMD_PAUSE 0x0E // pause current track
#define CMD_SINGLE_CYCLE 0x19 //loop current track
#define CMD_PLAY_FOLDER_FILE 0x0F

#define DEV_TF 0x02 //device / mircosd
#define SINGLE_CYCLE_ON 0x00 //yes loop
#define SINGLE_CYCLE_OFF 0x01 // no loop
#define CMD_TRACK_FINISHED 0x3D  //when a track finishes

#define CMD_QUERY_STATUS 0x42
#define CMD_QUERY_VOLUME 0x43
#define CMD_QUERY_CURRENT_TRACK 0x4C
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS 0x48
#define CMD_QUERY_FLDR_COUNT 0x4f

// ***** end of commands *****


#define ARDUINO_RX 7  // TX of the Serial MP3 Player module
#define ARDUINO_TX 6  // RX of the Serial MP3 Player module

//list of functions for complier
void mp3_command(int8_t command, int16_t dat, int8_t feedback = 0x00);
int16_t  mp3_query(int8_t queryCommand, int16_t queryDat = 0x0000);
bool mp3_read_response(int8_t* command, int16_t* data);
void mp3_debug_dump();
void play_folder(uint8_t folder, uint8_t file);
void play_next_track();
void play_prev_track();
void play_next_folder();
// end of func

SoftwareSerial mp3(ARDUINO_RX, ARDUINO_TX);

//vars
ezButton b_next(2);
ezButton b_prev(3);
ezButton b_pnp(4);
ezButton b_fldr(5);

uint8_t cur_fldr = 1;
uint8_t cur_file = 1;
const uint8_t fldr_track_nums[] = {0, 19, 10};
const uint8_t num_fldrs = (sizeof(fldr_track_nums) / sizeof(fldr_track_nums[0])) - 1;
uint8_t total_files_in_fldr = fldr_track_nums[cur_fldr];
int lastVolume = -1;
String output = "";
//***


void setup() {
  Serial.begin(9600);
  mp3.begin(9600);
  delay(500);  // wait chip initialization is complete

  mp3_command(CMD_SEL_DEV, DEV_TF);  // select the TF card
  delay(200);

  mp3_command(CMD_SET_VOLUME, 15);   // Change volume to 15
  delay(100);

  b_next.setDebounceTime(50);
  b_prev.setDebounceTime(50);
  b_pnp.setDebounceTime(50);
  b_fldr.setDebounceTime(50);

  //DOESNT WORK >:[ *****
  // // ask the module how many files are in folder 1 BEFORE we start playing,
  // // so we know when to wrap back to file 1 later
  // int16_t fldr_tracks = mp3_query(CMD_QUERY_FLDR_TRACKS, cur_fldr);
  // if (fldr_tracks > 0) {
  //   total_files_in_fldr = (uint8_t)fldr_tracks;
  // }
  //*********

  Serial.print("Tracks in folder ");
  Serial.print(cur_fldr);
  Serial.print(": ");
  Serial.println(total_files_in_fldr);

  play_folder(cur_fldr, cur_file);

  delay(500);
  mp3_query(CMD_QUERY_STATUS);
  mp3_query(CMD_QUERY_VOLUME);
  mp3_query(CMD_QUERY_CURRENT_TRACK);

  Serial.println("*************************");
}

void loop() {
  //volume control
  int analogVal = analogRead(A0);
  int volume = map(analogVal, 0, 1023, 0, 30);
  if (volume != lastVolume) {
    mp3_command(CMD_SET_VOLUME, volume);
    lastVolume = volume;
  }

  //button logic
  b_next.loop();
  b_prev.loop();
  b_pnp.loop();
  b_fldr.loop();

  if (b_next.isPressed()) {
    Serial.println("playing next track");
    play_next_track();
  }

  if (b_prev.isPressed()) {
    Serial.println("playing previous track");
    play_prev_track();
  }

  if (b_pnp.isPressed()) {
    Serial.println("checking stats");
    mp3_query(CMD_QUERY_STATUS);
    if(output == "Playing"){
      mp3_command(CMD_PAUSE, 0x0000);
    } else if(output == "Paused"){
      mp3_command(CMD_PLAY, 0x0000);
    }
  }

  if (b_fldr.isPressed()) {
    Serial.println("switching folder");
    play_next_folder();
  }

  //listen for if track finished
  int8_t respCommand;
  int16_t respData;
  if (mp3_read_response(&respCommand, &respData)) {
    if (respCommand == CMD_TRACK_FINISHED) {
      Serial.print("finished track: ");
      Serial.println(respData);
      play_next_track();
    }
  }
}

//***** playback stuff *****
void play_folder(uint8_t folder, uint8_t file) {
  cur_fldr = folder;
  cur_file = file;
  total_files_in_fldr = fldr_track_nums[folder];

  uint16_t dat = ((uint16_t)folder << 8) | file;
  mp3_command(CMD_PLAY_FOLDER_FILE, dat, 0x01);

  Serial.print("Now playing folder ");
  Serial.print(cur_fldr);
  Serial.print(", file ");
  Serial.println(cur_file);
}

void play_next_track() {
  cur_file++;
  if (cur_file > total_files_in_fldr) {
    cur_file = 1;  // wrap back to the first track in the folder
  }
  play_folder(cur_fldr, cur_file);
}

void play_prev_track() {
  if (cur_file <= 1) {
    cur_file = total_files_in_fldr;  // wrap to the last track in the folder
  } else {
    cur_file--;
  }
  play_folder(cur_fldr, cur_file);
}

void play_next_folder() {
  cur_fldr++;
  if (cur_fldr > num_fldrs) {
    cur_fldr = 1;  // wrap back to folder 1
  }
  play_folder(cur_fldr, 1);  // always start at file 1 of new folder
}


// ***** command n response functions *****
//formats the commands
void mp3_command(int8_t command, int16_t dat, int8_t feedback) {
  int8_t frame[8] = { 0 };
  frame[0] = 0x7e;                // starting byte
  frame[1] = 0xff;                // version
  frame[2] = 0x06;                // the number of bytes of the command without starting byte and ending byte
  frame[3] = command;             //
  frame[4] = feedback;                // 0x00 = no feedback, 0x01 = feedback
  frame[5] = (int8_t)(dat >> 8);  // data high byte
  frame[6] = (int8_t)(dat);       // data low byte
  frame[7] = 0xef;                // ending byte
  for (uint8_t i = 0; i < 8; i++) {
    mp3.write(frame[i]);
  }
}


// Reads one 10-byte response frame from the MP3 module, if available
bool mp3_read_response(int8_t* command, int16_t* data) {
  if (mp3.available() < 10) return false;

  int8_t frame[10];
  for (uint8_t i = 0; i < 10; i++) {
    frame[i] = mp3.read();
  }

  if (frame[0] != 0x7E || frame[9] != (int8_t)0xEF) return false;

  *command = frame[3];
  *data = ((int16_t)frame[5] << 8) | (frame[6] & 0xFF);
  return true;
}

//writes the query
int16_t  mp3_query(int8_t queryCommand, int16_t queryDat) {
  while (mp3.available()) mp3.read();

  mp3_command(queryCommand, queryDat, 0x01);
  delay(100);  // give the module time to respond

  int8_t respCommand;
  int16_t respData;
  if (mp3_read_response(&respCommand, &respData)) {
    switch (respCommand) {
      case CMD_QUERY_STATUS: {
      uint8_t device = (respData >> 8) & 0xFF;
      uint8_t state = respData & 0xFF;
      Serial.print("Status: ");
      if (state == 0) output = "Stopped";
      else if (state == 1) output = "Playing";
      else if (state == 2) output = "Paused";
      else output = String(state); 
      Serial.println(output);
      break;
    }
      case CMD_QUERY_VOLUME:
        Serial.print("volume: ");
        Serial.println(respData);
        break;
      case CMD_QUERY_CURRENT_TRACK:
        Serial.print("current track: ");
        Serial.println(respData);
        break;
      case CMD_QUERY_FLDR_TRACKS:
        Serial.print("tracks in folder: ");
        Serial.println(respData);
        break;
      case CMD_QUERY_TOT_TRACKS:
        Serial.print("number of tracks in folder: ");
        Serial.println(respData);
        break;
      case CMD_QUERY_FLDR_COUNT:
        Serial.print("number of folders: ");
        Serial.println(respData);
        break;
      default:
        Serial.print("Unknown response, command=0x");
        Serial.println(respCommand, HEX);
    }
    return respData;
  } else {
    Serial.println("No response received.");
    return -1;
  }
}

//check
void mp3_debug_dump() {
  Serial.print("Bytes available: ");
  Serial.println(mp3.available());
  while (mp3.available()) {
    Serial.print(mp3.read(), HEX);
    Serial.print(" ");
  }
  Serial.println();
}