--[[

--]]



local HOMINGPOSITION = {
  ["L_coxaA"] = 45,
  ["L_femurA"] = 90,
  ["L_tibiaA"] = 90,
  ["L_coxaB"] = 90,
  ["L_femurB"] = 90,
  ["L_tibiaB"] = 90,
  ["L_coxaC"] = 45,
  ["L_femurC"] = 90,
  ["L_tibiaC"] = 90,
  ["R_coxaA"] = 45,
  ["R_femurA"] = 90,
  ["R_tibiaA"] = 90,
  ["R_coxaB"] = 90,
  ["R_femurB"] = 90,
  ["R_tibiaB"] = 90,
  ["R_coxaC"] = 45,
  ["R_femurC"] = 90,
  ["R_tibiaC"] = 90,
}


function homing(t_legname)
  return HOMINGPOSITION[t_legname]
end
