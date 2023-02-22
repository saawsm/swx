target_compile_definitions(${PROJECT_NAME} PRIVATE 

  PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=15000

  # -------- PINS -------- 
  PIN_LED_OK=15
  PIN_REG_EN=14   
  PIN_LDAC=4

  PIN_IO0=20
  PIN_IO1=21
  PIN_IO2=22

  PIN_I2C_SDA=2
  PIN_I2C_SCL=3

  PIN_SPI_SCK=18
  PIN_SPI_MOSI=16
  PIN_SPI_MISO=19
  PIN_SPI_CS=17
  PIN_SPI_INT=5

  # -------- I2C --------  
  I2C_PORT=i2c1
  I2C_FREQ=400000
  I2C_DEVICE_TIMEOUT=2000
  #I2C_CHECK_WRITE
  
  # -------- SPI --------
  SPI_PORT=spi0
  SPI_FREQ=1000000

  # -------- Dev --------
  ADC_ADDRESS=0x48
  DAC_ADDRESS=0x60

   # -------- Channels --------
   CHANNEL_COUNT=4
   MINIMUM_PULSE_GAP_US=150
   #CH_IGNORE_CAL_ERRORS
   
   # -------- Channel 1 --------
   PIN_CH1_GA=12
   PIN_CH1_GB=13

   CH1_DAC_ADDRESS=0
   CH1_ADC_ADDRESS=0

   CH1_CAL_THRESHOLD_OK=0.15 # 150mV - 0.1R 10AV = 150mA
   CH1_CAL_THRESHOLD_OVER=0.18 # 180mV - 0.1R 10AV = 180mA
   CH1_CAL_OFF_POINT=400

   # -------- Channel 2 --------
   PIN_CH2_GA=10
   PIN_CH2_GB=11

   CH2_DAC_ADDRESS=1
   CH2_ADC_ADDRESS=1

   CH2_CAL_THRESHOLD_OK=0.15
   CH2_CAL_THRESHOLD_OVER=0.18
   CH2_CAL_OFF_POINT=400

   # -------- Channel 3 --------
   PIN_CH3_GA=8
   PIN_CH3_GB=9

   CH3_DAC_ADDRESS=2
   CH3_ADC_ADDRESS=2

   CH3_CAL_THRESHOLD_OK=0.15
   CH3_CAL_THRESHOLD_OVER=0.18
   CH3_CAL_OFF_POINT=400

   # -------- Channel 4 --------
   PIN_CH4_GA=6
   PIN_CH4_GB=7
   
   CH4_DAC_ADDRESS=3
   CH4_ADC_ADDRESS=3

   CH4_CAL_THRESHOLD_OK=0.15
   CH4_CAL_THRESHOLD_OVER=0.18
   CH4_CAL_OFF_POINT=400

)