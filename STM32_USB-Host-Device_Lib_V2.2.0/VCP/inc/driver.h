
#define 	TIM_MAX_TICKS		50000





void Driver_Init(void);
void drv_trace_uart_init(void);

void drv_power_off(void);
void drv_power_on(void);
void drv_boot_high(void);
void drv_boot_low(void);

void drv_comm_uart_init(void);
void drv_comm_uart_init_ChangeSetting(void);



