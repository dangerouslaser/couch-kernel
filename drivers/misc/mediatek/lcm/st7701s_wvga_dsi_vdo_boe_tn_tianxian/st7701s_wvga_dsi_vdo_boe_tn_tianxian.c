#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
#else
    #include <mt-plat/mt_gpio.h>
    #include "mach/gpio_const.h"
    #ifndef GPIO_LCD_MAKER_ID
        #define GPIO_LCD_MAKER_ID    (GPIO4|0x80000000) 
    #endif
#endif


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT (800)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xDD   // END OF REGISTERS MARKER
#define LCM_ID       (0x8802)


//#define LCM_DSI_CMD_MODE									0


#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif
	
#ifdef BUILD_LK
#define LCM_PRINT printf
#else
#define LCM_PRINT printk
#endif

#define LCM_DBG(fmt, arg...) \
	LCM_PRINT("[LCM_ILI9806E_FWVGA_DSI_VDO_TXD] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

//#define FRAME_WIDTH  (480)
//#define FRAME_HEIGHT (854)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

#define AUXADC_LCM_VOLTAGE_CHANNEL     12
#define AUXADC_ADC_FDD_RF_PARAMS_DYNAMIC_CUSTOM_CH_CHANNEL     1

#define MIN_VOLTAGE (900)     // zhoulidong  add for lcm detect
#define MAX_VOLTAGE (1600)     // zhoulidong  add for lcm detect

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
// extern int LcmPowerOnPMIC(void);
// extern int LcmPowerOffPMIC_V18(void);
// extern int LcmPowerOffPMIC_V28(void);

static unsigned int lcm_compare_id(void);


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_read_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
 
 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] =
{
// PacketSize(1, 
{0x11,0,{0x00}},
{REGFLAG_DELAY, 120, {}},
//---------------------------------------Bank0 Setting-------------------------------------------------//
//------------------------------------Display Control setting----------------------------------------------//
// PacketSize(6, 
{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x10}},

// PacketSize(3, 
{0xC0, 2, {0x63, 0x00}}, 

// PacketSize(3, 
{0xC1, 2, {0x0A, 0x02}}, 

// PacketSize(3, 
{0xC2, 2, {0x31, 0x08}}, 

// PacketSize(2, 
{0xCC, 1, {0x10}}, 

//-------------------------------------Gamma Cluster Setting-------------------------------------------//
// PacketSize(17, 
{0xB0, 16, {0x40, 0x05, 0x12, 0x13, 0x19, 0x0C, 0x11, 0x0A, 0x0A, 0x23, 0x09, 0x13, 0x0E, 0x12, 0x16, 0x19}}, 

// PacketSize(17, 
{0xB1, 16, {0x40, 0x05, 0xD1, 0x13, 0x18, 0x0B, 0x13, 0x09, 0x09, 0x2A, 0x09, 0x17, 0x15, 0x9E, 0x22, 0x19}}, 

//---------------------------------------End Gamma Setting----------------------------------------------//
//------------------------------------End Display Control setting----------------------------------------//
//-----------------------------------------Bank0 Setting End---------------------------------------------//
//-------------------------------------------Bank1 Setting---------------------------------------------------//

//-------------------------------- Power Control Registers Initial --------------------------------------//
// PacketSize(6, 
{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x11}}, 

// PacketSize(2, 
{0xB0, 1, {0x4D}}, 
//-------------------------------------------Vcom Setting---------------------------------------------------//
// PacketSize(2, 
{0xB1, 1, {0x5A}}, 
//-----------------------------------------End Vcom Setting-----------------------------------------------//
// PacketSize(2, 
{0xB2, 1, {0x07}}, 

// PacketSize(2, 
{0xB3, 1, {0x80}}, 

// PacketSize(2, 
{0xB5, 1, {0x47}}, 

// PacketSize(2, 
{0xB7, 1, {0x85}}, 

// PacketSize(2, 
{0xB8, 1, {0x21}}, 

// PacketSize(2, 
{0xB9, 1, {0x10}}, 

// PacketSize(2, 
{0xC0, 1, {0x09}}, 

// PacketSize(2, 
{0xC1, 1, {0x78}}, 

// PacketSize(2, 
{0xC2, 1, {0x78}}, 

// PacketSize(2, 
{0xD0, 1, {0x88}}, 

//---------------------------------End Power Control Registers Initial -------------------------------//
//Delayms (100, 
//---------------------------------------------GIP Setting----------------------------------------------------//
// PacketSize(4, 
{0xE0, 3, {0x00, 0x00, 0x02}}, 

// PacketSize(12, 
{0xE1, 11, {0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20}}, 

