-- Copyright (C) 2017 XiaoShan https://www.mivm.cn

module("luci.controller.k3screenctrl", package.seeall)

function index()
    if not nixio.fs.access("/etc/config/k3screenctrl") then
        return
    end
    entry({"admin","system","k3screenctrl"}, cbi("k3screenctrl"), _("Screen"),60)
end
