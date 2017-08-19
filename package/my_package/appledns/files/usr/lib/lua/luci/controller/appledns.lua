module("luci.controller.appledns",package.seeall)
function index()
entry({"admin","services","appledns"},cbi("appledns"),_("AppleDNS"),100)
end
