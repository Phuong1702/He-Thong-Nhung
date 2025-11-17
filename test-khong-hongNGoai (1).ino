#include <Arduino.h>


// 1. KHAI BÁO CHÂN GPIO
const int CHAN_ENA = 5;
const int CHAN_IN1 = 8;
const int CHAN_IN2 = 7;
const int CHAN_ENB = 6;
const int CHAN_IN3 = 9;
const int CHAN_IN4 = 10;

const int CHAN_ENC = 11;
const int CHAN_INC1 = 12;
const int CHAN_INC2 = 13;

const int CHAN_TRIG = A0;
const int CHAN_ECHO = A1;

// Cảm biến chống rơi 
const int IR_DUOI  = A2;  // dưới
const int IR_TREN  = A3;  // trên
const int IR_TRAI  = A4;  // trái
const int IR_PHAI  = A5;  // phải


// 2. HẰNG SỐ
#define TOC_DO_MAX 200
#define TOC_DO_QUAT 210
#define NGUONG_VAT_CAN 20 // cm
#define DELAY_LUI 2000
#define DELAY_RE 600
#define DELAY_TIEN 800
#define DELAY_CHONG_ROI 500  // 0.5 giây


// 3. BIẾN TRẠNG THÁI
bool daReTrai = false;
bool daRePhai = false;

// 4. HÀM ĐIỀU KHIỂN
void dieuKhienMotBanhXe(int huong, int tocDo, int chanDau1, int chanDau2, int chanPWM) {
  tocDo = constrain(tocDo, 0, 255);
  analogWrite(chanPWM, (huong != 0) ? tocDo : 0);

  if (huong > 0) {
    digitalWrite(chanDau1, HIGH);
    digitalWrite(chanDau2, LOW);
  } else if (huong < 0) {
    digitalWrite(chanDau1, LOW);
    digitalWrite(chanDau2, HIGH);
  } else {
    digitalWrite(chanDau1, LOW);
    digitalWrite(chanDau2, LOW);
  }
}

void datTocDoCuaCacBanhXe(int huongTrai, int huongPhai, int tocDo) {
  dieuKhienMotBanhXe(huongTrai, tocDo, CHAN_IN1, CHAN_IN2, CHAN_ENA);
  dieuKhienMotBanhXe(huongPhai, tocDo, CHAN_IN3, CHAN_IN4, CHAN_ENB);
}

void dungLai() { datTocDoCuaCacBanhXe(0, 0, 0); }
void diThang() { datTocDoCuaCacBanhXe(1, 1, TOC_DO_MAX); }
void diLui()   { datTocDoCuaCacBanhXe(-1, -1, TOC_DO_MAX); }
void reTrai()  { datTocDoCuaCacBanhXe(-1, 1, TOC_DO_MAX); }
void rePhai()  { datTocDoCuaCacBanhXe(1, -1, TOC_DO_MAX); }

void datTocDoQuat(int tocDo) {
  digitalWrite(CHAN_INC1, HIGH);
  digitalWrite(CHAN_INC2, LOW);
  tocDo = constrain(tocDo, 0, 255);
  analogWrite(CHAN_ENC, tocDo);
}

