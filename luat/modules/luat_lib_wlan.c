
#include "luat_base.h"
#include "luat_log.h"
#include "luat_timer.h"
#include "luat_malloc.h"
#include "rtthread.h"

#ifdef RT_WLAN_MANAGE_ENABLE && RT_USING_WIFI
#include "wlan_dev.h"
#include <wlan_mgnt.h>
#include <wlan_prot.h>
#include <wlan_cfg.h>

static int l_wlan_get_mode(lua_State *L) {
    rt_device_t dev = rt_device_find(luaL_checkstring(L, 1));
    if (dev == RT_NULL) {
        return 0;
    }
    int mode = rt_wlan_get_mode(dev);
    lua_pushinteger(L, mode);
    return 1;
}

static int l_wlan_set_mode(lua_State *L) {
    rt_device_t dev = rt_device_find(luaL_checkstring(L, 1));
    if (dev == RT_NULL) {
        return 0;
    }
    int mode = luaL_checkinteger(L, 2);
    int re = rt_wlan_set_mode(dev, mode);
    lua_pushinteger(L, re);
    return 1;
}

static int l_wlan_join(lua_State *L) {
    const char* ssid = luaL_checkstring(L, 1);
    if (lua_isstring(L, 2)) {
        const char* passwd = luaL_checkstring(L, 2);
        int re = rt_wlan_connect(ssid, passwd);
        lua_pushinteger(L, re);
    }
    else {
        int re = rt_wlan_connect(ssid, RT_NULL);
        lua_pushinteger(L, re);
    }
    return 1;
}

static int l_wlan_disconnect(lua_State *L) {
    if (rt_wlan_is_connected()) {
        rt_wlan_disconnect();
        lua_pushboolean(L, 1);
    }
    else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static int l_wlan_connected(lua_State *L) {
    lua_pushboolean(L, rt_wlan_is_connected() == 1 ? 1 : 0);
    return 1;
}

static int l_wlan_autoreconnect(lua_State *L) {
    if (lua_gettop(L) > 0) {
        rt_wlan_config_autoreconnect(luaL_checkinteger(L, 1));
        return 0;
    }
    else {
        lua_pushboolean(L, rt_wlan_get_autoreconnect_mode());
        return 1;
    }
}

static int l_wlan_scan(lua_State *L) {
    lua_pushinteger(L, rt_wlan_scan());
    return 1;
}

static int l_wlan_scan_get_info_num(lua_State *L) {
    lua_pushinteger(L, rt_wlan_scan_get_info_num());
    return 1;
}

static int l_wlan_scan_get_info(lua_State *L) {
    //rt_wlan_ap_get_sta_info();
    return 0;
}

//MAC地址
static int l_wlan_get_mac(lua_State *L) {
    rt_uint8_t mac[6];
    char buff[14];
    mac[0] = 0x00;
    rt_wlan_get_mac(mac);
    if (mac[0] != 0x00) {
        rt_snprintf(buff, 14, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        lua_pushlstring(L, buff, 12);
        return 1;
    }
    return 0;
}

//MAC地址
static int l_wlan_get_mac_raw(lua_State *L) {
    rt_uint8_t mac[6];
    mac[0] = 0x00;
    rt_wlan_get_mac(mac);
    if (mac[0] != 0x00) {
        lua_pushlstring(L, mac, 6);
        return 1;
    }
    return 0;
}

static int l_wlan_ready(lua_State *L) {
    lua_pushinteger(L, rt_wlan_is_ready());
    return 1;
}

// 注册回调
static void wlan_cb(int event, struct rt_wlan_buff *buff, void *parameter) {
    rt_kprintf("wlan event -> %d\n", event);
}
static void reg_wlan_callbacks(void) {
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_SCAN_DONE, wlan_cb, RT_NULL);
    //rt_wlan_register_event_handler(RT_WLAN_EVT_SCAN_REPORT, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_CONNECTED, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_CONNECTED_FAIL, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_DISCONNECTED, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_AP_START, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_AP_STOP, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_AP_ASSOCIATED, wlan_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_AP_DISASSOCIATED, wlan_cb, RT_NULL);
}

#include "rotable.h"
static const rotable_Reg reg_wlan[] =
{
    { "getMode" ,  l_wlan_get_mode , 0},
    { "setMode" ,  l_wlan_set_mode , 0},
    { "join" ,     l_wlan_join , 0},
    { "disconnect",l_wlan_disconnect , 0},
    { "connected" ,l_wlan_connected , 0},
    { "ready" ,    l_wlan_ready , 0},
    { "autoreconnect", l_wlan_autoreconnect, 0},
    { "scan",      l_wlan_scan, 0},
    { "scan_get_info_num", l_wlan_scan_get_info_num, 0},
    { "scan_get_info", l_wlan_scan_get_info, 0},
    { "get_mac", l_wlan_get_mac, 0},
    { "get_mac_raw", l_wlan_get_mac_raw, 0},
    //{ "set_mac", l_wlan_set_mac},
    
    { "NONE",      NULL , RT_WLAN_NONE},
    { "STATION",   NULL , RT_WLAN_STATION},
    { "AP",        NULL , RT_WLAN_AP},
	{ NULL, NULL , 0}
};

LUAMOD_API int luaopen_wlan( lua_State *L ) {
    reg_wlan_callbacks();

    rotable_newlib(L, reg_wlan);

    return 1;
}

#endif