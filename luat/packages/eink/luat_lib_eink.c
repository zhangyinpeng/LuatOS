/*
@module  eink
@summary 墨水屏操作库
@version 1.0
@date    2020.11.14
*/
#include "luat_base.h"
#include "luat_log.h"
#include "luat_sys.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_malloc.h"
#include "luat_spi.h"

#include "luat_gpio.h"

// #include "epd1in54.h"
// #include "epd2in9.h"

#include "epd.h"
#include "epdpaint.h"
#include "imagedata.h"
#include "../qrcode/qrcode.h"

#include <stdlib.h>

#include "u8g2.h"

#define Pin_BUSY        (18)
#define Pin_RES         (7)
#define Pin_DC          (9)
#define Pin_CS          (16)

#define SPI_ID          (0)

#define COLORED      0
#define UNCOLORED    1

#define LUAT_LOG_TAG "luat.eink"


// static EPD epd;
static Paint paint;
static unsigned char* frame_buffer;

/**
初始化eink
@api eink.setup(full, spiid)
@int 全屏刷新1,局部刷新0,默认是全屏刷新
@int 所在的spi,默认是0
@return boolean 成功返回true,否则返回false
*/
static int l_eink_setup(lua_State *L) {
    int num = luaL_optinteger(L, 1, 1);
    int spi_id = luaL_optinteger(L, 2, 0);

    if (frame_buffer != NULL) {
        lua_pushboolean(L, 1);
        return 0;
    }

    luat_spi_t spi_config = {0};
    spi_config.bandrate = 2000000U;//luaL_optinteger(L, 1, 2000000U); // 2000000U
    spi_config.id = spi_id;
    spi_config.cs = 255; // 默认无
    spi_config.CPHA = 0; // CPHA0
    spi_config.CPOL = 0; // CPOL0
    spi_config.dataw = 8; // 8bit
    spi_config.bit_dict = 1; // MSB=1, LSB=0
    spi_config.master = 1; // master=1,slave=0
    spi_config.mode = 1; // FULL=1, half=0

    //LLOGD("setup GPIO for epd");
    // TODO 事实上无法配置
    luat_gpio_mode(luaL_optinteger(L, 3, Pin_BUSY), Luat_GPIO_INPUT, Luat_GPIO_PULLUP, Luat_GPIO_LOW);
    luat_gpio_mode(luaL_optinteger(L, 4, Pin_RES), Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_LOW);
    luat_gpio_mode(luaL_optinteger(L, 5, Pin_DC), Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_LOW);
    luat_gpio_mode(luaL_optinteger(L, 6, Pin_CS), Luat_GPIO_OUTPUT, Luat_GPIO_PULLUP, Luat_GPIO_LOW);

    //LLOGD("spi setup>>>");
    int status = luat_spi_setup(&spi_config);
    //LLOGD("spi setup<<<");
    //EPD_Model(MODEL_1in54f);
    //EPD_Model(MODEL_1in54_V2);
    // EPD_Model(MODEL_2in13b_V3);

    size_t epd_w = 0;
    size_t epd_h = 0;
    if(status == 0)
    {
        LLOGD("spi setup complete, now setup epd");
        if(num)
            status = EPD_Init(1, &epd_w, &epd_h);
        else
            status = EPD_Init(0, &epd_w, &epd_h);

        if (status != 0) {
            LLOGD("e-Paper init failed");
            return 0;
        }
        frame_buffer = (unsigned char*)luat_heap_malloc(epd_w * epd_h / 8);
        //   Paint paint;
        Paint_Init(&paint, frame_buffer, epd_w, epd_h);
        Paint_Clear(&paint, UNCOLORED);
    }
    //LLOGD("epd init complete");
    lua_pushboolean(L, 1);
    return 1;
}


/**
清除绘图缓冲区
@api eink.clear()
@return nil 无返回值,不会马上刷新到设备
*/
static int l_eink_clear(lua_State *L)
{
    int colored = luaL_optinteger(L, 1, 1);
    Paint_Clear(&paint, colored);
    return 0;
}

/**
设置窗口
@api eink.setWin(width, height, rotate)
@int width  宽度
@int height 高度
@int rotate 显示方向,0/1/2/3, 相当于旋转0度/90度/180度/270度
@return nil 无返回值
*/
static int l_eink_setWin(lua_State *L)
{
    int width = luaL_checkinteger(L, 1);
    int height = luaL_checkinteger(L, 2);
    int rotate = luaL_checkinteger(L, 3);

    Paint_SetWidth(&paint, width);
    Paint_SetHeight(&paint, height);
    Paint_SetRotate(&paint, rotate);
    return 0;
}

