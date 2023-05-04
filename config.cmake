target_compile_definitions(${PROJECT_NAME} PRIVATE 

    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=15000

    # -------- PINS -------- 
    PIN_REG_EN=14
    
    PIN_LDAC=4
    PIN_INT=5

    PIN_I2C_SDA=2
    PIN_I2C_SCL=3

    PIN_SPI_SCK=18
    PIN_SPI_MOSI=16
    PIN_SPI_MISO=19
    PIN_SPI_CS=17
    
    PIN_AUDIO_LEFT=27
    PIN_AUDIO_RIGHT=28
    PIN_AUDIO_MIC=29

    PIN_I2S_BCLK=20
    PIN_I2S_WS=21
    PIN_I2S_SD=22

    # -------- I2C --------  
    I2C_PORT=i2c1
    I2C_FREQ=400000
    I2C_DEVICE_TIMEOUT=2000
    #I2C_CHECK_WRITE
    
    # -------- SPI --------
    SPI_PORT=spi0
    SPI_FREQ=1000000

    # -------- I2C Devices --------
    ADC_ADDRESS=0x48
    DAC_ADDRESS=0x60

    # -------- Channels --------
    CHANNEL_COUNT=4
    #CH_CAL_IGNORE_ERRORS
 
    # USE_ADC_MEAN  # Uncomment to use trimmed mean for ADC samples. Not recommended for I2C ADCs (e.g. ADS1015) since sampling takes too long
    ADC_MEAN=10
    ADC_MEAN_TRIM_AMOUNT=2

    # Channel Defaults
    # Use CHn_CAL_ for channel specific overrides
    CH_CAL_THRESHOLD_OK=0.15f
    CH_CAL_THRESHOLD_OVER=0.18f
    CH_CAL_OFFSET=400

    # -------- Channel 1 --------
    PIN_CH1_GA=12
    PIN_CH1_GB=13
    CH1_DAC_CHANNEL=0
    CH1_ADC_CHANNEL=0

    # -------- Channel 2 --------
    PIN_CH2_GA=10
    PIN_CH2_GB=11
    CH2_DAC_CHANNEL=1
    CH2_ADC_CHANNEL=1

    # -------- Channel 3 --------
    PIN_CH3_GA=8
    PIN_CH3_GB=9    
    CH3_DAC_CHANNEL=2
    CH3_ADC_CHANNEL=2

    # -------- Channel 4 --------
    PIN_CH4_GA=6
    PIN_CH4_GB=7    
    CH4_DAC_CHANNEL=3
    CH4_ADC_CHANNEL=3
   
)