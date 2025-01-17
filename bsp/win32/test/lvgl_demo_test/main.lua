_G.sys = require "sys"

local arc_demo = require "arc_demo"
local bar_demo = require "bar_demo"
local btn_demo = require "btn_demo"
local btnmatrix_demo = require "btnmatrix_demo"
local calendar_demo = require "calendar_demo"
local canvas = require "canvas"
local checkbox_demo = require "checkbox_demo"
local chart_demo = require "chart_demo"
local cont_demo = require "cont_demo"
local cpicker_demo = require "cpicker_demo"
local dropdown = require "dropdown"
local gauge_demo = require "gauge_demo"
local img_demo = require "img_demo"
local imgbtn_demo = require "imgbtn_demo"
local keyboard = require "keyboard"
local label_demo = require "label_demo"
local led_demo = require "led_demo"
local line_demo = require "line_demo"
local list_demo = require "list_demo"
local lmeter_demo = require "lmeter_demo"
local msdbox_demo = require "msdbox_demo"
local objmask = require "objmask"
local page_demo = require "page_demo"
local roller = require "roller"
local slider_demo = require "slider_demo"
local spinbox_demo = require "spinbox_demo"
local spinner_demo = require "spinner_demo"
local switch_demo = require "switch_demo"
local table_demo = require "table_demo"
local tabview_demo = require "tabview_demo"
local textarea = require "textarea"
local tileview_demo = require "tileview_demo"
local win_demo = require "win_demo"

_G.symbol = require "symbol"

sys.taskInit(function ()
    log.info("lvgl", lvgl.init(480,320))

    -- arc_demo.demo()
    -- bar_demo.demo()
    -- btn_demo.demo()
    -- btnmatrix_demo.demo()
    -- calendar_demo.demo()
    -- canvas.demo()
    -- checkbox_demo.demo()
    -- chart_demo.demo()
    -- cont_demo.demo()
    -- cpicker_demo.demo()
    -- dropdown.demo()
    -- gauge_demo.demo()
    -- img_demo.demo()
    -- imgbtn_demo.demo()
    -- keyboard.demo()
    -- label_demo.demo()
    -- led_demo.demo()
    -- line_demo.demo()
    -- list_demo.demo()
    -- lmeter_demo.demo()
    -- msdbox_demo.demo()
    -- objmask.demo()
    -- page_demo.demo()
    -- roller.demo()
    -- slider_demo.demo()
    -- spinbox_demo.demo()
    -- spinner_demo.demo()
    -- switch_demo.demo()
    -- table_demo.demo()
    -- tabview_demo.demo()
    -- textarea.demo()
    -- tileview_demo.demo()
    win_demo.demo()

    while true do
        sys.wait(1000)
    end
end)

sys.run()