/**
获取窗口信息
@api eink.getWin()
@return int width  宽
@return int height 高
@return int rotate 旋转方向
*/
static int l_eink_getWin(lua_State *L)
{
    int width =  Paint_GetWidth(&paint);
    int height = Paint_GetHeight(&paint);
    int rotate = Paint_GetRotate(&paint);

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);
    lua_pushinteger(L, rotate);
    return 3;
}

/**
绘制字符串(仅ASCII)
@api eink.print(x, y, str, colored, font)
@int x坐标
@int y坐标
@string 字符串
@int 默认是0
@font 字体大小,默认12
@return nil 无返回值
*/
static int l_eink_print(lua_State *L)
{
    size_t len;
    int x           = luaL_checkinteger(L, 1);
    int y           = luaL_checkinteger(L, 2);
    const char *str = luaL_checklstring(L, 3, &len);
    int colored     = luaL_optinteger(L, 4, 0);
    int font        = luaL_optinteger(L, 5, 12);


    switch (font)
    {
        case 8:
            Paint_DrawStringAt(&paint, x, y, str, &Font8, colored);
            break;
        case 12:
            Paint_DrawStringAt(&paint, x, y, str, &Font12, colored);
            break;
        case 16:
            Paint_DrawStringAt(&paint, x, y, str, &Font16, colored);
            break;
        case 20:
            Paint_DrawStringAt(&paint, x, y, str, &Font20, colored);
            break;
        case 24:
            Paint_DrawStringAt(&paint, x, y, str, &Font24, colored);
            break;
        default:
            break;
    }
    return 0;
}

// void u8g2_read_font_info(u8g2_font_info_t *font_info, const uint8_t *font);
uint8_t luat_u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
const uint8_t *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding);
uint8_t * font_pix_find(uint16_t e, int font_pix_find);
/**
绘制字符串,支持中文
@api eink.printcn(x, y, str, colored, font)
@int x坐标
@int y坐标
@string 字符串
@int 默认是0
@font 字体大小,默认12
@return nil 无返回值
*/
static int l_eink_printcn(lua_State *L)
{
    size_t len;
    int x           = luaL_checkinteger(L, 1);
    int y           = luaL_checkinteger(L, 2);
    const char *str = luaL_checklstring(L, 3, &len);
    int colored     = luaL_optinteger(L, 4, 0);
    int font_size        = luaL_optinteger(L, 5, 12);

    //LLOGD("printcn font_size %d len %d", font_size, len);

    // 解码数据 TODO 使用无u8g2的方案
    u8g2_t *u8g2 = luat_heap_malloc(sizeof(u8g2_t));
    u8g2_Setup_ssd1306_i2c_128x64_noname_f( u8g2, U8G2_R0, u8x8_byte_sw_i2c, luat_u8x8_gpio_and_delay);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2->u8x8.next_cb = u8x8_utf8_next;

    uint16_t e;
    //u8g2_uint_t delta, sum;
    u8x8_utf8_init(u8g2_GetU8x8(u8g2));
    //sum = 0;
    uint8_t* str2 = (uint8_t*)str;
    for(;;)
    {
        e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str2);
        //LLOGD("chinese >> 0x%04X", e);
        if ( e == 0x0ffff )
            break;
        str2++;
        // if (e != 0x0fffe && e < 0x007e) {
        //     char ch[2] = {e, 0};
        //     if (font_size == 16)
        //         Paint_DrawStringAt(&paint, x, y, ch, &Font16, colored);
        //     else if (font_size == 24)
        //         Paint_DrawStringAt(&paint, x, y, ch, &Font24, colored);
        //     x += font_size;
        // }
        // else
        if ( e != 0x0fffe )
        {
            //delta = u8g2_DrawGlyph(u8g2, x, y, e);
            uint8_t * f = font_pix_find(e, font_size);
            if (f != NULL) {
                // 当前仅支持16p和24p
                int datalen = font_size == 16 ? 32 : 72;
                int xlen = font_size == 16 ? 2 : 3;
                //LLOGD("found FONT DATA 0x%04X datalen=%d xlen=%d", e, datalen, xlen);
                for (size_t i = 0; i < datalen; )
                {
                    for (size_t k = 0; k < xlen; k++)
                    {
                        uint8_t pix = f[i];
                        //LLOGD("pix data %02X   x+j=%d y+j/2=%d", pix, x, y+i/2);
                        for (size_t j = 0; j < 8; j++)
                        {
                            if ((pix >> (7-j)) & 0x01) {
                                Paint_DrawPixel(&paint, x+j+k*8, y+i/xlen, colored);
                            }
                        }
                        i++;
                    }
                }
            }
            else {
                LLOGD("NOT found FONT DATA 0x%04X", e);
            }

            if (e <= 0x7E)
                x += font_size / 2;
            else
            {
                x += font_size;
            }

        }
    }

    luat_heap_free(u8g2);

    return 0;
}