// 5. CẢM BIẾN SIÊU ÂM
long docKhoangCachCm() {
  digitalWrite(CHAN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(CHAN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(CHAN_TRIG, LOW);

  long thoiGian = pulseIn(CHAN_ECHO, HIGH, 30000);
  if (thoiGian == 0) return 999;
  return thoiGian * 0.034 / 2;
}

// 6. KIỂM TRA CHỐNG RƠI 
bool kiemTraChongRoi() {
  int tren  = digitalRead(IR_TREN);
  int duoi  = digitalRead(IR_DUOI);
  int trai  = digitalRead(IR_TRAI);
  int phai  = digitalRead(IR_PHAI);

  bool roiTren  = (tren == HIGH);
  bool roiDuoi  = (duoi == HIGH);
  bool roiTrai  = (trai == HIGH);
  bool roiPhai  = (phai == HIGH);

  // Nếu không có cảm biến nào báo rơi
  if (!roiTren && !roiDuoi && !roiTrai && !roiPhai) return false;

  Serial.print("⚠️ PHAT HIEN NGUY CO ROI - ");
  Serial.print("Tren="); Serial.print(roiTren);
  Serial.print(" Duoi="); Serial.print(roiDuoi);
  Serial.print(" Trai="); Serial.print(roiTrai);
  Serial.print(" Phai="); Serial.println(roiPhai);

  // DỪNG NGAY 0.1 giây trước khi phản ứng
  dungLai();
  delay(100);

  if (roiTren) {
    // IR trên: lùi 0.2 giây → quay 180°
    diLui();
    delay(200);
    dungLai();
    reTrai();  // Quay 180° bằng cách reTrai sau khi lùi
    delay(600); // thời gian quay đủ 180° (có thể điều chỉnh)
    dungLai();
  }
  else if (roiDuoi) {
    // IR dưới: tiến 0.2 giây
    diThang();
    delay(200);
    dungLai();
  }
  else if (roiTrai) {
    // IR trái: rẽ phải
    rePhai();
    delay(200);
    dungLai();
  }
  else if (roiPhai) {
    // IR phải: rẽ trái
    reTrai();
    delay(200);
    dungLai();
  }

  return true;
}

// 7. SETUP & LOOP
void setup() {
  Serial.begin(9600);

  // Cấu hình chân động cơ
  pinMode(CHAN_IN1, OUTPUT);
  pinMode(CHAN_IN2, OUTPUT);
  pinMode(CHAN_IN3, OUTPUT);
  pinMode(CHAN_IN4, OUTPUT);
  pinMode(CHAN_ENA, OUTPUT);
  pinMode(CHAN_ENB, OUTPUT);
  pinMode(CHAN_ENC, OUTPUT);
  pinMode(CHAN_INC1, OUTPUT);
  pinMode(CHAN_INC2, OUTPUT);

  // Cấu hình cảm biến
  pinMode(CHAN_TRIG, OUTPUT);
  pinMode(CHAN_ECHO, INPUT);

  // Cảm biến chống rơi
  pinMode(IR_TREN, INPUT);
  pinMode(IR_DUOI, INPUT);
  pinMode(IR_TRAI, INPUT);
  pinMode(IR_PHAI, INPUT);

  // Bật quạt hút bụi liên tục
  datTocDoQuat(TOC_DO_QUAT);

  Serial.println("=== ROBOT UNO - TRÁNH VẬT CẢN + CHỐNG RƠI 4 HƯỚNG (LOGIC MỚI) ===");
}

void loop() {
  // 1️⃣ Kiểm tra chống rơi
  if (kiemTraChongRoi()) {
    delay(200);  // Tránh vật cản trong lúc xử lý rơi
    return;
  }

  // 2️⃣ Kiểm tra tránh vật cản bằng siêu âm
  long kc = docKhoangCachCm();
  Serial.print("Khoang cach truoc: ");
  Serial.print(kc);
  Serial.println(" cm");

  if (kc > NGUONG_VAT_CAN) {
    diThang();
  } else {
    dungLai();

    // Lùi 0.2 giây để tránh vật cản
    diLui();
    delay(200);
    dungLai();
    delay(100);

    // Quay theo trạng thái để tránh kẹt
    if (!daReTrai && !daRePhai) {
      reTrai();
      daReTrai = true;
      Serial.println("⬅️ Re trai");
    } 
    else if (daReTrai && !daRePhai) {
      rePhai();
      daRePhai = true;
      Serial.println("➡️ Re phai");
    } 
    else if (daReTrai && daRePhai) {
      reTrai();
      daReTrai = false;
      daRePhai = false;
      Serial.println("🔄 Quay 180 do");
    }

    delay(DELAY_RE);
    diThang();
    delay(DELAY_TIEN);
  }

  delay(200);
}
