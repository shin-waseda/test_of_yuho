#include "main.h"

// TIM6の1カウント ≒ 11.9us (84MHz / 1000)
// LED点灯後の応答待ち（2カウント ≒ 約24us）
#define IR_WAIT_COUNT 2

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim6;

extern uint16_t ad_r, ad_fr, ad_fl, ad_l;

// 指定したADCチャンネルを1回だけ手動取得する関数
static uint16_t get_adc_single(uint32_t channel) {
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = channel;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;

  // チャンネルを設定して変換開始
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
  HAL_ADC_Start(&hadc1);

  // 変換完了待ち（正常なら数usで完了します）
  if (HAL_ADC_PollForConversion(&hadc1, 2) == HAL_OK) {
    uint16_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
  }
  
  HAL_ADC_Stop(&hadc1);
  return 0;
}

// TIM6のカウンタを利用したウェイト関数
static void tim6_delay(uint16_t count) {
  uint16_t start = __HAL_TIM_GET_COUNTER(&htim6);
  while (1) {
    uint16_t now = __HAL_TIM_GET_COUNTER(&htim6);
    uint16_t elapsed = (now >= start) ? (now - start) : (1000 + now - start);
    if (elapsed >= count)
      break;
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  // htim6.Instance ではなく TIM6 と直接比較する
  if (htim->Instance == TIM6) {

    // --- 1. 全LED消灯状態で OFF（環境光）を取得 ---
    HAL_GPIO_WritePin(IR_R_GPIO_Port,  IR_R_Pin,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IR_FR_GPIO_Port, IR_FR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IR_FL_GPIO_Port, IR_FL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IR_L_GPIO_Port,  IR_L_Pin,  GPIO_PIN_RESET);
    tim6_delay(2); // 消灯の応答待ち

    uint16_t r_off  = get_adc_single(ADC_CHANNEL_1);  // Sensor_R (PA1)
    uint16_t fr_off = get_adc_single(ADC_CHANNEL_0);  // Sensor_FR (PA0)
    uint16_t fl_off = get_adc_single(ADC_CHANNEL_2);  // Sensor_FL (PA2)
    uint16_t l_off  = get_adc_single(ADC_CHANNEL_3);  // Sensor_L (PA3)
    uint16_t vbat   = get_adc_single(ADC_CHANNEL_10); // Vol_Check (PC0)

    // --- 2. 各LEDを順番に点灯させて ON 値を取得 ---
    // 右（R）
    HAL_GPIO_WritePin(IR_R_GPIO_Port, IR_R_Pin, GPIO_PIN_SET);
    tim6_delay(IR_WAIT_COUNT);
    uint16_t r_on = get_adc_single(ADC_CHANNEL_1);
    HAL_GPIO_WritePin(IR_R_GPIO_Port, IR_R_Pin, GPIO_PIN_RESET);

    // 前右（FR）
    HAL_GPIO_WritePin(IR_FR_GPIO_Port, IR_FR_Pin, GPIO_PIN_SET);
    tim6_delay(IR_WAIT_COUNT);
    uint16_t fr_on = get_adc_single(ADC_CHANNEL_0);
    HAL_GPIO_WritePin(IR_FR_GPIO_Port, IR_FR_Pin, GPIO_PIN_RESET);

    // 前左（FL）
    HAL_GPIO_WritePin(IR_FL_GPIO_Port, IR_FL_Pin, GPIO_PIN_SET);
    tim6_delay(IR_WAIT_COUNT);
    uint16_t fl_on = get_adc_single(ADC_CHANNEL_2);
    HAL_GPIO_WritePin(IR_FL_GPIO_Port, IR_FL_Pin, GPIO_PIN_RESET);

    // 左（L）
    HAL_GPIO_WritePin(IR_L_GPIO_Port, IR_L_Pin, GPIO_PIN_SET);
    tim6_delay(IR_WAIT_COUNT);
    uint16_t l_on = get_adc_single(ADC_CHANNEL_3);
    HAL_GPIO_WritePin(IR_L_GPIO_Port, IR_L_Pin, GPIO_PIN_RESET);

    // --- 3. 差分計算（アンダーフロー防止） ---
    ad_r  = (r_on > r_off)   ? (r_on - r_off)   : 0;
    ad_fr = (fr_on > fr_off) ? (fr_on - fr_off) : 0;
    ad_fl = (fl_on > fl_off) ? (fl_on - fl_off) : 0;
    ad_l  = (l_on > l_off)   ? (l_on - l_off)   : 0;
  }
}