/**
将缓冲区图像输出到屏幕
@api eink.show(x, y)
@int x 输出的x坐标,默认0
@int y 输出的y坐标,默认0
@return nil 无返回值
*/
static int l_eink_show(lua_State *L)
{
    int x     = luaL_optinteger(L, 1, 0);
    int y     = luaL_optinteger(L, 2, 0);
    /* Display the frame_buffer */
    //EPD_SetFrameMemory(&epd, frame_buffer, x, y, Paint_GetWidth(&paint), Paint_GetHeight(&paint));
    //EPD_DisplayFrame(&epd);
    EPD_Clear();
    EPD_Display(frame_buffer, NULL);
    return 0;
}


/**
缓冲区绘制线
@api eink.line(x, y, x2, y2, colored)
@int 起点x坐标
@int 起点y坐标
@int 终点x坐标
@int 终点y坐标
@return nil 无返回值
@usage
eink.line(0, 0, 10, 20, 0)
*/
static int l_eink_line(lua_State *L)
{
    int x       = luaL_checkinteger(L, 1);
    int y       = luaL_checkinteger(L, 2);
    int x2      = luaL_checkinteger(L, 3);
    int y2      = luaL_checkinteger(L, 4);
    int colored = luaL_optinteger(L, 5, 0);

    Paint_DrawLine(&paint, x, y, x2, y2, colored);
    return 0;
}

/**
缓冲区绘制矩形
@api eink.rect(x, y, x2, y2, colored, fill)
@int 左上顶点x坐标
@int 左上顶点y坐标
@int 右下顶点x坐标
@int 右下顶点y坐标
@int 默认是0
@int 是否填充,默认是0,不填充
@return nil 无返回值
@usage
eink.rect(0, 0, 10, 20)
eink.rect(0, 0, 10, 20, 1) -- Filled
*/
static int l_eink_rect(lua_State *L)
{
    int x      = luaL_checkinteger(L, 1);
    int y      = luaL_checkinteger(L, 2);
    int x2      = luaL_checkinteger(L, 3);
    int y2      = luaL_checkinteger(L, 4);
    int colored = luaL_optinteger(L, 5, 0);
    int fill    = luaL_optinteger(L, 6, 0);

    if(fill)
        Paint_DrawFilledRectangle(&paint, x, y, x2, y2, colored);
    else
        Paint_DrawRectangle(&paint, x, y, x2, y2, colored);
    return 0;
}

/**
缓冲区绘制圆形
@api eink.circle(x, y, radius, colored, fill)
@int 圆心x坐标
@int 圆心y坐标
@int 半径
@int 默认是0
@int 是否填充,默认是0,不填充
@return nil 无返回值
@usage
eink.circle(0, 0, 10)
eink.circle(0, 0, 10, 1, 1) -- Filled
*/
static int l_eink_circle(lua_State *L)
{
    int x       = luaL_checkinteger(L, 1);
    int y       = luaL_checkinteger(L, 2);
    int radius  = luaL_checkinteger(L, 3);
    int colored = luaL_optinteger(L, 4, 0);
    int fill    = luaL_optinteger(L, 5, 0);

    if(fill)
        Paint_DrawFilledCircle(&paint, x, y, radius, colored);
    else
        Paint_DrawCircle(&paint, x, y, radius, colored);
    return 0;
}

