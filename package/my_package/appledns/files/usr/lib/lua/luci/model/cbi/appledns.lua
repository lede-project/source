local e=require"luci.sys"
local e=luci.model.uci.cursor()
local o=require"nixio.fs"
require("luci.sys")
local a,t,e
a=Map("appledns",translate("AppleDNS"),
translate("AppleDNS solves Apple's slow pace in some parts of China by collecting Apple's CDN data in China.").."<br /> "..
translate("This configuration file is currently on Unicom, telecommunications user-friendly."))
t=a:section(TypedSection,"base",translate("Base Settings"))
t.anonymous=true
e=t:option(Flag,"enable",translate("Enable"))
e.rmempty=false
e=t:option(ListValue,"operators",translate("Broadband Operators"))
e:value("cmcc",translate("China Mobile"))
e:value("cncc",translate("China Telecom"))
e:value("cucc",translate("China Unicom"))
e.rmempty=false
e=t:option(Flag,"restart",translate("Regenerate"),translate("Replace the operator or regenerate"))
e.rmempty=false
t=a:section(TypedSection,"base",translate("The currently generated configuration"))
t.anonymous=true
local i="/etc/dnsmasq.d/appledns.conf"
e=t:option(TextValue,"weblist")
e.description=translate("The first time you need to wait for the configuration to open, please open some time to come again!.")
e.rows=30
e.wrap="off"
e.cfgvalue=function(t,t)
return o.readfile(i)or""
end
e.write=function(t,t,e)
o.writefile("/tmp/appledns",e:gsub("\r\n","\n"))
if(luci.sys.call("cmp -s /tmp/appledns /etc/dnsmasq.d/appledns.conf")==1)then
o.writefile(i,e:gsub("\r\n","\n"))
end
o.remove("/tmp/appledns")
end
return a
