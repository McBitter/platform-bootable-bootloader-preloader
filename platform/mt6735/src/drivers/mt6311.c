#include "platform.h"
#include "i2c.h"
#include "gpio.h"
#include "mt6311.h"

#if CFG_FPGA_PLATFORM
#else
#include "cust_i2c.h"
//#include "cust_gpio_usage.h"
#endif

/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define mt6311_SLAVE_ADDR_WRITE   0xD6
#define mt6311_SLAVE_ADDR_Read    0xD7

/**********************************************************
  *   Global Variable 
  *********************************************************/
#ifdef I2C_EXT_BUCK_CHANNEL
#define mt6311_I2C_ID I2C_EXT_BUCK_CHANNEL
#else
#define mt6311_I2C_ID I2C2
#endif

#ifdef GPIO_EXT_BUCK_VSEL_PIN
unsigned int g_vproc_vsel_gpio_number = GPIO_EXT_BUCK_VSEL_PIN; 
#else
unsigned int g_vproc_vsel_gpio_number = 0;
#endif

static struct mt_i2c_t mt6311_i2c;

int g_mt6311_driver_ready=0;
int g_mt6311_hw_exist=0;

#define mt6311_print(fmt, args...)   \
do {									\
    printf(fmt, ##args); \
} while(0)

kal_uint32 g_mt6311_cid=0;

/**********************************************************
  *
  *   [I2C Function For Read/Write mt6311] 
  *
  *********************************************************/
kal_uint32 mt6311_write_byte(kal_uint8 addr, kal_uint8 value)
{
    int ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    mt6311_i2c.id = mt6311_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set mt6311 I2C address to >>1 */
    mt6311_i2c.addr = (mt6311_SLAVE_ADDR_WRITE >> 1);
    mt6311_i2c.mode = ST_MODE;
    mt6311_i2c.speed = 100;
    mt6311_i2c.pushpull = 1;
    len = 2;
    
    ret_code = i2c_write(&mt6311_i2c, write_data, len);        
    //mt6311_print("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    if(ret_code == 0)
        return 1; // ok
    else
        return 0; // fail
}

kal_uint32 mt6311_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer) 
{
    int ret_code = I2C_OK;
    kal_uint16 len;
    *dataBuffer = addr;

    mt6311_i2c.id = mt6311_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set mt6311 I2C address to >>1 */
    mt6311_i2c.addr = (mt6311_SLAVE_ADDR_WRITE >> 1);
    mt6311_i2c.mode = ST_MODE;
    mt6311_i2c.speed = 100;
    mt6311_i2c.pushpull = 1;
    len = 1;

    ret_code = i2c_write_read(&mt6311_i2c, dataBuffer, len, len);    
    //mt6311_print("%s: i2c_read: ret_code: %d\n", __func__, ret_code);

    if(ret_code == 0)
        return 1; // ok
    else
        return 0; // fail
}

/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
kal_uint32 mt6311_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 mt6311_reg = 0;
    kal_uint32 ret = 0;
    
    //mt6311_print("--------------------------------------------------PL\n");

    ret = mt6311_read_byte(RegNum, &mt6311_reg);
    //mt6311_print("[mt6311_read_interface] Reg[%x]=0x%x\n", RegNum, mt6311_reg);
    
    mt6311_reg &= (MASK << SHIFT);
    *val = (mt6311_reg >> SHIFT);    
    //mt6311_print("[mt6311_read_interface] val=0x%x\n", *val);

    return ret;
}

kal_uint32 mt6311_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 mt6311_reg = 0;
    kal_uint32 ret = 0;

    //mt6311_print("--------------------------------------------------PL\n");

    ret = mt6311_read_byte(RegNum, &mt6311_reg);
    //mt6311_print("[mt6311_config_interface] Reg[%x]=0x%x\n", RegNum, mt6311_reg);
    
    mt6311_reg &= ~(MASK << SHIFT);
    mt6311_reg |= (val << SHIFT);

    ret = mt6311_write_byte(RegNum, mt6311_reg);
    //mt6311_print("[mt6311_config_interface] write Reg[%x]=0x%x\n", RegNum, mt6311_reg);

    // Check
    //mt6311_read_byte(RegNum, &mt6311_reg);
    //mt6311_print("[mt6311_config_interface] Check Reg[%x]=0x%x\n", RegNum, mt6311_reg);

    return ret;
}

kal_uint32 mt6311_get_reg_value(kal_uint32 reg)
{
    kal_uint32 ret=0;
    kal_uint8 reg_val=0;

    ret=mt6311_read_interface( (kal_uint8) reg, &reg_val, 0xFF, 0x0);

    if(ret==0) mt6311_print("%d", ret);
    return reg_val;
}

void mt6311_dump_register(void)
{
    kal_uint8 i=0x0;
    kal_uint8 i_max=0x2;//0xD5
    
    for (i=0x0;i<=i_max;i++) {     
        mt6311_print("[0x%x]=0x%x ", i, mt6311_get_reg_value(i));
    } 
}

int get_mt6311_i2c_ch_num(void)
{
    return mt6311_I2C_ID;
}

int mt6311_check_point=0;