/**
缓冲区绘制QRCode
@api eink.qrcode(x, y, str, version)
@int x坐标
@int y坐标
@string 二维码的内容
@int 二维码版本号
@return nil 无返回值
*/
static int l_eink_qrcode(lua_State *L)
{
    size_t len;
    int x           = luaL_checkinteger(L, 1);
    int y           = luaL_checkinteger(L, 2);
    const char* str = luaL_checklstring(L, 3, &len);
    int version     = luaL_checkinteger(L, 4);
    // Create the QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(version)];
    qrcode_initText(&qrcode, qrcodeData, version, 0, str);

    for(int i = 0; i < qrcode.size; i++)
    {
        for (int j = 0; j < qrcode.size; j++)
        {
            qrcode_getModule(&qrcode, j, i) ? Paint_DrawPixel(&paint, x+j, y+i, COLORED) : Paint_DrawPixel(&paint, x+j, y+i, UNCOLORED);
        }
    }
    return 0;
}

/**
缓冲区绘制电池
@api eink.bat(x, y, bat)
@int x坐标
@int y坐标
@int 电池电压,单位毫伏
@return nil 无返回值
*/
static int l_eink_bat(lua_State *L)
{
    int x           = luaL_checkinteger(L, 1);
    int y           = luaL_checkinteger(L, 2);
    int bat         = luaL_checkinteger(L, 3);
    int batnum      = 0;
        // eink.rect(0, 3, 2, 6)
        // eink.rect(2, 0, 20, 9)
        // eink.rect(9, 1, 19, 8, 0, 1)
    if(bat < 4200 && bat > 4080)batnum = 100;
    if(bat < 4080 && bat > 4000)batnum = 90;
    if(bat < 4000 && bat > 3930)batnum = 80;
    if(bat < 3930 && bat > 3870)batnum = 70;
    if(bat < 3870 && bat > 3820)batnum = 60;
    if(bat < 3820 && bat > 3790)batnum = 50;
    if(bat < 3790 && bat > 3770)batnum = 40;
    if(bat < 3770 && bat > 3730)batnum = 30;
    if(bat < 3730 && bat > 3700)batnum = 20;
    if(bat < 3700 && bat > 3680)batnum = 15;
    if(bat < 3680 && bat > 3500)batnum = 10;
    if(bat < 3500 && bat > 2500)batnum = 5;
    batnum = 20 - (int)(batnum / 5) + 3;
    // w外框
    Paint_DrawRectangle(&paint, x+0, y+3, x+2, y+6, COLORED);
    Paint_DrawRectangle(&paint, x+2, y+0, x+23, y+9, COLORED);
    // 3 ~21   100 / 5
    Paint_DrawFilledRectangle(&paint, x+batnum, y+1, x+22, y+8, COLORED);

    return 0;
}

/**
缓冲区绘制天气图标
@api eink.weather_icon(x, y, code)
@int x坐标
@int y坐标
@int 天气代号
@return nil 无返回值
*/
static int l_eink_weather_icon(lua_State *L)
{
    size_t len;
    int x           = luaL_checkinteger(L, 1);
    int y           = luaL_checkinteger(L, 2);
    int code        = luaL_checkinteger(L, 3);
    const char* str = luaL_optlstring(L, 4, "nil", &len);
    const unsigned char * icon = gImage_999;

    if (strcmp(str, "xue")      == 0)code = 401;
    if (strcmp(str, "lei")      == 0)code = 302;
    if (strcmp(str, "shachen")  == 0)code = 503;
    if (strcmp(str, "wu")       == 0)code = 501;
    if (strcmp(str, "bingbao")  == 0)code = 504;
    if (strcmp(str, "yun")      == 0)code = 103;
    if (strcmp(str, "yu")       == 0)code = 306;
    if (strcmp(str, "yin")      == 0)code = 101;
    if (strcmp(str, "qing")     == 0)code = 100;

    // xue、lei、shachen、wu、bingbao、yun、yu、yin、qing

    //if(code == 64)
    // for(int i = 0; i < 64; i++)
    // {
    //     for (int j = 0; j < 8; j++)
    //     {
    //         for (int k = 0; k < 8; k++)
    //             (gImage_103[i*8+j] << k )& 0x80 ? Paint_DrawPixel(&paint, x+j*8+k, y+i, COLORED) : Paint_DrawPixel(&paint, x+j*8+k, y+i, UNCOLORED);
    //     }
    // }

    switch (code)
    {
        case 100:icon = gImage_100;break;
        case 101:icon = gImage_101;break;
        case 102:icon = gImage_102;break;
        case 103:icon = gImage_103;break;
        case 104:icon = gImage_104;break;
        case 200:icon = gImage_200;break;
        case 201:icon = gImage_201;break;
        case 205:icon = gImage_205;break;
        case 208:icon = gImage_208;break;

        case 301:icon = gImage_301;break;
        case 302:icon = gImage_302;break;
        case 303:icon = gImage_303;break;
        case 304:icon = gImage_304;break;
        case 305:icon = gImage_305;break;
        case 306:icon = gImage_306;break;
        case 307:icon = gImage_307;break;
        case 308:icon = gImage_308;break;
        case 309:icon = gImage_309;break;
        case 310:icon = gImage_310;break;
        case 311:icon = gImage_311;break;
        case 312:icon = gImage_312;break;
        case 313:icon = gImage_313;break;

        case 400:icon = gImage_400;break;
        case 401:icon = gImage_401;break;
        case 402:icon = gImage_402;break;
        case 403:icon = gImage_403;break;
        case 404:icon = gImage_404;break;
        case 405:icon = gImage_405;break;
        case 406:icon = gImage_406;break;
        case 407:icon = gImage_407;break;

        case 500:icon = gImage_500;break;
        case 501:icon = gImage_501;break;
        case 502:icon = gImage_502;break;
        case 503:icon = gImage_503;break;
        case 504:icon = gImage_504;break;
        case 507:icon = gImage_507;break;
        case 508:icon = gImage_508;break;
        case 900:icon = gImage_900;break;
        case 901:icon = gImage_901;break;
        case 999:icon = gImage_999;break;

        default:
            break;
    }
    for(int i = 0; i < 48; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            for (int k = 0; k < 8; k++)
                (icon[i*6+j] << k )& 0x80 ? Paint_DrawPixel(&paint, x+j*8+k, y+i, COLORED) : Paint_DrawPixel(&paint, x+j*8+k, y+i, UNCOLORED);
        }
    }


    return 0;
}

