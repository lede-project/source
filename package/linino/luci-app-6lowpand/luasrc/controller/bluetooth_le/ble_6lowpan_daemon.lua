module("luci.controller.bluetooth_le.ble_6lowpan_daemon", package.seeall)

function index()
    local page = node("admin", "bluetooth_le", "ble_daemon_status")
    page.target= call("ble_daemon_status")
    page.title = "Daemon status"
    page.order = 1 

    page = entry({"admin", "bluetooth_le", "ble_daemon_scan"}, call("ble_scan"), "Scan", nil)
    page.leaf = true
    page.order = 2
    
    page = entry({"admin", "bluetooth_le", "ble_daemon_mac_delete"}, call("mac_delete"), nil)
    page.leaf = true

    page = entry({"admin", "bluetooth_le", "ble_daemon_mac_add"}, call("mac_add"), nil)
    page.leaf = true

    page = entry({"admin", "bluetooth_le", "ble_daemon_hci_reset"}, call("hci_reset"), nil)
    page.leaf = true

    page = entry({"admin", "bluetooth_le", "ble_daemon_start"}, call("daemon_start"), nil)
    page.leaf = true

    page = entry({"admin", "bluetooth_le", "ble_daemon_stop"}, call("daemon_stop"), nil)
    page.leaf = true
end

function ble_daemon_status()
    local p = assert(io.popen("bluetooth_6lowpand lswl"))

    local i = 1
    local devices = {}
    for line in p:lines() do
       devices[i] = {}
       devices[i].mac = line
       i = i + 1
    end

    p:close()
    
    local c = assert(io.popen("bluetooth_6lowpand lscon"))

    local i = 1
    local con_devices = {}
    for line in c:lines() do
       con_devices[i] = {}
       con_devices[i].mac = line
       con_devices[i].name = device_name(line)
       con_devices[i].address = ipv6_address(line)
       i = i + 1
    end

    c:close()
    
    local hci_devices = list_hci_devices()
    local daemon_running = daemon_running()   
 
    luci.template.render("bluetooth_le/ble_6lowpan_daemon_status", {dvs=devices, con=con_devices, hci_devices=hci_devices, daemon_running=daemon_running})
end

function mac_delete(mac)
    
    local p = assert(io.popen("bluetooth_6lowpand rmwl " .. mac))
    p:close()

    luci.http.redirect(luci.dispatcher.build_url("admin/bluetooth_le/ble_daemon_status"))

    luci.http.status(404, "No such interface " .. mac)
end

function mac_add()
    local function param(x)
        return luci.http.formvalue(x)
    end

    local mac = param("mac")

    if mac then
	local p = assert(io.popen("bluetooth_6lowpand addwl " .. mac))
	p:close()

	luci.http.redirect(luci.dispatcher.build_url("admin/bluetooth_le/ble_daemon_status"))
    else
        luci.template.render("bluetooth_le/ble_daemon_scan")
    end
end

function ble_connections()
    local p = assert(io.popen("cat /sys/kernel/debug/bluetooth/6lowpan_control"))
    local con = {}
    local i = 0
    for line in p:lines() do
        if i ~= 0 then
            con[i] = string.match(line, "(%w+:%w+:%w+:%w+:%w+:%w+)")
        end
        i = i + 1
    end
    p:close()
    return con
end

function device_name(dev)
    local p = assert(io.popen("gatttool --char-desc -t random -b " .. dev))
    local temp = ""
    local name_hex = ""
    local name_str = ""
    for line in p:lines() do
        if string.match(line, "uuid = 2a00") or string.match(line, "uuid = 00002a00") then
            temp = line
        end
    end

    if temp == "" then
        return("Unknown")
    end

    local handle = string.match(temp, "0x%w%w%w%w")
    p:close()

    p = assert(io.popen("gatttool --char-read -a " .. handle .. " -t random -b " .. dev))
    name_hex = string.gsub(p:read("*a"), "Characteristic value/descriptor: ", "")
    for word in string.gmatch(name_hex, "%w+") do
        name_str = name_str .. string.char(tonumber(word,16))
    end

    p:close()
    return name_str
end

function ipv6_address(dev)
    local ipv6 = {}
    local uci = require("luci.model.uci").cursor()
    local ula_prefix = uci:get("network", "globals", "ula_prefix")
    if ula_prefix then
        ula_prefix = ula_prefix:match("^[(%x+:)]+[^/::]")
        local _, count = string.gsub(ula_prefix, ":", "") 
        if count > 2 then
            ipv6.global = ula_prefix .. ':2' .. string.lower(dev:sub(4,8)) .. "ff:fe" .. string.lower(dev:sub(10,14)) .. string.lower(dev:sub(16,17))
        else
            ipv6.global = ula_prefix .. '::2' .. string.lower(dev:sub(4,8)) .. "ff:fe" .. string.lower(dev:sub(10,14)) .. string.lower(dev:sub(16,17))
        end
    else 
        ipv6.global = "ULA prefix missing"
    end

    ipv6.link = 'fe80::2' .. string.lower(dev:sub(4,8)) .. "ff:fe" .. string.lower(dev:sub(10,14)) .. string.lower(dev:sub(16,17))
    return ipv6
end

function ble_scan()
    local p2 = assert(io.popen("hcitool lescan & sleep 5 && LE_PID=$! && kill -2 $LE_PID"))

    local i = 0
    local devices = {}
    for line in p2:lines() do
        if i ~= 0 then
            devices[i] = {}
            devices[i].name = string.match(line, "%s.+")
            devices[i].mac = string.match(line, "[A-F,0-9,:]+%s")
        end
        i = i + 1
    end
    p2:close()
    local temp = {}
    new_devices = {}
    for i,v in pairs(devices) do
        if temp[v.mac] == nil then
            temp[v.mac] = 1
            table.insert(new_devices, v)
        else
            for key, value in pairs(new_devices) do
                if value.mac == v.mac and string.match(value.name, "unknown") then
                    new_devices[key].name = v.name
                end
            end
        end
    end

    luci.template.render("bluetooth_le/ble_daemon_scan", {found_devices=new_devices})
end

function list_hci_devices()
    local p = assert(io.popen("hciconfig"))
    local i = 1
    local hci_devices = {}
    for line in p:lines() do
        hci_name = line:match("^hci%d+")
        if hci_name then
            hci_devices[i] = {}
            hci_devices[i].name = hci_name
            i = i + 1
        end
    end
    p:close()
    return hci_devices
end

function hci_reset(hci_name)
    local p = assert(io.popen("hciconfig " .. hci_name .. " reset"))
    p:close()

    luci.http.redirect(luci.dispatcher.build_url("admin/bluetooth_le/ble_daemon_status"))
end

function daemon_running()
    local p = assert(io.popen("ps | grep 6lowpand"))
    
    local daemon_running = false
    for line in p:lines() do
        process = line:match("bluetooth_6lowpand")
        if process then
            daemon_running = true
        end
    end

    p:close()

    return daemon_running
end

function daemon_start()
    local p = assert(io.popen("/etc/init.d/bluetooth_6lowpand start"))
    p:close()

    luci.http.redirect(luci.dispatcher.build_url("admin/bluetooth_le/ble_daemon_status"))
end

function daemon_stop()
    local p = assert(io.popen("/etc/init.d/bluetooth_6lowpand stop"))
    p:close()

    luci.http.redirect(luci.dispatcher.build_url("admin/bluetooth_le/ble_daemon_status"))
end