void ext_buck_vproc_vsel(int val)
{   
    if(g_vproc_vsel_gpio_number != 0)
    {
        //TBD
        //mt_set_gpio_mode(g_vproc_vsel_gpio_number,0); // 0:GPIO mode
        //mt_set_gpio_dir(g_vproc_vsel_gpio_number,1);  // dir = output
        //mt_set_gpio_out(g_vproc_vsel_gpio_number,val);

        mt6311_check_point=1;
    }
    else
    {
        mt6311_check_point=2;
    }

    //mt6311_print("[ext_buck_vproc_vsel] done. (%d)\n", g_vproc_vsel_gpio_number);
}

void mt6311_hw_init(void)
{
   kal_uint32 ret=0;

   //TBD
   mt6311_print("[mt6311_hw_init] TBD (%d)\n", mt6311_check_point);  

   if(ret==0) mt6311_print("%d", ret);
}

kal_uint8 mt6311_get_cid(void)
{
  kal_uint8 ret=0;
  kal_uint8 val=0;

  ret=mt6311_read_interface( (kal_uint8)(MT6311_CID),
                           (&val),
                           (kal_uint8)(MT6311_PMIC_CID_MASK),
                           (kal_uint8)(MT6311_PMIC_CID_SHIFT)
	                       );

  if(ret==0) mt6311_print("%d", ret);
  return val;
}

kal_uint8 mt6311_get_swcid(void)
{
  kal_uint8 ret=0;
  kal_uint8 val=0;

  ret=mt6311_read_interface( (kal_uint8)(MT6311_SWCID),
                           (&val),
                           (kal_uint8)(MT6311_PMIC_SWCID_MASK),
                           (kal_uint8)(MT6311_PMIC_SWCID_SHIFT)
	                       );

  if(ret==0) mt6311_print("%d", ret);
  return val;
}

kal_uint32 update_mt6311_chip_id(void)
{
    kal_uint32 id=0;
    kal_uint32 id_l=0;
    kal_uint32 id_r=0;

    id_l=mt6311_get_cid();
    id_r=mt6311_get_swcid();
    id=((id_l<<8)|(id_r));

    g_mt6311_cid=id;

    mt6311_print("[update_mt6311_chip_id] id_l=0x%x, id_r=0x%x, id=0x%x\n", id_l, id_r, id);

    return id;
}

kal_uint32 mt6311_get_chip_id(void)
{
    if(g_mt6311_cid==0)
        update_mt6311_chip_id();

    mt6311_print("[mt6311_get_chip_id] g_mt6311_cid=0x%x\n", g_mt6311_cid);
    
    return g_mt6311_cid;
}

void mt6311_hw_component_detect(void)
{
    update_mt6311_chip_id();
        
    if( (mt6311_get_chip_id()==PMIC6311_E1_CID_CODE) ||
        (mt6311_get_chip_id()==PMIC6311_E2_CID_CODE) ||
        (mt6311_get_chip_id()==PMIC6311_E3_CID_CODE)
    ){
        g_mt6311_hw_exist=1;
    }
    else
    {
        g_mt6311_hw_exist=0;
    }
    mt6311_print("[mt6311_hw_component_detect] exist=%d\n", g_mt6311_hw_exist);
}

int is_mt6311_sw_ready(void)
{
    mt6311_print("g_mt6311_driver_ready=%d\n", g_mt6311_driver_ready);
    
    return g_mt6311_driver_ready;
}

int is_mt6311_exist(void)
{
    mt6311_print("g_mt6311_hw_exist=%d\n", g_mt6311_hw_exist);
    
    return g_mt6311_hw_exist;
}

int mt6311_vosel(unsigned long val)
{
    int ret=1;

    //TBD    
#if 0    
    unsigned long reg_val=0;

    //reg_val = ( (val) - 30000 ) / 1000; //300mV~1570mV, step=10mV
    reg_val = ((((val*10)-300000)/1000)+9)/10;

    if(reg_val > 127)
        reg_val = 127;

    ret=mt6311_write_byte(0xD8, reg_val);

    mt6311_print("[mt6311_vosel] val=%ld, reg_val=%ld, Reg[0xD8]=0x%x\n", 
        val, reg_val, mt6311_get_reg_value(0xD8));
#endif

    return ret;
}

void mt6311_driver_probe(void) 
{       
    mt6311_hw_component_detect();        
    if(g_mt6311_hw_exist==1)
    {
        mt6311_hw_init();
        mt6311_dump_register();
    }
    else
    {
        mt6311_print("[mt6311_driver_probe] PL mt6311 is not exist\n");
    }    
    g_mt6311_driver_ready=1;
    
    mt6311_print("[mt6311_driver_probe] PL g_mt6311_hw_exist=%d, g_mt6311_driver_ready=%d\n", 
        g_mt6311_hw_exist, g_mt6311_driver_ready);
   
    //--------------------------------------------------------

    #ifdef I2C_EXT_BUCK_CHANNEL
    mt6311_print("[mt6311_driver_probe] PL I2C_EXT_BUCK_CHANNEL=%d.\n", I2C_EXT_BUCK_CHANNEL);
    #else
    mt6311_print("[mt6311_driver_probe] PL No I2C_EXT_BUCK_CHANNEL (%d)\n", mt6311_I2C_ID);
    #endif

    #ifdef GPIO_EXT_BUCK_VSEL_PIN
    mt6311_print("[mt6311_driver_probe] PL GPIO_EXT_BUCK_VSEL_PIN=0x%x.\n", GPIO_EXT_BUCK_VSEL_PIN);
    #else
    mt6311_print("[mt6311_driver_probe] PL No GPIO_EXT_BUCK_VSEL_PIN (0x%x)\n", g_vproc_vsel_gpio_number);
    #endif
}