/**
设置墨水屏驱动型号
@api eink.model(m)
@int 型号名称, 例如 eink.model(eink.MODEL_1in54_V2)
@return nil 无返回值
*/
static int l_eink_model(lua_State *L) {
    EPD_Model(luaL_checkinteger(L, 1));
    return 0;
}

#include "rotable.h"
static const rotable_Reg reg_eink[] =
{
    { "setup",          l_eink_setup,           0},
    { "clear",          l_eink_clear,           0},
    { "setWin",         l_eink_setWin,          0},
    { "getWin",         l_eink_getWin,          0},
    { "print",          l_eink_print,           0},
    { "printcn",        l_eink_printcn,         0},
    { "show",           l_eink_show,            0},
    { "rect",           l_eink_rect,            0},
    { "circle",         l_eink_circle,          0},
    { "line",           l_eink_line,            0},

    { "qrcode",         l_eink_qrcode,          0},
    { "bat",            l_eink_bat,             0},
    { "weather_icon",   l_eink_weather_icon,    0},

    { "model",          l_eink_model,           0},

    { "MODEL_1in02d",         NULL,                 MODEL_1in02d},
    { "MODEL_1in54",          NULL,                 MODEL_1in54},
    { "MODEL_1in54_V2",       NULL,                 MODEL_1in54_V2},
    { "MODEL_1in54b",         NULL,                 MODEL_1in54b},
    { "MODEL_1in54b_V2",      NULL,                 MODEL_1in54b_V2},
    { "MODEL_1in54c",         NULL,                 MODEL_1in54c},
    { "MODEL_1in54f",         NULL,                 MODEL_1in54f},
    { "MODEL_2in54b_V3",      NULL,                 MODEL_2in13b_V3},
    { "MODEL_2in7",           NULL,                 MODEL_2in7},
    { "MODEL_2in7b",          NULL,                 MODEL_2in7b},
    { "MODEL_2in9",           NULL,                 MODEL_2in9},
    { "MODEL_2in9_V2",        NULL,                 MODEL_2in9_V2},
    { "MODEL_2in9bc",         NULL,                 MODEL_2in9bc},
    { "MODEL_2in9b_V3",       NULL,                 MODEL_2in9b_V3},
    { "MODEL_2in9d",          NULL,                 MODEL_2in9d},
    { "MODEL_2in9f",          NULL,                 MODEL_2in9f},
    { "MODEL_3in7",           NULL,                 MODEL_3in7},

	{ NULL,             NULL,                   0}
};

LUAMOD_API int luaopen_eink( lua_State *L ){
    luat_newlib(L, reg_eink);
    return 1;
}
