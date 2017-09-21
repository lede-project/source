--[[
This file is part of YunWebUI.

YunWebUI is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

As a special exception, you may use this file as part of a free software
library without restriction.  Specifically, if other files instantiate
templates or use macros or inline functions from this file, or you compile
this file and link it with other files to produce an executable, this
file does not by itself cause the resulting executable to be covered by
the GNU General Public License.  This exception does not however
invalidate any other reasons why the executable file might be covered by
the GNU General Public License.

Copyright 2013 Arduino LLC (http://www.arduino.cc/)
]]

module("luci.controller.linino.index", package.seeall)

local function file_exists(file)
  local f = io.open(file, "rb")
  if f then f:close() end
  return f ~= nil
end

local function lines_from(file)
  local lines = {}
  for line in io.lines(file) do
    lines[#lines + 1] = line
  end
  return lines
end

--[[
local function rfind(s, c)
  local last = 1
  while string.find(s, c, last, true) do
    last = string.find(s, c, last, true) + 1
  end
  return last
end
]]

local function param(name)
  local val = luci.http.formvalue(name)
  if val then
    val = luci.util.trim(val)
    if string.len(val) > 0 then
      return val
    end
    return nil
  end
  return nil
end

local function not_nil_or_empty(value)
  return value and value ~= ""
end

local function check_update_file()
  local update_file = luci.util.exec("update-file-available")
  if update_file and string.len(update_file) > 0 then
    return update_file
  end
  return nil
end

local function get_first(cursor, config, type, option)
  return cursor:get_first(config, type, option)
end

local function set_first(cursor, config, type, option, value)
  cursor:foreach(config, type, function(s)
    if s[".type"] == type then
      cursor:set(config, s[".name"], option, value)
    end
  end)
end

local function delete_first(cursor, config, type, option, value)
  cursor:foreach(config, type, function(s)
    if s[".type"] == type then
      cursor:delete(config, s[".name"], option)
    end
  end)
end

local function to_key_value(s)
  local parts = luci.util.split(s, ":")
  parts[1] = luci.util.trim(parts[1])
  parts[2] = luci.util.trim(parts[2])
  return parts[1], parts[2]
end

function http_error(code, text)
  luci.http.prepare_content("text/plain")
  luci.http.status(code)
  if text then
    luci.http.write(text)
  end
end

function read_gpg_pub_key()
  local gpg_pub_key_ascii_file = io.open("/etc/arduino/arduino_gpg.asc")
  local gpg_pub_key_ascii = gpg_pub_key_ascii_file:read("*a")
  gpg_pub_key_ascii_file:close()
  return string.gsub(gpg_pub_key_ascii, "\n", "\\n")
end

dec_params = ""

function decrypt_pgp_message()
  local pgp_message = luci.http.formvalue("pgp_message")
  if pgp_message then
    if #dec_params > 0 then
      return dec_params
    end

    local pgp_enc_file = io.open("/tmp/pgp_message.txt", "w+")
    pgp_enc_file:write(pgp_message)
    pgp_enc_file:close()

    local json_input = luci.util.exec("cat /tmp/pgp_message.txt | gpg --no-default-keyring --secret-keyring /etc/arduino/arduino_gpg.sec --keyring /etc/arduino/arduino_gpg.pub --decrypt")
    local json = require("luci.json")
    dec_params = json.decode(json_input)
    return dec_params
  end
  return nil
end

function index()
  function luci.dispatcher.authenticator.lininoauth(validator, accs, default)
    require("luci.controller.linino.index")

    local dec_params = luci.controller.linino.index.decrypt_pgp_message()
    local user = luci.http.formvalue("username") or (dec_params and dec_params["username"])
    local pass = luci.http.formvalue("password") or (dec_params and dec_params["password"])
    local basic_auth = luci.http.getenv("HTTP_AUTHORIZATION")

    if user and validator(user, pass) then
      return user
    end

    if basic_auth and basic_auth ~= "" then
      local decoded_basic_auth = nixio.bin.b64decode(string.sub(basic_auth, 7))
      user = string.sub(decoded_basic_auth, 0, string.find(decoded_basic_auth, ":") - 1)
      pass = string.sub(decoded_basic_auth, string.find(decoded_basic_auth, ":") + 1)
    end

    if user then
      if #pass ~= 64 and validator(user, pass) then
        return user
      elseif #pass == 64 then
        local uci = luci.model.uci.cursor()
        uci:load("arduino")
        local stored_encrypted_pass = uci:get_first("arduino", "arduino", "password")
        if pass == stored_encrypted_pass then
          return user
        end
      end
    end

    local url = luci.http.getenv("PATH_INFO")
    local is_webpanel = string.find(luci.http.getenv("PATH_INFO"), "/webpanel")
    local tmpsystem, tmpmodel = luci.sys.sysinfo()


    if is_webpanel then
      local gpg_pub_key_ascii = luci.controller.linino.index.read_gpg_pub_key()
      luci.template.render("linino/set_password", { duser = default, fuser = user, pub_key = gpg_pub_key_ascii, login_failed = dec_params ~= nil, model = tmpmodel })
    else
      if user then
        luci.http.status(403)
      else
        luci.http.header("WWW-Authenticate", "Basic realm=\"linino\"")
        luci.http.status(401)
      end
    end

    return false
  end

  local function make_entry(path, target, title, order)
    local page = entry(path, target, title, order)
    page.leaf = true
    return page
  end

  -- web panel
  local webpanel = entry({ "webpanel" }, alias("webpanel", "go_to_homepage"), _("Linino Web Panel"), 10)
  webpanel.sysauth = "root"
  webpanel.sysauth_authenticator = "lininoauth"

  make_entry({ "webpanel", "homepage" }, call("homepage"), _("Linino Web Panel"), 10)
  make_entry({ "webpanel", "go_to_homepage" }, call("go_to_homepage"), nil)
  make_entry({ "webpanel", "set_password" }, call("go_to_homepage"), nil)
  make_entry({ "webpanel", "config" }, call("config"), nil)
  make_entry({ "webpanel", "wifi_detect" }, call("wifi_detect"), nil)
  make_entry({ "webpanel", "rebooting" }, template("linino/rebooting"), nil)
  make_entry({ "webpanel", "reset_board" }, call("reset_board"), nil)
  make_entry({ "webpanel", "toogle_rest_api_security" }, call("toogle_rest_api_security"), nil)

  --api security level
  local uci = luci.model.uci.cursor()
  uci:load("arduino")
  local secure_rest_api = uci:get_first("arduino", "arduino", "secure_rest_api")
  local rest_api_sysauth = false
  if secure_rest_api == "true" then
    rest_api_sysauth = webpanel.sysauth
  end

  --storage api
  local data_api = node("data")
  data_api.sysauth = rest_api_sysauth
  data_api.sysauth_authenticator = webpanel.sysauth_authenticator
  make_entry({ "data", "get" }, call("storage_send_request"), nil).sysauth = rest_api_sysauth
  make_entry({ "data", "put" }, call("storage_send_request"), nil).sysauth = rest_api_sysauth
  make_entry({ "data", "delete" }, call("storage_send_request"), nil).sysauth = rest_api_sysauth
  local mailbox_api = node("mailbox")
  mailbox_api.sysauth = rest_api_sysauth
  mailbox_api.sysauth_authenticator = webpanel.sysauth_authenticator
  make_entry({ "mailbox" }, call("build_bridge_mailbox_request"), nil).sysauth = rest_api_sysauth
  
  --plain socket endpoint
  local plain_socket_endpoint = make_entry({ "arduino" }, call("board_plain_socket"), nil)
  plain_socket_endpoint.sysauth = rest_api_sysauth
  plain_socket_endpoint.sysauth_authenticator = webpanel.sysauth_authenticator
end

function go_to_homepage()
  luci.http.redirect(luci.dispatcher.build_url("webpanel/homepage"))
end

local function csv_to_array(text, callback)
  local array = {}
  local line_parts;
  local lines = string.split(text, "\n")
  for i, line in ipairs(lines) do
    line_parts = string.split(line, "\t")
    callback(line_parts, array)
  end
  return array
end

function homepage()
  local wa = require("luci.tools.webadmin")
  local network = luci.util.exec("LANG=en ifconfig | grep HWaddr")
  network = string.split(network, "\n")
  local ifnames = {}
  for i, v in ipairs(network) do
    local ifname = luci.util.trim(string.split(network[i], " ")[1])
    if not_nil_or_empty(ifname) then
      table.insert(ifnames, ifname)
    end
  end

  local ifaces_pretty_names = {
    wlan0 = "WiFi",
    eth1 = "Wired Ethernet"
  }

  local ifaces = {}
  for i, ifname in ipairs(ifnames) do
    local ix = luci.util.exec("LANG=en ifconfig " .. ifname)
    local mac = ix and ix:match("HWaddr ([^%s]+)") or "-"

    ifaces[ifname] = {
      mac = mac:upper(),
      pretty_name = ifaces_pretty_names[ifname]
    }

    local address = ix and ix:match("inet addr:([^%s]+)")
    local netmask = ix and ix:match("Mask:([^%s]+)")
    if address then
      ifaces[ifname]["address"] = address
      ifaces[ifname]["netmask"] = netmask
    end
  end

  local deviceinfo = luci.sys.net.deviceinfo()
  for k, v in pairs(deviceinfo) do
    if ifaces[k] then
      ifaces[k]["rx"] = v[1] and wa.byte_format(tonumber(v[1])) or "-"
      ifaces[k]["tx"] = v[9] and wa.byte_format(tonumber(v[9])) or "-"
    end
  end

  local sysinfo = {}
  local tmpsystem, tmpmodel, tmpmemtot, tmpmemcac, tmpmembuf, tmpmemfree, tmpmips = luci.sys.sysinfo()
  local l1, l5, l15 = luci.sys.loadavg()

  sysinfo["systemtype"] = tmpsystem
  sysinfo["machine"] = tmpmodel
  sysinfo["bogomips"] = tmpmips
  sysinfo["kernel"] = luci.sys.exec("uname -r")
  sysinfo["time"] = os.date()
  sysinfo["uptime"] = luci.sys.uptime()
  sysinfo["load"] = math.floor(l1 * 100)
  sysinfo["memtot"] = tmpmemtot
  sysinfo["memfree"] = tmpmemfree
  

  local ctx = {
    hostname = luci.sys.hostname(),
    ifaces = ifaces,
    sysinfo = sysinfo,
  }

  --Traslates status codes extracted from include/linux/ieee80211.h to a more friendly version
  local function parse_dmesg(lines)
    local function get_error_message_from(file_suffix, code)
      local function wifi_error_message_code_callback(line_parts, array)
        if line_parts[1] and line_parts[2] then
          table.insert(array, { message = line_parts[1], code = tonumber(line_parts[2]) })
        end
      end

      local wifi_errors = csv_to_array(luci.util.exec("zcat /etc/arduino/wifi_error_" .. file_suffix .. ".csv.gz"), wifi_error_message_code_callback)
      for idx, wifi_error in ipairs(wifi_errors) do
        if code == wifi_error.code then
          return lines, wifi_error.message
        end
      end
      return lines, nil
    end

    local function find_dmesg_wifi_error_reason(lines)
      for idx, line in ipairs(lines) do
        if string.find(line, "disassociated from") then
          return string.match(line, "Reason: (%d+)")
        end
      end
      return nil
    end

    local code = tonumber(find_dmesg_wifi_error_reason(lines))
    if code then
      return get_error_message_from("reasons", code)
    end

    local function find_dmesg_wifi_error_status(lines)
      for idx, line in ipairs(lines) do
        if string.find(line, "denied authentication") then
          return string.match(line, "status (%d+)")
        end
      end
      return nil
    end

    code = tonumber(find_dmesg_wifi_error_status(lines))
    if code then
      return get_error_message_from("statuses", code)
    end

    return lines, nil
  end

  if file_exists("/last_dmesg_with_wifi_errors.log") then
    local lines, error_message = parse_dmesg(lines_from("/last_dmesg_with_wifi_errors.log"))
    ctx["last_log"] = lines
    ctx["last_log_error_message"] = error_message
  end

  local update_file = check_update_file()
  if update_file then
    ctx["update_file"] = update_file
  end

  luci.template.render("linino/homepage", ctx)
end

local function timezone_file_parse_callback(line_parts, array)
  if line_parts[1] and line_parts[2] and line_parts[3] then
    table.insert(array, { label = line_parts[1], timezone = line_parts[2], code = line_parts[3] })
  end
end

function config_get()
  local uci = luci.model.uci.cursor()
  uci:load("system")
  uci:load("wireless")

  local timezones_wifi_reg_domains = csv_to_array(luci.util.exec("zcat /etc/arduino/wifi_timezones.csv.gz"), timezone_file_parse_callback)

  local encryptions = {}
  encryptions[1] = { code = "none", label = "None" }
  encryptions[2] = { code = "wep", label = "WEP" }
  encryptions[3] = { code = "psk", label = "WPA" }
  encryptions[4] = { code = "psk2", label = "WPA2" }

  local uci = luci.model.uci.cursor()
  uci:load("arduino")
  local rest_api_is_secured = uci:get_first("arduino", "arduino", "secure_rest_api") == "true"

  local ctx = {
    hostname = get_first(uci, "system", "system", "hostname"),
    timezone_desc = get_first(uci, "system", "system", "timezone_desc"),
    wifi = {
      ssid = get_first(uci, "arduino", "wifi-iface", "ssid"),
      encryption = get_first(uci, "arduino", "wifi-iface", "encryption"),
      password = get_first(uci, "arduino", "wifi-iface", "key"),
    },
    timezones_wifi_reg_domains = timezones_wifi_reg_domains,
    encryptions = encryptions,
    pub_key = luci.controller.linino.index.read_gpg_pub_key(),
    rest_api_is_secured = rest_api_is_secured
  }

  luci.template.render("linino/config", ctx)
end

function config_post()
  local params = decrypt_pgp_message()

  local uci = luci.model.uci.cursor()
  uci:load("system")
  uci:load("wireless")
  uci:load("network")
  --uci:load("dhcp")
  uci:load("arduino")

  if not_nil_or_empty(params["password"]) then
    local password = params["password"]
    luci.sys.user.setpasswd("root", password)

    local sha256 = require("luci.sha256")
    set_first(uci, "arduino", "arduino", "password", sha256.sha256(password))
  end

  if not_nil_or_empty(params["hostname"]) then
    local hostname = string.gsub(params["hostname"], " ", "_")
    set_first(uci, "system", "system", "hostname", hostname)
  end

  if params["timezone_desc"] then
    local function find_tz_regdomain(timezone_desc)
      local tz_regdomains = csv_to_array(luci.util.exec("zcat /etc/arduino/wifi_timezones.csv.gz"), timezone_file_parse_callback)
      for i, tz in ipairs(tz_regdomains) do
        if tz["label"] == timezone_desc then
          return tz
        end
      end
      return nil
    end

    local tz_regdomain = find_tz_regdomain(params["timezone_desc"])
    if tz_regdomain then
      set_first(uci, "system", "system", "timezone", tz_regdomain.timezone)
      set_first(uci, "system", "system", "timezone_desc", params["timezone_desc"])
      params["wifi.country"] = tz_regdomain.code
    end
  end

  if params["wifi.ssid"] and params["wifi.encryption"] then
    uci:set("wireless", "radio0", "channel", "auto")
    uci:set("arduino", "radio0", "channel", "auto")
    set_first(uci, "wireless", "wifi-iface", "mode", "sta")
    set_first(uci, "arduino", "wifi-iface", "mode", "sta")

    if params["wifi.ssid"] then
      set_first(uci, "wireless", "wifi-iface", "ssid", params["wifi.ssid"])
      set_first(uci, "arduino", "wifi-iface", "ssid", params["wifi.ssid"])
    end
    if params["wifi.encryption"] then
      set_first(uci, "wireless", "wifi-iface", "encryption", params["wifi.encryption"])
      set_first(uci, "arduino", "wifi-iface", "encryption", params["wifi.encryption"])
    end
    if params["wifi.password"] then
      set_first(uci, "wireless", "wifi-iface", "key", params["wifi.password"])
      set_first(uci, "arduino", "wifi-iface", "key", params["wifi.password"])
    end
    if params["wifi.country"] then
      uci:set("wireless", "radio0", "country", params["wifi.country"])
      uci:set("arduino", "radio0", "country", params["wifi.country"])
    end

    uci:delete("network", "lan", "ipaddr")
    uci:delete("network", "lan", "netmask")
    --delete_first(uci, "dhcp", "dnsmasq", "address")

    uci:set("network", "lan", "proto", "dhcp")
    uci:set("arduino", "lan", "proto", "dhcp")

    set_first(uci, "arduino", "arduino", "wifi_reset_step", "clear")
  end

  uci:commit("system")
  uci:commit("wireless")
  uci:commit("network")
  --uci:commit("dhcp")
  uci:commit("arduino")

  --[[
    local new_httpd_conf = ""
    for line in io.lines("/etc/httpd.conf") do
      if string.find(line, "C:192.168") == 1 then
        line = "#" .. line
      end
      new_httpd_conf = new_httpd_conf .. line .. "\n"
    end
    local new_httpd_conf_file = io.open("/etc/httpd.conf", "w+")
    new_httpd_conf_file:write(new_httpd_conf)
    new_httpd_conf_file:close()
  ]]

  local ctx = {
    hostname = get_first(uci, "system", "system", "hostname"),
    ssid = get_first(uci, "wireless", "wifi-iface", "ssid")
  }

  luci.template.render("linino/rebooting", ctx)

  luci.util.exec("reboot")
end

function config()
  if luci.http.getenv("REQUEST_METHOD") == "POST" then
    config_post()
  else
    config_get()
  end
end

function wifi_detect()
  local sys = require("luci.sys")
  local iw = sys.wifi.getiwinfo("radio0")
  local wifis = iw.scanlist
  local result = {}
  for idx, wifi in ipairs(wifis) do
    if not_nil_or_empty(wifi.ssid) then
      local name = wifi.ssid
      local encryption = "none"
      local pretty_encryption = "None"
      if wifi.encryption.wep then
        encryption = "wep"
        pretty_encryption = "WEP"
      elseif wifi.encryption.wpa == 1 then
        encryption = "psk"
        pretty_encryption = "WPA"
      elseif wifi.encryption.wpa >= 2 then
        encryption = "psk2"
        pretty_encryption = "WPA2"
      end
      table.insert(result, { name = name, encryption = encryption, pretty_encryption = pretty_encryption })
    end
  end

  luci.http.prepare_content("application/json")
  local json = require("luci.json")
  luci.http.write(json.encode(result))
end

function reset_board()
  local update_file = check_update_file()
  if param("button") and update_file then
    local ix = luci.util.exec("LANG=en ifconfig wlan0 | grep HWaddr")
    local macaddr = string.gsub(ix:match("HWaddr ([^%s]+)"), ":", "")

    luci.template.render("linino/board_reset", { name = "Arduino Yun-" .. macaddr })

    luci.util.exec("blink-start 50")
    luci.util.exec("run-sysupgrade " .. update_file)
  end
end

function toogle_rest_api_security()
  local uci = luci.model.uci.cursor()
  uci:load("arduino")

  local rest_api_secured = luci.http.formvalue("rest_api_secured")
  if rest_api_secured == "true" then
    set_first(uci, "arduino", "arduino", "secure_rest_api", "true")
  else
    set_first(uci, "arduino", "arduino", "secure_rest_api", "false")
  end

  uci:commit("arduino")
end

local function build_bridge_request_digital_analog(command, pin, padded_pin, value)
  local data = { command, "/", padded_pin };

  if value then
    if command == "digital" then
      if value ~= 0 and value ~= 1 then
        return nil
      end
      table.insert(data, "/")
      table.insert(data, value)
    else
      if value > 999 then
        return nil
      end
      table.insert(data, "/")
      table.insert(data, string.format("%03d", value))
    end
  end

  local bridge_request = {
    command = "raw",
    data = table.concat(data)
  }
  return bridge_request
end

local function build_bridge_request(command, params)

  local bridge_request = {
    command = command
  }
  
  if command == "raw" then
    params = table.concat(params, "/")
    if not_nil_or_empty(params) then
      bridge_request["data"] = params
    end
    return bridge_request
  end

  if command == "get" then
    if not_nil_or_empty(params[1]) then
      bridge_request["key"] = params[1]
    end
    return bridge_request
  end

  if command == "put" and not_nil_or_empty(params[1]) and params[2] then
    bridge_request["key"] = params[1]
    bridge_request["value"] = params[2]
    return bridge_request
  end

  if command == "delete" and not_nil_or_empty(params[1]) then
    bridge_request["key"] = params[1]
    return bridge_request
  end

  return nil
end

local function extract_jsonp_param(query_string)
  if not not_nil_or_empty(query_string) then
    return nil
  end

  local qs_parts = string.split(query_string, "&")
  for idx, value in ipairs(qs_parts) do
    if string.find(value, "jsonp") == 1 then
      return string.sub(value, string.find(value, "=") + 1)
    end
  end
end

local function parts_after(url_part)
  local url = luci.http.getenv("PATH_INFO")
  local url_after_part = string.find(url, "/", string.find(url, url_part) + 1)
  if not url_after_part then
    return {}
  end
  return luci.util.split(string.sub(url, url_after_part + 1), "/")
end

function storage_send_request()
  local method = luci.http.getenv("REQUEST_METHOD")
  local jsonp_callback = extract_jsonp_param(luci.http.getenv("QUERY_STRING"))
  local parts = parts_after("data")
  local command = parts[1]
  if not command or command == "" then
    luci.http.status(404)
    return
  end
  local params = {}
  for idx, param in ipairs(parts) do
    if idx > 1 and not_nil_or_empty(param) then
      table.insert(params, param)
    end
  end

  -- TODO check method?
  local bridge_request = build_bridge_request(command, params)
  if not bridge_request then
    luci.http.status(403)
    return
  end

  local sock, code, msg = nixio.connect("127.0.0.1", 5700)
  if not sock then
    code = code or ""
    msg = msg or ""
    http_error(500, "nil socket, " .. code .. " " .. msg)
    return
  end

  sock:setopt("socket", "sndtimeo", 5)
  sock:setopt("socket", "rcvtimeo", 5)
  sock:setopt("tcp", "nodelay", 1)

  local json = require("luci.json")

  sock:write(json.encode(bridge_request))
  sock:writeall("\n")

  local response_text = {}
  while true do
    local bytes = sock:recv(4096)
    if bytes and #bytes > 0 then
      table.insert(response_text, bytes)
    end

    local json_response = json.decode(table.concat(response_text))
    if json_response then
      sock:close()
      luci.http.status(200)
      if jsonp_callback then
        luci.http.prepare_content("application/javascript")
        luci.http.write(jsonp_callback)
        luci.http.write("(")
        luci.http.write_json(json.encode(json_response))
        luci.http.write(");")
      else
        luci.http.prepare_content("application/json")
        luci.http.write(json.encode(json_response))
      end
      return
    end

    if not bytes or #response_text == 0 then
      sock:close()
      http_error(500, "Empty response")
      return
    end
  end

  sock:close()
end

function board_plain_socket()
  local function send_response(response_text, jsonp_callback)
    if not response_text then
      luci.http.status(500)
      return
    end

    local rows = luci.util.split(response_text, "\r\n")
    if #rows == 1 or string.find(rows[1], "Status") ~= 1 then
      luci.http.prepare_content("text/plain")
      luci.http.status(200)
      luci.http.write(response_text)
      return
    end

    local body_start_at_idx = -1
    local content_type = "text/plain"
    for idx, row in ipairs(rows) do
      if row == "" then
        body_start_at_idx = idx
        break
      end

      local key, value = to_key_value(row)
      if string.lower(key) == "status" then
        luci.http.status(tonumber(value))
      elseif string.lower(key) == "content-type" then
        content_type = value
      else
        luci.http.header(key, value)
      end
    end

    local response_body = table.concat(rows, "\r\n", body_start_at_idx + 1)
    if content_type == "application/json" and jsonp_callback then
      local json = require("luci.json")
      luci.http.prepare_content("application/javascript")
      luci.http.write(jsonp_callback)
      luci.http.write("(")
      luci.http.write_json(json.encode(json.decode(response_body)))
      luci.http.write(");")
    else
      luci.http.prepare_content(content_type)
      luci.http.write(response_body)
    end
  end

  local method = luci.http.getenv("REQUEST_METHOD")
  local jsonp_callback = extract_jsonp_param(luci.http.getenv("QUERY_STRING"))
  local parts = parts_after("arduino")
  local params = {}
  for idx, param in ipairs(parts) do
    if not_nil_or_empty(param) then
      table.insert(params, param)
    end
  end

  if #params == 0 then
    luci.http.status(404)
    return
  end

  params = table.concat(params, "/")

  local sock, code, msg = nixio.connect("127.0.0.1", 5555)
  if not sock then
    code = code or ""
    msg = msg or ""
    http_error(500, "Could not connect to YunServer " .. code .. " " .. msg)
    return
  end

  sock:setopt("socket", "sndtimeo", 5)
  sock:setopt("socket", "rcvtimeo", 5)
  sock:setopt("tcp", "nodelay", 1)

  sock:write(params)
  sock:writeall("\r\n")

  local response_text = sock:readall()
  sock:close()

  send_response(response_text, jsonp_callback)
end

function build_bridge_mailbox_request()
  local method = luci.http.getenv("REQUEST_METHOD")
  local jsonp_callback = extract_jsonp_param(luci.http.getenv("QUERY_STRING"))
  local parts = parts_after("mailbox")
  local params = {}
  for idx, param in ipairs(parts) do
    if not_nil_or_empty(param) then
      table.insert(params, param)
    end
  end

  if #params == 0 then
    luci.http.status(400)
    return
  end

  local bridge_request = build_bridge_request("raw", params)
  if not bridge_request then
    luci.http.status(403)
    return
  end

  local uci = luci.model.uci.cursor()
  uci:load("arduino")
  local socket_timeout = uci:get_first("arduino", "arduino", "socket_timeout", 5)

  local sock, code, msg = nixio.connect("127.0.0.1", 5700)
  if not sock then
    code = code or ""
    msg = msg or ""
    http_error(500, "nil socket, " .. code .. " " .. msg)
    return
  end

  sock:setopt("socket", "sndtimeo", socket_timeout)
  sock:setopt("socket", "rcvtimeo", socket_timeout)
  sock:setopt("tcp", "nodelay", 1)

  local json = require("luci.json")

  sock:write(json.encode(bridge_request))
  sock:writeall("\n")
  sock:close()

  luci.http.status(200)
end
