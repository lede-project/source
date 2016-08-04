module("luci.controller.bluetooth_le.bluetooth_le", package.seeall)

function index()
    entry({"admin", "bluetooth_le"}, firstchild(), "BLE", 30).dependent = false
end