// PacketSize(14, 
{0xE2, 13, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, 

// PacketSize(5, 
{0xE3, 4, {0x00, 0x00, 0x33, 0x00}}, 

// PacketSize(3, 
{0xE4, 2, {0x22, 0x00}}, 

// PacketSize(17, 
{0xE5, 16, {0x04, 0x34, 0xAA, 0xAA, 0x06, 0x34, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, 

// PacketSize(5, 
{0xE6, 4, {0x00, 0x00, 0x33, 0x00}}, 

// PacketSize(3, 
{0xE7, 2, {0x22, 0x00}}, 

// PacketSize(17, 
{0xE8, 16, {0x05, 0x34, 0xAA, 0xAA, 0x07, 0x34, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, 

// PacketSize(17, 
// {0xEA, 16, {0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00}}, // Jiangde, coe 

// PacketSize(8, 
{0xEB, 7, {0x02, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}}, 

// PacketSize(3, 
{0xEC, 2, {0x00, 0x00}}, 

// PacketSize(17, 
{0xED, 16, {0xFA, 0x45, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0, 0x54, 0xAF}}, 

// PacketSize(13, 
// {0xEF, 12, {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}}, // Jiangde, coe
//---------------------------------------------End GIP Setting-----------------------------------------------//
//------------------------------ Power Control Registers Initial End-----------------------------------//
//------------------------------------------Bank1 Setting----------------------------------------------------//

// PacketSize(6, 
{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x00}}, 


// PacketSize(1, 
{0x29,0,{0x00}},
    
{REGFLAG_DELAY, 20, {}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_set_window[] = {

};


static struct LCM_setting_table lcm_sleep_out_setting[] = {

    // Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
     {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {

    // Display off sequence
    {0x28, 1, {0x00}},        
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {

};


static struct LCM_setting_table lcm_compare_id_setting[] = {

	{0xD3,	3,	{0xFF, 0x83, 0x79}},
	{REGFLAG_DELAY, 10, {}}, 	

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
		        dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    
    // enable tearing-free
    // params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_mode                 = LCM_DBI_TE_MODE_DISABLED;
    //  params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

    params->dsi.mode   = SYNC_PULSE_VDO_MODE;


    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM                = LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;

    // Video mode setting        
    params->dsi.intermediat_buffer_num = 2;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.word_count=480*3;

    //edit by Magnum 2013-7-25 , solve esd read id error
    //     cycle_time = (4 * 1000 * div2 * div1 * pre_div * post_div)/ (fbk_sel * (fbk_div+0x01) * 26) + 
    // 1 = 
    // ui = (1000 * div2 * div1 * pre_div * post_div)/ (fbk_sel * (fbk_div+0x01) * 26 * 2) + 1;

    params->dsi.vertical_sync_active    = 10;
    params->dsi.vertical_backporch      = 20;
    params->dsi.vertical_frontporch     = 18;
    params->dsi.vertical_active_line    = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active  = 10;
    params->dsi.horizontal_backporch    = 70;
    params->dsi.horizontal_frontporch   = 80;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.compatibility_for_nvk   = 0;    
    params->dsi.ssc_disable             = 1; // 1==close ssc?
    params->dsi.ssc_range               = 2;
    params->dsi.PLL_CLOCK               = 200; // 200;224

    params->dsi.esd_check_enable                    = 0;
    params->dsi.customization_esd_check_enable      = 1;
    params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
    params->dsi.lcm_esd_check_table[0].count        = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

    /* ESD or noise interference recovery For video mode LCM only. */
    // Send TE packet to LCM in a period of n frames and check the response.
    /*    params->dsi.lcm_int_te_monitor = FALSE;
    params->dsi.lcm_int_te_period = 1;        // Unit : frames

    // Need longer FP for more opportunity to do int. TE monitor applicably.
    if(params->dsi.lcm_int_te_monitor)
    params->dsi.vertical_frontporch *= 2;

    // Monitor external TE (or named VSYNC) from LCM once per 2 sec. (LCM VSYNC must be wired to baseband TE pin.)
    params->dsi.lcm_ext_te_monitor = FALSE;
    // Non-continuous clock
    params->dsi.noncont_clock = TRUE;
    params->dsi.noncont_clock_period = 2;    // Unit : frames  */
}


static void lcm_init(void)
{
    LCM_PRINT("[lcm st7701 coe]: lcm_init. \n");
    // LcmPowerOnPMIC(); // for debug purpose, VGP3

    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(25);
    SET_RESET_PIN(1);
    MDELAY(120);
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{

	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(50);

}


static void lcm_resume(void)
{
	lcm_init();
}


//extern void Tinno_set_HS_read();
//extern void Tinno_restore_HS_read();
//static unsigned int lcm_esd_test = FALSE; 


static unsigned int rgk_lcm_compare_id(void)
{
    s32 iMakerID = 0;

    iMakerID = mt_get_gpio_in(GPIO_LCD_MAKER_ID);
    LCM_PRINT("[adc_uboot boe]: GPIO_LCD_MAKER_ID=%d, iMakerID= %d\n", GPIO_LCD_MAKER_ID, iMakerID);

    if (iMakerID) {
        return 1;
    }

    return 0;    
}


static unsigned int lcm_compare_id(void)
{
    unsigned int id = 0;
    unsigned char buffer[3];
    unsigned int array[16];

    SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    array[0] = 0x00023700; // 0x00033700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0xa1, buffer, 3);
    id =buffer[0]<<8|buffer[1]; //we only need ID
    
    LCM_PRINT("lcm st7701s boe]: id=0x%x,  b0=0x%x, b1=0x%x, b2=0x%x \n", id, buffer[0], buffer[1], buffer[2]);

    if(LCM_ID == id) {
        LCM_PRINT("lcm st7701s boe]: it is st7701s. \n");
        return rgk_lcm_compare_id();
    }

    return 0;
}


LCM_DRIVER st7701s_wvga_dsi_vdo_boe_tn_tianxian_lcm_drv = 
{
    .name			= "st7701s_wvga_dsi_vdo_boe_tn_tianxian",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    = lcm_compare_id,
};


