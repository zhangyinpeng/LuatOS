local roller = {}

local function event_handler(obj, event)
    if(event == lvgl.EVENT_VALUE_CHANGED) then
        -- char buf[32];
        -- lvgl.roller_get_selected_str(obj, buf, sizeof(buf));
        -- printf("Selected month: %s\n", buf);
        -- print(string.format("Clicked: %s\n", lvgl.list_get_btn_text(obj)));
    end
end

function roller.demo()
    local roller1 = lvgl.roller_create(lvgl.scr_act(), nil);
    lvgl.roller_set_options(roller1,
[[January
February
March
April
May
June
July
August
September
October
November
December]],
lvgl.ROLLER_MODE_INFINITE);

    lvgl.roller_set_visible_row_count(roller1, 4);
    lvgl.obj_align(roller1, nil, lvgl.ALIGN_CENTER, 0, 0);
    lvgl.obj_set_event_cb(roller1, event_handler);
end

return roller
